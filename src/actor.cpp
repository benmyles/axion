#include "axion/actor.hpp"
#include "axion/journal.hpp"
#include "individual_io.hpp"
#include "axion/dialogue.hpp"
#include "life_runtime.hpp"
#include <array>
#include <algorithm>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace axion::life {
namespace {
using namespace cuda;
using namespace runtime;
using namespace storage;
constexpr std::string_view usage = R"(axion agent  |  one CUDA creature on an external sensory stream

  ./axion agent --from habitat.eco --lesson examples/grasps.lesson --save pupil.mind
  ./axion agent --load pupil.mind --lesson another.lesson --save pupil.mind

  --from FILE    hatch a descendant with an occupied site's inherited program
  --resident FILE  carry a living animal's tissue, acquired connections and recent sensory history into the external stream
  --site N       choose a row-major donor site (default first occupied site)
  --load FILE    resume an agent's neural tissue, synapses and pending outcomes
  --inspect      print the inherited synaptic truth tables and exit
  --lesson FILE  read events from a file (default stdin)
                 a comment beginning with "# phase " names subsequent scored choices in the journal
  --save FILE    atomically save the resulting agent
  --map-cut intact|silent|fixed|one-pulse|no-appetite|no-soma|no-dendrites|isolated|disinhibited
            raw|trace-fixed|immediate|no-hold|labile|no-novelty|fixed-support for temporal circuits
  --batch-bench N   benchmark frozen orientation on N separately allocated CUDA actors
  --decisions N     decisions per actor in each timed launch (64)
  --backend gpu  execute all neural and response updates on CUDA

Each event supplies a 32-bit receptor pattern and a 32-bit ambient field.
Numbers accept decimal or hexadecimal. Text after # is a comment.

  trial RAW FIELD MASK VALUE  act and deliver the physical effector test as feedback
  probe RAW FIELD [MASK VALUE]  read the choice on a copy of the current state
  step RAW FIELD +|-|0        present an external metabolic signal and act
  travel RAW GOAL EXITS [NEEDS]   use a grip and sense its destination (four byte-sized landmark indices)
  pace RAW GOAL EXITS [NEEDS]     use the locomotor port to move through the same doors
  stride RAW GOAL [NEEDS FOOD]   receive the next sensation of a continuous walk and choose a muscle
  orient RAW GOAL [GRIP [NEEDS]]   make a frozen choice, optionally score its physical grip
  face RAW GOAL [DIRECTION [NEEDS]]   score the locomotor port on a frozen choice
  savour RAW FOOD   receive food 0..3 at a landmark without moving
  sample RAW FIELD NEEDS SURFACE [RECIPE]   choose a grip and sense its consequence
  taste FLAVOUR FOOD            passively expose one flavour and one food (indices 0..3)
  seek RAW FIELD NEEDS SURFACE RECIPE      frozen choice for a specified appetite

NEEDS is a four-bit mask of bodily deficits. SURFACE packs four two-bit flavours
indexed by physical grip. RECIPE similarly maps flavours to foods. Omit RECIPE
in sample to explore an object without receiving food. These events use a
balanced-metabolism individual. Only sensory consequences reach the brain.

A trial succeeds when the emitted effector byte ANDed with MASK equals VALUE.
Trial feedback arrives after the donor's digestive delay. A step replaces the
signal due at that tick. Probes preserve the complete agent, including the clock.
)";
Word number(std::string_view token, Word limit) {
    int radix = 10;
    if (token.starts_with("0x") || token.starts_with("0X")) { token.remove_prefix(2); radix = 16; }
    Word x{};
    auto [ptr, err] = std::from_chars(token.data(), token.data()+token.size(), x, radix);
    if (token.empty() || err != std::errc{} || ptr != token.data()+token.size() || x > limit)
        throw std::invalid_argument("invalid unsigned integer");
    return x;
}
struct Options {
    std::string from,load,save,lesson;
    std::optional<Word> site;
    Word map_cut=0,copies=0,decisions=64;
    bool inspect=false,resident=false;
};
Options parse(int argc, char **argv) {
    Options o;
    for (int i = 2; i < argc; ++i) {
        std::string_view flag = argv[i];
        if (flag == "--inspect") { o.inspect = true; continue; }
        if (++i == argc) throw std::invalid_argument("missing value for " + std::string(flag));
        std::string_view val = argv[i];
        if (flag=="--from" || flag=="--resident") {
            if (!o.from.empty()) throw std::invalid_argument("choose one donor habitat");
            o.from=val; o.resident=flag=="--resident";
        }
        else if (flag == "--load") o.load = val;
        else if (flag == "--save") o.save = val;
        else if (flag == "--lesson") o.lesson = val;
        else if (flag == "--site") o.site = number(val, 1024*1024-1);
        else if (flag=="--batch-bench" || flag=="--decisions") {
            Word n=number(val,flag=="--batch-bench" ? 1048576u : 4096u);
            if (!n) throw std::invalid_argument("a benchmark size must be positive");
            if (flag=="--batch-bench") o.copies=n; else o.decisions=n;
        } else if (flag=="--map-cut") {
            constexpr std::array names{"intact","silent","fixed","one-pulse","erased","no-appetite","no-soma","no-dendrites","isolated","disinhibited","raw","trace-fixed","immediate","no-hold","labile","no-novelty","fixed-support"};
            auto it=std::find(names.begin(),names.end(),std::string_view(val));
            if (it==names.end() || val=="erased") throw std::invalid_argument("unknown external navigation intervention");
            o.map_cut=Word(it-names.begin());
        }
        else if (flag != "--backend" || val != "gpu") throw std::invalid_argument("unknown agent option or value for " + std::string(flag));
    }
    if (o.from.empty() == o.load.empty() || (o.site && o.from.empty()))
        throw std::invalid_argument("choose --from or --resident with an optional site, or --load a saved individual");
    if (o.inspect && (!o.lesson.empty() || !o.save.empty())) throw std::invalid_argument("--inspect displays a genome without executing a lesson");
    return o;
}

void inspect(const AgentImage &image) {
    const auto &a=image.brain;const auto &links=image.links;const auto &contexts=image.contexts;const auto &expression=image.expression;
    Word metabolism=image.balanced,navigation=image.navigation;
    auto food=metabolism ? &image.food : nullptr;auto map=navigation ? &image.map : nullptr;auto trace=navigation>=4 ? &image.trace : nullptr;
    auto ear=image.auditory ? &image.ear : nullptr;auto memory=image.recollects ? &image.recollection : nullptr;
    auto state_bytes=payload(a.tiles)+association_stride(a.has_relations,1+Word(a.alternate_program!=0))+(food ? sizeof(OutcomeLayer):0)+(map ? sizeof(MapLayer) : 0)+(trace ? sizeof(TraceLayer) : 0)+(ear ? sizeof(AuditoryLayer) : 0)+(memory ? sizeof(Recollection) : 0);
    auto inherited_bytes=sizeof(Genome)+(a.bits==2 ? sizeof(SynapticGenes) : 0)
                       +(a.has_relations ? sizeof(LinkGenes)+sizeof(Word) : 0)
                       +(a.has_relations==3 ? sizeof(ContextGenes) : 0)+(a.alternate_program ? sizeof(ExpressionGenes) : 0)+(food ? sizeof(OutcomeGenes) : 0)+(map ? sizeof(MapGenes) : 0)+(trace ? sizeof(TraceGenes) : 0)+(ear ? sizeof(Word) : 0)+(memory ? sizeof(RecollectionGenes) : 0);
    state_bytes+=(image.attends ? sizeof(AttentiveEar) : 0)+(image.orients ? sizeof(Iris) : 0);
    if (image.imagines) state_bytes+=sizeof(Imagery);
    if (image.binds) {state_bytes+=sizeof(Bindings);inherited_bytes+=sizeof(BindingGenes);}
    if (image.conjoins) {state_bytes+=sizeof(Conjunctions);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.reports) {state_bytes+=sizeof(Reports);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.conceptual) {state_bytes+=sizeof(Concepts);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.potter) {
        state_bytes += sizeof(clay::Mind);
        inherited_bytes += sizeof(clay::Law);
        std::cout << "context_assemblies=" << clay::count(image.ceramic.mind)
                  << "  context_capacity=" << clay::capacity(image.ceramic.mind) << '\n';
    }
    if (image.calling) {
        state_bytes += sizeof(siphon::Animal);
        inherited_bytes += sizeof(siphon::Genes);
        auto &a = image.calls.animal;
        std::cout << "native_contact_bytes=12  native_encounters=" << a.occasions << "  native_heard=" << a.listened
                  << "  native_harvest=" << a.harvest << '\n';
    }
    if (image.saline) {
        auto &r = image.brine_mind.cortex;
        state_bytes += sizeof(r);
        inherited_bytes += sizeof(r.genes);
        std::cout << "brine_neurons=" << std::popcount(r.genes.active)
                  << "  modified_predictive_cells=" << std::popcount(r.memory.low | r.memory.high)
                  << "  inherited_bytes=" << sizeof(r.genes) << "  predictive_bytes=" << sizeof(brine::Trace) << '\n';
    }
    if (image.investigates) {
        state_bytes += sizeof(image.inquiry) + (image.field_speech.count ? sizeof(image.field_speech) : 0);
        inherited_bytes += 2 * sizeof(Word);
        std::cout << "inquiry_bytes=" << sizeof(image.inquiry)
                  << "  surveyed_substrates=" << std::popcount(image.inquiry.sampled & 65535u)
                  << "  field_questions=" << image.field_speech.count
                  << "  catalyst_contacts=" << image.inquiry.chemistry << '\n';
    }
    if (image.projects) {
        state_bytes += sizeof(image.forecast);
        std::cout << "spatial_prediction_bytes=" << sizeof(image.forecast)
                  << "  seen_face_pairs=" << std::popcount(image.forecast.appetite >> 16)
                  << "  sampled_muscles=" << std::popcount((image.forecast.appetite >> 8) & 255)
                  << "  learned_terminals=" << image.forecast.contacts << '\n';
    }
    if (image.handling) {
        state_bytes += sizeof(image.worker);
        inherited_bytes += sizeof(material::Genes);
    }
    if (image.grows_cells) {
        state_bytes += sizeof(image.cortex);
        inherited_bytes += sizeof(flood::GrowthGenes);
    }
    if (image.delayed_pods) {
        state_bytes += sizeof(image.pod_organ);
        inherited_bytes += sizeof(pods::Inheritance);
    }
    if (image.riverine) {state_bytes+=sizeof(image.resident);inherited_bytes+=sizeof(flood::Germ)-sizeof(Genome)-sizeof(RecollectionGenes);}
    if (image.grounded) {state_bytes+=sizeof(image.referent);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.procedural) {state_bytes+=sizeof(image.practice);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.composes) {state_bytes+=sizeof(composition::Mind);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.sequential) {state_bytes+=sizeof(SequenceMind);inherited_bytes+=sizeof(RecollectionGenes);}
    if (image.causal) {state_bytes+=sizeof(Agency);inherited_bytes+=sizeof(RecollectionGenes);}
    inherited_bytes+=sizeof(Word)*(Word(image.attends)+Word(image.orients));
    std::cout << "cells=" << 32 * a.tiles << "  learned_ticks=" << a.ticks << "  synapse_bits=" << a.bits
              << "  developed_cells="
              << 32 * (image.grows_cells ? image.cortex.active : living_tiles(a.tiles, a.genes.development))
              << "  state_bytes=" << state_bytes << "  germline_bytes=" << inherited_bytes
              << "  inherited_delay=" << (a.delayed_path ? 1 + ((a.genes.edges[0] >> 10) & 7) : 1) << '\n';
    std::cout << "effector_ports=" << std::hex << std::setfill('0');
    for (Word motor=0; motor<8; ++motor)
        std::cout << (motor ? "," : "") << std::setw(2) << ((a.genes.effectors[motor/4]>>(8*(motor&3))) & 255);
    std::cout << std::dec << std::setfill(' ') << '\n';
    if (image.riverine) std::cout<<"floodplain_body="<<image.resident.animal.body.shape<<"  neural_populations="<<image.resident.germ.module_count
        <<"  heard_in_field="<<image.resident.animal.witnesses<<"  source_tick="<<image.resident.tick<<'\n';
    if (image.grounded) std::cout<<"grounded_forms="<<image.referent.count<<"  question_frames="<<image.referent.constructions<<"  grounded_bytes="<<sizeof(image.referent)<<'\n';
    if (image.procedural) std::cout<<"procedural_cells="<<image.practice.circuit.count<<"  procedural_bytes="<<sizeof(image.practice)<<"  learned_code_bytes="<<image.practice.circuit.bytes<<'\n';
    if (image.composes) std::cout<<"compositional_fields="<<image.composition.anatomy.fields_n<<"  constructions="<<image.composition.anatomy.phrases_n<<"  live_contacts="<<image.composition.memory.n<<"  compositional_bytes="<<sizeof(composition::Mind)<<'\n';
    if (image.sequential) std::cout<<"developmental_cells="<<image.sequence.map.size<<"  developmental_muscles="<<image.sequence.motors
        <<"  developmental_sensations="<<image.sequence.senses<<"  developmental_events="<<image.sequence.events<<"  developmental_bytes="<<sizeof(SequenceMind)<<'\n';
    if (image.causal) {
        Word compiled=0;for (auto &row:image.agency.model.fields) for (auto &f:row) compiled+=f.compiled!=0;
        std::cout<<"causal_fields="<<traits*operations<<"  consolidated_fields="<<compiled<<"  observed_plots="<<std::popcount(image.agency.observed)
                 <<"  observed_mask="<<image.agency.observed<<"  remembered_actions="<<image.agency.intention.length<<"  learned_explanation="<<Word(image.agency.explanation.length)
                 <<"  conditional_phases="<<Word(image.agency.conditional.length)<<"  taste_observations="<<image.agency.appetite.bites
                 <<"  acquired_goals="<<std::popcount(image.agency.assembly.active)<<"  agency_bytes="<<sizeof(Agency)<<'\n';
    }
    if (image.conceptual) {
        Word names=0,routes=0,compiled=0;
        for (auto mask:image.concepts.referents) names+=std::has_single_bit(mask);
        for (auto &w:image.concepts.words) {routes+=w.form!=0;compiled+=w.walk!=0;}
        std::cout<<"named_objects="<<names<<"  relational_words="<<routes<<"  compiled_relations="<<compiled
                 <<"  concept_rules="<<std::hex<<image.concepts.genes.low<<','<<image.concepts.genes.high<<','<<image.concepts.genes.expression<<std::dec<<'\n';
    }
    if (image.reports) {
        Word forms=0;for (Count sound:image.report.syllables) forms+=sound!=0;
        std::cout<<"speech_forms="<<forms<<"  speech_capacity="<<report_words<<"  event_history="<<image.report.history.recent
                 <<"  earlier_event="<<image.report.history.earlier<<"  speech_rules="<<std::hex<<image.report.genes.low<<','
                 <<image.report.genes.high<<','<<image.report.genes.expression<<std::dec<<'\n';
    }
    if (image.conjoins) std::cout<<"joint_expression=0x"<<std::hex<<image.conjunctions.gates.low<<"  joint_inhibition=0x"
                                 <<image.conjunctions.gates.high<<"  joint_dynamics=0x"<<image.conjunctions.gates.expression<<std::dec<<'\n';
    if (image.attends) {
        Word forms=0;for (Word i=0;i<8;++i) forms+=bool((image.attention.contacts[i/2]>>(16*(i&1)))&65535);
        std::cout<<"attention_forms="<<forms<<"  attention_capacity=8  emitted_pitch="<<image.attention.previous
                 <<"  attention_rule=0x"<<std::hex<<image.attention.rule<<std::dec<<'\n';
    }
    if (image.orients) std::cout<<"iris_rule=0x"<<std::hex<<image.iris.rule<<"  iris_contacts=0x"<<image.iris.contacts<<std::dec<<'\n';
    if (image.binds) std::cout<<"binding_plastic=0x"<<std::hex<<image.bindings.genes.plastic<<"  binding_competition=0x"<<image.bindings.genes.competition
                             <<"  binding_exploration=0x"<<image.bindings.genes.exploration<<"  source_strengths=0x"<<image.bindings.support<<std::dec<<'\n';
    if (image.imagines) std::cout<<"action_fields="<<std::count_if(std::begin(image.imagery.weights),std::end(image.imagery.weights),[](Word w){return w!=0;})
                               <<"  action_capacity=6  transition_contacts=96\n";
    if (memory) std::cout<<"visual_contacts=16  visual_rule_low=0x"<<std::hex<<memory->genes.low
                         <<"  visual_rule_high=0x"<<memory->genes.high<<"  visual_expression=0x"<<memory->genes.expression
                         <<"  visual_weights=0x"<<memory->weights<<std::dec<<'\n';
    if (ear) {
        Word bound=0;
        for (Word j=0;j<16;++j) bound+=bool((ear->efficacy[j/8]>>(4*(j&7)))&15);
        std::cout<<"auditory_forms="<<bound<<"  auditory_capacity=16  auditory_rule=0x"<<std::hex<<ear->rule<<std::dec<<'\n';
    }
    if (trace) std::cout << "trace_plastic=0x" << std::hex << trace->genes.plastic << "  trace_charge=0x" << trace->genes.charge << "  trace_attention=0x" << trace->genes.attention << std::dec << "  recent_events=" << trace->recent << "  held_event=" << (trace->held&63) << (navigation>=7 ? "  supported_event=" : "  adaptation_event=") << ((trace->held>>6)&63) << "  conflict_sites=" << std::popcount(trace->conflicts) << '\n';
    if (map) std::cout << "map_plastic=0x" << std::hex << map->genes.plastic << "  map_wiring=0x" << map->genes.wiring
                       << "  spread=0x" << map->genes.spread << "  map_currents=0x" << map->genes.currents << std::dec
                       << "  pulse_budget=" << 1+((map->genes.spread>>8)&15) << '\n';
    if (food) {
        std::cout << "metabolism=" << (metabolism==2 ? "food-fuel" : "balanced") << "  outcome_plastic=0x" << std::hex << food->genes.plastic << "  wiring=0x" << food->genes.wiring
                  << "  valuation=0x" << food->genes.valuation << "  gates=0x" << food->genes.gates << "  associations=0x"
                  << (food->associations&65535) << "  pouches=0x" << contents(food->stores) << std::dec
                  << "  handling_ticks=" << chewing(food->stores) << '\n';
    }
    if (!a.has_dendrites) return;
    Word primary = a.genes.expression >> 16, conditional = a.layer.genes.conditional, mod = a.layer.genes.modulation;
    Word states = 1u << a.bits;
    std::cout << "primary_table=0x" << std::hex << primary;
    if (a.bits == 2) std::cout << "  conditional_table=0x" << (conditional & 65535) << "  state_table=0x" << mod;
    std::cout << std::dec << "\nreceptor  motor  gain";
    for (Word i=0; i<states; ++i) std::cout << "  from_" << i;
    std::cout << '\n';
    for (Word receptor=0; receptor<2; ++receptor) for (Word motor=0; motor<2; ++motor) for (Word gain=0; gain<2; ++gain) {
        std::cout << std::setw(8) << receptor << std::setw(7) << motor << std::setw(6) << gain;
        for (Word i=0; i<states; ++i) {
            Word address = (i & 1) + 2*receptor + 4*motor + 8*gain;
            Word next = ((i < 2 ? primary : conditional) >> address) & 1;
            if (states == 4) next |= ((mod >> (address + 16*(i >> 1))) & 1) << 1;
            std::cout << std::setw(8) << next;
        }
        std::cout << '\n';
    }
    if (!a.has_relations) return;
    const LinkGenes &g = links.genes;
    std::cout << "pair_table=0x" << std::hex << (g.plastic & 65535) << "  recall_table=0x" << (g.plastic >> 16)
              << "  convergence_tables=0x" << links.convergence << "  switches=0x" << g.switches
              << "  routing=0x" << g.wiring << std::dec << '\n'
              << "sensory_rotation=" << ((g.wiring >> 6) & 31) << "  proprioceptive_bits=" << (g.wiring & 7)
              << ',' << ((g.wiring >> 3) & 7) << "  direct_gain=" << ((g.wiring >> 13) & 15)
              << "  traversal_gain=" << ((g.wiring >> 17) & 15) << "  recall_gain=" << ((g.wiring >> 21) & 15) << '\n';
    for (Word code=0; code<4; ++code) {
        std::cout << "motor_fibre=" << code << "  phases=";
        for (Word phase=0; phase<4; ++phase) std::cout << (phase ? "," : "") << route_fibre(g.switches,code,phase);
        std::cout << '\n';
    }
    if (a.has_relations >= 2) {
        Word allele = (links.convergence >> 8) & 3;
        std::cout << "propagation_steps=" << (allele ? (4u<<allele)-1 : 0) << "  distal_gain=" << 2*(1+((links.convergence>>12)&7)) << '\n';
    }
    if (a.has_relations==3) {
        auto p=contexts.genes;
        std::cout << "context_plastic=0x" << std::hex << p.plastic << "  evidence=0x" << p.evidence
                  << "  dynamics=0x" << p.dynamics << "  coordination=0x" << p.coordination << "  receptors=0x" << p.receptors
                  << std::dec << "  assemblies=" << 1+((p.coordination>>26)&3)
                  << "  expressed=" << ((contexts.activity>>20)&3) << "  used_mask=" << ((contexts.activity>>22)&15) << '\n';
        if (a.alternate_program) {
            std::cout << "synaptic_polarity=" << (a.alternate_program==2 ? "evolvable" : "shared") << '\n';
            auto other=expression.genes.alternative;
            std::cout << "alternate_plastic=0x" << std::hex << other.plastic << "  evidence=0x" << other.evidence
                      << "  dynamics=0x" << other.dynamics << "  coordination=0x" << other.coordination << "  receptors=0x" << other.receptors
                      << "  expression_gate=0x" << expression.genes.gate << "  expression_sensors=0x" << expression.genes.sensors
                      << std::dec << "  last_program=" << (expression.history&1) << '\n';
        }
    }
}

std::optional<Encounter> event(std::string line) {
    auto comment = line.find('#');
    if (comment != std::string::npos) line.resize(comment);
    std::istringstream input(line);
    std::string op, raw, field, a, b, tail;
    if (!(input >> op)) return {};
    if (!(input >> raw >> field)) throw std::invalid_argument("expected a receptor pattern and ambient field");
    Encounter e{number(raw,~0u), number(field,~0u),0,0,0,EventKind::trial};
    if (op=="savour") {
        if (!std::has_single_bit(e.receptors)) throw std::invalid_argument("savour needs one visible landmark");
        e.kind=EventKind::savour;e.value=number(field,3)+1;e.ambient=0;
    } else if (op=="stride") {
        e.kind=EventKind::stride;
        if (input>>a) {
            if (!(input>>b)) throw std::invalid_argument("stride needs both a need mask and an ingested food code (0 absent, 1..4 present)");
            e.mask=number(a,15);e.value=number(b,4);
        }
    } else if (op=="travel" || op=="orient" || op=="pace" || op=="face") {
        if (!std::has_single_bit(e.receptors)) throw std::invalid_argument("a doorway has one visible landmark");
        e.kind=(op=="travel" || op=="pace") ? EventKind::travel : EventKind::orient;
        Word activate=op=="pace" || op=="face" ? 4u : 8u;
        if (e.kind==EventKind::travel) {
            if (!(input>>a)) throw std::invalid_argument("travel needs four packed exit destinations");
            e.mask=number(a,~0u);e.value=activate;
            if (e.mask&0xe0e0e0e0u) throw std::invalid_argument("a destination uses a landmark from 0 through 31");
            if (input>>b) e.signal=number(b,15)<<1;
        } else if (input>>a) { e.mask=activate|3;e.value=activate|number(a,3);e.signal=1; if (input>>b) e.signal|=number(b,15)<<1; }
    } else if (op=="taste") {
        e.kind=EventKind::taste; e.mask=number(raw,3)+1; e.value=number(field,3)+1; e.receptors=0; e.ambient=0;
    } else if (op=="sample" || op=="seek") {
        e.kind=op=="sample" ? EventKind::sample : EventKind::seek;
        if (!(input>>a>>b)) throw std::invalid_argument("expected a need mask and a surface");
        e.signal=number(a,15); e.mask=number(b,255); e.value=256;
        if (input>>a) e.value=number(a,255);
        if (e.kind==EventKind::seek && e.value==256) throw std::invalid_argument("a scored food probe needs a recipe");
    } else if (op == "step") {
        if (!(input >> a) || (a != "+" && a != "-" && a != "0")) throw std::invalid_argument("step expects +, - or 0");
        e.kind = EventKind::step; e.signal = a == "+" ? 1 : (a == "-" ? 2 : 0);
    } else if (op == "trial" || op == "probe") {
        e.kind = op == "probe" ? EventKind::probe : EventKind::trial;
        if (input >> a) {
            if (!(input >> b)) throw std::invalid_argument("expected an effector mask and value");
            e.mask = number(a,255); e.value = number(b,255);
            if ((e.value & ~e.mask) != 0) throw std::invalid_argument("effector value sets bits outside its mask");
            e.signal = 1; // score this event's output
        } else if (e.kind == EventKind::trial) throw std::invalid_argument("trial needs an effector mask and value");
    } else throw std::invalid_argument("unknown sensory event");
    if (input >> tail) throw std::invalid_argument("unexpected extra event field");
    return e;
}
void benchmark(Session &session,AgentImage &image,const Options &o,std::istream &input,DevicePtr source,DevicePtr connections,DevicePtr organ) {
    if (!image.navigation || !o.save.empty()) throw std::invalid_argument("a navigation benchmark reads a saved navigation circuit");
    std::vector<Encounter> questions;
    std::string line;
    while (std::getline(input,line)) if (auto e=event(line)) {
        if (e->kind!=EventKind::orient || !(e->signal&1)) throw std::invalid_argument("the benchmark takes scored orient events");
        questions.push_back(*e);
    }
    if (questions.empty() || questions.size()>65536) throw std::invalid_argument("the benchmark needs 1..65536 scored orient events");
    auto &d=session.d;
    Word n=o.copies,body=Word(payload(image.brain.tiles)),synapses=association_stride(image.brain.has_relations,1+Word(image.brain.alternate_program!=0));
    Word perceptual=sizeof(OutcomeLayer)+map_bytes(image.navigation),decisions=o.decisions,kinds=Word(questions.size()),cut=o.map_cut;
    Temporary bodies(d,size_t(n)*body),circuits(d,size_t(n)*synapses),maps(d,size_t(n)*perceptual),
              cues(d,questions.size()*sizeof(Encounter)),outputs(d,size_t(n)*2*sizeof(Word));
    d.check(d.upload(cues.ptr,questions.data(),questions.size()*sizeof(Encounter)),"upload benchmark cues");
    Function copy{},test{};
    session.bind(copy,"replicate_words",Image::mapactors);
    session.bind(test,"bearings"+std::to_string(image.brain.has_relations)+"_"+std::to_string(image.brain.tiles),image.navigation>=4 ? Image::sequence_actors : Image::mapactors);
    auto broadcast=[&](DevicePtr src,DevicePtr dest,Word bytes) {
        if (!bytes) return;
        Word words=bytes/sizeof(Word);void *args[]{&src,&dest,&words,&n};
        d.check(d.launch(copy,(n*words+255)/256,1,1,256,1,1,0,nullptr,args,nullptr),"replicate acquired neural states");
    };
    broadcast(source,bodies.ptr,body);broadcast(connections,circuits.ptr,synapses);broadcast(organ,maps.ptr,perceptual);
    auto run=[&](Word count) {
        void *args[]{&bodies.ptr,&circuits.ptr,&maps.ptr,&cues.ptr,&outputs.ptr,&n,&count,&kinds,&body,&synapses,&image.navigation,&cut};
        d.check(d.launch(test,(n*image.brain.tiles+127)/128,1,1,128,1,1,0,nullptr,args,nullptr),"evaluate independent learned actors");
    };
    struct Timer {
        Driver &d; Event start{},end{};
        ~Timer() { if (start) d.event_free(start);if (end) d.event_free(end); }
    } timer{d};
    d.check(d.event_new(&timer.start,0),"allocate benchmark timer");d.check(d.event_new(&timer.end,0),"allocate benchmark timer");
    int registers=0,local=0;d.check(d.func_attr(&registers,4,test),"read benchmark registers");d.check(d.func_attr(&local,3,test),"read benchmark local memory");
    run(std::min(4u,decisions));
    std::vector<Word> result(size_t(n)*2);
    for (Word repeat=1;repeat<=3;++repeat) {
        d.check(d.record(timer.start,nullptr),"start neural benchmark");run(decisions);
        d.check(d.record(timer.end,nullptr),"stop neural benchmark");d.check(d.wait(timer.end),"finish neural benchmark");
        float ms{};d.check(d.elapsed(&ms,timer.start,timer.end),"time neural decisions");
        d.check(d.download(result.data(),outputs.ptr,result.size()*sizeof(Word)),"verify neural choices");
        Count correct=0,fingerprint=0,choices=Count(n)*decisions;
        for (Word i=0;i<n;++i) { correct+=result[2*i];fingerprint+=mix(result[2*i+1]^i); }
        if (repeat==3) if (auto *j=journal::current()) {
            j->metrics["replicas"]=n;j->metrics["decisions"]=choices;
            j->scores.push_back({"Batched orientation",correct,choices});
        }
        std::cout << "repeat=" << repeat << "  actors=" << n << "  choices=" << choices << "  correct=" << correct
                  << "  state_bytes=" << body+synapses+perceptual << "  replica_bytes=" << Count(n)*(body+synapses+perceptual)
                  << "  registers=" << registers << "  local_bytes=" << local << "  fingerprint=" << fingerprint
                  << "  GPU_ms=" << std::fixed << std::setprecision(3) << ms << "  Mchoices/s=" << double(choices)/(1000*ms) << '\n';
    }
}
} // namespace

int actor_command(int argc, char **argv) {
    if (argc == 3 && std::string_view(argv[2]) == "--help") { std::cout << usage; return 0; }
    auto o = parse(argc,argv);
    AgentImage image;
    if (!o.from.empty()) {
        Word site{};
        image.brain = hatch_from_habitat(o.from.c_str(),o.site.value_or(~0u),site,image.links,image.contexts,image.expression,o.resident,&image.food,&image.balanced,&image.map,&image.navigation,&image.trace);
        std::cout << (o.resident ? "resident_site=" : "donor_site=") << site << '\n';
    } else image.read(o.load);
    if (o.inspect) {
        inspect(image);
        return 0;
    }
    std::ifstream file;
    if (!o.lesson.empty()) { file.open(o.lesson); if (!file) throw std::runtime_error("cannot open sensory lesson " + o.lesson); }
    std::istream &input = o.lesson.empty() ? std::cin : file;
    bool interactive = o.lesson.empty() && ::isatty(STDIN_FILENO);
    Session session; session.open();
    Function run{};
    if (image.balanced) session.bind(run,"food"+std::to_string(image.brain.has_relations)+"_actor"+std::to_string(image.brain.tiles),image.navigation>=4 ? Image::sequence_actors : image.navigation ? Image::mapactors : Image::foodactors);
    else session.bind(run,std::string(neural_prefix(image.brain.has_relations))+"actor"+std::to_string(image.brain.tiles),Image::actors);
    constexpr Word batch = 256;
    Temporary brain(session.d, payload(image.brain.tiles)), events(session.d, batch*sizeof(Encounter)), results(session.d,batch*sizeof(Response));
    size_t synaptic_bytes=association_stride(image.brain.has_relations,1+Word(image.brain.alternate_program!=0));
    Temporary links(session.d,synaptic_bytes),food(session.d,(image.balanced ? sizeof(OutcomeLayer) : 0)+map_bytes(image.navigation));
    if (image.navigation>=4) session.d.check(session.d.upload(food.ptr+sizeof(OutcomeLayer)+sizeof(MapLayer),&image.trace,sizeof(TraceLayer)),"restore temporal memory");
    if (image.navigation) session.d.check(session.d.upload(food.ptr+sizeof(OutcomeLayer),&image.map,sizeof(MapLayer)),"restore navigation contacts");
    if (food.ptr) session.d.check(session.d.upload(food.ptr,&image.food,sizeof(image.food)),"restore food associations");
    if (links.ptr) session.d.check(session.d.upload(links.ptr,&image.links,sizeof(image.links)),"restore recurrent associations");
    if (image.brain.has_relations==3) session.d.check(session.d.upload(links.ptr+sizeof(image.links),&image.contexts,sizeof(image.contexts)),"restore contextual associations");
    if (image.brain.alternate_program) session.d.check(session.d.upload(links.ptr+sizeof(image.links)+sizeof(image.contexts),&image.expression,sizeof(image.expression)),"restore expression state");
    session.d.check(session.d.upload(brain.ptr,&image.brain,payload(image.brain.tiles)), "restore individual");
    auto record=[&] {
        if (auto *j=journal::current()) {
            j->device = session.name;
            j->metrics["neurons"] = 32 * (image.grows_cells ? image.cortex.active : image.brain.tiles);
            j->metrics["body"] = image.brain.genes.body;
            j->metrics["tape"]=1+(image.brain.genes.expression&7);
            j->metrics["state_bytes"]=payload(image.brain.tiles)+synaptic_bytes+(image.balanced ? sizeof(image.food) : 0)+map_bytes(image.navigation);
            if (image.navigation>=4) {
                session.d.check(session.d.download(&image.trace,food.ptr+sizeof(OutcomeLayer)+sizeof(MapLayer),sizeof(TraceLayer)),"observe temporal contacts");
                j->metrics["held_event"]=image.trace.held&63;
                j->metrics[image.navigation>=7 ? "supported_event" : "adaptation_event"]=(image.trace.held>>6)&63;
                for (Word cue=0;cue<32;++cue) if (image.trace.sources[cue]&32) for (Word action=0;action<4;++action) {
                    Word contact=(image.trace.predictions[cue]>>(8*action))&255;
                    Word earlier=cue;
                    if (image.navigation>=6 && (image.trace.genes.attention&(1u<<11))) earlier^=trace_offset(image.trace.genes,image.trace.sources[cue]&31);
                    Word support=image.navigation>=7 ? image.trace.sources[cue]>>6 : 0;
                    if (contact>>5) j->conditional.push_back(earlier|((image.trace.sources[cue]&31)<<5)|(action<<10)|(contact<<12)|(support<<20));
                }
            }
            if (image.navigation) {
                session.d.check(session.d.download(&image.map,food.ptr+sizeof(OutcomeLayer),sizeof(MapLayer)),"observe acquired navigation state");
                Word flag=5+((image.map.genes.wiring>>20)%3);
                for (Word a=0;a<32;++a) for (Word m=0;m<4;++m) {
                    Word contact=(image.map.contacts[a]>>(8*m))&255;
                    if (contact&(1u<<flag)) j->transitions.push_back(a|((contact&31)<<5)|(m<<10));
                }
            }
        }
    };
    if (o.copies) { benchmark(session,image,o,input,brain.ptr,links.ptr,food.ptr);record();return 0; }
    std::cout << session.name << "  cells=" << 32*image.brain.tiles << "  ticks=" << image.brain.ticks
              << "  state_bytes=" << payload(image.brain.tiles)+synaptic_bytes+(image.balanced ? sizeof(image.food) : 0)+map_bytes(image.navigation) << '\n' << std::flush;
    std::array<Encounter,batch> tape;
    std::array<Response,batch> response;
    std::array<Word,batch> stages{};
    std::vector<journal::Score> phase_scores;
    Word stage=~0u;
    Word ticks = image.brain.ticks, count = 0;
    Count ordinal = 0, trials = 0, correct = 0, probes = 0, matches = 0;
    auto flush = [&] {
        if (!count) return;
        session.d.check(session.d.upload(events.ptr,tape.data(),count*sizeof(Encounter)), "upload sensory events");
        Word frozen=count>1 && std::all_of(tape.begin(),tape.begin()+count,[](const Encounter &e){return e.kind==EventKind::orient;});
        if (image.grows_cells)
            frozen |= Word(image.cortex.active) << 1;
        ActorBatch layout{1,0,0,0};
        void *args[]{&brain.ptr,&events.ptr,&results.ptr,&count,&links.ptr,&food.ptr,&image.navigation,&o.map_cut,&frozen,&layout};
        session.d.check(session.d.launch(run,1,1,1,image.brain.tiles,1,1,0,nullptr,args,nullptr), "run neural actor");
        session.d.check(session.d.download(response.data(),results.ptr,count*sizeof(Response)), "read actor choices");
        for (Word i = 0; i < count; ++i) {
            auto e = tape[i]; auto r = response[i];
            if (auto *j=journal::current()) {
                j->action(r.action,r.ports,Word(e.kind));
                if (image.navigation && e.kind==EventKind::travel) {
                    Word wiring=image.map.genes.wiring,shift=(wiring>>9)&31;
                    Word from=(Word(__builtin_ctz(e.receptors))+shift)&31,to=(r.received+shift)&31;
                    Word motor=((r.ports>>(wiring&7))&1)|(((r.ports>>((wiring>>3)&7))&1)<<1);
                    ++j->journeys[from|(to<<5)|(motor<<10)];
                    ++j->metrics["travel"];j->metrics["displacements"]+=from!=to;
                }
            }
            constexpr const char *names[]{"trial","step","probe","sample","seek","taste","travel","orient","stride","savour"};
            const char *kind=names[Word(e.kind)];
            if (e.kind==EventKind::savour) { std::cout << "savour=" << ordinal++ << "  food=" << e.value-1 << '\n';continue; }
            if (e.kind==EventKind::travel) { std::cout << "travel=" << ordinal++ << "  motor=" << r.action << "  effector=" << r.ports << "  arrival=" << r.received << '\n';continue; }
            if (e.kind==EventKind::taste) { std::cout << kind << '=' << ordinal++ << "  flavour=" << e.mask-1 << "  food=" << e.value-1 << '\n'; continue; }
            std::cout << kind << '=' << ordinal++ << "  motor=" << r.action << "  effector=0x"
                      << std::hex << std::setw(2) << std::setfill('0') << r.ports << std::dec << std::setfill(' ')
                      << "  received=" << ((r.received&3)==1 ? "+" : (r.received&3)==2 ? "-" : "0");
            if (e.kind==EventKind::sample || e.kind==EventKind::seek) {
                Word flavour=(r.received>>2)&7, nutrient=(r.received>>5)&7;
                std::cout << "  flavour=" << (flavour ? std::to_string(flavour-1) : "none")
                          << "  food=" << (nutrient ? std::to_string(nutrient-1) : "none");
            }
            if ((e.kind==EventKind::trial || e.kind==EventKind::probe || e.kind==EventKind::orient) ? e.signal!=0 : e.value<256 && (e.kind==EventKind::sample || e.kind==EventKind::seek)) {
                std::cout << "  match=" << r.matched;
                if (stages[i]!=~0u) {auto &score=phase_scores[stages[i]];score.correct+=r.matched;++score.total;}
                if (e.kind==EventKind::trial || e.kind==EventKind::sample) { ++trials; correct += r.matched; }
                else { ++probes; matches += r.matched; }
            }
            std::cout << '\n';
        }
        std::cout << std::flush; count = 0;
    };
    std::string line;
    Count lineno = 0;
    while (std::getline(input,line)) {
        ++lineno;
        if (journal::current() && line.starts_with("# phase ")) {
            std::string label=line.substr(8);
            auto end=label.find_last_not_of(" \t\r");
            if (end==std::string::npos) {stage=~0u;continue;}
            label.erase(end+1);
            auto found=std::find_if(phase_scores.begin(),phase_scores.end(),[&](const auto &s){return s.name==label;});
            if (found!=phase_scores.end()) stage=Word(found-phase_scores.begin());
            else if (phase_scores.size()<64) {stage=Word(phase_scores.size());phase_scores.push_back({std::move(label),0,0});}
            else {stage=64;if (phase_scores.size()==64) phase_scores.push_back({"Other marked choices",0,0});}
            continue;
        }
        std::optional<Encounter> e;
        try { e = event(line); }
        catch (const std::invalid_argument &error) { throw std::invalid_argument("lesson line "+std::to_string(lineno)+" "+error.what()); }
        if (!e) continue;
        if (e->kind==EventKind::savour && !image.navigation) throw std::invalid_argument("a landmark taste needs a navigation circuit");
        if (e->kind>=EventKind::sample && e->kind<=EventKind::taste && !image.balanced) throw std::invalid_argument("food events need an individual with a balanced metabolism");
        if (e->kind!=EventKind::probe && e->kind!=EventKind::seek && e->kind!=EventKind::orient) {
            if (ticks == ~0u) throw std::overflow_error("neural clock is full");
            ++ticks;
        }
        stages[count]=stage;tape[count++] = *e;
        if (interactive || count == batch) flush();
    }
    if (input.bad()) throw std::runtime_error("error reading sensory stream");
    flush();
    std::cout << "ticks=" << ticks << "  training_matches=" << correct << '/' << trials
              << "  probe_matches=" << matches << '/' << probes << '\n';
    record();
    if (auto *j=journal::current()) {
        j->metrics["ticks"]=ticks;
        if (trials) j->scores.push_back({"Training choices",correct,trials});
        if (probes) j->scores.push_back({"Frozen choices",matches,probes});
        for (const auto &score:phase_scores) if (score.total) {j->scores.push_back(score);++j->metrics["score_phases"];}
    }
    if (!o.save.empty()) {
        if (image.navigation>=4) session.d.check(session.d.download(&image.trace,food.ptr+sizeof(OutcomeLayer)+sizeof(MapLayer),sizeof(TraceLayer)),"save temporal memory");
        if (image.navigation) session.d.check(session.d.download(&image.map,food.ptr+sizeof(OutcomeLayer),sizeof(MapLayer)),"save navigation contacts");
        session.d.check(session.d.download(&image.brain,brain.ptr,payload(image.brain.tiles)), "save individual state");
        if (food.ptr) session.d.check(session.d.download(&image.food,food.ptr,sizeof(image.food)),"save food associations");
        if (image.brain.ticks != ticks) throw std::runtime_error("neural event clock mismatch");
        if (links.ptr) session.d.check(session.d.download(&image.links,links.ptr,sizeof(image.links)),"save recurrent associations");
        if (image.brain.has_relations==3) session.d.check(session.d.download(&image.contexts,links.ptr+sizeof(image.links),sizeof(image.contexts)),"save contextual associations");
        if (image.brain.alternate_program) session.d.check(session.d.download(&image.expression,links.ptr+sizeof(image.links)+sizeof(image.contexts),sizeof(image.expression)),"save expression state");
        write_checked(o.save,image);
    }
    return 0;
}
} // namespace axion::life
