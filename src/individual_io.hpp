#pragma once
#include "axion/siphon_words.hpp"
#include "axion/rattle.hpp"
#include "axion/braid_gate.hpp"
#include "axion/braid_discourse.hpp"
#include "axion/narrative.hpp"
#include "axion/hollow_words.hpp"
#include "axion/actor.hpp"
#include "axion/auditory.hpp"
#include "axion/attention.hpp"
#include "axion/binding.hpp"
#include "axion/recollection.hpp"
#include "axion/report.hpp"
#include "axion/causal.hpp"
#include "axion/capillary_voice.hpp"
#include "axion/material_signal.hpp"
#include "axion/signal_voice.hpp"
#include "axion/sequence.hpp"
#include "composition_storage.hpp"
#include "procedure_storage.hpp"
#include "grounding_storage.hpp"
#include "axion/floodplain.hpp"
#include "field_voice_storage.hpp"
#include "life_io.hpp"
#include <bit>
#include <cstddef>
#include <cstring>
#include <fstream>

namespace axion::life::storage {
inline size_t payload(Word tiles) { return offsetof(Individual, tissue) + size_t(tiles)*sizeof(Tissue); }
struct AgentImage {
    struct { char magic[8]; Word version, bytes; } header{"AXMIND1",1,0};
    Individual brain{};
    AssociationLayer links{};
    ContextLayer contexts{};
    ExpressionLayer expression{};
    OutcomeLayer food{};
    bool auditory=false;AuditoryLayer ear{};
    bool recollects=false;Recollection recollection{};
    bool attends=false,orients=false;AttentiveEar attention{};Iris iris{};
    bool imagines=false;Imagery imagery{};
    bool binds=false;Bindings bindings{};
    bool conjoins=false;Conjunctions conjunctions{};
    bool reports=false;Reports report{};
    bool conceptual=false;Concepts concepts{};
    bool causal=false;Agency agency{};
    siphon::Vocabulary native_words{};
    bool narrates_floats=false;braid::ReportSaved braid_report{};
    bool describes=false;utterance::Saved accounts{};
    bool hydraulic=false;capillary::Saved channels{};
    bool material_signals=false,compound_signals=false,voices_signs=false;capillary::SignalVoice signal_voice{};capillary::Tissue appetitive{};Word compound_contacts=0;
    bool registers_gates=false;braid::GateSaved sluice{};
    bool binds_floats=false;braid::Saved braided{};
    bool hears_banks=false; rattle::Saved rattle_organ{};
    bool folds_reeds=false;runnel::Saved reed{};
    bool practises=false;practice::Saved training{};
    bool invents=false;linkage::Saved mechanism{};
    bool values_effort=false;effort::Saved prospects{};
    bool ranges = false;
    hollow::Saved path_memory{};
    hollow::Lexicon path_speech{};
    bool potter = false;
    clay::Saved ceramic{};
    bool calling = false;
    siphon::Saved calls{};
    bool saline = false;
    brine::Saved brine_mind{};
    material::FieldLanguage field_speech{};
    bool investigates = false;
    material::Inquiry inquiry{};
    bool projects = false;
    material::Foresight forecast{};
    bool handling = false;
    material::Worker worker{};
    bool grows_cells = false;
    flood::Cortex cortex{};
    bool delayed_pods = false;
    pods::Animal pod_organ{};
    bool riverine=false;flood::Resident resident{};
    bool grounded=false;grounding::Mind referent{};
    bool procedural=false;procedure::Mind practice{};
    bool sequential=false;SequenceMind sequence{};
    bool composes=false;composition::Mind composition{};
    Word balanced=0,navigation=0; MapLayer map{};TraceLayer trace{};
    void specimen(const std::string &path,const std::string &role,Count checksum=0) const {
        auto *report=journal::current();if (!report) return;
        for (const auto &p:report->specimens) if (p.path==path && p.role==role && p.checksum==checksum) return;
        journal::Specimen p;p.path=path;p.role=role;p.checksum=checksum;auto &q=p.measures;
        q["body"]=brain.genes.body;q["tape"]=1+(brain.genes.expression&7);q["neurons"]=32*brain.tiles;q["ticks"]=brain.ticks;
        p.circuits={"Somatic spiking circuit"};
        q["effectors_low"]=brain.genes.effectors[0];q["effectors_high"]=brain.genes.effectors[1];
        Word active = grows_cells ? cortex.active : living_tiles(brain.tiles, brain.genes.development);
        q["developed_cells"] = 32 * active;
        if (grows_cells) {
            q["neurons"] = 32 * active;
            q["natal_cells"] = 32 * cortex.natal;
            q["peak_cells"] = 32 * cortex.peak;
            q["capacity_cells"] = 32 * brain.tiles;
        }
        for (Word tile = 0; tile < active; ++tile)
            q["spikes_" + std::to_string(tile)] = brain.tissue[tile].spike;
        auto circuit=[&](bool present,const char *name){if (present) p.circuits.emplace_back(name);};
        auto sense=[&](bool present,const char *name){if (present) p.senses.emplace_back(name);};
        sense(auditory,"Acoustic receptor bank");sense(orients,"Directional retina");sense(navigation!=0,"Spatial landmarks");
        sense(balanced!=0,"Appetitive outcomes");sense(procedural && practice.muscle.forms,"Muscle feedback");
        sense(procedural && practice.circuit.guard_n,"Conditional light sensing");
        circuit(brain.has_dendrites,"Dendritic contacts");circuit(brain.has_relations,"Relational synapses");circuit(navigation!=0,"Route memory");
        circuit(recollects,"Episodic recall");circuit(attends,"Selective attention");circuit(imagines,"Sensory prediction");
        circuit(binds,"Role binding");circuit(conjoins,"Conjunctive memory");circuit(reports,"Vocal reporting");circuit(conceptual,"Relational concepts");
        if(narrates_floats){
            auto &m=braid_report.memory;Word nouns=0,forms=0;
            for(Word i=0;i<braid::sounds_max;++i)nouns+=braid::ident(m,i)!=braid::none;
            for (auto &f : m.forms)
                forms += f.examples != 0 && !braid::sequence_form(f);
            q["event_names"]=nouns;q["question_forms"]=forms;q["remembered_ports"]=std::popcount(m.event.known);
        }
        if(registers_gates){q["pressure_pairs"]=std::popcount(sluice.gates.visited);sense(true,"Blocked or running liquid at a collar");}
        if(binds_floats){sense(true,"Moving pigments and acoustic sequences");circuit(true,"Shared float and motion contacts");
            q["float_capacity"]=braid::capacity(braided.mind.genes);q["float_motor_cells"]=std::popcount(braided.mind.lived&15u);q["float_word_cells"]=std::popcount(Word((braided.mind.lived>>4)&255));}
        if(hears_banks){sense(true,"Tensile pulses and delayed taste");circuit(true,"Timbre-selective contacts");
            q["rattle_contacts"]=rattle::contacts(rattle_organ.mind);q["rattle_timbre"]=(rattle_organ.mind.genes.drum>>16)&3;}
        if(folds_reeds){
            const auto &m=reed.mind;sense(true,"Seen fold and elastic deflection");circuit(true,"Groove, motor and flex coincidences");
            q["flex_contacts"]=runnel::population(m.cells.touched);q["peer_contacts"]=runnel::population(m.cells.witnessed);
            q["private_folds"]=m.practice&65535;q["witnessed_folds"]=m.practice>>16;
        }
        if(values_effort){
            const auto &m=prospects.mind;q["learned_yields"]=std::popcount(m.experience&15);
            q["learned_costs"]=std::popcount((m.experience>>4)&15);q["surface_transitions"]=std::popcount(m.changes.low|m.changes.high);
            q["cost_memory_bytes"]=sizeof(m);circuit(true,"Prospective effort and yield contacts");
        }
        if (ranges) {
            const auto &path=path_memory;
            Word cues=0,occupied=0;
            for(Word j=0;j<path.mind.used;++j)if(hollow::strength(path.marks[j])) {
                cues|=1u<<hollow::identity(path.marks[j]);++occupied;
            }
            q["place_cells"]=occupied;q["place_events"]=cues;q["place_capacity"]=hollow::capacity(path.mind,path.slot_limit);
            q["place_forms"]=path_speech.words.count;
            circuit(true,"Plastic spatial event cells");
            sense(bool(path.mind.genes.compass&(1u<<3)),"Bodily turn impulses");
            sense(bool(path.mind.genes.compass&(1u<<8)),"Local surface recognition");
        }
        if (potter) {
            circuit(true, "Context-selective acoustic assemblies");
            q["available_assemblies"] = clay::capacity(ceramic.mind);
            q["acquired_assemblies"] = clay::count(ceramic.mind);
        }
        if (calling) {
            circuit(true, "Reciprocal acoustic contacts");
            q["native_contact_bytes"] = 12;
            q["native_encounters"] = calls.animal.occasions;
            q["native_food"] = calls.animal.harvest;
        }
        if (saline) {
            circuit(true, "Plastic material predictions");
            q["predictive_cells"] = std::popcount(brine_mind.cortex.genes.active);
            q["predictive_trace_bytes"] = sizeof(brine::Trace);
        }
        if (investigates) {
            circuit(true, "Material investigation");
            q["sampled_substrates"] = std::popcount(inquiry.sampled & 65535u);
        }
        if (projects) {
            circuit(true, "Spatial prediction");
            q["spatial_contact_bytes"] = sizeof(forecast);
        }
        if (handling) {
            sense(true, "Contact geometry");
            circuit(true, "Visuomotor effect contacts");
            q["material_sightings"] = worker.sightings;
            q["material_plastic_edits"] = worker.edits;
        }
        if (delayed_pods) {
            sense(true, "Digestive consequences");
            circuit(true, "Temporal coincidence contacts");
            q["temporal_contacts"] = 256;
        }
        if (riverine) {
            const auto &a=resident.animal;q["body_plan"]=a.body.shape;q["legs"]=a.body.limbs;q["pores"]=a.body.pores;q["eyes"]=a.body.eyes;
            q["modules"]=std::popcount(a.body.shape);q["neural_modules"]=resident.germ.module_count;q["offspring"]=a.offspring;q["generation"]=a.generation;
            sense(bool(a.body.eyes),"Projecting photoreceptors");sense(bool(a.witnesses),"Wild acoustic reception");circuit(true,"Developmental body program");
        }
        if (grounded) {sense(true,"Retinal light field");circuit(true,"Cross-situational grounding");
            q["grounded_forms"]=referent.count;q["grounded_frames"]=referent.constructions;q["grounded_episodes"]=referent.experienced;}
        circuit(causal,"Intervention model");circuit(sequential,"Sensorimotor sequences");circuit(composes,"Compositional memory");circuit(procedural,"Motor procedures");
        if (composes) {q["speech_forms"]=composition.lexicon.n;q["constructions"]=composition.anatomy.phrases_n;}
        if (sequential) {q["sequence_motors"]=sequence.motors;q["sequence_senses"]=sequence.senses;}
        if (procedural) {q["productions"]=practice.circuit.count;q["receptors"]=practice.ear.size;q["gates"]=practice.circuit.guard_n;
            q["recursions"]=practice.recursion.n;q["muscle_forms"]=practice.muscle.forms;q["demonstrations"]=practice.memory.observed;}
        report->specimens.emplace_back(std::move(p));
    }
    template<class F> void chunks(F visit) {
        header.version = navigation ? 9+balanced+2*(navigation-1) : balanced ? 7+balanced : brain.alternate_program ? 5+brain.alternate_program : brain.has_relations ? 2+brain.has_relations : 1;
        if (auditory) header.version|=64;
        if (recollects) header.version|=128;
        if (attends) header.version|=256;
        if (orients) header.version|=512;
        if (imagines) header.version|=1024;
        if (binds) header.version|=2048|4096;
        if (conjoins) header.version|=8192;
        if (reports) header.version|=16384;
        if (conceptual) header.version|=32768;
        if (causal) header.version|=65536|131072|262144|524288;
        if (investigates)
            header.version |= 0x80000000u;
        if (projects)
            header.version |= 1073741824u;
        if (handling)
            header.version |= 536870912u;
        if (grows_cells)
            header.version |= 268435456u;
        if (delayed_pods)
            header.version |= 67108864u | 134217728u;
        if (riverine) {header.version|=33554432;resident.extent=sizeof(resident);}
        if (grounded) {header.version|=16777216;referent.extent=sizeof(referent);}
        if (procedural) {header.version|=8388608;practice.extent=sizeof(practice);}
        if (sequential) header.version|=1048576;
        if (composes) {header.version|=2097152|4194304;composition.extent=sizeof(composition); }
        header.bytes = Word(payload(brain.tiles)+association_stride(brain.has_relations,1+Word(brain.alternate_program!=0))+(balanced ? sizeof(food) : 0)+map_bytes(navigation)+(auditory ? sizeof(ear) : 0)+(recollects ? sizeof(recollection) : 0));
        header.bytes+=(attends ? sizeof(attention) : 0)+(orients ? sizeof(iris) : 0)+(imagines ? sizeof(imagery) : 0)+(binds ? sizeof(bindings) : 0);
        if (conjoins) header.bytes+=sizeof(conjunctions);
        if (reports) header.bytes+=sizeof(report);
        if (conceptual) header.bytes+=sizeof(concepts);
        if (causal) header.bytes+=sizeof(agency);
        if (investigates)
            header.bytes += sizeof(inquiry);
        if (projects)
            header.bytes += sizeof(forecast);
        if (handling)
            header.bytes += sizeof(worker);
        if (grows_cells)
            header.bytes += sizeof(cortex);
        if (delayed_pods)
            header.bytes += sizeof(pod_organ);
        if (riverine) header.bytes+=sizeof(resident);
        if (grounded) header.bytes+=sizeof(referent);
        if (procedural) header.bytes+=sizeof(practice);
        if (sequential) header.bytes+=sizeof(sequence);
        if (composes) header.bytes+=sizeof(composition);
        if (investigates && field_speech.count)
            header.bytes += sizeof(field_speech);
        if (saline)
            header.bytes += sizeof(brine_mind);
        if (potter)
            header.bytes += sizeof(ceramic);
        if (calling && native_words.count)
            header.bytes += sizeof(native_words);
        if (calling)
            header.bytes += sizeof(calls);
        if(invents)header.bytes+=sizeof(mechanism);
        if(practises)header.bytes+=sizeof(training);
        if(folds_reeds)header.bytes+=sizeof(reed);
        if(hears_banks)header.bytes+=sizeof(rattle_organ);
        if(binds_floats)header.bytes+=sizeof(braided);
        if(registers_gates)header.bytes+=sizeof(sluice);
        if(narrates_floats)header.bytes+=sizeof(braid_report);
        if(describes) {
            accounts.extent=accounts.memory.vocabulary.sounds[0]?sizeof(accounts):sizeof(accounts)-offsetof(utterance::Saved,trail);
            header.bytes+=accounts.extent;
        }
        if(hydraulic){
            channels.tag=voices_signs?capillary::episodic_signal_tag:compound_signals?capillary::compound_signal_tag:material_signals?capillary::chemical_signal_tag:capillary::hydraulic_tag;
            channels.extent=(voices_signs?sizeof(signal_voice):0)+(compound_signals?sizeof(compound_contacts):0)+(material_signals?sizeof(appetitive):0)+sizeof(channels)-offsetof(capillary::Saved,mind)+(capillary::schooled(channels.speech)?sizeof(channels.speech):0)+(channels.adaptive?offsetof(capillary::Saved,speech):0);
            header.bytes+=channels.extent;
        }
        if(values_effort)header.bytes+=sizeof(prospects);
        if (ranges) header.bytes+=sizeof(path_memory)+(path_speech.words.count?sizeof(path_speech):0);
        visit(&header,sizeof(header)); visit(&brain,payload(brain.tiles));
        if (brain.has_relations) visit(&links,sizeof(links));
        if (brain.has_relations==3) visit(&contexts,sizeof(contexts));
        if (brain.alternate_program) visit(&expression,sizeof(expression));
        if (balanced) visit(&food,sizeof(food));
        if (navigation) visit(&map,sizeof(map));
        if (navigation>=4) visit(&trace,sizeof(trace));
        if (auditory) visit(&ear,sizeof(ear));
        if (recollects) visit(&recollection,sizeof(recollection));
        if (attends) visit(&attention,sizeof(attention));
        if (orients) visit(&iris,sizeof(iris));
        if (imagines) visit(&imagery,sizeof(imagery));
        if (binds) visit(&bindings,sizeof(bindings));
        if (conjoins) visit(&conjunctions,sizeof(conjunctions));
        if (reports) visit(&report,sizeof(report));
        if (conceptual) visit(&concepts,sizeof(concepts));
        if (causal) visit(&agency,sizeof(agency));
        if (investigates)
            visit(&inquiry, sizeof(inquiry));
        if (projects)
            visit(&forecast, sizeof(forecast));
        if (handling)
            visit(&worker, sizeof(worker));
        if (grows_cells)
            visit(&cortex, sizeof(cortex));
        if (delayed_pods)
            visit(&pod_organ, sizeof(pod_organ));
        if (riverine) visit(&resident,sizeof(resident));
        if (grounded) visit(&referent,sizeof(referent));
        if (procedural) visit(&practice,sizeof(practice));
        if (sequential) visit(&sequence,sizeof(sequence));
        if (composes) visit(&composition,sizeof(composition));
        if (investigates && field_speech.count)
            visit(&field_speech, sizeof(field_speech));
        if (saline)
            visit(&brine_mind, sizeof(brine_mind));
        if (potter)
            visit(&ceramic, sizeof(ceramic));
        if (calling && native_words.count)
            visit(&native_words, sizeof(native_words));
        if (calling)
            visit(&calls, sizeof(calls));
        if (ranges) {
            visit(&path_memory,sizeof(path_memory));
            if(path_speech.words.count) visit(&path_speech,sizeof(path_speech));
        }
        if(values_effort)visit(&prospects,sizeof(prospects));
        if(invents)visit(&mechanism,sizeof(mechanism));
        if(practises)visit(&training,sizeof(training));
        if(folds_reeds)visit(&reed,sizeof(reed));
        if(hears_banks)visit(&rattle_organ,sizeof(rattle_organ));
        if(binds_floats)visit(&braided,sizeof(braided));
        if(registers_gates)visit(&sluice,sizeof(sluice));
        if(narrates_floats)visit(&braid_report,sizeof(braid_report));
        if(describes) {
            if(accounts.extent==sizeof(accounts))visit(&accounts,sizeof(accounts));
            else visit(&accounts.trail,accounts.extent);
        }
        if(hydraulic){
            if(voices_signs)visit(&signal_voice,sizeof(signal_voice));
            if(compound_signals)visit(&compound_contacts,sizeof(compound_contacts));
            if(material_signals)visit(&appetitive,sizeof(appetitive));
            if(channels.adaptive)visit(&channels,offsetof(capillary::Saved,speech));
            if(capillary::schooled(channels.speech))visit(&channels.speech,sizeof(channels.speech));
            visit(&channels.mind,sizeof(channels)-offsetof(capillary::Saved,mind));
        }
    }
    void read(const std::string &path) {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in) throw std::runtime_error("cannot open agent " + path);
        auto length = in.tellg(); in.seekg(0);
        if (!in.read(reinterpret_cast<char *>(&header), sizeof(header)) || std::memcmp(header.magic, "AXMIND1", 8) ||
            (header.version & 31u) < 1 || (header.version & 31u) > 23 ||
            ((header.version & 0x80000000u) && !(header.version & 0x40000000u)) ||
            ((header.version & 1073741824u) && !(header.version & 536870912u)) ||
            ((header.version & 536870912u) && !(header.version & 33554432u)) ||
            ((header.version & 268435456u) && !(header.version & 33554432u)) ||
            ((header.version & 134217728u) && !(header.version & 67108864u)) ||
            ((header.version & 67108864u) && !(header.version & 33554432u)) ||
            ((header.version & 4194304) && !(header.version & 2097152)) ||
            ((header.version & 65536) && !(header.version & 32768)) ||
            ((header.version & 131072) && !(header.version & 65536)) ||
            ((header.version & 262144) && !(header.version & 131072)) ||
            ((header.version & 524288) && !(header.version & 262144)) || (header.version & 96u) == 96u ||
            ((header.version & 8192) && !(header.version & 2048)) ||
            ((header.version & 4096) && !(header.version & 2048)) ||
            ((header.version & (1024 | 2048)) && !(header.version & 128)) ||
            length != std::streamoff(sizeof(header) + header.bytes + sizeof(Count)))
            throw std::runtime_error("invalid agent header or length");
        Word version=header.version&31u,ear_format=header.version&96u;auditory=ear_format!=0;recollects=(header.version&128u)!=0;
        attends=bool(header.version&256u);orients=bool(header.version&512u);
        imagines=bool(header.version&1024u);
        binds=bool(header.version&2048u);
        conjoins=bool(header.version&8192u);
        reports=bool(header.version&16384u);
        investigates = bool(header.version & 0x80000000u);
        projects = bool(header.version & 1073741824u);
        handling = bool(header.version & 536870912u);
        grows_cells = bool(header.version & 268435456u);
        delayed_pods = bool(header.version & 67108864u);
        riverine = bool(header.version & 33554432u);
        grounded = bool(header.version & 16777216u);
        procedural = bool(header.version & 8388608u);
        composes = bool(header.version & 2097152u);
        sequential = bool(header.version & 1048576u);
        conceptual = bool(header.version & 32768u);
        causal = bool(header.version & 65536u);
        size_t binding_bytes=binds ? (header.version&4096 ? sizeof(bindings) : offsetof(Bindings,attempted_form)) : 0;
        size_t ear_bytes=ear_format==64 ? sizeof(ear) : auditory ? offsetof(AuditoryLayer,projections) : 0;
        size_t extra = version == 1 ? 0 : version == 2 ? offsetof(AssociationLayer,convergence) : sizeof(links);
        size_t context=version>=5 ? sizeof(contexts) : 0, switch_bytes=version>=6 ? sizeof(expression) : 0;
        navigation=version>=10 ? 1+(version-10)/2 : 0;
        balanced=navigation ? 1+(version-10)%2 : version>=8 ? version-7 : 0;
        size_t food_bytes=(balanced ? sizeof(food) : 0)+map_bytes(navigation)+ear_bytes+(recollects ? sizeof(recollection) : 0)
                        +(attends ? sizeof(attention) : 0)+(orients ? sizeof(iris) : 0)+(imagines ? sizeof(imagery) : 0)+binding_bytes;
        if (conjoins) food_bytes+=sizeof(conjunctions);
        if (reports) food_bytes+=sizeof(report);
        if (conceptual) food_bytes+=sizeof(concepts);
        size_t agency_bytes=(header.version&524288) ? sizeof(agency) : (header.version&262144) ? offsetof(Agency,assembly) : (header.version&131072) ? offsetof(Agency,aims) : offsetof(Agency,appetite);
        if (causal) food_bytes+=agency_bytes;
        size_t crop_bytes = (header.version & 134217728u) ? sizeof(pod_organ) : offsetof(pods::Animal, frame_genes);
        if (investigates)
            food_bytes += sizeof(inquiry);
        if (projects)
            food_bytes += sizeof(forecast);
        if (handling)
            food_bytes += sizeof(worker);
        if (grows_cells)
            food_bytes += sizeof(cortex);
        if (delayed_pods)
            food_bytes += crop_bytes;
        if (riverine) food_bytes+=sizeof(resident);
        if (grounded) food_bytes+=sizeof(referent);
        if (procedural) food_bytes+=sizeof(practice);
        if (sequential) food_bytes+=sizeof(sequence);
        size_t composition_bytes=(header.version&4194304) ? sizeof(composition) : offsetof(composition::Mind,consolidated);
        if (composes) food_bytes+=composition_bytes;
        bool speaks_field = false;
        saline = false;
        brine_mind = {};
        bool captions = false;
        native_words = {};
        potter = false;
        ceramic = {};
        field_speech = {};
        calling = false;
        calls = {};
        narrates_floats=false;braid_report={};registers_gates=false;sluice={};binds_floats=false;braided={};hears_banks=false;rattle_organ={};
        folds_reeds=false;reed={};
        practises=false;training={};invents=false;mechanism={};
        values_effort=false;prospects={};
        ranges=false; path_memory={}; path_speech={}; bool knows_places=false;size_t path_bytes=0;
        describes=false; accounts={}; size_t account_bytes=0;hydraulic=false;channels={};material_signals=compound_signals=voices_signs=false;signal_voice={};compound_contacts=0;appetitive={};size_t channel_bytes=0,voice_bytes=0;Word voice_tag=0;
        if (riverine) {
            auto resume=in.tellg(); Word last[2]{};
            in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)),std::ios::end);
            if (!in.read(reinterpret_cast<char *>(last),sizeof(last))) throw std::runtime_error("short spatial cortex");
            constexpr size_t hydraulic_suffix=sizeof(channels)-offsetof(capillary::Saved,mind);
            voice_tag=last[0];voices_signs=voice_tag==capillary::auditory_signal_tag||voice_tag==capillary::episodic_signal_tag;
            voice_bytes=voice_tag==capillary::episodic_signal_tag?sizeof(signal_voice):voices_signs?offsetof(capillary::SignalVoice,episodes):0;
            compound_signals=voices_signs||last[0]==capillary::compound_signal_tag;
            material_signals=compound_signals||last[0]==capillary::chemical_signal_tag;
            auto length=last[1]-(material_signals?sizeof(appetitive):0)-(compound_signals?sizeof(compound_contacts):0)-voice_bytes;
            hydraulic=(last[0]==capillary::hydraulic_tag||material_signals) && (length==sizeof(channels) || length==sizeof(channels.speech)+hydraulic_suffix || length==offsetof(capillary::Saved,speech)+hydraulic_suffix || length==offsetof(capillary::Mouth,manual)+hydraulic_suffix || length==hydraulic_suffix);
            if(material_signals&&!hydraulic)throw std::runtime_error("invalid chemical contact extent");
            if(hydraulic){
                channel_bytes=last[1];path_bytes+=channel_bytes;
                in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short capillary cortex");
            }
            constexpr size_t suffix=sizeof(accounts)-offsetof(utterance::Saved,trail);
            describes=utterance::frame_tag(last[0]) && (last[1]==sizeof(accounts) || last[1]==offsetof(utterance::Memory,questions)+suffix || last[1]==suffix);
            if (describes) {
                account_bytes=last[1];path_bytes+=account_bytes;in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if (!in.read(reinterpret_cast<char*>(last),sizeof(last))) throw std::runtime_error("short narrative tissue");
            }
            narrates_floats=last[0]==braid_report.tag && last[1]==sizeof(braid_report);
            if(narrates_floats){path_bytes+=sizeof(braid_report);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short event memory");}
            registers_gates=last[0]==sluice.tag && last[1]==sizeof(sluice);
            if(registers_gates){path_bytes+=sizeof(sluice);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short pressure tissue");}
            binds_floats=last[0]==braided.tag && last[1]==sizeof(braided);
            if(binds_floats){path_bytes+=sizeof(braided);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short float tissue");}
            hears_banks=last[0]==rattle_organ.tag && last[1]==sizeof(rattle_organ);
            if(hears_banks){path_bytes+=sizeof(rattle_organ);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short bank acoustics");}
            folds_reeds=last[0]==reed.tag && last[1]==sizeof(reed);
            if(folds_reeds){path_bytes+=sizeof(reed);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short reed cortex");}
            practises=last[0]==training.tag && last[1]==sizeof(training);
            if(practises){path_bytes+=sizeof(training);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short practice cortex");}
            invents=last[0]==mechanism.tag && last[1]==sizeof(mechanism);
            if(practises&&!invents)throw std::runtime_error("practice cortex without material tissue");
            if(invents){path_bytes+=sizeof(mechanism);in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("short mechanism cortex");}
            values_effort=last[0]==prospects.magic && last[1]==sizeof(prospects);
            if(values_effort){
                path_bytes+=sizeof(prospects);
                in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char*>(last),sizeof(last)))throw std::runtime_error("missing economic cortex");
            }
            knows_places=last[0]==path_speech.tag && last[1]==sizeof(path_speech);
            if (knows_places) {
                path_bytes+=sizeof(path_speech);
                in.seekg(-std::streamoff(sizeof(Count)+sizeof(last)+path_bytes),std::ios::end);
                if(!in.read(reinterpret_cast<char *>(last),sizeof(last))) throw std::runtime_error("short remembered language");
            }
            ranges=last[0]==path_memory.tag && last[1]==sizeof(path_memory);
            if((knows_places || values_effort || invents) && !ranges) throw std::runtime_error("place words without a spatial cortex");
            if (ranges) path_bytes+=sizeof(path_memory);
            food_bytes+=path_bytes;
            in.seekg(resume);
        }
        if (handling) {
            auto position = in.tellg();
            Word tail[2]{};
            in.seekg(-std::streamoff(sizeof(Count) + sizeof(tail)+path_bytes), std::ios::end);
            if (!in.read(reinterpret_cast<char *>(tail), sizeof(tail)))
                throw std::runtime_error("short inquiry checkpoint");
            size_t extension=path_bytes;
            calling = tail[0] == calls.signature && tail[1] == sizeof(calls);
            if (calling) {
                extension+=sizeof(calls);
                food_bytes+=sizeof(calls);
                in.seekg(-std::streamoff(sizeof(Count) + sizeof(tail) + extension), std::ios::end);
                if (!in.read(reinterpret_cast<char *>(tail), sizeof(tail)))
                    throw std::runtime_error("short acoustic circuit");
                // The word circuit ends with extent and padding. Read its
                // signature at the preceding word only when the extent matches.
                if (tail[0] == sizeof(native_words) && tail[1] == 0) {
                    Word signature = 0;
                    in.seekg(-std::streamoff(sizeof(Count) + 12 + extension), std::ios::end);
                    if (!in.read(reinterpret_cast<char *>(&signature), 4))
                        throw std::runtime_error("short native vocabulary");
                    captions = signature == native_words.signature;
                    if (captions) {
                        food_bytes += sizeof(native_words);
                        extension += sizeof(native_words);
                        in.seekg(-std::streamoff(sizeof(Count) + sizeof(tail) + extension), std::ios::end);
                        if (!in.read(reinterpret_cast<char *>(tail), sizeof(tail)))
                            throw std::runtime_error("short vocal input");
                    }
                }
            }
            potter = calling && tail[0] == 0x434c4159u && tail[1] == sizeof(ceramic);
            if (potter) {
                food_bytes += sizeof(ceramic);
                extension += sizeof(ceramic);
                in.seekg(-std::streamoff(sizeof(Count) + sizeof(tail) + extension), std::ios::end);
                if (!in.read(reinterpret_cast<char *>(tail), sizeof(tail)))
                    throw std::runtime_error("short clay memory");
            }
            saline = investigates && tail[0] == brine_mind.signature && tail[1] == sizeof(brine_mind);
            if (saline) {
                food_bytes += sizeof(brine_mind);
                in.seekg(-std::streamoff(sizeof(Count) + sizeof(tail) + sizeof(brine_mind) + extension), std::ios::end);
                if (!in.read(reinterpret_cast<char *>(tail), sizeof(tail)))
                    throw std::runtime_error("short brine checkpoint");
            }
            in.seekg(position);
            speaks_field = investigates && tail[0] == field_speech.signature && tail[1] == sizeof(field_speech);
            if (speaks_field)
                food_bytes += sizeof(field_speech);
        }
        if (balanced) {
            if (!in.read(reinterpret_cast<char *>(&brain),payload(0)) || brain.has_relations>3 || brain.alternate_program>2 ||
                (brain.alternate_program && brain.has_relations!=3) || !brain.has_dendrites) throw std::runtime_error("invalid food circuit layout");
            extra=brain.has_relations ? sizeof(links) : 0; context=brain.has_relations==3 ? sizeof(contexts) : 0;
            switch_bytes=brain.alternate_program ? sizeof(expression) : 0; in.seekg(sizeof(header));
        }
        if (header.bytes < payload(2)+extra+context+switch_bytes+food_bytes || header.bytes > payload(32)+extra+context+switch_bytes+food_bytes)
            throw std::runtime_error("invalid agent size");
        size_t body=header.bytes-extra-context-switch_bytes-food_bytes;
        Count stored{}, hash = digest(hash_start,&header,sizeof(header));
        if (!in.read(reinterpret_cast<char *>(&brain),body)) throw std::runtime_error("incomplete agent");
        hash = digest(hash,&brain,body);
        if (extra) {
            if (!in.read(reinterpret_cast<char *>(&links),extra)) throw std::runtime_error("incomplete recurrent memory");
            hash = digest(hash,&links,extra);
        }
        if (context) {
            if (!in.read(reinterpret_cast<char *>(&contexts),context)) throw std::runtime_error("incomplete context memory");
            hash=digest(hash,&contexts,context);
        }
        if (switch_bytes) {
            if (!in.read(reinterpret_cast<char *>(&expression),switch_bytes)) throw std::runtime_error("incomplete expression state");
            hash=digest(hash,&expression,switch_bytes);
        }
        if (balanced) {
            if (!in.read(reinterpret_cast<char *>(&food),sizeof(food)) || tasted(food.associations)>4 || absorbed(food.associations)>4)
                throw std::runtime_error("invalid learned food consequences");
            hash=digest(hash,&food,sizeof(food));
        }
        if (navigation) {
            if (!in.read(reinterpret_cast<char *>(&map),sizeof(map))) throw std::runtime_error("incomplete navigation circuit");
            hash=digest(hash,&map,sizeof(map));
            if (navigation>=4) {
                if (!in.read(reinterpret_cast<char *>(&trace),sizeof(trace))) throw std::runtime_error("incomplete temporal circuit");
                hash=digest(hash,&trace,sizeof(trace));
            }
        }
        if (auditory) {
            if (!in.read(reinterpret_cast<char *>(&ear),ear_bytes)) throw std::runtime_error("incomplete auditory circuit");
            hash=digest(hash,&ear,ear_bytes);
        }
        if (recollects) {
            if (!in.read(reinterpret_cast<char *>(&recollection),sizeof(recollection))) throw std::runtime_error("incomplete visual memory");
            hash=digest(hash,&recollection,sizeof(recollection));
        }
        if (attends) {
            if (!in.read(reinterpret_cast<char *>(&attention),sizeof(attention)) || attention.previous>1) throw std::runtime_error("invalid auditory attention circuit");
            hash=digest(hash,&attention,sizeof(attention));
        }
        if (orients) {
            if (!in.read(reinterpret_cast<char *>(&iris),sizeof(iris))) throw std::runtime_error("incomplete attention muscles");
            hash=digest(hash,&iris,sizeof(iris));
        }
        if (imagines) {
            if (!in.read(reinterpret_cast<char *>(&imagery),sizeof(imagery))) throw std::runtime_error("incomplete prospective circuit");
            hash=digest(hash,&imagery,sizeof(imagery));
        }
        if (binds) {
            bindings={};
            if (!in.read(reinterpret_cast<char *>(&bindings),binding_bytes)) throw std::runtime_error("incomplete object-binding circuit");
            hash=digest(hash,&bindings,binding_bytes);
        }
        if (conjoins) {
            if (!in.read(reinterpret_cast<char *>(&conjunctions),sizeof(conjunctions))) throw std::runtime_error("incomplete conjunctive contacts");
            hash=digest(hash,&conjunctions,sizeof(conjunctions));
        }
        if (reports) {
            if (!in.read(reinterpret_cast<char *>(&report),sizeof(report)) || report.request>2 || report.heard_phases>report_length ||
                report.history.retina>255 || report.history.recent>63 || report.history.earlier>63)
                throw std::runtime_error("invalid acquired speech circuit");
            hash=digest(hash,&report,sizeof(report));
        }
        if (conceptual) {
            if (!in.read(reinterpret_cast<char *>(&concepts),sizeof(concepts))) throw std::runtime_error("incomplete relational concept circuit");
            hash=digest(hash,&concepts,sizeof(concepts));
            auto valid_walk=[](Count walk){return !walk || (63-std::countl_zero(walk))%3==0;};
            const auto &pending=concepts.pending;
            if (pending.form && (pending.source>=32 || !(pending.domain&(1u<<pending.source))))
                throw std::runtime_error("invalid remembered experiment");
            for (Count walk:pending.prior) if (!valid_walk(walk)) throw std::runtime_error("invalid remembered pathway");
            for (Word j=0;j<concept_words;++j) {
                const auto &w=concepts.words[j];
                if (w.threshold>4 || ((w.inputs&65535)&~((1u<<(8+j))-1)) || !valid_walk(w.walk))
                    throw std::runtime_error("invalid relational concept circuit");
            }
        }
        if (causal) {
            agency={};if (!in.read(reinterpret_cast<char *>(&agency),agency_bytes)) throw std::runtime_error("incomplete causal memory");
            hash=digest(hash,&agency,agency_bytes);
            const auto &goal=agency.intention;
            if (goal.object>32 || goal.start>63 || goal.require>63 || goal.value>63 || goal.length>utterance_limit || goal.branch>3 || agency.explanation.length>8)
                throw std::runtime_error("invalid remembered intention");
            for (Word k=0;k<goal.length;++k) if (goal.actions[k]>=operations) throw std::runtime_error("invalid remembered action");
            for (auto state:agency.scenes) if (state>63) throw std::runtime_error("invalid remembered plot");
            for (auto &row:agency.model.fields) for (auto &f:row) if (f.compiled) {
                Word code=f.compiled-1;if ((code&7)>=traits || ((code>>3)&7)>=traits || (code>>6)>15)
                    throw std::runtime_error("invalid compiled causal contact");
            }
            auto validate=[](const CausalField &f){if (f.compiled) {
                Word code=f.compiled-1;if ((code&7)>=traits || ((code>>3)&7)>=traits || (code>>6)>15) throw std::runtime_error("invalid goal contact");
            }};
            validate(agency.appetite.field);for (auto &category:agency.aims) validate(category.field);
            if (agency.assembly.active&~255u) throw std::runtime_error("invalid goal assembly");
            for (Word k=0;k<8;++k) {
                const auto &e=agency.assembly.field[k];
                if ((agency.assembly.inputs[k]&~((1u<<k)-1)) || (e.certain&~e.possible) || ((agency.assembly.active&(1u<<k)) && !agency.aims[k].form))
                    throw std::runtime_error("invalid acquired goal projection");
            }
            if (agency.conditional.length>8 || agency.branch.sense>traits || agency.branch.low>operations || agency.branch.high>operations)
                throw std::runtime_error("invalid conditional plan");
        }
        if (investigates) {
            if (!in.read(reinterpret_cast<char *>(&inquiry), sizeof(inquiry)))
                throw std::runtime_error("incomplete exploratory tissue");
            hash = digest(hash, &inquiry, sizeof(inquiry));
        }
        if (projects) {
            if (!in.read(reinterpret_cast<char *>(&forecast), sizeof(forecast)))
                throw std::runtime_error("incomplete spatial prediction circuit");
            hash = digest(hash, &forecast, sizeof(forecast));
        }
        if (handling) {
            if (!in.read(reinterpret_cast<char *>(&worker), sizeof(worker)) || !material::valid(worker.hand) ||
                (worker.recent >> 17))
                throw std::runtime_error("invalid manipulation circuit");
            hash = digest(hash, &worker, sizeof(worker));
        }
        if (grows_cells) {
            if (!riverine || !in.read(reinterpret_cast<char *>(&cortex), sizeof(cortex)) ||
                !flood::viable_cortex(cortex, brain.tiles))
                throw std::runtime_error("invalid exported spiking population");
            hash = digest(hash, &cortex, sizeof(cortex));
        }
        if (delayed_pods) {
            pod_organ = {};
            if (!in.read(reinterpret_cast<char *>(&pod_organ), crop_bytes))
                throw std::runtime_error("incomplete pod circuit");
            Word mass = 0;
            for (Word k = 0; k < 8; ++k)
                mass += pod_organ.gut.food[0][k] + pod_organ.gut.food[1][k] + pod_organ.gut.food[2][k] +
                        pod_organ.gut.chaff[k];
            if (mass > 120)
                throw std::runtime_error("invalid crop contents");
            for (auto &row : pod_organ.brain.support)
                for (auto q : row)
                    if (q < -63 || q > 63)
                        throw std::runtime_error("invalid crop plasticity");
            if (pod_organ.frame.active > 3 || (pod_organ.frame.anchors >> 16) || (pod_organ.frame.certainty >> 16))
                throw std::runtime_error("invalid sensorimotor frame");
            hash = digest(hash, &pod_organ, crop_bytes);
        }
        if (riverine) {
            if (!in.read(reinterpret_cast<char *>(&resident),sizeof(resident))) throw std::runtime_error("incomplete floodplain resident");
            const auto &a=resident.animal;const auto &g=resident.germ;
            if (resident.extent!=sizeof(resident) || !a.energy || a.born>resident.tick || !g.module_count || g.module_count>4 ||
                !a.body.shape || a.body.shape>>25 || !(a.body.shape&(1u<<flood::center)) || (a.body.eyes|a.body.pores|a.body.limbs)&~a.body.shape ||
                a.heading>3 || a.action>7 || a.heard>8 || a.signal>8 || a.cohort>2 || Word(a.body_origin>>32)>resident.tick ||
                Word(a.brain_origin>>32)>resident.tick) throw std::runtime_error("invalid exported floodplain resident");
            hash=digest(hash,&resident,sizeof(resident));
        }
        if (grounded) {
            if (!in.read(reinterpret_cast<char *>(&referent),sizeof(referent))) throw std::runtime_error("incomplete cross-situational memory");
            hash=digest(hash,&referent,sizeof(referent));grounding::validate(referent);
        }
        if (procedural) {
            if (!in.read(reinterpret_cast<char *>(&practice),sizeof(practice))) throw std::runtime_error("incomplete acquired procedure");
            hash=digest(hash,&practice,sizeof(practice));procedure::validate(practice);
        }
        if (sequential) {
            if (!in.read(reinterpret_cast<char *>(&sequence),sizeof(sequence))) throw std::runtime_error("incomplete developmental circuit");
            hash=digest(hash,&sequence,sizeof(sequence));
            const auto &s=sequence;
            if (s.motors>sequence_motors || s.senses>sequence_senses || s.map.size>sequence_cells || s.anchored>1 ||
                (s.map.size<64 && (s.active>>s.map.size)) || s.trace.length>trace_slots || s.trace.head>=trace_slots)
                throw std::runtime_error("invalid developmental circuit");
            for (Word q=0;q<s.map.size;++q) for (Word a=0;a<s.motors;++a) {
                auto e=s.map.edge[q][a];
                if (e && (cell_of(e)>=s.map.size || sense_of(e)>=s.senses)) throw std::runtime_error("invalid developmental contact");
            }
            for (Word j=0;j<s.trace.length;++j) {
                Word k=(s.trace.head+trace_slots-s.trace.length+j)%trace_slots;
                if ((s.trace.senses[k]&127)>=s.senses || ((s.trace.motors[k/8]>>(4*(k%8)))&15)>=s.motors)
                    throw std::runtime_error("invalid retained event");
            }
        }
        if (composes) {
            composition={};if (!in.read(reinterpret_cast<char *>(&composition),composition_bytes)) throw std::runtime_error("incomplete compositional circuit");
            hash=digest(hash,&composition,composition_bytes);
            if (header.version&4194304) {if (composition.extent!=sizeof(composition)) throw std::runtime_error("invalid compositional section length");}
            else {composition.consolidated=composition.lessons.seen;composition.extent=sizeof(composition);}
            axion::life::composition::validate(composition);
        }
        if (speaks_field) {
            if (!in.read(reinterpret_cast<char *>(&field_speech), sizeof(field_speech)) || !field_speech.count)
                throw std::runtime_error("invalid acquired field language");
            material::validate(field_speech);
            hash = digest(hash, &field_speech, sizeof(field_speech));
        }
        if (saline) {
            if (!in.read(reinterpret_cast<char *>(&brine_mind), sizeof(brine_mind)) ||
                brine_mind.signature != 0x42524e45u || brine_mind.extent != sizeof(brine_mind) ||
                ((brine_mind.cortex.memory.low | brine_mind.cortex.memory.high) & ~brine_mind.cortex.genes.active))
                throw std::runtime_error("invalid salt-dependent tissue");
            hash = digest(hash, &brine_mind, sizeof(brine_mind));
        }
        if (potter) {
            if (!in.read(reinterpret_cast<char *>(&ceramic), sizeof(ceramic)) || ceramic.signature != 0x434c4159u ||
                ceramic.extent != sizeof(ceramic) || ceramic.padding || !clay::valid(ceramic.mind) ||
                !clay::valid(ceramic.palm, resident.tick) || (clay::size(ceramic.palm) && !worker.hand.faces))
                throw std::runtime_error("invalid potter memory");
            hash = digest(hash, &ceramic, sizeof(ceramic));
        }
        if (captions) {
            if (!grounded || !in.read(reinterpret_cast<char *>(&native_words), sizeof(native_words)) ||
                !siphon::valid(native_words))
                throw std::runtime_error("invalid acquired acoustic vocabulary");
            hash = digest(hash, &native_words, sizeof(native_words));
        }
        if (calling) {
            if (!riverine || !in.read(reinterpret_cast<char *>(&calls), sizeof(calls)) ||
                calls.signature != 0x53595048u || calls.extent != sizeof(calls))
                throw std::runtime_error("invalid acoustic organ");
            hash = digest(hash, &calls, sizeof(calls));
        }
        if (ranges) {
            if (!in.read(reinterpret_cast<char *>(&path_memory),sizeof(path_memory)) ||
                path_memory.tag!=0x4a415953 || path_memory.extent!=sizeof(path_memory) ||
                !hollow::viable(path_memory.mind,path_memory.slot_limit)) throw std::runtime_error("invalid spatial cortex");
            for(Word i=0;i<hollow::limit;++i)
                if(!hollow::valid_mark(path_memory.marks[i]) || (i>=path_memory.mind.used && path_memory.marks[i]))
                    throw std::runtime_error("invalid spatial memory cell");
            hash=digest(hash,&path_memory,sizeof(path_memory));
            if(knows_places) {
                if(!in.read(reinterpret_cast<char *>(&path_speech),sizeof(path_speech)) || path_speech.tag!=0x484f4c57 ||
                   path_speech.extent!=sizeof(path_speech) || !siphon::valid(path_speech.words))
                    throw std::runtime_error("invalid acquired place words");
                hash=digest(hash,&path_speech,sizeof(path_speech));
            }
        }
        if(values_effort){
            if(!in.read(reinterpret_cast<char*>(&prospects),sizeof(prospects)) || prospects.magic!=0x45464654 ||
               prospects.bytes!=sizeof(prospects) || !effort::viable(prospects.mind))throw std::runtime_error("invalid effort cortex");
            hash=digest(hash,&prospects,sizeof(prospects));
        }
        if(invents){
            if(!in.read(reinterpret_cast<char*>(&mechanism),sizeof(mechanism)) || mechanism.tag!=0x4b4e494c ||
                mechanism.extent!=sizeof(mechanism)||!linkage::valid(mechanism.mind,mechanism.arena))throw std::runtime_error("invalid causal mechanism cortex");
            for(Word i=0;i<4;++i)if(!linkage::key_at(mechanism.mind,i)){linkage::Tissue blank{};
                if(std::memcmp(&blank,mechanism.tissue+i,sizeof(blank)))throw std::runtime_error("unowned mechanism tissue");}
            hash=digest(hash,&mechanism,sizeof(mechanism));
        }
        if(practises){
            if(!in.read(reinterpret_cast<char*>(&training),sizeof(training))||training.tag!=0x54434154||training.extent!=sizeof(training)||
                !practice::valid(training.mind,training.arena)||training.mind.focus)throw std::runtime_error("invalid portable practice tissue");
            for(Word i=0;i<4;++i)if(!practice::valid(training.bank[i],i<practice::size(training.mind)))throw std::runtime_error("unowned portable hypothesis");
            hash=digest(hash,&training,sizeof(training));
        }
        if(folds_reeds){
            if(!in.read(reinterpret_cast<char*>(&reed),sizeof(reed)) || reed.tag!=0x44454552u || reed.extent!=sizeof(reed) || !runnel::valid(reed.mind))
                throw std::runtime_error("invalid reed cortex");
            hash=digest(hash,&reed,sizeof(reed));
        }
        if(hears_banks){
            if(!in.read(reinterpret_cast<char*>(&rattle_organ),sizeof(rattle_organ)) || rattle_organ.tag!=0x4b4e4142u ||
               rattle_organ.extent!=sizeof(rattle_organ) || !rattle::valid(rattle_organ.mind))
                throw std::runtime_error("invalid bank acoustic tissue");
            hash=digest(hash,&rattle_organ,sizeof(rattle_organ));
        }
        if(binds_floats){
            if(!in.read(reinterpret_cast<char*>(&braided),sizeof(braided))||braided.tag!=0x44494142u||braided.extent!=sizeof(braided)||
               !braid::valid(braided.mind)||!braid::valid_words(braided.words))throw std::runtime_error("invalid float tissue");
            hash=digest(hash,&braided,sizeof(braided));
        }
        if(registers_gates){
            if(!binds_floats || !in.read(reinterpret_cast<char*>(&sluice),sizeof(sluice))||sluice.tag!=0x54414c46u||
               sluice.extent!=sizeof(sluice)||!braid::valid_gate(sluice.gates))throw std::runtime_error("invalid pressure receiver");
            hash=digest(hash,&sluice,sizeof(sluice));
        }
        if(narrates_floats){
            if(!binds_floats || !in.read(reinterpret_cast<char*>(&braid_report),sizeof(braid_report)) ||
               braid_report.tag!=0x4c4c4554u || braid_report.extent!=sizeof(braid_report) || !braid::valid_narration(braid_report.memory))
                throw std::runtime_error("invalid acoustic event memory");
            hash=digest(hash,&braid_report,sizeof(braid_report));
        }
        if (describes) {
            constexpr size_t suffix=sizeof(accounts)-offsetof(utterance::Saved,trail);
            size_t front=account_bytes-suffix;
            if (!binds_floats || !in.read(reinterpret_cast<char*>(&accounts.memory),front) ||
                !in.read(reinterpret_cast<char*>(&accounts.trail),suffix) ||
                !utterance::frame_tag(accounts.tag) || accounts.extent!=account_bytes || !accounts.trail.valid() || !utterance::valid(accounts.memory))
                throw std::runtime_error("invalid narrative cortex");
            hash=digest(hash,&accounts.memory,front);hash=digest(hash,&accounts.trail,suffix);accounts.extent=sizeof(accounts);
        }
        if(hydraulic){
            constexpr auto suffix=sizeof(channels)-offsetof(capillary::Saved,mind);
            constexpr auto context=offsetof(capillary::Saved,speech);
            auto extent=channel_bytes;
            if(voices_signs){
                if(!in.read(reinterpret_cast<char*>(&signal_voice),voice_bytes)||!capillary::valid(signal_voice))throw std::runtime_error("damaged auditory signal contacts");
                hash=digest(hash,&signal_voice,voice_bytes);extent-=voice_bytes;
            }
            if(compound_signals){
                if(!in.read(reinterpret_cast<char*>(&compound_contacts),sizeof(compound_contacts)))throw std::runtime_error("truncated two-port contacts");
                hash=digest(hash,&compound_contacts,sizeof(compound_contacts));extent-=sizeof(compound_contacts);
            }
            if(material_signals){
                if(!in.read(reinterpret_cast<char*>(&appetitive),sizeof(appetitive)))throw std::runtime_error("truncated appetitive contacts");
                auto gene=resident.germ.pigments[2];
                auto projected=capillary::expand_signs(capillary::pack_signs(appetitive,gene),gene);
                if(projected.on!=appetitive.on||projected.off!=appetitive.off)throw std::runtime_error("invalid appetitive sensory projection");
                hash=digest(hash,&appetitive,sizeof(appetitive));extent-=sizeof(appetitive);
            }
            bool extended=extent==sizeof(channels) || extent==suffix+context;
            if(extended){
                if(!in.read(reinterpret_cast<char*>(&channels),context) || (channels.adaptive<1 || channels.adaptive>3) || !capillary::valid(channels.history))
                    throw std::runtime_error("damaged material context");
                hash=digest(hash,&channels,context);
            }
            auto front=extent-suffix-(extended?context:0);
            if(!binds_floats || !in.read(reinterpret_cast<char*>(&channels.speech),front) ||
               !in.read(reinterpret_cast<char*>(&channels.mind),suffix) || channels.tag!=(voices_signs?voice_tag:compound_signals?capillary::compound_signal_tag:material_signals?capillary::chemical_signal_tag:capillary::hydraulic_tag) ||
               channels.extent!=channel_bytes || !capillary::valid(channels.mind) || !capillary::valid(channels.speech))
                throw std::runtime_error("invalid acquired intervention cortex");
            hash=digest(hash,&channels.speech,front);hash=digest(hash,&channels.mind,suffix);channels.extent=sizeof(channels);
        }
        if (!in.read(reinterpret_cast<char *>(&stored),sizeof(stored)) || hash != stored) throw std::runtime_error("agent checksum mismatch");
        journal::checkpoint(path,hash,false);
        if (!std::has_single_bit(brain.tiles) || brain.tiles < 2 || brain.tiles > 32 || body != payload(brain.tiles)
            || (!balanced && brain.has_relations != (version>=5 ? 3u : version>=3 ? version-2 : Word(extra!=0)))
            || (!balanced && brain.alternate_program!=(version>=6 ? version-5 : 0)) || (brain.has_relations && !brain.has_dendrites)
            || (brain.bits != 1 && brain.bits != 2) || brain.has_dendrites > 1 || brain.delayed_path > 1
            || brain.outcome_lag < 1 || brain.outcome_lag > 8 || brain.due > 65535 || brain.motor_history > 0xffffff)
            throw std::runtime_error("invalid agent state");
        for (Word i = 0; i < 8; ++i) if (((brain.due >> (2*i)) & 3) == 3) throw std::runtime_error("invalid pending outcome");
        specimen(path,"input",stored);
    }
};

}
