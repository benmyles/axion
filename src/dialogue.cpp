#include "evidence_file.hpp"
#include "axion/flume.hpp"
#include "axion/backwater.hpp"
#include "axion/braid_talk.hpp"
#include "axion/apprentice.hpp"
#include "axion/inlet.hpp"
#include "axion/cascade.hpp"
#include "retina_files.hpp"
#include "axion/practice_words.hpp"
#include "axion/linkage_words.hpp"
#include "axion/effort_words.hpp"
#include "axion/dialogue.hpp"
#include "axion/habitat_view.hpp"
#include "axion/commons.hpp"
#include "individual_io.hpp"
#include "audience_image.hpp"
#include "narrative_host.hpp"
#include "capillary_host.hpp"
#include "axion/capillary_play.hpp"
#include "clay_io.hpp"
#include "axion/ceramic_words.hpp"
#include "development_io.hpp"
#include "ground_host.hpp"
#include "grounding_eye.cuh"
#include "flood_tray.hpp"
#include "axion/mill.hpp"
#include "flood_material.cuh"
#include "journal_population.hpp"
#include "pulse_words.hpp"
#include "seed_bank.hpp"
#include "launch_graph.hpp"
#include "life_runtime.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <map>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <set>
#include <string_view>
#include <vector>

namespace axion::life {
namespace {
using namespace runtime;
using namespace storage;
constexpr std::string_view help = R"(axion dialogue  |  two embodied spiking animals on a private acoustic channel

  --speaker FILE --listener FILE   independent .eco founders or .mind individuals
  --speaker-site N --listener-site N   choose specific founders from a habitat
  --pairs N       separately allocated pairs (default 256)
  --practice N    shared-outcome interactions before each test (default 512)
  --play N        guided joint attention, object handling, requesting and retrieval before meeting
  --demonstrations N  let the speaker handle private objects for food before meeting its partner
  --outcomes shared|none|yoked   shared food, no social feedback, or food outcomes from another pair
  --motor-learning both|speaker|listener|neither   enable local reward-dependent synaptic updates while both animals act
  --plasticity both|speaker|listener|neither   which partners continue changing during exchanges
  --probes N      balanced frozen interactions, a multiple of 32 (default 32)
  --optics affine|shuffled   use structured sensory addresses or independently permute all 32 channels per partner
  --community-size N   independent pairs sharing the same optics and doors, for stranger tests
  --eligibility local|inherited   allow hereditary recruitment of earlier sensory-motor events at feedback
  --vision whole|gaze  simultaneous attributes or an actual orienting movement before answering
  --door-offset N   initial physical exit rotation (0..3)
  --facets 1|2    a private observation, or a private question and two visible attributes
  --generations N   breed both partners for acquiring and revising a private convention
  --replicates N    private environments per candidate pair in the nursery (default 4)
  --breeding select|drift   reproduction by joint outcomes or unselected inheritance
  --founder-speaker FILE --founder-listener FILE   save the selected untrained offspring
  --seed N        environment seed
  --reversal on|off   change the motor consequences and continue learning (default on)
  --controls on|off   run sensory interventions after the intact measurement (default on)
  --dispatch graph|direct
  --pair N        individual pair to save, report or speak with (default 0)
  /braid SEED   open the float table for /braid-show, /braid-teach and /braid-pass
  /braid-pull speaker 013   perform three paid pulls and remember the episode
  /braid-feedback speaker on|auditory|shared|quiet|off   connect an expressed teaching population
  /braid-gloss and /braid-pattern   teach grounded names and event questions
  /inlet-pattern                   teach the same question from real watercourse encounters
  /cap-cell ROW SOURCE PAD MINERAL or /cap-channel SEED DEPTH SOURCE PAD MINERAL   assemble a physical pressure workshop
  /cap-radio clear|silent|partial|patchy N [actual|withheld|exchanged]   set link and inquiry allowance
  /cap-private speaker nw|ne|sw|se SEED        let only the speaker operate and witness two junctions
  /cap-ear speaker open|closed                 change the auditory shutter
  /cap-name-port speaker AXIS WORD [QUESTION]  name one actual resonator response
  /cap-name-step speaker AXIS BIT WORD [QUESTION]  name one real reaching movement
  /cap-twin speaker INPUT GEOLOGY               dock a second conserved dye pair
  /cap-prime speaker                            pump both test paths and the two clear return paths
  /cap-message4 speaker nw|ne|sw|se SEED [practice|frozen] [single|plumes|pooled]   exchange two physical marks
  /cap-message speaker left|right SEED [practice|frozen]   let a second animal read the physical mark
  /cap-seal speaker                             dock a finite dye pair in a closed one-cell circuit
  /cap-recover speaker N                         recover spent portions and their dye at an energy cost
  /cap-mineral ROCK                               change only the bench material behind its surface
  /cap-probe speaker SEED                        choose a physical experiment and spend a portion
  /cap-context speaker adaptive|single|regional             select the material memory available to the animal
  /cap-explore speaker VIEWS GRIPS SEED   let the resident investigate the material
  /cap-grip speaker MASK and /cap-pour speaker   change real handles and release a drop
  /cap-ask speaker QUESTION and /cap-pass speaker QUESTION   speak from observed and imagined interventions
  /cap-build pupil STEPS and /cap-net pupil   build the pupil's proposed channel and commit a net
  /story-memory speaker             inspect a watershed resident's retained visual history
  /story-bed PALETTE DUCTS FIBRES [GROUND]   arrange four pigment carriers and a wet wick
  /cap-cell ROW FROM PAD ROCK        arrange a single physical junction with two dyed carriers
  /cap-channel SEED DEPTH FROM PAD ROCK [scarce]    open eight by eight seats with six reachable tools
  /cap-radio clear|silent|partial|patchy N [actual|withheld|exchanged]   set link and inquiry allowance
  /cap-private speaker nw|ne|sw|se SEED        let only the speaker operate and witness two junctions
  /cap-ear speaker open|closed                 change the auditory shutter
  /cap-name-port speaker AXIS WORD [QUESTION]  name one actual resonator response
  /cap-name-step speaker AXIS BIT WORD [QUESTION]  name one real reaching movement
  /cap-twin speaker INPUT GEOLOGY               dock a second conserved dye pair
  /cap-prime speaker                            pump both test paths and the two clear return paths
  /cap-message4 speaker nw|ne|sw|se SEED [practice|frozen] [single|plumes|pooled]   exchange two physical marks
  /cap-message speaker left|right SEED [practice|frozen]   let a second animal read the physical mark
  /cap-seal speaker                             dock a finite dye pair in a closed one-cell circuit
  /cap-recover speaker N                         recover spent portions and their dye at an energy cost
  /cap-mineral ROCK                               change only the bench material behind its surface
  /cap-probe speaker SEED                        choose a physical experiment and spend a portion
  /cap-context speaker adaptive|single|regional             select the material memory available to the animal
  /cap-explore speaker VIEWS PROBES SEED            handle a sample to learn its resistance
  /cap-grip speaker MASK and /cap-pour speaker      move parts, then release one conserved portion
  /cap-pass speaker QUESTION and /cap-follow pupil  speak a construction and carry it out
  /cap-net pupil                                   set the pupil's catch before pouring
  /cap-caption-grips speaker QUESTION | WORDS       name the last one or two felt actions
  /cap-memory speaker                              restore the last witnessed field change
  /story-pour INLET                  move a carrier in the shared brook
  /story-curtain pupil open|closed   regulate a listener's access to the stream
  /story-caption speaker QUESTION | WORDS   associate public sounds with a private observation
  /story-ask speaker QUESTION        address a learned descriptive or temporal question
  /story-pass speaker QUESTION       let a second creature hear and reconstruct the report
  /story-net pupil and /story-serve pupil   commit the listener's reaches and release the food
  /cascade SEED 1|2|4 LAYOUT [natural|bright]   open a series of physical watercourses
  /cascade-works FILE INDEX SEED [natural|bright]   use an actual bed, then /cascade-pour built to release food
  /cascade-watch N [live|displaced|independent|shutter|closed]   watch water without reaching or eating
  /cascade-name WORD GATES REPS [crowded|fixed|clear|unpaired]   teach a name by watching a sequence of lamps
  /cascade-rack N   offer paired inlets the resident can test after hearing your itinerary
  /cascade-stir N   let the animal choose which inlet to test by moving a finite grain
  /cascade-attend N [live|displaced|independent|shutter]   let the resident decide which passing glints to track
  /cascade-load DYE   place a marker, then speak learned lamp names in the desired order
  /cascade-extend WORDS   append up to eight learned names to the current sequence
  /cascade-pour DIGITS   physically open those gates in order and ask about the result
  /cascade-banks inherited|shared   choose how different lamps access the observed tissue
  /inlet-table SEED [field|seep|channels|basin|mixed [contextual|inherited|local|misaligned [TWIN [LAYOUT]]]]
  /inlet-practice N                act on the three available inlets
  /inlet-stage [DYE]               commit to the unseen inlet and talk about the forecast
  /inlet-turn                      turn the outlet couplings before release
  /inlet-pour                      release the food, observe the result and continue talking
  /inlet-clear                     clear the operator's contact bank
  /audience SEED LAYOUT first|second [ARM [ACCESS]]   return one of two previously visible pupils
  /audience-works FILE INDEX SEED LAYOUT first|second [ARM [ACCESS]]   use a recorded watercourse
  /audience-load first|second COLOR and /audience-pour   hold and then release eight food units
  /audience-ask guide|pupil QUESTION   or type a learned question to the pupil
  /audience-practice SEED EVEN-COUNT [live|shifted|fixed]   hear this guide alongside moving grains
  /audience-exit                   leave the audience table
  /braid-shutters and /braid-replies   choose the sight and testimony channels
  /braid-glass, /braid-compare and /braid-witness   align and consult different optical viewpoints
  /backwater WORLD [crossed]   ask a worker to sound one of two banks, then reach for the chosen pan
  /backwater-trials CASES EXPOSURE SEED [crossed]   replay public requests and explanations in paired worlds
  /backwater-pass, /backwater-pull, /backwater-work N, /backwater-divert and /backwater-exit
  /flume WORLD SPAN [handed]   open a pressure-driven gallery and read the next joint with /flume-order
  /flume-pass, /flume-build and /flume-pump T   consult, assemble and run the mechanism
  --save-speaker FILE --save-listener FILE   save complete individual checkpoints
  --language private|compass   privately assigned calls, or conventional names for four bearings
  --nouns A,B      words paired with the two resources during compass play (default food,water)
  --auditory-origin inherited|inert   allow an inactive auditory learning rule for comparison
  --hearing single|population|learned   a single projection, a sparse code, or outcome-trained auditory receptive fields
  --turn-readout last|peak   issue the final or strongest discriminative vocal discharge
  --references A,B   teach two additional word forms for the same and the other prior referent
  --scenes all|withheld   use disjoint attribute combinations during practice and probe
  --recall-cut intact|fixed|crossed|source|erased   intervene on visual memory during an interview
  --pitch intact|muted|inverted   intervene on the call that selects sight or recollection
  --imagination intact|muted   release or suppress learned action replay in the witness
  --source-learning intact|frozen --exploration learned|random   intervene on role credit or selection of an experiment
  /rack SEED [PIECES]   open a material workbench during an interview with an equipped resident
  /rack-study N, /rack-layout ID, /rack-order, /rack-pass   observe, invent and pass a spoken procedure
  --world FILE     open a living habitat as the private scene; /visit X Y or /advance N changes the observation
  --save-world FILE   save the habitat after the interview
  --interview      ask the two-view speaker about a scene using text after the session
                   /scene west north places the two resources; /quit ends the interview
  --interactive   exchange words with the chosen listener after the experiment
  --voice A,B,C,D   the four sounds made by its vocal muscles (default ka,lo,mi,tu)
  --report-wire open|muted|reversed   acoustic propagation from witness to reader

The speaker sees the scene. The listener receives actual muscle discharges through
the acoustic channel. Both receive the same metabolic consequence after a choice.
In the two-facet world only the listener sees the question, then both animals speak.
)";
struct Options {
    Word report_wire=0;
    std::string speaker="examples/learner.eco",listener="examples/learner.eco",save_s,save_l,founder_s,founder_l,world,save_world;
    std::optional<Word> s_site,l_site;
    Word pairs=256,practice=512,probes=32,facets=1,seed=1511,pair=0,plasticity=3,demonstrations=0,generations=0,replicates=4,drift=0,community=1,gaze=0,offset=0,eligibility=0,play=0,compass=0,noun_tags=0,withheld=0,discourse=0,salt=0,distributed=0,peak=0,matched=0,shuffled=0,outcomes=0,motor_learning=3;
    std::array<Word,4> form_hashes{};
    std::array<std::string,2> nouns{"food","water"},references{"again","other"};
    bool interview=false,voice_set=false,inert=false,replay=true;RecallCut recall_cut=RecallCut::intact;Word pitch_cut=0,binding_credit=1,random_experiment=0;
    bool reverse=true,graphs=true,interactive=false,controls=true;
    std::array<std::string,4> voice{"ka","lo","mi","tu"};
};
Word integer(std::string_view v,Word least,Word most) {
    Word n=0;auto [end,err]=std::from_chars(v.data(),v.data()+v.size(),n);
    if (err!=std::errc{} || end!=v.data()+v.size() || n<least || n>most) throw std::invalid_argument("invalid dialogue quantity "+std::string(v));
    return n;
}
Word word_tag(std::string_view token,Word salt=0) {return mix(word_digest(token)^salt)&15;}
Options options(int argc,char **argv,bool talk) {
    Options o;
    if (talk) {
        auto folder=std::filesystem::path("examples");
        if (!std::filesystem::exists(folder/"inquirer.mind")) {
            std::error_code ec;auto binary=std::filesystem::read_symlink("/proc/self/exe",ec);
            if (!ec) folder=binary.parent_path()/"examples";
        }
        o.speaker=(folder/"gardener.mind").string();o.listener=(folder/"apprentice.mind").string();
        o.pairs=1;o.facets=2;o.gaze=1;o.compass=1;o.discourse=1;o.matched=1;o.probes=128;o.practice=0;o.reverse=false;o.interview=true;
    }
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--interview") {o.interview=true;continue;}
        if (f=="--interactive") {o.interactive=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string v=argv[i];
        if (f=="--speaker") o.speaker=v;
        else if (f=="--listener") o.listener=v;
        else if (f=="--speaker-site") o.s_site=integer(v,0,1048575);
        else if (f=="--listener-site") o.l_site=integer(v,0,1048575);
        else if (f=="--recall-cut") {
            constexpr std::array names{"intact","fixed","crossed","source","erased"};auto it=std::find(names.begin(),names.end(),v);
            if (it==names.end()) throw std::invalid_argument("unknown memory intervention");
            o.recall_cut=static_cast<RecallCut>(it-names.begin());
        }
        else if (f=="--pitch" && (v=="intact" || v=="muted" || v=="inverted")) o.pitch_cut=v=="muted" ? 1 : v=="inverted" ? 2 : 0;
        else if (f=="--imagination" && (v=="intact" || v=="muted")) o.replay=v=="intact";
        else if (f=="--source-learning" && (v=="intact" || v=="frozen")) o.binding_credit=v=="intact";
        else if (f=="--exploration" && (v=="learned" || v=="random")) o.random_experiment=v=="random";
        else if (f=="--world") o.world=v;
        else if (f=="--save-world") o.save_world=v;
        else if (f=="--report-wire" && (v=="open" || v=="muted" || v=="reversed")) o.report_wire=v=="open" ? 0 : v=="muted" ? 1 : 2;
        else if (f=="--save-speaker") o.save_s=v;
        else if (f=="--save-listener") o.save_l=v;
        else if (f=="--pairs") o.pairs=integer(v,1,1048576);
        else if (f=="--pair") o.pair=integer(v,0,1048575);
        else if (f=="--eligibility" && (v=="local" || v=="inherited")) o.eligibility=v=="inherited";
        else if (f=="--optics" && (v=="affine" || v=="shuffled")) o.shuffled=v=="shuffled";
        else if (f=="--community-size") o.community=integer(v,1,1048576);
        else if (f=="--door-offset") o.offset=integer(v,0,3);
        else if (f=="--vision" && (v=="whole" || v=="gaze")) o.gaze=v=="gaze";
        else if (f=="--generations") o.generations=integer(v,0,65536);
        else if (f=="--replicates") o.replicates=integer(v,1,256);
        else if (f=="--breeding" && (v=="select" || v=="drift")) o.drift=v=="drift";
        else if (f=="--founder-speaker") o.founder_s=v;
        else if (f=="--founder-listener") o.founder_l=v;
        else if (f=="--play") o.play=integer(v,0,1048576);
        else if (f=="--demonstrations") o.demonstrations=integer(v,0,1048576);
        else if ((f=="--plasticity" || f=="--motor-learning") && (v=="both" || v=="speaker" || v=="listener" || v=="neither")) {
            Word mask=v=="both" ? 3u : v=="speaker" ? 1u : v=="listener" ? 2u : 0u;
            if (f=="--motor-learning") o.motor_learning=mask;else o.plasticity=mask;
        }
        else if (f=="--outcomes" && (v=="shared" || v=="none" || v=="yoked")) o.outcomes=v=="none" ? 1u : v=="yoked" ? 2u : 0u;
        else if (f=="--practice") o.practice=integer(v,0,1048576);
        else if (f=="--probes") o.probes=integer(v,32,65536);
        else if (f=="--facets") o.facets=integer(v,1,2);
        else if (f=="--seed") o.seed=integer(v,0,~0u);
        else if (f=="--reversal" && (v=="on" || v=="off")) o.reverse=v=="on";
        else if (f=="--controls" && (v=="on" || v=="off")) o.controls=v=="on";
        else if (f=="--dispatch" && (v=="graph" || v=="direct")) o.graphs=v=="graph";
        else if (f=="--auditory-origin" && (v=="inherited" || v=="inert")) o.inert=v=="inert";
        else if (f=="--hearing" && (v=="single" || v=="population" || v=="learned")) {o.distributed=v=="population";o.matched=v=="learned";}
        else if (f=="--turn-readout" && (v=="last" || v=="peak")) o.peak=v=="peak";
        else if (f=="--language" && (v=="private" || v=="compass")) o.compass=v=="compass";
        else if (f=="--scenes" && (v=="all" || v=="withheld")) o.withheld=v=="withheld";
        else if (f=="--nouns" || f=="--references") {
            auto split=v.find(',');
            if (!split || split==v.npos || split+1==v.size() || v.find(',',split+1)!=v.npos) throw std::invalid_argument("supply two noun forms");
            auto &forms=f=="--nouns" ? o.nouns : o.references;
            forms[0]=v.substr(0,split);forms[1]=v.substr(split+1);
            if (f=="--references") o.discourse=1;
        }
        else if (f=="--voice") {
            o.voice_set=true;
            std::istringstream words(v);std::string word;
            for (auto &name:o.voice) {
                if (!std::getline(words,word,',') || word.empty() || word.find_first_of(" \t\r\n")!=std::string::npos)
                    throw std::invalid_argument("a voice needs four nonempty single-word sounds");
                name=word;
            }
            for (size_t a=0;a<4;++a) for (size_t b=0;b<a;++b)
                if (o.voice[a]==o.voice[b]) throw std::invalid_argument("voice sounds must be distinct");
            if (std::getline(words,word,',')) throw std::invalid_argument("too many voice sounds");
        } else throw std::invalid_argument("invalid dialogue option "+std::string(f));
    }
    if (o.pair>=o.pairs || o.probes%32) throw std::invalid_argument("select an existing pair and a probe count divisible by 32");
    if (o.pairs%o.community || (o.gaze && o.facets!=2)) throw std::invalid_argument("communities must divide the pairs, and choosing a view needs two facets");
    if (o.outcomes==2 && o.pairs<2) throw std::invalid_argument("yoked feedback requires two or more independent pairs");
    if (o.generations && (o.pairs%o.replicates || o.pairs/o.replicates<2))
        throw std::invalid_argument("breeding requires at least two candidate pairs with equal replication");
    if (o.compass && !o.voice_set) o.voice={"north","east","south","west"};
    o.noun_tags=word_tag(o.nouns[0])|(word_tag(o.nouns[1])<<4);
    if (o.compass && (o.noun_tags&15)==(o.noun_tags>>4)) throw std::invalid_argument("the two nouns share a receptor address; choose different forms");
    o.form_hashes={word_digest(o.nouns[0]),word_digest(o.nouns[1]),word_digest(o.references[0]),word_digest(o.references[1])};
    if (o.discourse) {
        if (!o.gaze || !o.compass || o.probes%128) throw std::invalid_argument("reference episodes use --language compass --vision gaze and probes in multiples of 128");
        std::array forms{o.nouns[0],o.nouns[1],o.references[0],o.references[1]};
        for (;;++o.salt) {
            Word occupied=0,tags=0;
            for (Word k=0;k<4;++k) {Word slot=word_tag(forms[k],o.salt)&3;occupied|=1u<<slot;tags|=slot<<(4*k);}
            if (occupied==15) {o.noun_tags=tags;break;}
            if (o.salt==65535) throw std::invalid_argument("four distinguishable word forms are required");
        }
    }
    if (talk && (o.generations || o.play || o.demonstrations || o.practice)) throw std::invalid_argument("use axion dialogue for the developmental playground");
    if (o.interview && (!o.gaze || !o.compass || o.interactive)) throw std::invalid_argument("an interview uses --facets 2 --vision gaze --language compass");
    if (o.withheld && o.facets!=2) throw std::invalid_argument("withheld combinations require two attributes");
    if (o.play && (!o.gaze || o.facets!=2)) throw std::invalid_argument("joint attention play requires the two-view gaze environment");
    if ((!o.world.empty() && !o.interview) || (!o.save_world.empty() && o.world.empty())) throw std::invalid_argument("a living viewpoint needs --interview and a source habitat");
    std::array outputs{o.save_s,o.save_l,o.founder_s,o.founder_l,o.save_world};
    for (size_t a=0;a<outputs.size();++a) for (size_t b=0;b<a;++b)
        if (!outputs[a].empty() && !outputs[b].empty() && std::filesystem::absolute(outputs[a]).lexically_normal()==std::filesystem::absolute(outputs[b]).lexically_normal())
            throw std::invalid_argument("each saved brain needs its own output file");
    return o;
}
AgentImage read_brain(const std::string &path,std::optional<Word> site) {
    AgentImage image;
    char magic[8]{};std::ifstream in(path,std::ios::binary);
    if (!in.read(magic,sizeof(magic))) throw std::runtime_error("cannot read brain "+path);
    if (std::memcmp(magic,"AXMIND1",8)==0) {
        if (site) throw std::invalid_argument("site selection is for habitat founders");
        image.read(path);
    } else {
        Word origin=0;
        image.brain=hatch_from_habitat(path.c_str(),site.value_or(~0u),origin,image.links,image.contexts,image.expression,
                                       false,&image.food,&image.balanced,&image.map,&image.navigation,&image.trace);
    }
    return image;
}
void launch(Session &s,Function f,Count threads,void **args,Stream stream={}) {
    s.d.check(s.d.launch(f,unsigned((threads+127)/128),1,1,128,1,1,0,stream,args,nullptr),"advance acoustic exchange");
}
struct Partner {
    Session &s;
    AgentImage image;
    ActorBatch shape;
    Temporary bodies,paths,organs,ears,memory,caller,iris,imagery,bindings,joint,reports,concepts,agency,sequence,composition_body,practice,ground;
    Function run{};
    bool acoustic_chunk(const void *part) const {
        return part != &image.header && part != &image.resident && part != &image.pod_organ && part != &image.cortex &&
               part != &image.worker && part != &image.forecast && part != &image.inquiry &&
               part != &image.field_speech && part != &image.brine_mind && part != &image.calls &&
               part != &image.native_words && part != &image.ceramic && part != &image.path_memory && part != &image.path_speech && part != &image.prospects && part != &image.mechanism && part != &image.training && part != &image.reed && part != &image.rattle_organ && part != &image.braided && part != &image.sluice && part != &image.braid_report && part!=&image.accounts && part!=&image.accounts.trail && part!=&image.channels && part!=&image.channels.speech && part!=&image.channels.mind && part!=&image.appetitive && part!=&image.compound_contacts && part!=&image.signal_voice;
    }
    Partner(Session &s,AgentImage individual,Word copies,Word salt,bool hearing,bool inert=false):s(s),image(std::move(individual)),
        shape{copies,Word(payload(image.brain.tiles)),association_stride(image.brain.has_relations,1+Word(image.brain.alternate_program!=0)),
              Word((image.balanced ? sizeof(OutcomeLayer) : 0)+map_bytes(image.navigation))},
        bodies(s.d,Count(copies)*shape.body_bytes),paths(s.d,Count(copies)*shape.path_bytes),organs(s.d,Count(copies)*shape.organ_bytes),ears(s.d,(hearing || image.auditory) ? Count(copies)*sizeof(AuditoryLayer) : 0),
        memory(s.d,image.recollects ? Count(copies)*sizeof(Recollection) : 0),
        caller(s.d,image.attends ? Count(copies)*sizeof(AttentiveEar) : 0),iris(s.d,image.orients ? Count(copies)*sizeof(Iris) : 0),
        imagery(s.d,image.imagines ? Count(copies)*sizeof(Imagery) : 0),bindings(s.d,image.binds ? Count(copies)*sizeof(Bindings) : 0),
        joint(s.d,image.conjoins ? Count(copies)*sizeof(Conjunctions) : 0),reports(s.d,image.reports ? Count(copies)*sizeof(Reports) : 0),
        concepts(s.d,image.conceptual ? Count(copies)*sizeof(Concepts) : 0),agency(s.d,image.causal ? Count(copies)*sizeof(Agency) : 0),
        sequence(s.d,image.sequential ? Count(copies)*sizeof(SequenceMind) : 0),
        composition_body(s.d,image.composes ? Count(copies)*sizeof(composition::Mind) : 0),
        practice(s.d,image.procedural ? Count(copies)*sizeof(procedure::Mind) : 0),
        ground(s.d,image.grounded ? Count(copies)*sizeof(grounding::Mind) : 0) {
        image.auditory=bool(ears.ptr);if (inert) image.ear.rule=0;
        std::vector<unsigned char> bytes;
        image.chunks([&](void *p, size_t n) {
            if (!acoustic_chunk(p))
                return;
            auto *first=static_cast<unsigned char *>(p);
            bytes.insert(bytes.end(), first, first + n);
        });
        if (bytes.size()!=this->bytes()) throw std::logic_error("inconsistent dialogue brain layout");
        Function replicate{},seed{};s.bind(replicate,"voice_replicate",Image::dialogue);s.bind(seed,"voice_seed",Image::dialogue);
        size_t offset=0;
        auto fill=[&](DevicePtr ptr,Word n) {
            if (!n) return;
            s.d.check(s.d.upload(ptr,bytes.data()+offset,n),"upload independent animal");offset+=n;
            if (copies>1) {void *args[]{&ptr,&ptr,&n,&copies};launch(s,replicate,Count(copies)*n/4,args);}
        };
        fill(bodies.ptr,shape.body_bytes);fill(paths.ptr,shape.path_bytes);fill(organs.ptr,shape.organ_bytes);fill(ears.ptr,image.auditory ? sizeof(AuditoryLayer) : 0);fill(memory.ptr,image.recollects ? sizeof(Recollection) : 0);
        fill(caller.ptr,image.attends ? sizeof(AttentiveEar) : 0);fill(iris.ptr,image.orients ? sizeof(Iris) : 0);
        fill(imagery.ptr,image.imagines ? sizeof(Imagery) : 0);
        fill(bindings.ptr,image.binds ? sizeof(Bindings) : 0);
        fill(joint.ptr,image.conjoins ? sizeof(Conjunctions) : 0);
        fill(reports.ptr,image.reports ? sizeof(Reports) : 0);
        fill(concepts.ptr,image.conceptual ? sizeof(Concepts) : 0);fill(agency.ptr,image.causal ? sizeof(Agency) : 0);
        fill(ground.ptr,image.grounded ? sizeof(grounding::Mind) : 0);
        fill(practice.ptr,image.procedural ? sizeof(procedure::Mind) : 0);
        fill(sequence.ptr,image.sequential ? sizeof(SequenceMind) : 0);
        fill(composition_body.ptr,image.composes ? sizeof(composition::Mind) : 0);
        void *seeds[]{&bodies.ptr,&shape.body_bytes,&copies,&salt};launch(s,seed,copies,seeds);
        if (image.balanced) s.bind(run,"food"+std::to_string(image.brain.has_relations)+"_actor"+std::to_string(image.brain.tiles),
                                  image.navigation>=4 ? Image::sequence_actors : image.navigation ? Image::mapactors : Image::foodactors);
        else s.bind(run,std::string(neural_prefix(image.brain.has_relations))+"actor"+std::to_string(image.brain.tiles),Image::actors);
    }
    void act(DevicePtr events,DevicePtr answers,Stream stream={},Word first=0,Word copies=0) {
        Word one = 1, cut = 0, frozen = image.grows_cells ? Word(image.cortex.active) << 1 : 0;
        ActorBatch layout = shape;
        if (copies)
            layout.copies = copies;
        DevicePtr b=bodies.ptr+Count(first)*shape.body_bytes,p=paths.ptr+Count(first)*shape.path_bytes,m=organs.ptr+Count(first)*shape.organ_bytes;
        void *args[]{&b,&events,&answers,&one,&p,&m,&image.navigation,&cut,&frozen,&layout};
        launch(s,run,Count(layout.copies)*image.brain.tiles,args,stream);
    }
    Word tick(Word pair) const {
        Word value=0;s.d.check(s.d.download(&value,bodies.ptr+Count(pair)*shape.body_bytes+offsetof(Individual,ticks),sizeof(value)),"read individual clock");return value;
    }
    Word bytes() const {return shape.body_bytes+shape.path_bytes+shape.organ_bytes+(image.auditory ? sizeof(AuditoryLayer) : 0)+(image.recollects ? sizeof(Recollection) : 0)+(image.attends ? sizeof(AttentiveEar) : 0)+(image.orients ? sizeof(Iris) : 0)+(image.imagines ? sizeof(Imagery) : 0)+(image.binds ? sizeof(Bindings) : 0)+(image.conjoins ? sizeof(Conjunctions) : 0)+(image.reports ? sizeof(Reports) : 0)+(image.conceptual ? sizeof(Concepts) : 0)+(image.causal ? sizeof(Agency) : 0)+(image.sequential ? sizeof(SequenceMind) : 0)+(image.composes ? sizeof(composition::Mind) : 0)+(image.procedural ? sizeof(procedure::Mind) : 0)+(image.grounded ? sizeof(grounding::Mind) : 0);}
    VoiceBody form() const {
        const auto &b=image.brain;
        return {shape,b.tiles,b.bits,b.delayed_path,b.outcome_lag,b.has_dendrites,b.has_relations,b.alternate_program,image.balanced,image.navigation,Word(image.auditory),Word(image.recollects),Word(image.attends),Word(image.orients),Word(image.binds),Word(image.conjoins),Word(image.reports),Word(image.conceptual),Word(image.causal),Word(image.sequential),Word(image.composes),Word(image.procedural),Word(image.grounded)};
    }
    VoiceGermline germline() const {
        return {{image.brain.genes,image.brain.layer.genes,{image.links.genes,image.links.convergence},
                image.contexts.genes,image.expression.genes,image.food.genes,image.map.genes,image.trace.genes,0,0},image.ear.rule,image.recollection.genes,image.attention.rule,image.iris.rule,image.bindings.genes,image.conjunctions.gates,image.report.genes,image.concepts.genes,image.agency.model.genes,image.sequence.genes,image.composition.genes,image.practice.genes,image.referent.genes};
    }
    void hatch(Function kernel,DevicePtr parents,Word replicates,Word seed,Word scattered=0) {
        auto body = form();
        image.riverine = false;
        image.invents=false;image.mechanism={};image.practises=false;image.training={};image.folds_reeds=false;image.reed={};image.hears_banks=false;image.rattle_organ={};image.binds_floats=false;image.braided={};image.registers_gates=false;image.sluice={};image.narrates_floats=false;image.braid_report={};
        image.describes=false;image.accounts={};image.hydraulic=false;image.channels={};
        image.values_effort=false;image.prospects={};
        image.ranges=false;image.path_memory={};image.path_speech={};
        image.delayed_pods = false;
        image.grows_cells = false;
        image.handling = false;
        image.projects = false;
        image.investigates = false;
        image.field_speech = {};
        if (image.potter) {
            auto law = image.ceramic.mind.genes;
            image.ceramic = {};
            image.ceramic.mind.genes = law;
        }
        if (image.calling) {
            image.calls.animal = {image.calls.animal.genes};
            image.native_words = {};
        }
        image.brine_mind.cortex.memory = {};
        s.d.check(s.d.zero(bodies.ptr,0,Count(shape.copies)*shape.body_bytes),"reset newborn tissue");
        if (paths.ptr) s.d.check(s.d.zero(paths.ptr,0,Count(shape.copies)*shape.path_bytes),"reset newborn connections");
        if (organs.ptr) s.d.check(s.d.zero(organs.ptr,0,Count(shape.copies)*shape.organ_bytes),"reset newborn sensory organs");
        if (ears.ptr) s.d.check(s.d.zero(ears.ptr,0,Count(shape.copies)*sizeof(AuditoryLayer)),"clear newborn auditory contacts");
        if (memory.ptr) s.d.check(s.d.zero(memory.ptr,0,Count(shape.copies)*sizeof(Recollection)),"clear newborn visual contacts");
        if (caller.ptr) s.d.check(s.d.zero(caller.ptr,0,Count(shape.copies)*sizeof(AttentiveEar)),"clear newborn word projections");
        if (iris.ptr) s.d.check(s.d.zero(iris.ptr,0,Count(shape.copies)*sizeof(Iris)),"clear newborn iris contacts");
        if (imagery.ptr) s.d.check(s.d.zero(imagery.ptr,0,Count(shape.copies)*sizeof(Imagery)),"clear newborn action experience");
        if (bindings.ptr) s.d.check(s.d.zero(bindings.ptr,0,Count(shape.copies)*sizeof(Bindings)),"clear newborn role experience");
        if (joint.ptr) s.d.check(s.d.zero(joint.ptr,0,Count(shape.copies)*sizeof(Conjunctions)),"clear newborn conjunctive contacts");
        if (reports.ptr) s.d.check(s.d.zero(reports.ptr,0,Count(shape.copies)*sizeof(Reports)),"clear newborn speech and event memory");
        if (concepts.ptr) s.d.check(s.d.zero(concepts.ptr,0,Count(shape.copies)*sizeof(Concepts)),"clear newborn relational memories");
        if (agency.ptr) s.d.check(s.d.zero(agency.ptr,0,Count(shape.copies)*sizeof(Agency)),"clear newborn causal experience");
        if (sequence.ptr) s.d.check(s.d.zero(sequence.ptr,0,Count(shape.copies)*sizeof(SequenceMind)),"clear acquired developmental tissue");
        if (ground.ptr) s.d.check(s.d.zero(ground.ptr,0,Count(shape.copies)*sizeof(grounding::Mind)),"clear newborn grounded tissue");
        if (practice.ptr) s.d.check(s.d.zero(practice.ptr,0,Count(shape.copies)*sizeof(procedure::Mind)),"clear acquired procedural tissue");
        if (composition_body.ptr) s.d.check(s.d.zero(composition_body.ptr,0,Count(shape.copies)*sizeof(composition::Mind)),"clear newborn compositional memory");
        void *args[]{&parents,&bodies.ptr,&paths.ptr,&organs.ptr,&body,&replicates,&seed,&ears.ptr,&memory.ptr,&caller.ptr,&iris.ptr,&bindings.ptr,&scattered,&joint.ptr,&reports.ptr,&concepts.ptr,&agency.ptr,&sequence.ptr,&composition_body.ptr,&practice.ptr,&ground.ptr};
        launch(s,kernel,shape.copies,args);
    }
    void save(const std::string &file,Word pair,bool capture=false) {
        if (file.empty() && !(capture && journal::current())) return;
        std::vector<unsigned char> bytes(this->bytes());
        size_t offset=0;
        auto take=[&](DevicePtr ptr,Word n) {if (n) {s.d.check(s.d.download(bytes.data()+offset,ptr+Count(pair)*n,n),"read learned conversational state");offset+=n;}};
        take(bodies.ptr,shape.body_bytes);take(paths.ptr,shape.path_bytes);take(organs.ptr,shape.organ_bytes);take(ears.ptr,image.auditory ? sizeof(AuditoryLayer) : 0);take(memory.ptr,image.recollects ? sizeof(Recollection) : 0);
        take(caller.ptr,image.attends ? sizeof(AttentiveEar) : 0);take(iris.ptr,image.orients ? sizeof(Iris) : 0);
        take(imagery.ptr,image.imagines ? sizeof(Imagery) : 0);
        take(bindings.ptr,image.binds ? sizeof(Bindings) : 0);
        take(joint.ptr,image.conjoins ? sizeof(Conjunctions) : 0);
        take(reports.ptr,image.reports ? sizeof(Reports) : 0);
        take(concepts.ptr,image.conceptual ? sizeof(Concepts) : 0);take(agency.ptr,image.causal ? sizeof(Agency) : 0);
        take(ground.ptr,image.grounded ? sizeof(grounding::Mind) : 0);
        take(practice.ptr,image.procedural ? sizeof(procedure::Mind) : 0);
        take(sequence.ptr,image.sequential ? sizeof(SequenceMind) : 0);
        take(composition_body.ptr,image.composes ? sizeof(composition::Mind) : 0);
        offset=0;
        image.chunks([&](void *p, size_t n) {
            if (acoustic_chunk(p)) {
                std::memcpy(p, bytes.data() + offset, n);
                offset += n;
            }
        });
        if (capture) image.specimen(file.empty() && journal::current() ? journal::current()->source : file,"speaker");
        if (!file.empty()) write_checked(file,image);
    }
};
#include "flood_pod_interview.hpp"
#include "flood_workbench.hpp"
#include "siphon_talk.hpp"
#include "commons_host.hpp"
#include "concept_dialogue.hpp"
#include "garden_dialogue.hpp"
struct AcousticWorld {
    Session &s;
    Partner &speaker,&listener;
    const Options &o;
    Temporary states,events,answers,scores,prepared,gate,blocks,maps;
    std::optional<Word> optical_seed;Word optical_worlds=0;
    Function scene{},question{},answer{},settle{},word{},demonstrate{},handle{},glance{},play_scene{},play_food{},keep{},rehear{},point{},reduce{},shuffle{},focus_lesson{},motion_lesson{},pair_lesson{},experiment{},arrival{};
    GraphApi api{};
    Stream stream{};
    Event began{},ended{};
    AcousticWorld(Session &s,Partner &a,Partner &b,const Options &o):s(s),speaker(a),listener(b),o(o),
        states(s.d,Count(o.pairs)*sizeof(Exchange)),events(s.d,Count(o.pairs)*sizeof(Encounter)),
        answers(s.d,Count(o.pairs)*sizeof(Response)),scores(s.d,Count(o.pairs)*sizeof(DialogueScore)),prepared(s.d,(o.peak || o.matched) ? sizeof(Response) : 0),gate(s.d,(o.peak || o.matched) ? sizeof(Word) : 0),
        blocks(s.d,Count((o.pairs+127)/128)*sizeof(SpeechBlock)),maps(s.d,o.shuffled ? Count(o.pairs)*sizeof(OpticalMap) : 0) {
        s.bind(scene,"voice_scene",Image::dialogue);s.bind(question,"voice_question",Image::dialogue);
        s.bind(answer,"voice_answer",Image::dialogue);s.bind(settle,"voice_result",Image::dialogue);s.bind(word,"voice_human",Image::dialogue);
        s.bind(demonstrate,"voice_demonstration",Image::dialogue);s.bind(handle,"voice_handle",Image::dialogue);
        s.bind(glance,"voice_glance",Image::dialogue);
        s.bind(play_scene,"voice_play_scene",Image::dialogue);s.bind(play_food,"voice_play_food",Image::dialogue);
        s.bind(keep,"voice_keep",Image::dialogue);
        s.bind(focus_lesson,"voice_focus_lesson",Image::dialogue);
        s.bind(motion_lesson,"voice_motion_lesson",Image::dialogue);
        s.bind(pair_lesson,"voice_pair_lesson",Image::dialogue);
        s.bind(experiment,"voice_experiment",Image::dialogue);s.bind(arrival,"voice_arrival",Image::dialogue);
        s.bind(shuffle,"voice_shuffle",Image::dialogue);
        s.bind(reduce,"voice_reduce",Image::dialogue);
        s.bind(rehear,"voice_rehear",Image::dialogue);s.bind(point,"voice_point",Image::dialogue);
        api.bind(s.d);s.d.check(api.create_stream(&stream,0),"open conversation stream");
        s.d.check(s.d.event_new(&began,0),"create conversation timer");s.d.check(s.d.event_new(&ended,0),"create conversation timer");
    }
    ~AcousticWorld() {
        if (ended) s.d.event_free(ended);
        if (began) s.d.event_free(began);
        if (stream) api.destroy_stream(stream);
    }
    void exchange(DialogueConfig cfg) {
        if (cfg.learning==3) {
            for (Word part=0;part<4;++part) {
                auto &animal=(part==0 || part==3) ? listener : speaker;
                void *present[]{&states.ptr,&events.ptr,&animal.bodies.ptr,&animal.shape.body_bytes,&cfg,&part};
                launch(s,play_scene,cfg.pairs,present,stream);
                animal.act(events.ptr,answers.ptr,stream);
                void *food[]{&states.ptr,&answers.ptr,&scores.ptr,&animal.bodies.ptr,&animal.shape.body_bytes,&cfg,&part,&listener.ears.ptr};
                launch(s,play_food,cfg.pairs,food,stream);
            }
            return;
        }
        if (cfg.learning==2) {
            void *show[]{&states.ptr,&events.ptr,&speaker.bodies.ptr,&speaker.shape.body_bytes,&cfg};
            launch(s,demonstrate,cfg.pairs,show,stream);
            speaker.act(events.ptr,answers.ptr,stream);
            void *food[]{&states.ptr,&answers.ptr,&scores.ptr,&speaker.bodies.ptr,&speaker.shape.body_bytes,&cfg};
            launch(s,handle,cfg.pairs,food,stream);return;
        }
        auto &first=cfg.facets==2 ? listener : speaker;
        void *start[]{&states.ptr,&events.ptr,&first.bodies.ptr,&first.shape.body_bytes,&cfg};
        launch(s,scene,cfg.pairs,start,stream);
        if (cfg.facets==2) {
            listener.act(events.ptr,answers.ptr,stream);
            void *ask[]{&states.ptr,&answers.ptr,&events.ptr,&speaker.bodies.ptr,&speaker.shape.body_bytes,&cfg,&listener.bodies.ptr,&listener.shape.body_bytes};
            launch(s,question,cfg.pairs,ask,stream);
            if (cfg.active_vision) {
                speaker.act(events.ptr,answers.ptr,stream);
                launch(s,glance,cfg.pairs,ask,stream);
            }
        }
        speaker.act(events.ptr,answers.ptr,stream);
        void *hear[]{&states.ptr,&answers.ptr,&events.ptr,&listener.bodies.ptr,&listener.shape.body_bytes,&cfg};
        launch(s,answer,cfg.pairs,hear,stream);
        listener.act(events.ptr,answers.ptr,stream);
        void *outcome[]{&states.ptr,&answers.ptr,&scores.ptr,&speaker.bodies.ptr,&speaker.shape.body_bytes,&listener.bodies.ptr,&listener.shape.body_bytes,&cfg,&listener.ears.ptr};
        launch(s,settle,cfg.pairs,outcome,stream);
    }
    void optics(Word seed,Word worlds) {
        if (!maps.ptr || (optical_seed==seed && optical_worlds==worlds)) return;
        auto n=o.pairs,community=o.community;
        void *args[]{&maps.ptr,&n,&seed,&worlds,&community};launch(s,shuffle,Count(n)*32,args);
        optical_seed=seed;optical_worlds=worlds;
    }
    float run(DialogueConfig cfg,Word rounds) {
        optics(cfg.seed,cfg.worlds);cfg.optics=maps.ptr;
        cfg.recall=speaker.memory.ptr;cfg.recall_cut=Word(o.recall_cut);
        cfg.outcomes=o.outcomes;cfg.distributed=o.distributed;std::copy(o.form_hashes.begin(),o.form_hashes.end(),cfg.forms);
        s.d.check(s.d.zero(states.ptr,0,Count(cfg.pairs)*sizeof(Exchange)),"clear observations");
        s.d.check(s.d.zero(scores.ptr,0,Count(cfg.pairs)*sizeof(DialogueScore)),"clear communication scores");
        struct Program {
            GraphApi &api;
            Graph graph{};
            GraphExec executable{};
            ~Program(){if (executable) api.destroy_exec(executable);if (graph) api.destroy(graph);}
        } p{api};
        constexpr Word period=8;
        if (o.graphs && rounds>=period) {
            s.d.check(api.begin(stream,1),"capture acoustic exchanges");
            try {for (Word j=0;j<period;++j) exchange(cfg);}
            catch (...) {api.end(stream,&p.graph);throw;}
            s.d.check(api.end(stream,&p.graph),"finish acoustic graph");
            s.d.check(api.instantiate(&p.executable,p.graph,0),"instantiate acoustic graph");
        }
        s.d.check(s.d.record(began,stream),"start communication timer");
        while (p.executable && rounds>=period) {s.d.check(api.launch(p.executable,stream),"run acoustic graph");rounds-=period;}
        while (rounds) {exchange(cfg);--rounds;}
        s.d.check(s.d.record(ended,stream),"end communication timer");s.d.check(s.d.wait(ended),"complete acoustic exchanges");
        float ms=0;s.d.check(s.d.elapsed(&ms,began,ended),"time acoustic exchanges");return ms;
    }
    void stage(std::string label,Word rounds,bool learn,Word reversal,Wire wire=Wire::open) {
        if (!rounds) return;
        constexpr std::array names{"open","silent","shifted","hidden","no-question","shift-question","stranger","fixed-gaze","muted-self","shifted-self"};
        DialogueConfig cfg{o.pairs,o.seed,o.facets,(o.offset+reversal)&3,label=="play" ? 3u : label=="handling" ? 2u : Word(learn),o.plasticity,0,o.community,o.gaze,o.compass,o.noun_tags,o.withheld,o.discourse,wire};
        float ms=run(cfg,rounds);
        void *reduction[]{&scores.ptr,&cfg.pairs,&blocks.ptr};launch(s,reduce,o.pairs,reduction);
        std::vector<SpeechBlock> partial((o.pairs+127)/128);
        s.d.check(s.d.download(partial.data(),blocks.ptr,partial.size()*sizeof(SpeechBlock)),"read communication totals");
        std::array<Count,8> values{};double information=0;
        for (const auto &block:partial) {
            for (size_t j=0;j<values.size();++j) values[j]+=block.counts[j];
            information+=block.information;
        }
        auto [correct,attempts,spoken,acted,perfect,gazes,relative,relative_correct]=values;
        std::cout<<"dialogue="<<label<<"  wire="<<names[unsigned(wire)]<<"  pairs="<<o.pairs<<"  facets="<<o.facets
                 <<"  correct="<<correct<<"  exchanges="<<attempts<<"  voiced="<<spoken<<"  acted="<<acted<<"  perfect_pairs="<<perfect
                 <<"  relative="<<relative<<"  relative_correct="<<relative_correct<<"  relevant_gazes="<<gazes<<"  bits_per_call="<<std::fixed<<std::setprecision(4)<<information/o.pairs<<"  GPU_ms="<<std::setprecision(3)<<ms<<'\n';
        if (label=="learned" || label=="revised") {
            if (wire==Wire::open) {
                DialogueScore sample{};s.d.check(s.d.download(&sample,scores.ptr+Count(o.pair)*sizeof(sample),sizeof(sample)),"read the observed dialect");
                std::cout<<"dialect="<<label<<"  pair="<<o.pair<<"  meanings=4  sounds=5  counts=";
                for (int x=0;x<4;++x) for (int y=0;y<5;++y) std::cout<<(x||y ? "," : "")<<sample.calls[x][y];
                std::cout<<"  requests=";
                for (int x=0;x<2;++x) for (int y=0;y<5;++y) std::cout<<(x||y ? "," : "")<<sample.requests[x][y];
                std::cout<<'\n';
            }
        }
        if (auto *j=journal::current();j && !learn) j->scores.push_back({label+(wire==Wire::open ? "" : " / "+std::string(names[unsigned(wire)])),correct,attempts});
    }
    void breed() {
        if (!o.generations) return;
        Word count=o.pairs/o.replicates,replicates=o.replicates;
        Temporary first(s.d,2*Count(count)*sizeof(VoiceGermline)),second(s.d,2*Count(count)*sizeof(VoiceGermline));
        Temporary fitness(s.d,Count(count)*sizeof(Word)),summary(s.d,3*sizeof(Count));
        Function hatch{},assess{},brood{},survey{};
        s.bind(hatch,"voice_hatch",Image::vocal_brood);s.bind(assess,"voice_merit",Image::vocal_brood);
        s.bind(brood,"voice_select",Image::vocal_brood);s.bind(survey,"voice_survey",Image::vocal_brood);
        std::vector<VoiceGermline> founders(2*count,speaker.germline());
        std::fill(founders.begin()+count,founders.end(),listener.germline());
        s.d.check(s.d.upload(first.ptr,founders.data(),founders.size()*sizeof(VoiceGermline)),"found the vocal nursery");
        DevicePtr parents=first.ptr,children=second.ptr;
        Word best=0;auto a=speaker.form(),b=listener.form();
        for (Word g=0;g<o.generations;++g) {
            Word key=mix(o.seed^g*0xe7954d9bu^0x7de469c3u);
            speaker.hatch(hatch,parents,replicates,key);
            listener.hatch(hatch,parents+Count(count)*sizeof(VoiceGermline),replicates,key^0x76fb2e95u);
            DialogueConfig cfg{o.pairs,mix(key^0xb532e1f5u),o.facets,0,1,o.plasticity,replicates,o.community,o.gaze,o.compass,o.noun_tags,o.withheld,o.discourse,Wire::open};
            if (o.play) { cfg.learning=3;run(cfg,o.play); }
            if (o.demonstrations) { cfg.learning=2;run(cfg,o.demonstrations); }
            for (Word epoch=0;epoch<(o.reverse ? 2u : 1u);++epoch) {
                cfg.learning=1;cfg.reversal=(o.offset+epoch)&3;
                if (o.practice) run(cfg,o.practice);
                cfg.learning=0;run(cfg,o.probes);
                void *score_args[]{&scores.ptr,&fitness.ptr,&cfg.pairs,&replicates,&epoch};
                launch(s,assess,count,score_args);
            }
            if (g==0 || g+1==o.generations || (g+1)%std::max(1u,o.generations/8)==0) {
                void *read[]{&fitness.ptr,&count,&summary.ptr};launch(s,survey,128,read);
                Count r[3];s.d.check(s.d.download(r,summary.ptr,sizeof(r)),"read nursery progress");best=Word(r[2]);
                std::cout<<"vocal_generation="<<g+1<<"  candidates="<<count<<"  total="<<r[0]<<"  best="<<r[1]<<"  family="<<best<<'\n';
                if (auto *j=journal::current()) j->breeding.push_back({g+1,count,{r[0],r[1]}});
            }
            if (g+1<o.generations) {
                Word drift=o.drift;void *breed_args[]{&parents,&children,&fitness.ptr,&count,&key,&drift,&a,&b};
                launch(s,brood,count,breed_args);std::swap(parents,children);
            }
        }
        // An independently seeded common garden starts every descendant
        // from its inherited program. No acquired sound or action is copied.
        speaker.hatch(hatch,parents+Count(best)*sizeof(VoiceGermline),o.pairs,mix(o.seed^0x78af219bu));
        listener.hatch(hatch,parents+Count(count+best)*sizeof(VoiceGermline),o.pairs,mix(o.seed^0xd34ece51u));
        std::cout<<"vocal_nursery="<<(o.drift ? "drift" : "selection")<<"  generations="<<o.generations<<"  candidates="<<count
                 <<"  paired_lifetimes="<<Count(o.pairs)*o.generations<<"  selected_family="<<best<<'\n';
    }
    void teach(const std::string &name,Word expected,Word place_a,Word place_b) {
        Exchange physical{};physical.shape=place_a;physical.colour=place_b;
        s.d.check(s.d.upload(states.ptr,&physical,sizeof(physical)),"show the pointing scene");
        Word pair=o.pair,local=pair/o.community,seed=o.seed,hash=word_digest(name),zero=0,one=1;
        DevicePtr asker=listener.bodies.ptr+Count(pair)*listener.shape.body_bytes,witness=speaker.bodies.ptr+Count(pair)*speaker.shape.body_bytes;
        DevicePtr ear=listener.ears.ptr+Count(pair)*sizeof(AuditoryLayer),no_gate=0;
        DialogueConfig cfg{1,seed^(local*0x9e3779b9u),2,0,1,3,0,1,1,1,o.noun_tags,0,1,Wire::open};
        cfg.optics=maps.ptr ? maps.ptr+Count(pair)*sizeof(OpticalMap) : 0;
        auto present=[&](const std::string &token,Word exploration) {
            Word stimulus=word_digest(token),tag=word_tag(token,o.salt)&3;
            ImaginedWord idle{};
            void *hear[]{&events.ptr,&listener.bodies.ptr,&listener.shape.body_bytes,&pair,&local,&seed,&tag,&zero,&one,
                         &stimulus,&zero,&zero,&listener.ears.ptr,&no_gate,&exploration,&maps.ptr,&no_gate,&idle};
            launch(s,word,1,hear);listener.act(events.ptr,answers.ptr,nullptr,pair,1);
        };
        // A different afferent branch is tried against each preceding
        // referent. Only the physical gaze outcome reaches the new contacts.
        for (Word trial=0;trial<192;++trial) {
            Word phase=mix(seed^hash^((trial/8)*0x736228b1u));
            Word draw=((trial&7)*((phase>>8)|1)+phase)&7,projection=draw&3;
            present(o.nouns[draw>>2],~0u);
            void *remember[]{&asker,&answers.ptr};launch(s,rehear,1,remember);
            present(name,projection);
            void *wire[]{&states.ptr,&answers.ptr,&events.ptr,&witness,&speaker.shape.body_bytes,&cfg,&asker,&listener.shape.body_bytes};
            launch(s,question,1,wire);speaker.act(events.ptr,answers.ptr,nullptr,pair,1);launch(s,glance,1,wire);
            void *feedback[]{&ear,&asker,&states.ptr,&expected,&hash,&projection};launch(s,point,1,feedback);
        }
        if (auto *j=journal::current()) ++j->metrics["taught_forms"];
        std::cout<<"Completed 192 pointing trials.\n"<<std::flush;
    }
    void record_auditory(std::string_view key) {
        auto *j=journal::current();
        if (!j || !listener.ears.ptr) return;
        AuditoryLayer ear{};s.d.check(s.d.download(&ear,listener.ears.ptr+Count(o.pair)*sizeof(ear),sizeof(ear)),"observe auditory projections");
        Word threshold=2+((listener.image.brain.genes.receptors>>15)&3),active=0;
        for (Word i=0;i<16;++i) active+=((ear.efficacy[i/8]>>(4*(i&7)))&15)>=threshold;
        j->metrics[std::string(key)]=active;
    }
    void interview() {
        record_auditory("initial_word_forms");
        Count asker_ticks=listener.tick(o.pair),witness_ticks=speaker.tick(o.pair);
        Word place_a=0,place_b=1,local=o.pair/o.community,shutters=0,dim=0;
        Word seed=o.seed^(local*0x9e3779b9u),pair=o.pair;
        ConceptConversation concepts(s,speaker,listener,pair);
        GardenConversation garden(s,speaker,listener,pair,concepts);
        DevicePtr raw=speaker.sequence.ptr ? speaker.sequence.ptr+Count(pair)*sizeof(SequenceMind) : 0;
        DevicePtr learned=speaker.composition_body.ptr ? speaker.composition_body.ptr+Count(pair)*sizeof(composition::Mind) : 0;
        if (raw) s.d.check(s.d.download(&speaker.image.sequence,raw,sizeof(SequenceMind)),"open this resident’s acquired motor inventory");
        if (learned) s.d.check(s.d.download(&speaker.image.composition,learned,sizeof(composition::Mind)),"open this resident’s acquired constructions");
        DevicePtr procedure=speaker.practice.ptr ? speaker.practice.ptr+Count(pair)*sizeof(axion::life::procedure::Mind) : 0;
        DevicePtr grounded=speaker.ground.ptr ? speaker.ground.ptr+Count(pair)*sizeof(grounding::Mind) : 0;
        grounding::GroundedSpeech field(s,speaker.image.referent,bool(grounded),grounded);
        DevicePtr listening_eye = listener.ground.ptr ? listener.ground.ptr + Count(pair) * sizeof(grounding::Mind) : 0;
        SiphonInterview native(s,speaker.image,listener.image,
            speaker.bodies.ptr+Count(pair)*speaker.shape.body_bytes+offsetof(Individual,tissue),
            listener.bodies.ptr+Count(pair)*listener.shape.body_bytes+offsetof(Individual,tissue));
        MaterialInterview building(s, speaker.image, listener.image, grounded, listening_eye, field);
        PodInterview feeding(s, speaker.image, grounded);
        DevelopmentalSpeech pupil(s,speaker.image,raw,learned,procedure);
        DevicePtr partner_practice=listener.practice.ptr ? listener.practice.ptr+Count(pair)*sizeof(axion::life::procedure::Mind) : 0;
        axion::life::procedure::MotorSpeech partner_motor(s,listener.image.practice,bool(partner_practice),partner_practice);
        bool self_hearing=speaker.image.causal && bool(raw || learned || procedure);
        auto feedback=[&](std::vector<Count> waves,const std::string &line) {
            if (waves.empty()) return false;
            for (auto &wave:waves) wave=acoustic::unwave(wave);
            bool acted=(procedure && garden.committed(pupil.motor_port(),waves)) || garden.heard(waves,false,line);
            if (acted && journal::current()) ++journal::current()->metrics["vocal_plans"];
            return acted;
        };
        DevicePtr witness=speaker.bodies.ptr+Count(pair)*speaker.shape.body_bytes;
        DialogueConfig cfg{1,seed,2,0,1,3,0,1,1,o.compass,o.noun_tags,0,o.discourse,Wire::open};
        optics(o.seed,0);cfg.optics=maps.ptr ? maps.ptr+Count(pair)*sizeof(OpticalMap) : 0;
        cfg.recall=speaker.memory.ptr ? speaker.memory.ptr+Count(pair)*sizeof(Recollection) : 0;cfg.recall_cut=Word(o.recall_cut);
        cfg.caller=listener.caller.ptr ? listener.caller.ptr+Count(pair)*sizeof(AttentiveEar) : 0;
        cfg.iris=speaker.iris.ptr ? speaker.iris.ptr+Count(pair)*sizeof(Iris) : 0;cfg.pitch_cut=o.pitch_cut;
        Temporary projection(s.d,o.replay && speaker.image.imagines ? sizeof(Projection) : 0);
        DevicePtr imagined=speaker.imagery.ptr ? speaker.imagery.ptr+Count(pair)*sizeof(Imagery) : 0;
        cfg.projection=projection.ptr;
        Temporary role_wave(s.d,o.replay && speaker.image.binds ? sizeof(RoleProjection) : 0);
        cfg.roles=role_wave.ptr;cfg.bindings=speaker.bindings.ptr ? speaker.bindings.ptr+Count(pair)*sizeof(Bindings) : 0;
        Temporary scenes(s.d,role_wave.ptr && speaker.joint.ptr ? sizeof(SceneWave) : 0);
        cfg.scenes=scenes.ptr;cfg.joint=speaker.joint.ptr ? speaker.joint.ptr+Count(pair)*sizeof(Conjunctions) : 0;
        Temporary choice(s.d,cfg.bindings ? sizeof(Word) : 0);
        DevicePtr reporter=speaker.reports.ptr ? speaker.reports.ptr+Count(pair)*sizeof(Reports) : 0;
        DevicePtr audience=listener.reports.ptr ? listener.reports.ptr+Count(pair)*sizeof(Reports) : 0;
        Temporary emission(s.d,reporter || audience ? sizeof(Utterance) : 0);
        Function observe_report{},learn_report{},say_report{},converse{};Word shared_view=0;
        if (emission.ptr) {
            s.bind(observe_report,"voice_report_watch",Image::dialogue);
            s.bind(learn_report,"voice_report_lesson",Image::dialogue);s.bind(say_report,"voice_report_say",Image::dialogue);
            s.bind(converse,"voice_report_converse",Image::dialogue);
        }
        auto hear_statement=[&](std::istream &in,Utterance &sample) {
            std::string term;
            while (in>>term) {
                Count sound=sound_pattern(term);
                if (!sound || sample.count==report_length) return false;
                sample.sound[sample.count++]=sound;
            }
            return sample.count!=0;
        };
        auto sentence=[](const Utterance &speech) {
            std::string out;
            for (Word j=0;j<speech.count;++j) {if (j) out+=' ';out+=transcribe(speech.sound[j]);}
            return out.empty() ? std::string("(silence)") : out;
        };
        auto observe_motion=[&]() {
            if (!emission.ptr) return;
            Word closed=shutters|dim;
            void *args[]{&reporter,&audience,&place_a,&place_b,&closed,&shared_view};launch(s,observe_report,1,args);
        };
        auto bearings=[&](std::istream &input,Word n,Word &packed) {
            std::string label;packed=0;
            for (Word k=0;k<n;++k) {
                if (!(input>>label)) return false;
                auto pos=std::find(o.voice.begin(),o.voice.end(),label);if (pos==o.voice.end()) return false;
                packed|=Word(pos-o.voice.begin())<<(2*k);
            }
            return !(input>>label);
        };
        std::unique_ptr<HabitatObserver> habitat;
        Word x=0,y=0;std::optional<ResidentView> followed;
        auto look=[&]() {
            if (followed) {
                if (habitat->locate(*followed,x,y,false)) {
                    x=followed->position%habitat->side();y=followed->position/habitat->side();
                } else {
                    std::cout<<"Resident "<<followed->slot<<" is no longer alive at tick "<<habitat->tick()<<".\n";
                    if (auto *j=journal::current()) j->line("resident_lost="+std::to_string(followed->slot)+"  tick="+std::to_string(habitat->tick()));
                    followed.reset();
                }
            }
            auto seen=habitat->observe(x,y);place_a=seen.food_bearing;place_b=seen.water_bearing;
            observe_motion();
            std::cout<<"Habitat tick "<<habitat->tick()<<", viewpoint ("<<x<<", "<<y<<").\n"<<std::flush;
            if (auto *j=journal::current()) {
                journal::Viewpoint v{j->metrics["human_turns"],habitat->tick(),x,y,habitat->side(),{},{},~0u,0,0,0};
                std::copy_n(seen.food,4,v.food.begin());std::copy_n(seen.water,4,v.water.begin());
                if (followed) {v.slot=followed->slot;v.founder=followed->founder;v.born=followed->born;v.generation=followed->generation;}
                j->viewpoints.push_back(v);
                std::ostringstream record;record<<"habitat_view="<<habitat->tick()<<"  x="<<x<<"  y="<<y<<"  side="<<habitat->side()<<"  food=";
                for (Word k=0;k<4;++k) record<<(k ? "," : "")<<seen.food[k];
                record<<"  water=";for (Word k=0;k<4;++k) record<<(k ? "," : "")<<seen.water[k];
                if (followed) record<<"  resident="<<followed->slot<<"  founder="<<followed->founder<<"  born="<<followed->born<<"  generation="<<followed->generation;
                j->line(record.str());
            }
        };
        if (!o.world.empty()) {
            habitat=open_habitat(o.world);x=habitat->side()/2;y=x;look();
            std::cout<<"Use /visit X Y to move, /follow to track a living resident, and /advance N to run the habitat.\n";
        } else std::cout<<"Place "<<o.nouns[0]<<" and "<<o.nouns[1]<<" with /scene followed by two bearings. Type a question to hear the speaker. /teach WORD 1 or 2 gives pointing lessons. /quit ends the session.\n"<<std::flush;
        if (reporter) std::cout<<"After moving one object, ask what happened. /heard reads the partner's event cells, and /recount CUE lets it speak from what it heard.\n"<<std::flush;
        if (speaker.image.conceptual) {
            const auto &c=speaker.image.concepts;Count route=c.words[4].form;
            if (!route) for (const auto &word:c.words) if (word.form) route=word.form;
            if (route && c.nouns[0]) std::cout<<"Ask “Which "<<transcribe(route)<<" for "<<transcribe(c.nouns[0])<<"?” about the map. ";
            std::cout<<"Use /example WORD SOURCE DESTINATION to teach a relation.\n"<<std::flush;
        }
        if (speaker.image.delayed_pods)
            std::cout << "Open a feeding tray with /tidalpool SEED. /ripen 768 offers mixed fruit. /grip-word WORD "
                         "asks for a grip on a named object in the tabletop scene.\n";
        if(speaker.image.potter && listener.image.calling) {
            std::cout<<"Open /wheel SEED to watch tools, sample water, and ask learned questions about which tool will work.\n";
            std::cout<<"At a reed table, /kiln FILE covered places a worked object. The resident can sound it when a learned question needs evidence.\n";
        }
        if (speaker.image.calling && listener.image.calling)
            std::cout << "Open /reed SEED and enter /duet for the two residents' native exchange. /caption WORDS binds "
                         "a witnessed event.\n";
        if (speaker.image.saline)
            std::cout << "Open a new solvent with /brine-table SCENE WATER SOLUTES. /consult-water auto 9 lets the "
                         "speaker ask a peer for reference experiments.\n";
        if (speaker.image.investigates)
            std::cout << "Set out two samples and two soils with /assay-table SEED. Ask a learned question, "
                         "or demonstrate an answer with /touch-coupons CHIP and /show-question CHIP SOIL WORDS.\n";
        if (speaker.image.projects)
            std::cout << "Open a loose construction bed with /shore SEED [SPAN]. /shore-requests on lets a learned "
                         "name or the answer to a spatial question direct the builder.\n";
        if (speaker.image.handling)
            std::cout << "Open a mineral tray with /workbench SEED [SPAN]. /handle-scraps N explores the motors, "
                         "/show-work peer observes the partner and /sun N measures what the arrangement produces.\n";
        if (procedure) std::cout<<"Use /imitate HEARD | MOVES and /rehearse to teach. /begin SCORE and /step VIEW perform a score one motor pulse at a time.\n";
        if (speaker.image.causal) std::cout<<"Open a physical room with /greenhouse 16 0. You can ask for a plan, ask why, or ask the animal to carry it out.\n"<<std::flush;
        for (std::string line;std::getline(std::cin,line);) {
            if (line=="/quit") break;
            if ((raw || learned || procedure) && (line=="/self-hear on" || line=="/self-hear off")) {
                self_hearing=line.ends_with(" on");std::cout<<"self_hearing="<<self_hearing<<std::endl;continue;
            }
            if ((raw || learned || procedure) && line.starts_with("/enact ")) {
                std::vector<Count> waves;pupil.hear(line.substr(7),true,&waves,true);
                if (!feedback(waves,line)) std::cout<<"No grounded action."<<std::endl;
                continue;
            }
            if (native.control(line))
                continue;
            if (building.control(line))
                continue;
            if (feeding.control(line))
                continue;
            if (grounded) {
                std::string reply;if (field.hear(line,reply)) {std::cout<<"animal> "<<reply<<std::endl;continue;}
            }
            if (concepts.control(line)) continue;
            if (procedure && garden.procedure(pupil.motor_port(),partner_motor,line)) continue;
            if (garden.control(line)) continue;
            if (raw || learned || procedure) {
                std::vector<Count> waves;
                if (pupil.hear(line,true,self_hearing ? &waves : nullptr)) {
                    if (self_hearing) feedback(waves,line);
                    continue;
                }
            }
            if (!line.empty() && line.back()=='.' && garden.respond(line)) continue;
            if (!line.empty() && line.back()=='.' && concepts.respond(line,true)) continue;
            if (audience && (line.starts_with("/say ") || line.starts_with("/recount ") || (!line.empty() && line[0]!='/' && line.back()=='.'))) {
                bool cued=line.starts_with("/say "),retell=line.starts_with("/recount ");
                std::string phrase=retell ? line.substr(9) : cued ? line.substr(5) : line.substr(0,line.size()-1);
                std::istringstream in(phrase);std::string label,extra;Word cue=0,operation=retell,cut=o.report_wire;Utterance speech{};
                if (cued || retell) {in>>label;cue=word_digest(label);}
                if ((retell && (label.empty() || in>>extra)) || (!retell && !hear_statement(in,speech))) {
                    std::cout<<"Use /say CUE followed by the spoken words, or /recount CUE.\n"<<std::flush;continue;
                }
                if (!retell) s.d.check(s.d.upload(emission.ptr,&speech,sizeof(speech)),"present a statement to the addressee alone");
                void *args[]{&audience,&emission.ptr,&cue,&operation,&cut};launch(s,converse,1,args);
                if (retell) {
                    s.d.check(s.d.download(&speech,emission.ptr,sizeof(speech)),"hear the listener's retelling");
                    std::cout<<"listener> "<<sentence(speech)<<'\n'<<std::flush;
                    if (auto *j=journal::current()) {++j->metrics["retellings"];j->metrics["retold_words"]+=speech.count;j->line("retelling="+std::to_string(speech.count)+"  speech="+sentence(speech));}
                } else {
                    std::cout<<"The listener heard the statement.\n"<<std::flush;
                    if (auto *j=journal::current()) {
                        Word belief[2]{};s.d.check(s.d.download(belief,audience+offsetof(Reports,heard),sizeof(belief)),"record the listener's interpretation");
                        ++j->metrics["incoming_reports"];j->line("incoming_report="+std::to_string(speech.count)+"  decoded="+std::to_string(belief[0])
                                                                 +"  decoded_prior="+std::to_string(belief[1])+"  speech="+sentence(speech));
                    }
                }
                continue;
            }
            if (line=="/audience open" || line=="/audience closed") {
                shared_view=line=="/audience open";observe_motion();
                std::cout<<"Audience view "<<(shared_view ? "open" : "closed")<<".\n"<<std::flush;continue;
            }
            if (line=="/heard") {
                Word heard[2]{};
                if (audience) s.d.check(s.d.download(heard,audience+offsetof(Reports,heard),sizeof(heard)),"read the listener's own event cells");
                for (Word age=0;age<2;++age) {
                    std::cout<<(age ? "earlier> " : "receiver> ");
                    if (std::has_single_bit(heard[age])) {
                        Word e=std::countr_zero(heard[age]);std::cout<<o.nouns[e&1]<<' '<<o.voice[(e>>1)&3]<<' '<<o.voice[e>>3];
                    } else std::cout<<"(uncertain)";
                    std::cout<<'\n';
                }
                std::cout<<std::flush;continue;
            }
            if (line.starts_with("/teach-report ")) {
                std::istringstream in(line.substr(14));std::string cue;in>>cue;Utterance sample{};
                if (!emission.ptr || cue.empty() || !hear_statement(in,sample)) {std::cout<<"Use /teach-report CUE and one to eight words of up to twelve letters with a reporting individual.\n"<<std::flush;continue;}
                Word form=word_digest(cue);s.d.check(s.d.upload(emission.ptr,&sample,sizeof(sample)),"present spoken demonstration");
                void *args[]{&reporter,&audience,&form,&emission.ptr,&shared_view};launch(s,learn_report,1,args);
                s.d.check(s.d.download(&sample,emission.ptr,sizeof(sample)),"observe speech acquisition");
                std::cout<<"report lesson> "<<sample.handled<<'\n'<<std::flush;
                if (auto *j=journal::current()) j->metrics["report_lessons"]+=bool(sample.handled);
                continue;
            }
            if (line.starts_with("/show-pair ")) {
                std::istringstream input(line.substr(11));std::string name;input>>name;Word seen=0;
                if (!cfg.bindings) {std::cout<<"This animal needs object-binding contacts.\n";continue;}
                if (name.empty() || !bearings(input,4,seen)) {std::cout<<"Use /show-pair WORD FOOD-BEFORE WATER-BEFORE FOOD-AFTER WATER-AFTER.\n";continue;}
                Word form=word_digest(name),before=seen&15,after=seen>>4,credit=o.binding_credit;
                void *args[]{&cfg.bindings,&cfg.recall,&form,&before,&after,&credit,&cfg.joint};
                launch(s,pair_lesson,1,args);if (auto *j=journal::current()) ++j->metrics["observed_pairs"];
                std::cout<<"Observed both departures and arrivals.\n"<<std::flush;continue;
            }
            if (line.starts_with("/try-pair ")) {
                std::istringstream input(line.substr(10));std::string name;input>>name;Word offered=0;
                if (!cfg.bindings || habitat) {std::cout<<"Experiment selection requires the tabletop and object-binding contacts.\n"<<std::flush;continue;}
                if (name.empty() || !bearings(input,8,offered)) {std::cout<<"Use /try-pair WORD and four pairs of bearings.\n"<<std::flush;continue;}
                Word form=word_digest(name),seed=o.seed,random=o.random_experiment,chosen=0;
                void *args[]{&cfg.bindings,&cfg.recall,&form,&offered,&seed,&random,&choice.ptr};
                launch(s,experiment,1,args);s.d.check(s.d.download(&chosen,choice.ptr,sizeof(chosen)),"observe the selected experiment");
                place_a=(offered>>(4*chosen))&3;place_b=(offered>>(4*chosen+2))&3;
                if (auto *j=journal::current()) {++j->metrics["experiments_chosen"];j->line("experiment_choice="+std::to_string(chosen)+"  offered="+std::to_string(offered));}
                std::cout<<"experiment> "<<chosen+1<<" "<<o.voice[place_a]<<" "<<o.voice[place_b]<<'\n'<<std::flush;continue;
            }
            if (line.starts_with("/arrived ")) {
                std::istringstream input(line.substr(9));Word after=0;
                if (!cfg.bindings || habitat || !bearings(input,2,after)) {std::cout<<"Use /arrived FOOD WATER at the tabletop.\n"<<std::flush;continue;}
                Word accepted=0,credit=o.binding_credit;void *args[]{&cfg.bindings,&cfg.recall,&after,&credit,&choice.ptr,&cfg.joint};
                launch(s,arrival,1,args);s.d.check(s.d.download(&accepted,choice.ptr,sizeof(accepted)),"observe arrival credit");
                if (!accepted) {std::cout<<"There is no outstanding action.\n"<<std::flush;continue;}
                place_a=after&3;place_b=after>>2;if (auto *j=journal::current()) ++j->metrics["experiment_arrivals"];
                std::cout<<"Observed the result of the chosen action.\n"<<std::flush;continue;
            }
            if (line.starts_with("/show-motion ")) {
                std::istringstream input(line.substr(13));std::string token,from,to,extra;
                input>>token>>from>>to;
                auto start=std::find(o.voice.begin(),o.voice.end(),from),end=std::find(o.voice.begin(),o.voice.end(),to);
                if (!imagined) {std::cout<<"This individual needs prospective contacts.\n";continue;}
                if (token.empty() || start==o.voice.end() || end==o.voice.end() || input>>extra) {std::cout<<"Use /show-motion WORD BEFORE AFTER with two bearings.\n";continue;}
                Word sound=word_digest(token),a=Word(start-o.voice.begin()),b=Word(end-o.voice.begin());
                void *args[]{&imagined,&cfg.recall,&sound,&a,&b};launch(s,motion_lesson,1,args);
                if (auto *j=journal::current()) ++j->metrics["observed_motions"];
                std::cout<<"Observed one movement.\n";continue;
            }
            if (line=="/follow") {
                if (!habitat) {std::cout<<"Tracking requires a living habitat.\n";continue;}
                ResidentView nearest{};
                if (!habitat->locate(nearest,x,y,true)) {std::cout<<"This habitat has no living residents.\n";continue;}
                followed=nearest;std::cout<<"Following resident "<<nearest.slot<<" born at tick "<<nearest.born<<" of founder "<<nearest.founder<<".\n";
                look();continue;
            }
            if (line.starts_with("/teach-focus ")) {
                std::istringstream input(line.substr(13));std::string name,effect,extra;
                constexpr std::array<std::string_view,4> operations{"look","switch","keep","remember"};
                input>>name>>effect;auto op=std::find(operations.begin(),operations.end(),effect);
                if (!cfg.caller || !cfg.iris) { std::cout<<"These two animals need auditory attention organs.\n";continue; }
                if (name.empty() || op==operations.end() || input>>extra) {std::cout<<"Use /teach-focus WORD look, remember, switch or keep.\n";continue;}
                Word form=word_digest(name),operation=Word(op-operations.begin()),key=mix(o.seed^form);
                void *args[]{&cfg.caller,&cfg.iris,&form,&operation,&key};launch(s,focus_lesson,1,args);
                if (auto *j=journal::current()) ++j->metrics["taught_attention_forms"];
                std::cout<<"Completed 96 attention trials.\n";continue;
            }
            if (line.starts_with("/cover ") || line.starts_with("/uncover ") || line.starts_with("/dim ") || line.starts_with("/bright ")) {
                std::istringstream control(line);std::string command,target,extra;control>>command>>target;
                if ((target!="1" && target!="2" && target!="both") || control>>extra) {std::cout<<"Choose view 1, 2 or both.\n";continue;}
                Word mask=target=="both" ? 3u : target=="1" ? 1u : 2u;
                if (command=="/cover") shutters|=mask;
                if (command=="/uncover") shutters&=~mask;
                if (command=="/dim") dim|=mask;
                if (command=="/bright") dim&=~mask;
                continue;
            }
            if (line.starts_with("/visit ") || line.starts_with("/advance ")) {
                if (!habitat) {std::cout<<"Open a habitat with --world for this command.\n";continue;}
                std::istringstream cmd(line);std::string operation,extra;Word a,b;cmd>>operation;
                if (operation=="/visit" && cmd>>a>>b && !(cmd>>extra) && a<habitat->side() && b<habitat->side()) {x=a;y=b;followed.reset();look();}
                else if (operation=="/advance" && cmd>>a && !(cmd>>extra) && a<=1048576) {habitat->advance(a);look();}
                else std::cout<<"Use a position inside the habitat or 0 to 1048576 ticks.\n";
                continue;
            }
            if (line.starts_with("/teach ")) {
                std::istringstream lesson(line.substr(6));std::string name,target,extra;
                if (!o.matched || !o.discourse) {std::cout<<"Pointing lessons use learned hearing and reference memory.\n";continue;}
                if (!(lesson>>name>>target) || lesson>>extra || (target!="1" && target!="2")) {std::cout<<"Use /teach WORD 1 or /teach WORD 2 to point to a view.\n";continue;}
                if (asker_ticks+384>~0u || witness_ticks+192>~0u) throw std::overflow_error("pointing lesson exceeds the individual clock");
                teach(name,target=="2",place_a,place_b);asker_ticks+=384;witness_ticks+=192;continue;
            }
            if (line.starts_with("/scene ")) {
                if (habitat) {std::cout<<"Use /visit X Y to change this living viewpoint.\n";continue;}
                std::istringstream values(line.substr(7));std::string a,b,extra;
                if (!(values>>a>>b) || values>>extra) {std::cout<<"Use /scene and two bearings.\n";continue;}
                auto first=std::find(o.voice.begin(),o.voice.end(),a),second=std::find(o.voice.begin(),o.voice.end(),b);
                if (first==o.voice.end() || second==o.voice.end()) {std::cout<<"Use a bearing from the four-word voice.\n";continue;}
                place_a=Word(first-o.voice.begin());place_b=Word(second-o.voice.begin());observe_motion();continue;
            }
            // All words enter the same receptor encoder. Only the requester's
            // physical final discharge crosses into the witness's circuit.
            bool assertion=!line.empty() && line.back()=='.';
            for (char &c:line) if (std::string_view("?.!,;:").find(c)!=std::string_view::npos) c=' ';
            std::istringstream input(line);Word heard_words=0;std::string token;
            for (;input>>token;) {
                if (++asker_ticks>~0u) throw std::overflow_error("questioner clock exhausted");
                Word tag=o.discourse ? word_tag(token,o.salt)&3 : 16+word_tag(token),feedback=0,cfg_seed=o.seed,loop=o.discourse;
                Word hash=word_digest(token),distributed=o.distributed,peak=o.peak,first=heard_words==0,explore=~0u;
                DevicePtr ear=o.matched ? listener.ears.ptr : 0;
                ImaginedWord projected{imagined,cfg.recall,projection.ptr,first,cfg.bindings,cfg.roles,cfg.joint,cfg.scenes,reporter,audience};
                void *stimulus[]{&events.ptr,&listener.bodies.ptr,&listener.shape.body_bytes,&pair,&local,&cfg_seed,&tag,&feedback,&loop,&hash,&distributed,&peak,&ear,&gate.ptr,&explore,&maps.ptr,&listener.caller.ptr,&projected};
                launch(s,word,1,stimulus);listener.act(events.ptr,answers.ptr,nullptr,pair,1);
                if (prepared.ptr) {void *retain[]{&prepared.ptr,&answers.ptr,&first,&gate.ptr,&peak};launch(s,keep,1,retain);}
                ++heard_words;
            }
            if (!heard_words) continue;
            if (!assertion && (garden.respond(line) || concepts.respond(line,false))) continue;
            if (reporter) {
                Word cut=o.report_wire;void *args[]{&reporter,&audience,&emission.ptr,&cut};launch(s,say_report,1,args);
                Utterance said{};s.d.check(s.d.download(&said,emission.ptr,sizeof(said)),"hear the articulated report");
                if (said.handled) {
                    std::string text=sentence(said);
                    std::cout<<"animal> "<<text<<'\n'<<std::flush;
                    if (auto *j=journal::current()) {
                        ++j->metrics["report_requests"];j->metrics["report_words"]+=said.count;++j->metrics["human_turns"];
                        Episodes sensory{};Word belief[2]{};
                        s.d.check(s.d.download(&sensory,reporter+offsetof(Reports,history),sizeof(sensory)),"record the witness's event memory");
                        if (audience) s.d.check(s.d.download(belief,audience+offsetof(Reports,heard),sizeof(belief)),"record acoustic evidence in the addressee");
                        j->line("report_turn="+std::to_string(j->metrics["report_requests"])+"  words="+std::to_string(said.count)
                                +"  witnessed="+std::to_string(sensory.recent)+"  prior="+std::to_string(sensory.earlier)
                                +"  decoded="+std::to_string(belief[0])+"  decoded_prior="+std::to_string(belief[1])+"  speech="+text);
                    }
                    continue;
                }
            }
            Exchange physical{};physical.shape=place_a;physical.colour=place_b;physical.shutters=shutters;physical.dim=dim;
            s.d.check(s.d.upload(states.ptr,&physical,sizeof(physical)),"present the interview scene");
            DevicePtr asker=listener.bodies.ptr+Count(pair)*listener.shape.body_bytes;
            if (o.matched) {
                Response intended{};s.d.check(s.d.download(&intended,prepared.ptr,sizeof(intended)),"read auditory release");
                if (!intended.received) {
                    std::cout<<"animal> (silence)\n"<<std::flush;
                    if (auto *j=journal::current()) {
                        j->turn(heard_words,place_a,place_b,4,2,4,shutters,dim,0);
                    }
                    continue;
                }
            }
            witness_ticks+=2;
            if (witness_ticks>~0u) throw std::overflow_error("speaker clock exhausted");
            DevicePtr request=prepared.ptr ? prepared.ptr : answers.ptr;
            void *wire[]{&states.ptr,&request,&events.ptr,&witness,&speaker.shape.body_bytes,&cfg,&asker,&listener.shape.body_bytes};
            launch(s,question,1,wire);speaker.act(events.ptr,answers.ptr,nullptr,pair,1);
            request=answers.ptr;launch(s,glance,1,wire);speaker.act(events.ptr,answers.ptr,nullptr,pair,1);
            Response response{};s.d.check(s.d.download(&response,answers.ptr,sizeof(response)),"hear the speaker");
            s.d.check(s.d.download(&physical,states.ptr,sizeof(physical)),"observe the selected receptive field");
            if (!physical.retinal) response.ports&=~8u;
            std::cout<<"animal> "<<(response.ports&8 ? o.voice[response.ports&3] : "(silence)")<<'\n'<<std::flush;
            if (auto *j=journal::current()) {
                j->action(response.action,response.ports,0);
                Word spoken=response.ports&8 ? response.ports&3 : 4u;
                j->turn(heard_words,place_a,place_b,physical.question,physical.looked,spoken,shutters,dim,physical.retinal,physical.attention);
            }
        }
        record_auditory("final_word_forms");
        if (habitat && !o.save_world.empty()) habitat->save(o.save_world);
        pupil.report();
    }
    void converse() {
        std::cout<<"The listener's four motor sounds are";
        for (const auto &sound:o.voice) std::cout<<' '<<sound;
        std::cout<<". Type words to stimulate its auditory receptors. /yes and /no give feedback on its last response. /quit ends the session.\n"<<std::flush;
        optics(o.seed,0);Word feedback=0;Count ticks=listener.tick(o.pair);
        for (std::string line;std::getline(std::cin,line);) {
            if (line=="/quit") break;
            if (line=="/yes" || line=="/no") {feedback=line=="/yes" ? 1u : 2u;continue;}
            std::istringstream tokens(line);std::string token;bool spoke=false;Word tokens_seen=0;
            for (;tokens>>token;) {
                if (++ticks>~0u) throw std::overflow_error("listener clock exhausted");
                auto it=std::find(o.voice.begin(),o.voice.end(),token);
                Word tag=0;
                if (it!=o.voice.end()) tag=8+Word(it-o.voice.begin());
                else {
                    Word hash=2166136261u;
                    for (unsigned char c:token) hash=(hash^c)*16777619u;
                    tag=16+(mix(hash)&15);
                }
                Word pair=o.pair,seed=o.seed;
                Word local=o.pair/o.community,loop=0;
                Word hash=0,distributed=0,peak=0,explore=~0u;
                DevicePtr no_ear=0,no_gate=0;
                ImaginedWord idle{};
                void *heard[]{&events.ptr,&listener.bodies.ptr,&listener.shape.body_bytes,&pair,&local,&seed,&tag,&feedback,&loop,&hash,&distributed,&peak,&no_ear,&no_gate,&explore,&maps.ptr,&no_ear,&idle};
                launch(s,word,1,heard);listener.act(events.ptr,answers.ptr,nullptr,pair,1);feedback=0;spoke=true;++tokens_seen;
            }
            if (spoke) {
                Response response{};s.d.check(s.d.download(&response,answers.ptr,sizeof(response)),"hear the animal's reply");
                std::cout<<"animal> "<<(response.ports&8 ? o.voice[response.ports&3] : "(silence)")<<'\n'<<std::flush;
                if (auto *j=journal::current()) {j->metrics["human_words"]+=tokens_seen;++j->metrics["human_turns"];j->action(response.action,response.ports,0);}
            }
        }
    }
};
} // namespace

int dialogue_command(int argc,char **argv) {
    if (std::string_view(argv[1])=="commons") return commons_command(argc,argv);
    bool talk=std::string_view(argv[1])=="talk";
    for (int i=2;i<argc;++i) if (std::string_view(argv[i])=="--help") {
        if (talk)
            std::cout
                << "axion talk  |  a live interview with two learned spiking animals\n\n"
                << "An animal reared with develop or compose hears ordinary words directly. /enact PHRASE feeds its "
                   "acquired response to its own planning circuit. /raw, /home, /lost, /show, /seek and /sleep expose "
                   "its developmental circuit.\n"
                << "Place food and water with /scene west north. Ask where the food is, where it was, or where it "
                   "would be if the two exchanged places.\n"
                << "/cover, /uncover, /dim and /bright take 1, 2 or both as a view.\n"
                << "/teach brook 2 teaches a name. /teach-focus yesterday remember teaches a source. /show-motion "
                   "blink west east demonstrates a movement.\n"
                << "/show-pair WORD FOOD-BEFORE WATER-BEFORE FOOD-AFTER WATER-AFTER demonstrates a joint action.\n"
                << "/try-pair WORD takes four pairs of bearings. /arrived FOOD WATER shows the result of the animal's "
                   "choice.\n"
                << "/board N [fresh], /name WORD TAGS, /link PORT FROM TO and /map PORT TARGETS present a relational "
                   "world.\n"
                << "/example WORD FROM TO teaches a route. /gaze both|witness|peer|neither sets who sees the next "
                   "change.\n"
                << "/watch-answer CUE WORD FROM lets the witness hear its peer. /earshot shared|private controls "
                   "spoken updates.\n"
                << "/devise GOAL [STARTS_HEX [DEPTH]] composes a motor score. /preview SITE SCORE imagines its "
                   "effects.\n"
                << "/focus-site NAME grounds a spoken score. /perform SITE [SCORE] and /pass-score SITE open execute a "
                   "retained score.\n"
                << "/tidalpool SEED, /ripen TICKS, /offer VARIETY TICKS and /turn QUARTERS change a physical feeding "
                   "tray. /grip VARIETY, /grip-word WORD and /handle WORD [TICKS] use the resident’s own motor and "
                   "visual circuits.\n"
                << "/greenhouse N SEED and /plot SITE STATE present a physical room. /apply MOTOR SITE performs an "
                   "intervention.\n"
                << "/trial MOTOR BEFORE AFTER [VISIBLE-BEFORE VISIBLE-AFTER] presents an observed effect. /practice N "
                   "SEED samples real effects.\n"
                << "/cohear MASK WORDS presents simultaneous semantic cues. /approve WORD SITE 0|1 labels a perceived "
                   "example.\n"
                << "/pass-plan SITE open|muted|reversed transmits the speaker's actual sounds to the other animal.\n"
                << "/ask WORD [TAGS] offers a choice of examples. Give a place name followed by a period to answer. "
                   "/quit exits.\n";
        else std::cout<<help;
        return 0;
    }
    auto o=options(argc,argv,talk);Session s;s.open();
    auto individual=read_brain(o.speaker,o.s_site);
    if (talk && !individual.grounded && (individual.sequential || individual.composes || individual.procedural) && !(individual.auditory || individual.conceptual || individual.causal || individual.reports)) return developmental_interview(individual,o.speaker,o.save_s);
    Partner speaker(s,std::move(individual),o.pairs,0xa796384bu,false);
    Partner listener(s,read_brain(o.listener,o.l_site),o.pairs,0x379a18cdu,o.matched,o.inert);
    speaker.shape.eligibility=o.eligibility;listener.shape.eligibility=o.eligibility;
    speaker.shape.motor_learning=o.motor_learning&1;listener.shape.motor_learning=(o.motor_learning>>1)&1;
    Count social=Count(o.practice)*(o.reverse ? 2 : 1);
    Count sender_ticks=2ull*o.play+o.demonstrations+((o.plasticity&1) ? social*(o.gaze ? 2u : 1u) : 0);
    Count receiver_ticks=2ull*o.play+((o.plasticity&2) ? social*o.facets : 0);
    if (!o.generations && (sender_ticks+speaker.image.brain.ticks>~0u || receiver_ticks+listener.image.brain.ticks>~0u))
        throw std::overflow_error("dialogue exceeds an individual clock");
    AcousticWorld garden(s,speaker,listener,o);
    std::cout<<s.name<<"  speaker="<<o.speaker<<"  listener="<<o.listener<<"  pairs="<<o.pairs<<"  seed="<<o.seed<<"  facets="<<o.facets
             <<"  speaker_bytes="<<speaker.bytes()<<"  listener_bytes="<<listener.bytes()
             <<"  GPU_world_bytes="<<Count(o.pairs)*(speaker.bytes()+listener.bytes()+sizeof(Exchange)+sizeof(Encounter)+sizeof(Response)+sizeof(DialogueScore))
                                        +Count(o.shuffled)*o.pairs*sizeof(OpticalMap)+Count((o.pairs+127)/128)*sizeof(SpeechBlock)+(garden.prepared.ptr ? sizeof(Response)+sizeof(Word) : 0)
                                        +((o.interview && o.replay && speaker.image.imagines) ? sizeof(Projection) : 0)
                                        +((o.interview && o.replay && speaker.image.binds) ? sizeof(RoleProjection) : 0)
                                        +((o.interview && o.replay && speaker.image.conjoins) ? sizeof(SceneWave) : 0)
                                        +((o.interview && (speaker.reports.ptr || listener.reports.ptr)) ? sizeof(Utterance) : 0)
                                        +(o.interview && speaker.image.binds ? sizeof(Word) : 0)
                                        +(o.interview && speaker.image.conceptual ? sizeof(ConceptReply) : 0)
                                        +(o.interview && speaker.image.causal ? sizeof(Apparatus)+sizeof(GardenReply) : 0)<<'\n';
    if (auto *j=journal::current()) {
        j->source=o.speaker;j->device=s.name;j->metrics["pairs"]=o.pairs;j->metrics["facets"]=o.facets;j->metrics["practice"]=o.practice;j->metrics["vocal_generations"]=o.generations;j->metrics["compass"]=o.compass;j->metrics["discourse"]=o.discourse;
    }
    auto record_bodies=[&]() {
        if (auto *j=journal::current()) for (auto *partner:{&speaker,&listener}) {
            Genome genome{};
            s.d.check(s.d.download(&genome,partner->bodies.ptr+Count(o.pair)*partner->shape.body_bytes,sizeof(genome)),"observe the communicating organisms");
            std::string role=partner==&speaker ? "speaker_" : "listener_";
            j->metrics[role+"body"]=genome.body;j->metrics[role+"tape"]=1+(genome.expression&7);
        }
    };
    if (talk) {
        record_bodies();garden.interview();speaker.save(o.save_s,o.pair,true);listener.save(o.save_l,o.pair);return 0;
    }
    garden.breed();record_bodies();speaker.save(o.founder_s,o.pair);listener.save(o.founder_l,o.pair);
    garden.stage("play",o.play,true,0);
    garden.stage("handling",o.demonstrations,true,0);
    garden.stage("naive",o.probes,false,0);
    garden.stage("practice",o.practice,true,0);
    auto test=[&](std::string name,Word epoch) {
        garden.stage(name,o.probes,false,epoch);
        if (!o.controls) return;
        for (Wire w:{Wire::silent,Wire::shifted,Wire::hidden}) garden.stage(name,o.probes,false,epoch,w);
        if (o.discourse) for (Wire w:{Wire::muted_self,Wire::shifted_self}) garden.stage(name,o.probes,false,epoch,w);
        if (o.community>1) garden.stage(name,o.probes,false,epoch,Wire::stranger);
        if (o.gaze) garden.stage(name,o.probes,false,epoch,Wire::blind_gaze);
        if (o.facets==2) for (Wire w:{Wire::no_question,Wire::shift_question}) garden.stage(name,o.probes,false,epoch,w);
    };
    test("learned",0);
    if (o.reverse) {
        garden.stage("surprise",o.probes,false,1);garden.stage("new_doors",o.practice,true,1);test("revised",1);
    }
    if (o.interactive) garden.converse();
    if (o.interview) garden.interview();
    speaker.save(o.save_s,o.pair,true);listener.save(o.save_l,o.pair);
    return 0;
}
} // namespace axion::life
