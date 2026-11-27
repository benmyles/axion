#include "axion/recollection.hpp"
#include "individual_io.hpp"
#include "life_runtime.hpp"
#include "conjunction_batch.cuh"
#include "report.cuh"
#include "concept.cuh"
#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <optional>
#include <string_view>
#include <vector>

namespace axion::life {
namespace {
using namespace runtime;
using namespace storage;
struct Conditions {
    Word generations=256,candidates=256,worlds=8,cycles=24,delay=6,tests=256,seed=73219,probe_seed=160481;
    Word origin=0,drift=0,varied=1,invariant=0;
    std::string source="examples/discourse-speaker.mind",destination;
    std::string partner="examples/witness.mind",save_partner,words="is,look,now,was,remember,recall,switch,keep";
    Word dialect=0,replicas=0,steps=64,census=0,sparse=0;
};
Word quantity(std::string_view text,Word low,Word high) {
    Word n{};auto [after,status]=std::from_chars(text.data(),text.data()+text.size(),n);
    if (status!=std::errc{} || after!=text.data()+text.size() || n<low || n>high)
        throw std::invalid_argument("invalid recollection quantity "+std::string(text));
    return n;
}
Conditions parse(int argc,char **argv,bool attention=false,Conditions q={}) {
    bool batch=attention || std::string_view(argv[1])=="conjoin";
    if (attention) {q.source="examples/interpreter.mind";q.delay=8;}
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load" || (attention && f=="--listener")) q.source=v;
        else if (f=="--save" || (attention && f=="--save-listener")) q.destination=v;
        else if (attention && f=="--speaker") q.partner=v;
        else if (attention && f=="--save-speaker") q.save_partner=v;
        else if (attention && f=="--words") q.words=v;
        else if (attention && f=="--dialect") q.dialect=quantity(v,0,1);
        else if (attention && f=="--length") q.delay=quantity(v,1,1024);
        else if (batch && f=="--replicas") q.replicas=quantity(v,1,1048576);
        else if (batch && f=="--decisions") q.steps=quantity(v,1,65536);
        else if (!attention && batch && f=="--enumerate" && (v=="representative" || v=="all")) q.census=v=="all" ? 2 : 1;
        else if (!attention && batch && f=="--lessons" && (v=="broad" || v=="sparse")) q.sparse=v=="sparse";
        else if (f=="--generations") q.generations=quantity(v,0,65536);
        else if (f=="--candidates") q.candidates=quantity(v,1,65536);
        else if (f=="--worlds") q.worlds=quantity(v,1,128);
        else if (f=="--probe-worlds") q.tests=quantity(v,1,65536);
        else if (f=="--cycles") q.cycles=quantity(v,1,256);
        else if (f=="--delay") q.delay=quantity(v,0,1024);
        else if (!attention && f=="--spacing" && (v=="fixed" || v=="varied")) q.varied=v=="varied";
        else if (!attention && f=="--objective" && (v=="sampled" || v=="invariant")) q.invariant=v=="invariant";
        else if (f=="--seed") q.seed=quantity(v,0,~0u);
        else if (f=="--probe-seed") q.probe_seed=quantity(v,0,~0u);
        else if (f=="--origin" && (v=="inherited" || v=="inert" || v=="random")) q.origin=v=="inert" ? 1 : v=="random" ? 2 : 0;
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else throw std::invalid_argument("invalid recollection option "+std::string(f));
    }
    return q;
}
class Nursery : public Session {
    Function seed{},live{},rank{},select{},survey{},verify{},grade_proof{};
    RecollectionGenes vision{};
    BindingGenes sources{};
    Word sparse=0;
    bool population_audit=false;
    bool joint_fitness=false;
    bool report_fitness=false;
    bool concept_fitness=false,causal_fitness=false;
public:
    explicit Nursery(const char *environment="recollect_live",RecollectionGenes visual={},BindingGenes binding={},Word sparse_lessons=0):vision(visual),sources(binding),sparse(sparse_lessons),population_audit(std::string_view(environment)=="binding_live" || std::string_view(environment)=="conjunction_live") {
        open();
        bind(seed,"recollect_seed",Image::recollection);causal_fitness=std::string_view(environment)=="causal_live";bind(live,environment,causal_fitness ? Image::causes : Image::recollection);
        joint_fitness=std::string_view(environment)=="conjunction_live";
        report_fitness=std::string_view(environment)=="report_live";concept_fitness=std::string_view(environment)=="concept_live";population_audit|=report_fitness || concept_fitness || causal_fitness;
        bind(rank,causal_fitness ? "causal_merit" : concept_fitness ? "concept_merit" : report_fitness ? "report_merit" : joint_fitness ? "conjunction_merit" : "recollect_merit",causal_fitness ? Image::causes : Image::recollection);bind(select,"recollect_brood",Image::recollection);
        bind(survey,"voice_survey",Image::vocal_brood);
        bind(verify,"recollect_certify",Image::recollection);
        bind(grade_proof,"recollect_proof_merit",Image::recollection);
    }
    void launch(Function f,Word n,void **arguments) {
        d.check(d.launch(f,(n+127)/128,1,1,128,1,1,0,nullptr,arguments,nullptr),"execute visual memory world");
    }
    void evaluate(DevicePtr population,DevicePtr output,Word count,Word worlds,Word salt,Word cycles,Word delay,Word varied,RecallCut cut) {
        Word total=count*worlds;
        Word granule=std::min(131072u,std::max(128u,16000000u/(cycles*(delay+10))));
        for (Word first=0;first<total;first+=granule) {
            Word batch=std::min(total-first,granule);
            void *arguments[]{&population,&output,&count,&worlds,&salt,&cycles,&delay,&varied,&cut,&first,&batch,&vision,&sources,&sparse};
            launch(live,batch,arguments);
        }
    }
    RecollectionGenes choose(Conditions q,RecollectionGenes initial) {
        if (!q.generations && !q.origin) return initial;
        Temporary a(d,Count(q.candidates)*sizeof(initial)),b(d,Count(q.candidates)*sizeof(initial));
        Temporary scores(d,Count(q.candidates)*(q.invariant ? sizeof(MemoryProof) : q.worlds*sizeof(RecallScore))),fitness(d,Count(q.candidates)*sizeof(Word)),totals(d,3*sizeof(Count));
        DevicePtr parents=a.ptr,children=b.ptr;
        void *begin[]{&parents,&q.candidates,&initial,&q.seed,&q.origin};launch(seed,q.candidates,begin);
        Word best=0;
        for (Word gen=0;gen<q.generations;++gen) {
            Word salt=mix(q.seed^gen*0x385ab749u);
            if (q.invariant) {
                void *args[]{&parents,&scores.ptr};
                d.check(d.launch(verify,q.candidates,1,1,256,1,1,0,nullptr,args,nullptr),"explore each candidate's reachable states");
                void *grade[]{&scores.ptr,&fitness.ptr,&q.candidates};launch(grade_proof,q.candidates,grade);
            } else {
                evaluate(parents,scores.ptr,q.candidates,q.worlds,salt,q.cycles,q.delay,q.varied,RecallCut::intact);
                void *grade[]{&scores.ptr,&fitness.ptr,&q.candidates,&q.worlds};launch(rank,q.candidates,grade);
            }
            if (!gen || gen+1==q.generations || (gen+1)%std::max(1u,q.generations/8)==0) {
                void *measure[]{&fitness.ptr,&q.candidates,&totals.ptr};launch(survey,128,measure);
                Count values[3];d.check(d.download(values,totals.ptr,sizeof(values)),"read cohort outcome");
                best=Word(values[2]);
                std::cout<<"memory_generation="<<gen+1<<"  candidates="<<q.candidates<<"  best="<<values[1]<<"  total="<<values[0]<<'\n';
                if (auto *j=journal::current()) j->breeding.push_back({gen+1,q.candidates,{values[0],values[1]}});
            }
            if (gen+1<q.generations) {
                void *breed[]{&parents,&children,&fitness.ptr,&q.candidates,&salt,&q.drift,&q.invariant};launch(select,q.candidates,breed);std::swap(parents,children);
            }
        }
        if (population_audit && q.generations) {
            evaluate(parents,scores.ptr,q.candidates,q.worlds,q.probe_seed,q.cycles,q.delay,0,RecallCut::intact);
            void *grade[]{&scores.ptr,&fitness.ptr,&q.candidates,&q.worlds};launch(rank,q.candidates,grade);
            std::vector<Word> results(q.candidates);
            d.check(d.download(results.data(),fitness.ptr,results.size()*sizeof(Word)),"audit the final population");
            Count total=0;Word perfect=0,maximum=((causal_fitness || concept_fitness) ? 80 : report_fitness ? 34 : joint_fitness ? 14 : 13)*q.worlds*q.cycles;
            for (Word f:results) {total+=f;perfect+=f==maximum;}
            std::cout<<(causal_fitness ? "causal_population" : concept_fitness ? "concept_population" : report_fitness ? "report_population" : joint_fitness ? "conjunction_population" : "binding_population")<<"=final  probe_seed="<<q.probe_seed<<"  individuals="<<q.candidates
                     <<"  perfect="<<perfect<<"  total="<<total<<"  available="<<Count(maximum)*q.candidates<<'\n';
        }
        RecollectionGenes selected;
        d.check(d.download(&selected,parents+Count(best)*sizeof(selected),sizeof(selected)),"read selected visual program");
        return selected;
    }
    void audit(Conditions q,RecollectionGenes program) {
        constexpr std::array names{"intact","fixed","crossed","source","erased","last-per-cue","last-event"};
        constexpr std::array phases{"one","delay","noise","revision","faint","empty"};
        Temporary parent(d,sizeof(program)),output(d,Count(q.tests)*sizeof(RecallScore));
        d.check(d.upload(parent.ptr,&program,sizeof(program)),"present inherited visual program");
        std::vector<RecallScore> rows(q.tests);
        for (unsigned arm=0;arm<names.size();++arm) {
            evaluate(parent.ptr,output.ptr,1,q.tests,q.probe_seed,q.cycles,q.delay,q.varied,RecallCut(arm));
            d.check(d.download(rows.data(),output.ptr,rows.size()*sizeof(RecallScore)),"read independent visual lifetimes");
            std::array<Count,6> score{};
            for (const auto &v:rows) {
                score[0]+=v.one;score[1]+=v.delay;score[2]+=v.noise;score[3]+=v.revision;score[4]+=v.faint;score[5]+=v.empty;
            }
            std::cout<<"recollection="<<names[arm]<<"  seed="<<q.probe_seed<<"  worlds="<<q.tests<<"  delay_events="<<q.delay<<"  spacing="<<(q.varied ? "varied" : "fixed")<<"  questions="<<Count(q.tests)*q.cycles;
            for (unsigned k=0;k<score.size();++k) std::cout<<"  "<<phases[k]<<'='<<score[k];
            std::cout<<"  empty_questions="<<4*q.tests<<'\n';
            if (auto *j=journal::current();j && !arm) for (unsigned k=0;k<5;++k)
                j->scores.push_back({"Retinal recall / "+std::string(phases[k]),score[k],Count(q.tests)*q.cycles});
        }
        Temporary proof(d,sizeof(MemoryProof));MemoryProof result{};
        void *args[]{&parent.ptr,&proof.ptr};
        d.check(d.launch(verify,1,1,1,256,1,1,0,nullptr,args,nullptr),"check the reachable visual states");
        d.check(d.download(&result,proof.ptr,sizeof(result)),"read the memory invariant checks");
        constexpr std::array claims{"unseen","clear","outlier","consistent-dim"};
        for (unsigned i=0;i<claims.size();++i) {
            std::cout<<"memory_proof="<<claims[i]<<"  reachable_rows="<<result.reachable<<"  checked="<<result.checked[i]
                     <<"  violations="<<result.failed[i];
            if (result.failed[i]) {
                Word code=result.first[i];std::cout<<"  first_depth="<<(code>>16)<<"  expected="<<((code>>12)&15)
                    <<"  actual="<<((code>>8)&15)<<"  row="<<(code&255);
            }
            std::cout<<'\n';
        }
    }
    void attend(Conditions q,RecollectionGenes program,AgentImage &reader,AgentImage &witness) {
        const std::array<Word,8> operations{0,0,0,3,3,3,1,2};
        std::array<Word,8> forms{};std::istringstream labels(q.words);std::string token;
        for (unsigned i=0;i<forms.size();++i) {
            if (!std::getline(labels,token,',') || token.empty() || token.find_first_of(" \r\t\n")!=std::string::npos)
                throw std::invalid_argument("--words requires eight distinct single-token forms");
            forms[i]=word_digest(token);
            if (std::find(forms.begin(),forms.begin()+i,forms[i])!=forms.begin()+i) throw std::invalid_argument("duplicate attention form");
        }
        if (labels.peek()!=std::char_traits<char>::eof() || q.words.back()==',') throw std::invalid_argument("too many attention forms");
        Temporary parent(d,sizeof(program)),out(d,Count(q.tests)*sizeof(RecallScore));
        d.check(d.upload(parent.ptr,&program,sizeof(program)),"present inherited auditory plasticity");
        std::vector<RecallScore> rows(q.tests);
        constexpr std::array kinds{"intact","inert","muted","inverted","shuffled-food"};
        constexpr std::array names{"single","pair","chain","revised","revised_chain"};
        for (unsigned arm=0;arm<kinds.size();++arm) {
            evaluate(parent.ptr,out.ptr,1,q.tests,q.probe_seed,q.cycles,q.delay,0,RecallCut(arm));
            d.check(d.download(rows.data(),out.ptr,rows.size()*sizeof(RecallScore)),"observe source selection");
            std::array<Count,5> scores{};
            for (const auto &s:rows) {scores[0]+=s.one;scores[1]+=s.delay;scores[2]+=s.noise;scores[3]+=s.revision;scores[4]+=s.faint;}
            Count n=Count(q.tests)*q.cycles;
            std::cout<<"attention="<<kinds[arm]<<"  worlds="<<q.tests<<"  length="<<q.delay<<"  probes="<<n;
            for (size_t i=0;i<scores.size();++i) {
                std::cout<<"  "<<names[i]<<'='<<scores[i];
                if (auto *j=journal::current();j && !arm) j->scores.push_back({"Attention / "+std::string(names[i]),scores[i],n});
            }
            std::cout<<'\n';
        }
        if (q.destination.empty() && q.save_partner.empty() && !q.replicas) return;
        Function school{};bind(school,"focus_school",Image::recollection);
        Temporary ear(d,sizeof(AttentiveEar)),iris(d,sizeof(Iris)),sounds(d,sizeof(forms)),tasks(d,sizeof(operations));
        d.check(d.upload(sounds.ptr,forms.data(),sizeof(forms)),"present the teacher's sounds");
        d.check(d.upload(tasks.ptr,operations.data(),sizeof(operations)),"set the teacher's physical tasks");
        Word n=forms.size();void *lesson[]{&ear.ptr,&iris.ptr,&program,&sounds.ptr,&tasks.ptr,&n,&q.dialect,&q.seed};
        d.check(d.launch(school,1,1,1,1,1,1,0,nullptr,lesson,nullptr),"rear the two attention organs");
        reader.attends=true;witness.orients=true;
        d.check(d.download(&reader.attention,ear.ptr,sizeof(AttentiveEar)),"read acquired word contacts");
        d.check(d.download(&witness.iris,iris.ptr,sizeof(Iris)),"read acquired attention muscles");
        if (!q.destination.empty()) write_checked(q.destination,reader);
        if (!q.save_partner.empty()) write_checked(q.save_partner,witness);
        if (q.replicas) scale(q,ear.ptr,iris.ptr,sounds.ptr,witness.recollection);
    }
    void scale(Conditions q,DevicePtr ear,DevicePtr iris,DevicePtr forms,Recollection visual) {
        Function start{},direct{},packed{};
        bind(start,"focus_batch_seed",Image::recollection);bind(direct,"focus_batch_direct",Image::recollection);
        bind(packed,"focus_batch_compiled",Image::recollection);
        struct Timer { Driver &d;Event begin{},end{};~Timer(){if (begin) d.event_free(begin);if (end) d.event_free(end);} } t{d};
        d.check(d.event_new(&t.begin,0),"create decision timer");d.check(d.event_new(&t.end,0),"create decision timer");
        Word groups=(q.replicas+127)/128;Temporary stats(d,Count(groups)*2*sizeof(Word));
        std::vector<Word> rows(2*groups);std::optional<std::pair<Count,Word>> reference;
        for (Word layout=0;layout<2;++layout) {
            Word bytes=layout ? compiled_attention_bytes : sizeof(AttentiveEar)+sizeof(Iris)+sizeof(Recollection);
            Temporary bodies(d,Count(q.replicas)*bytes);
            void *init[]{&bodies.ptr,&q.replicas,&ear,&iris,&visual,&forms,&layout};
            void *args[]{&bodies.ptr,&q.replicas,&forms,&q.steps,&q.probe_seed,&stats.ptr};
            for (Word repeat=0;repeat<4;++repeat) {
                launch(start,q.replicas,init);
                d.check(d.record(t.begin,nullptr),"start decision timer");
                launch(layout ? packed : direct,q.replicas,args);
                d.check(d.record(t.end,nullptr),"stop decision timer");d.check(d.wait(t.end),"complete decision interval");
                if (!repeat) continue;
                float ms{};d.check(d.elapsed(&ms,t.begin,t.end),"measure attention interval");
                d.check(d.download(rows.data(),stats.ptr,rows.size()*sizeof(Word)),"verify independent decisions");
                Word checksum=0;Count score=0,items=Count(q.replicas)*q.steps;
                for (Word j=0;j<groups;++j) {score+=rows[2*j];checksum^=rows[2*j+1];}
                auto value=std::pair{score,checksum};
                if (reference && value!=*reference) throw std::runtime_error("compiled and direct source decisions diverged");
                reference=value;
                std::cout<<"attention_batch="<<(layout ? "compiled" : "direct")<<"  repeat="<<repeat<<"  pairs="<<q.replicas
                         <<"  decisions="<<items<<"  bytes_per_pair="<<bytes<<"  correct="<<score<<"  checksum="<<checksum
                         <<"  GPU_ms="<<std::fixed<<std::setprecision(3)<<ms<<"  Mdecisions/s="<<double(items)/(1000*ms)<<'\n';
            }
        }
    }
};
}
int recollection_command(int argc,char **argv) {
    for (int i=2;i<argc;++i) if (std::string_view(argv[i])=="--help") {
        std::cout<<"axion recollect  |  evolve a visual memory for an independent spiking individual\n\n"
                 <<"--load FILE --save FILE     source and resulting .mind\n"
                 <<"--generations N --candidates N --worlds N --seed N\n"
                 <<"--origin inherited|inert|random --breeding select|drift --objective sampled|invariant\n"
                 <<"--probe-worlds N --probe-seed N --cycles N --delay N --spacing varied|fixed\n"
                 <<"Each birth clears the sixteen plastic contacts. Fitness measures one-shot acquisition, retention, resistance to weak counterevidence, revision, and learning from repeated faint sightings.\n";
        return 0;
    }
    auto q=parse(argc,argv);AgentImage brain;brain.read(q.source);
    Nursery lab;
    auto genes=lab.choose(q,brain.recollection.genes);
    std::cout<<lab.name<<"  recollection_bytes="<<sizeof(Recollection)<<"  generations="<<q.generations
             <<"  candidates="<<q.candidates<<"  lifetimes="<<(q.invariant ? 0 : Count(q.generations)*q.candidates*q.worlds)
             <<"  verified_candidates="<<(q.invariant ? Count(q.generations)*q.candidates : 0)
             <<"  rule_low="<<std::hex<<genes.low<<"  rule_high="<<genes.high<<"  expression="<<genes.expression<<std::dec<<'\n';
    if (auto *j=journal::current()) {
        j->source=q.source;j->device=lab.name;j->metrics["memory_generations"]=q.generations;j->metrics["genotypes"]=q.candidates;
        j->metrics["invariant_selection"]=q.invariant;
    }
    lab.audit(q,genes);
    if (!q.destination.empty()) {
        if (q.generations || q.origin || !brain.recollects) brain.recollection={genes,0};
        brain.recollects=true;write_checked(q.destination,brain);
    }
    return 0;
}
int attention_command(int argc,char **argv) {
    for (int i=2;i<argc;++i) if (std::string_view(argv[i])=="--help") {
        std::cout<<"axion attention  |  learn to request a look or a recollection\n\n"
                 <<"--listener FILE --speaker FILE --save-listener FILE --save-speaker FILE\n"
                 <<"--generations N --candidates N --worlds N --cycles N --length N --probe-worlds N\n"
                 <<"--origin inherited|inert|random --breeding select|drift --seed N --probe-seed N --dialect 0|1\n"
                 <<"--replicas N --decisions N   verify and time direct and compiled sensory-attention circuits\n"
                 <<"--words A,B,C,D,E,F,G,H   classroom forms for looking (A–C), recalling (D–F), switching (G) and keeping (H) attention\n";
        return 0;
    }
    auto q=parse(argc,argv,true);AgentImage listener,speaker;listener.read(q.source);speaker.read(q.partner);
    if (!q.destination.empty() && !q.save_partner.empty() && std::filesystem::absolute(q.destination).lexically_normal()==std::filesystem::absolute(q.save_partner).lexically_normal())
        throw std::invalid_argument("save the two individuals to different files");
    Nursery lab("focus_live");auto rule=lab.choose(q,{listener.attention.rule,speaker.iris.rule,0});
    std::cout<<lab.name<<"  caller_rule="<<std::hex<<rule.low<<"  witness_rule="<<rule.high<<std::dec
             <<"  generations="<<q.generations<<"  candidate_lifetimes="<<Count(q.generations)*q.candidates*q.worlds
             <<"  caller_bytes="<<sizeof(AttentiveEar)<<"  iris_bytes="<<sizeof(Iris)<<"  dialect="<<q.dialect<<'\n';
    if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;j->metrics["genotypes"]=q.candidates;}
    lab.attend(q,rule,listener,speaker);return 0;
}
int imagery_command(int argc,char **argv) {
    std::string source="examples/observer.mind",destination,vocab="clockwise,counterclockwise,opposite,stay";
    Word worlds=256,seed=141169,bouts=32,probes=16,length=8,geometry=0,teaching=0;bool fresh=false;
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--help") {
            std::cout<<"axion imagine  |  acquire a model of the effects of an action\n\n"
                     <<"--load FILE --save FILE --sounds A,B,C,D --seed N\n"
                     <<"--dynamics compass|permutation|arbitrary --observations N --worlds N --probes N --length N\n"
                     <<"--origin fresh|experienced --teaching intact|fixed|displaced   experience for the saved individual\n";
            return 0;
        }
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") source=v;else if (f=="--save") destination=v;else if (f=="--sounds") vocab=v;
        else if (f=="--worlds") worlds=quantity(v,1,65536);else if (f=="--seed") seed=quantity(v,0,~0u);
        else if (f=="--observations") bouts=quantity(v,0,65536);else if (f=="--probes") probes=quantity(v,1,256);
        else if (f=="--length") length=quantity(v,1,256);
        else if (f=="--dynamics" && (v=="compass" || v=="permutation" || v=="arbitrary")) geometry=v=="permutation" ? 1 : v=="arbitrary" ? 2 : 0;
        else if (f=="--teaching" && (v=="intact" || v=="fixed" || v=="displaced")) teaching=v=="fixed" ? 1 : v=="displaced" ? 2 : 0;
        else if (f=="--origin" && (v=="fresh" || v=="experienced")) fresh=v=="fresh";
        else throw std::invalid_argument("unknown imagery option "+std::string(f));
    }
    std::array<Word,4> sounds{};std::string token;std::istringstream names(vocab);
    for (unsigned i=0;i<sounds.size();++i) {
        if (!std::getline(names,token,',') || token.empty() || token.find_first_of(" \r\t\n")!=std::string::npos) throw std::invalid_argument("supply four distinct sound forms");
        sounds[i]=word_digest(token);
        if (std::find(sounds.begin(),sounds.begin()+i,sounds[i])!=sounds.begin()+i) throw std::invalid_argument("duplicate action sound");
    }
    if (names.peek()!=std::char_traits<char>::eof() || vocab.back()==',') throw std::invalid_argument("too many action sounds");
    AgentImage brain;brain.read(source);
    if (!brain.recollects) throw std::invalid_argument("a prospective circuit requires a visual plasticity program");
    Session s;s.open();Function trials{},school{};
    s.bind(trials,"imagery_trials",Image::recollection);s.bind(school,"imagery_school",Image::recollection);
    Temporary receptors(s.d,sizeof(sounds)),tallies(s.d,Count(worlds)*sizeof(RecallScore));
    s.d.check(s.d.upload(receptors.ptr,sounds.data(),sizeof(sounds)),"present action sounds");
    if (auto *j=journal::current()) {j->source=source;j->device=s.name;j->metrics["worlds"]=worlds;j->metrics["observations"]=bouts;}
    constexpr std::array arms{"intact","fixed","displaced","silent","reverse","locked","compiled"};
    constexpr std::array phases{"single","pair","sequence","changed","changed_sequence"};
    constexpr std::array laws{"compass","permutation","arbitrary"};
    std::cout<<s.name<<"  action_bytes="<<sizeof(Imagery)<<"  shared_rule_low="<<std::hex<<brain.recollection.genes.low
             <<"  shared_rule_high="<<brain.recollection.genes.high<<std::dec<<"  observations="<<bouts<<"  sequence="<<length<<'\n';
    std::vector<RecallScore> rows(worlds),reference;
    struct Timers { Driver &d;Event begin{},end{};~Timers(){if(begin) d.event_free(begin);if(end) d.event_free(end);} } times{s.d};
    s.d.check(s.d.event_new(&times.begin,0),"allocate forecast timer");s.d.check(s.d.event_new(&times.end,0),"allocate forecast timer");
    for (Word arm=0;arm<arms.size();++arm) {
        Word granule=std::max(128u,2097152u/(probes*(length+3)+bouts));
        s.d.check(s.d.record(times.begin,nullptr),"start prospective experience");
        for (Word first=0;first<worlds;first+=granule) {
            Word count=std::min(granule,worlds-first);
            void *args[]{&brain.recollection.genes,&receptors.ptr,&worlds,&seed,&bouts,&probes,&length,&geometry,&arm,&tallies.ptr,&first,&count};
            s.d.check(s.d.launch(trials,(count+127)/128,1,1,128,1,1,0,nullptr,args,nullptr),"observe actions and replay unfamiliar sequences");
        }
        s.d.check(s.d.record(times.end,nullptr),"end prospective experience");s.d.check(s.d.wait(times.end),"finish prospective experience");
        float ms{};s.d.check(s.d.elapsed(&ms,times.begin,times.end),"time physical experience and replay");
        s.d.check(s.d.download(rows.data(),tallies.ptr,rows.size()*sizeof(RecallScore)),"read predicted transitions");
        if (!arm) reference=rows;
        if (arm==6 && std::memcmp(reference.data(),rows.data(),rows.size()*sizeof(RecallScore))) throw std::runtime_error("compiled forecasts diverged from the direct circuit");
        std::array<Count,5> total{};for (const auto &r:rows) {total[0]+=r.one;total[1]+=r.delay;total[2]+=r.noise;total[3]+=r.revision;total[4]+=r.faint;}
        Count n=Count(worlds)*probes;
        std::cout<<"imagery="<<arms[arm]<<"  dynamics="<<laws[geometry]<<"  seed="<<seed<<"  worlds="<<worlds<<"  length="<<length<<"  probes="<<n;
        for (unsigned k=0;k<phases.size();++k) {
            std::cout<<"  "<<phases[k]<<'='<<total[k];
            if (auto *j=journal::current();j && !arm) j->scores.push_back({"Imagined / "+std::string(phases[k]),total[k],n});
        }
        std::cout<<"  GPU_ms="<<std::fixed<<std::setprecision(3)<<ms<<"  replay_steps="<<n*(4+2*length)<<'\n';
    }
    if (!destination.empty()) {
        Temporary image(s.d,sizeof(Imagery));
        if (fresh) brain.imagery={};
        s.d.check(s.d.upload(image.ptr,&brain.imagery,sizeof(Imagery)),"restore previous action experience");
        void *args[]{&image.ptr,&brain.recollection.genes,&receptors.ptr,&seed,&bouts,&geometry,&teaching};
        s.d.check(s.d.launch(school,1,1,1,1,1,1,0,nullptr,args,nullptr),"experience the physical apparatus");
        s.d.check(s.d.download(&brain.imagery,image.ptr,sizeof(Imagery)),"read acquired transition contacts");
        brain.imagines=true;write_checked(destination,brain);
    }
    return 0;
}
namespace {
std::vector<Word> four_bearing_laws() {
    std::vector<Word> rows,laws;
    std::array<Word,4> values{0,1,2,3};
    do {rows.push_back(values[0]|(values[1]<<2)|(values[2]<<4)|(values[3]<<6));}
    while (std::next_permutation(values.begin(),values.end()));
    auto compatible=[](Word a,Word b) {Word unequal=a^b;return ((unequal|(unequal>>1))&85)==85;};
    for (Word a:rows) for (Word b:rows) if (compatible(a,b))
        for (Word c:rows) if (compatible(c,a) && compatible(c,b))
            for (Word d:rows) if (compatible(d,a) && compatible(d,b) && compatible(d,c))
                laws.push_back(a|(b<<8)|(c<<16)|(d<<24));
    if (laws.size()!=576) throw std::logic_error("incomplete external apparatus family");
    return laws;
}
void certify_joint(Nursery &lab,const Conditions &q,RecollectionGenes vision,BindingGenes sources,RecollectionGenes gates) {
    auto laws=four_bearing_laws(),reference=laws;
    if (q.census==1) {
        reference.assign(2,0u);
        for (Word i=0;i<16;++i) {
            reference[0]|=(((i&3)+(i>>2))&3)<<(2*i);
            reference[1]|=((i&3)^(i>>2))<<(2*i);
        }
    }
    constexpr Word batch=8;Word n=Word(reference.size()),seed=q.probe_seed;
    auto &d=lab.d;Temporary external(d,laws.size()*4),physical(d,n*4),table(d,Count(batch)*65536*2*4),counts(d,17*7*4);
    d.check(d.upload(external.ptr,laws.data(),laws.size()*4),"place possible apparatuses in the external scorer");
    d.check(d.upload(physical.ptr,reference.data(),n*4),"place physical laws in the environment");
    d.check(d.zero(counts.ptr,0,17*7*4),"clear exhaustive audit counts");
    Function pairs{},closure{},check{};lab.bind(pairs,"joint_oracle_seed",Image::recollection);
    lab.bind(closure,"joint_oracle_closure",Image::recollection);lab.bind(check,"joint_certify",Image::recollection);
    for (Word start=0;start<n;start+=batch) {
        Word size=std::min(n-start,batch);auto scenes=physical.ptr+Count(start)*4;
        d.check(d.zero(table.ptr,0,Count(size)*65536*2*4),"clear compatible apparatus sets");
        void *begin[]{&table.ptr,&scenes,&external.ptr,&size};lab.launch(pairs,576*size,begin);
        for (Word bit=0;bit<16;++bit) {
            void *spread[]{&table.ptr,&size,&bit};lab.launch(closure,size*65536,spread);
        }
        void *audit[]{&table.ptr,&scenes,&size,&seed,&vision,&sources,&gates,&counts.ptr};
        lab.launch(check,size*65536,audit);
    }
    std::array<Word,17*7> results{};d.check(d.download(results.data(),counts.ptr,sizeof(results)),"read exhaustive results");
    constexpr std::array names{"forced_correct","forced_silent","forced_wrong","ambiguous_lucky","ambiguous_silent","ambiguous_wrong"};
    std::array<Count,6> total{};Count choose=1,queries=0;
    for (Word clues=1;clues<16;++clues) {
        choose=choose*(17-clues)/clues;Count items=choose*(16-clues)*n,observed=0;
        std::cout<<"joint_clues="<<clues<<"  references="<<n<<"  queries="<<items;
        for (Word k=0;k<6;++k) {
            Word value=results[7*clues+k];observed+=value;total[k]+=value;
            std::cout<<"  "<<names[k]<<'='<<value;
        }
        std::cout<<'\n';queries+=items;
        if (observed!=items || results[7*clues+6]) throw std::runtime_error("external completion audit lost a physical possibility");
    }
    std::cout<<"joint_census="<<(q.census==1 ? "representative" : "all")<<"  protocol=shared-order  references="<<n<<"  histories="<<Count(n)*65534<<"  queries="<<queries;
    for (Word k=0;k<6;++k) std::cout<<"  "<<names[k]<<'='<<total[k];
    std::cout<<'\n';
}
void benchmark_joint(Nursery &lab,const Conditions &q,RecollectionGenes vision,BindingGenes sources,RecollectionGenes gates) {
    auto &d=lab.d;Word n=q.replicas,steps=q.steps,salt=q.probe_seed;
    constexpr Word raw_bytes=sizeof(conjunction_batch::Acquired),packed_bytes=sizeof(conjunction_batch::Frozen);
    Temporary raw(d,Count(n)*raw_bytes),packed(d,Count(n)*packed_bytes),output(d,5*Count(n)*sizeof(Word));
    Function seed{},compile{},predict{};lab.bind(seed,"joint_batch_seed",Image::recollection);
    lab.bind(compile,"joint_batch_compile",Image::recollection);lab.bind(predict,"joint_batch_predict",Image::recollection);
    void *init[]{&raw.ptr,&n,&salt,&vision,&sources};lab.launch(seed,n,init);
    struct Interval {Driver &d;Event start{},end{};~Interval(){if (start) d.event_free(start);if (end) d.event_free(end);}} timer{d};
    d.check(d.event_new(&timer.start,0),"create joint replay timer");d.check(d.event_new(&timer.end,0),"create joint replay timer");
    auto time=[&](Function f,void **args) {
        d.check(d.record(timer.start,nullptr),"start joint replay timer");lab.launch(f,n,args);
        d.check(d.record(timer.end,nullptr),"stop joint replay timer");d.check(d.wait(timer.end),"complete joint replay");
        float ms{};d.check(d.elapsed(&ms,timer.start,timer.end),"measure joint replay");return ms;
    };
    void *freeze[]{&raw.ptr,&packed.ptr,&n,&vision,&sources,&gates};
    float prepare=time(compile,freeze);
    std::vector<Word> reference,results(5*size_t(n));
    for (Word layout=0;layout<2;++layout) {
        auto bank=layout ? packed.ptr : raw.ptr;
        void *args[]{&bank,&n,&salt,&steps,&layout,&output.ptr,&vision,&sources,&gates};
        for (Word repeat=0;repeat<4;++repeat) {
            float ms=time(predict,args);
            if (!repeat) continue;
            d.check(d.download(results.data(),output.ptr,results.size()*sizeof(Word)),"verify joint arrivals for every resident");
            if (reference.empty()) reference=results;
            else if (reference!=results) throw std::runtime_error("compiled joint reports diverged from acquired contacts");
            Count spoken=0;for (Word i=0;i<n;++i) spoken+=results[n+i];
            Count queries=Count(n)*steps;
            std::cout<<"joint_batch="<<(layout ? "compiled" : "direct")<<"  repeat="<<repeat<<"  individuals="<<n
                     <<"  queries="<<queries<<"  voiced="<<spoken<<"  bytes_per_action="<<(layout ? packed_bytes : raw_bytes)
                     <<"  compile_ms="<<std::fixed<<std::setprecision(3)<<prepare<<"  GPU_ms="<<ms<<"  Mqueries/s="<<double(queries)/(1000*ms)<<'\n';
        }
    }
}
}
#include "causal_certificate.hpp"
int cultivate_command(int argc,char **argv) {
    Conditions q;q.source="examples/cartographer.mind";q.worlds=6;q.cycles=3;q.delay=36;q.generations=96;q.candidates=128;
    bool newborn=false;std::string inherit,census;bool selected_source=false;
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--help") {std::cout<<"axion cultivate | evolve learning of action effects, then plan with a private model\n"
            <<"--load MIND --save MIND --germline MIND --newborn --generations N --candidates N --worlds N --cycles N\n"
            <<"--lessons N --probe-worlds N --seed N --probe-seed N --origin inherited|inert|random --breeding select|drift\n"
            <<"--certify FILE   check every goal, starting state and motor subset against 36 physical truth tables\n";return 0;}
        if (f=="--newborn") {newborn=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") {q.source=v;selected_source=true;}else if (f=="--germline") inherit=v;else if (f=="--save") q.destination=v;
        else if (f=="--certify") census=v;
        else if (f=="--generations") q.generations=quantity(v,0,65536);
        else if (f=="--candidates") q.candidates=quantity(v,1,65536);
        else if (f=="--worlds") q.worlds=quantity(v,1,128);
        else if (f=="--cycles") q.cycles=quantity(v,1,128);
        else if (f=="--lessons") q.delay=quantity(v,1,256);
        else if (f=="--probe-worlds") q.tests=quantity(v,1,65536);
        else if (f=="--seed") q.seed=quantity(v,0,~0u);
        else if (f=="--probe-seed") q.probe_seed=quantity(v,0,~0u);
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else if (f=="--origin" && (v=="inherited" || v=="inert" || v=="random")) q.origin=v=="inert" ? 1 : v=="random" ? 2 : 0;
        else throw std::invalid_argument("invalid cultivation option "+std::string(f));
    }
    if (!selected_source && !census.empty()) q.source="examples/gardener.mind";
    AgentImage pupil;pupil.read(q.source);if (!pupil.conceptual) throw std::invalid_argument("causal rearing needs an animal with grounded object contacts");
    Nursery lab("causal_live");
    if (!census.empty()) {
        if (!q.destination.empty() || newborn || !inherit.empty()) throw std::invalid_argument("the action census reads a frozen individual");
        if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;}
        return certify_causal(lab,pupil,census);
    }
    auto genes=pupil.agency.model.genes;bool fresh=newborn || q.generations || q.origin || !pupil.causal;
    if (!inherit.empty()) {AgentImage parent;parent.read(inherit);if (!parent.causal) throw std::invalid_argument("the donor has no causal germline");genes=parent.agency.model.genes;fresh=true;}
    if (q.origin==1) {genes={0,0,0};q.origin=0;}
    genes=lab.choose(q,genes);Function probe{};lab.bind(probe,"causal_trials",Image::causes);
    Temporary models(lab.d,Count(q.tests)*sizeof(CausalModel)),plans(lab.d,Count(q.tests)*sizeof(Intention)),results(lab.d,Count(q.tests)*sizeof(CausalAudit));
    std::cout<<lab.name<<"  causal_model_bytes="<<sizeof(CausalModel)<<"  agency_bytes="<<sizeof(Agency)<<"  plasticity="<<std::hex<<genes.low
             <<"  selection="<<genes.high<<"  replay="<<genes.expression<<std::dec<<'\n';
    const char *names[]{"intact","unexposed","shuffled","episodic","one-beat"};
    if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;}
    std::vector<CausalAudit> rows(q.tests);
    for (Word cut=0;cut<5;++cut) {
        void *args[]{&genes,&models.ptr,&plans.ptr,&results.ptr,&q.tests,&q.probe_seed,&q.delay,&cut};lab.launch(probe,32*q.tests,args);
        lab.d.check(lab.d.download(rows.data(),results.ptr,rows.size()*sizeof(CausalAudit)),"read physical trials and omission interventions");
        Count totals[8]{};for (auto &r:rows) {totals[0]+=r.forecasts;totals[1]+=r.reached;totals[2]+=r.length;totals[3]+=r.reason;totals[4]+=r.unreachable;totals[5]+=r.decided;totals[6]+=r.compiled;totals[7]+=r.necessary;}
        const char *fields[]{"forecasts","goals","actions","reasons","impossible","decided","compiled","essential"};
        std::cout<<"cultivation="<<names[cut]<<"  worlds="<<q.tests;
        for (Word i=0;i<8;++i) std::cout<<"  "<<fields[i]<<'='<<totals[i];
        std::cout<<'\n';
    }
    if (!q.destination.empty()) {
        if (fresh) pupil.agency={};
        pupil.causal=true;pupil.agency.model.genes=genes;write_checked(q.destination,pupil);
    }
    return 0;
}
int concept_command(int argc,char **argv) {
    Conditions q;q.source="examples/storyteller.mind";q.partner="examples/confidant.mind";q.generations=96;q.candidates=128;
    q.worlds=4;q.cycles=2;q.delay=8;bool newborn=false,specified_source=false;std::string census,law,measure_word="circuit";
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--help") {std::cout<<"axion infer  |  evolve the acquisition of reusable relations and private perspectives\n"
            <<"--load FILE --save FILE --listener FILE --save-listener FILE --newborn\n"
            <<"--census WORD --law DIGITS   audit every three-object graph against signed ports 0..7\n"
            <<"--batch N --word WORD --decisions N   verify and time a frozen word on independent maps\n"
            <<"--generations N --candidates N --worlds N --cycles N --exposures N --probe-worlds N\n"
            <<"--seed N --probe-seed N --origin inert|random|inherited --breeding select|drift\n";return 0;}
        if (f=="--newborn") {newborn=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") {q.source=v;specified_source=true;}else if (f=="--save") q.destination=v;
        else if (f=="--census") census=v;else if (f=="--law") law=v;
        else if (f=="--batch") q.replicas=quantity(v,1,1048576);else if (f=="--decisions") q.steps=quantity(v,1,65536);else if (f=="--word") measure_word=v;
        else if (f=="--listener") q.partner=v;else if (f=="--save-listener") q.save_partner=v;
        else if (f=="--generations") q.generations=quantity(v,0,65536);
        else if (f=="--candidates") q.candidates=quantity(v,1,65536);
        else if (f=="--worlds") q.worlds=quantity(v,1,128);
        else if (f=="--cycles") q.cycles=quantity(v,1,128);
        else if (f=="--exposures") q.delay=quantity(v,1,256);
        else if (f=="--probe-worlds") q.tests=quantity(v,1,65536);
        else if (f=="--seed") q.seed=quantity(v,0,~0u);
        else if (f=="--probe-seed") q.probe_seed=quantity(v,0,~0u);
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else if (f=="--origin" && (v=="inert" || v=="random" || v=="inherited")) q.origin=v=="inert" ? 1 : v=="random" ? 2 : 0;
        else throw std::invalid_argument("invalid concept option "+std::string(f));
    }
    if (q.replicas || !census.empty() || !law.empty()) {
        if (!specified_source) q.source="examples/cartographer.mind";
        if (newborn || !q.destination.empty() || !q.save_partner.empty()) throw std::invalid_argument("a frozen readout does not rear or save an individual");
    }
    if (!q.destination.empty() && !q.save_partner.empty() && std::filesystem::absolute(q.destination).lexically_normal()==std::filesystem::absolute(q.save_partner).lexically_normal())
        throw std::invalid_argument("save the independently stateful individuals to different files");
    AgentImage witness,peer;witness.read(q.source);if (!q.save_partner.empty()) peer.read(q.partner);
    Nursery lab("concept_live");
    if (q.replicas) {
        if (!census.empty() || !law.empty()) throw std::invalid_argument("use separate runs for timing and a graph census");
        Word op=meaning::word(witness.concepts,sound_pattern(measure_word)),n=q.replicas;
        if (!witness.conceptual || op>=concept_words) throw std::invalid_argument("the frozen readout needs an acquired word");
        Function show{},compile{},verify{},direct{},packed{};
        for (auto [f,name]:{std::pair{&show,"concept_batch_show"},{&compile,"concept_batch_compile"},{&verify,"concept_batch_verify"},
                {&direct,"concept_batch_read"},{&packed,"concept_batch_projected"}}) lab.bind(*f,name,Image::recollection);
        Temporary mind(lab.d,sizeof(Concepts)),scenes(lab.d,Count(n)*sizeof(RelationalView)),fields(lab.d,Count((n+31)/32)*32*sizeof(EndpointProjection)),out(lab.d,Count(n)*sizeof(Word));
        lab.d.check(lab.d.upload(mind.ptr,&witness.concepts,sizeof(Concepts)),"present the frozen word");
        void *draw[]{&mind.ptr,&scenes.ptr,&n,&q.seed};lab.launch(show,n*32,draw);
        struct Clock {Driver &d;Event a{},b{};~Clock(){if (a) d.event_free(a);if (b) d.event_free(b);}} timer{lab.d};
        lab.d.check(lab.d.event_new(&timer.a,0),"open projection timer");lab.d.check(lab.d.event_new(&timer.b,0),"open readout timer");
        auto timed=[&](Function f,Word threads,void **args) {
            lab.d.check(lab.d.record(timer.a,nullptr),"start readout clock");lab.launch(f,threads,args);
            lab.d.check(lab.d.record(timer.b,nullptr),"stop readout clock");lab.d.check(lab.d.wait(timer.b),"finish timed readout");
            float ms{};lab.d.check(lab.d.elapsed(&ms,timer.a,timer.b),"read device time");return ms;
        };
        void *build[]{&mind.ptr,&op,&scenes.ptr,&fields.ptr,&n};
        float preparation=timed(compile,32*n,build);
        void *check[]{&mind.ptr,&op,&scenes.ptr,&fields.ptr,&n,&out.ptr};lab.launch(verify,32*n,check);
        std::vector<Word> observed(n),reference(n);
        lab.d.check(lab.d.download(observed.data(),out.ptr,Count(n)*sizeof(Word)),"check every projected endpoint");
        if (std::any_of(observed.begin(),observed.end(),[](Word v){return v!=0;})) throw std::runtime_error("projected endpoint changed the frozen response");
        for (Word pass=0;pass<4;++pass) {
            Word salt=mix(q.seed+pass);void *ask[]{&mind.ptr,&op,&scenes.ptr,&fields.ptr,&n,&q.steps,&salt,&out.ptr};
            for (Word layout=0;layout<2;++layout) {
                float ms=timed(layout ? packed : direct,n,ask);
                auto &r=layout ? observed : reference;lab.d.check(lab.d.download(r.data(),out.ptr,Count(n)*sizeof(Word)),"verify frozen decisions");
                if (layout && r!=reference) throw std::runtime_error("projected query stream diverged");
                if (pass) std::cout<<"endpoint_batch="<<(layout ? "projected" : "direct")<<"  word="<<measure_word<<"  replicas="<<n
                    <<"  bytes="<<(layout ? sizeof(EndpointProjection) : sizeof(RelationalView))<<"  verified="<<32ull*n
                    <<"  queries="<<Count(n)*q.steps<<"  preparation_ms="<<std::fixed<<std::setprecision(4)<<preparation<<"  GPU_ms="<<ms
                    <<"  Mqueries_per_s="<<Count(n)*q.steps/(1000.0*ms)<<'\n';
            }
        }
        if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;}return 0;
    }
    if (!census.empty() || !law.empty()) {
        Word op=meaning::word(witness.concepts,sound_pattern(census));
        if (!witness.conceptual || op>=concept_words || law.empty() || law.size()>256 || law.find_first_not_of("01234567")!=std::string::npos)
            throw std::invalid_argument("the graph census needs an acquired word and a reference sequence of 1..256 directed ports");
        constexpr Word graphs=531441;
        Function probe{};lab.bind(probe,"concept_census",Image::recollection);
        Temporary individual(lab.d,sizeof(Concepts)),predictions(lab.d,3*graphs);
        lab.d.check(lab.d.upload(individual.ptr,&witness.concepts,sizeof(Concepts)),"introduce the acquired concept");
        Word total=graphs;void *args[]{&individual.ptr,&op,&predictions.ptr,&total};lab.launch(probe,total,args);
        std::vector<unsigned char> answers(3*graphs);lab.d.check(lab.d.download(answers.data(),predictions.ptr,answers.size()),"read every graph prediction");
        Count correct=0,unique=0,empty=0,ambiguous=0;
        for (Word i=0;i<graphs;++i) {
            Word code=i,edge[4][3];
            for (auto &row:edge) for (auto &target:row) {target=code%3;code/=3;}
            for (Word x=0;x<3;++x) {
                Word active=1u<<x;bool gap=false;
                for (char symbol:law) {
                    Word port=Word(symbol-'0'),next=0;
                    for (Word a=0;a<3;++a) if (active&(1u<<a)) {
                        Word successors=0;
                        for (Word b=0;b<3;++b) if (port<4 ? edge[port][a]==b : edge[port&3][b]==a) successors|=1u<<b;
                        next|=successors;gap|=successors==0;
                    }
                    active=next;
                }
                correct+=answers[3*i+x]==(active|(Word(gap)<<3));
                if (!active) ++empty;else if (!gap && std::has_single_bit(active)) ++unique;else ++ambiguous;
            }
        }
        std::cout<<"concept_census="<<census<<"  law="<<law<<"  graphs="<<graphs<<"  trials="<<answers.size()<<"  exact="<<correct
                 <<"  unique="<<unique<<"  empty="<<empty<<"  ambiguous="<<ambiguous<<"  digest="<<std::hex<<digest(hash_start,answers.data(),answers.size())<<std::dec<<'\n';
        if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;}return 0;
    }
    auto g=witness.concepts.genes;bool reset=newborn || q.generations || q.origin;
    if (q.origin==1) {g={0,0,0};q.origin=0;}
    g=lab.choose(q,g);
    std::cout<<lab.name<<"  concept_bytes="<<sizeof(Concepts)<<"  generations="<<q.generations<<"  exposures="<<q.delay
             <<"  gates="<<std::hex<<g.low<<"  induction="<<g.high<<"  expression="<<g.expression<<std::dec<<'\n';
    auto *j=journal::current();if (j) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;}
    Temporary parent(lab.d,sizeof(g)),output(lab.d,Count(q.tests)*sizeof(RecallScore));
    lab.d.check(lab.d.upload(parent.ptr,&g,sizeof(g)),"present the inherited relational program");
    std::vector<RecallScore> rows(q.tests);constexpr std::array arms{"intact","no-examples","own-view","no-reuse","fixed","random-question"};
    constexpr std::array names{"own","other","transfer","recursive","inquiry","naming"};
    for (Word arm=0;arm<arms.size();++arm) {
        lab.evaluate(parent.ptr,output.ptr,1,q.tests,q.probe_seed,q.cycles,q.delay,0,RecallCut(arm));
        lab.d.check(lab.d.download(rows.data(),output.ptr,rows.size()*sizeof(RecallScore)),"read held-out relational decisions");
        std::array<Count,6> totals{};for (auto v:rows) {totals[0]+=v.one;totals[1]+=v.delay;totals[2]+=v.noise;totals[3]+=v.revision;totals[4]+=v.faint;totals[5]+=v.empty;}
        Count n=Count(q.tests)*q.cycles;std::cout<<"inference="<<arms[arm]<<"  trials="<<n<<"  view_trials="<<8*n;
        for (Word k=0;k<totals.size();++k) {std::cout<<"  "<<names[k]<<'='<<totals[k];if (j && !arm) j->scores.push_back({"Relations / "+std::string(names[k]),totals[k],k<2 ? 8*n : n});}
        std::cout<<'\n';
    }
    auto save=[&](AgentImage &image,const std::string &path) {
        if (path.empty()) return;
        if (reset || !image.conceptual) image.concepts={};
        image.conceptual=true;image.concepts.genes=g;meaning::consolidate(image.concepts,0);write_checked(path,image);
    };
    save(witness,q.destination);save(peer,q.save_partner);return 0;
}
int report_command(int argc,char **argv) {
    Conditions q;q.source="examples/weaver-sparse.mind";q.partner="examples/inquirer.mind";
    q.generations=128;q.candidates=128;q.worlds=4;q.cycles=8;q.delay=64;q.sparse=4;
    bool newborn=false;std::array<std::string_view,5> kinds{"recent","earlier","two","variable","mixed"};
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--help") {
            std::cout<<"axion narrate  |  learn to report a private event through an ordered acoustic channel\n"
                     <<"--load FILE --save FILE --listener FILE --save-listener FILE --newborn\n"
                     <<"--generations N --candidates N --worlds N --cycles N --exposures N --probe-worlds N\n"
                     <<"--seed N --probe-seed N --origin inert|random|inherited --breeding select|drift\n"
                     <<"--dialect recent|earlier|two|variable|mixed\n";return 0;
        }
        if (f=="--newborn") {newborn=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") q.source=v;else if (f=="--save") q.destination=v;
        else if (f=="--listener") q.partner=v;else if (f=="--save-listener") q.save_partner=v;
        else if (f=="--generations") q.generations=quantity(v,0,65536);
        else if (f=="--candidates") q.candidates=quantity(v,1,65536);
        else if (f=="--worlds") q.worlds=quantity(v,1,128);
        else if (f=="--cycles") q.cycles=quantity(v,1,128);
        else if (f=="--exposures") q.delay=quantity(v,1,512);
        else if (f=="--probe-worlds") q.tests=quantity(v,1,65536);
        else if (f=="--seed") q.seed=quantity(v,0,~0u);
        else if (f=="--probe-seed") q.probe_seed=quantity(v,0,~0u);
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else if (f=="--origin" && (v=="inert" || v=="random" || v=="inherited")) q.origin=v=="inert" ? 1 : v=="random" ? 2 : 0;
        else if (f=="--dialect" && std::find(kinds.begin(),kinds.end(),v)!=kinds.end()) q.sparse=Word(std::find(kinds.begin(),kinds.end(),v)-kinds.begin());
        else throw std::invalid_argument("invalid report option "+std::string(f));
    }
    if (!q.destination.empty() && !q.save_partner.empty() && std::filesystem::absolute(q.destination).lexically_normal()==std::filesystem::absolute(q.save_partner).lexically_normal())
        throw std::invalid_argument("save the two independent individuals to different files");
    AgentImage speaker,reader;speaker.read(q.source);if (!q.save_partner.empty()) reader.read(q.partner);
    Nursery lab("report_live",{},{},q.sparse);
    auto initial=speaker.report.genes;bool reset=newborn || q.generations || q.origin;
    if (q.origin==1) {initial={0,0,0};q.origin=0;}
    auto genes=lab.choose(q,initial);
    std::cout<<lab.name<<"  report_bytes="<<sizeof(Reports)<<"  generations="<<q.generations<<"  exposures="<<q.delay
             <<"  dialect="<<kinds[q.sparse]<<"  plastic="<<std::hex<<genes.low<<"  write="<<genes.high<<"  expression="<<genes.expression<<std::dec<<'\n';
    auto *j=journal::current();if (j) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;}
    Temporary parent(lab.d,sizeof(genes)),output(lab.d,Count(q.tests)*sizeof(RecallScore));
    lab.d.check(lab.d.upload(parent.ptr,&genes,sizeof(genes)),"present the inherited speech program");
    std::vector<RecallScore> rows(q.tests);constexpr std::array arms{"intact","frozen","displaced","muted","reversed","latest-only"};
    for (Word arm=0;arm<arms.size();++arm) {
        lab.evaluate(parent.ptr,output.ptr,1,q.tests,q.probe_seed,q.cycles,q.delay,0,RecallCut(arm));
        lab.d.check(lab.d.download(rows.data(),output.ptr,rows.size()*sizeof(RecallScore)),"read the two private event histories");
        std::array<Count,6> s{};for (auto v:rows) {s[0]+=v.one;s[1]+=v.delay;s[2]+=v.noise;s[3]+=v.revision;s[4]+=v.faint;s[5]+=v.empty;}
        Count n=Count(q.tests)*q.cycles;
        std::cout<<"narration="<<arms[arm]<<"  trials="<<n<<"  phase_credit="<<s[0]<<"  exact="<<s[1]<<"  decoded="<<s[2]
                 <<"  hidden_silence="<<s[3]<<"  memory="<<s[4]<<"  partial_context="<<s[5]<<'\n';
        if (j && !arm) {j->scores.push_back({"Exact utterances",s[1],n});j->scores.push_back({"Private events communicated",s[2],n});}
    }
    auto save=[&](AgentImage &image,const std::string &path) {
        if (path.empty()) return;
        if (reset || !image.reports) image.report={};
        else if ((image.report.genes.expression^genes.expression)&127) report::invalidate(image.report);
        image.reports=true;image.report.genes=genes;write_checked(path,image);
    };
    save(speaker,q.destination);save(reader,q.save_partner);return 0;
}
int conjunction_command(int argc,char **argv) {
    if (argc>2 && std::string_view(argv[2])=="--help") {
        std::cout<<"axion conjoin  |  evolve predictions from the joint motion of two objects\n"
                 <<"--load FILE --save FILE --generations N --candidates N --worlds N --cycles N\n"
                 <<"--seed N --probe-seed N --probe-worlds N --origin inert|inherited|random --breeding select|drift\n"
                 <<"--replicas N --decisions N   verify and time frozen single-step inference on independent animals\n"
                 <<"--enumerate representative|all   test every nonempty partial board with an independent CUDA oracle\n"
                 <<"--lessons broad|sparse   include sparse evidence and misleading familiar cues in selection\n";
        return 0;
    }
    Conditions defaults;defaults.source="examples/naturalist.mind";defaults.generations=96;defaults.candidates=128;
    defaults.worlds=4;defaults.cycles=2;defaults.delay=0;
    auto q=parse(argc,argv,false,defaults);
    if (q.invariant || q.delay || !q.varied) throw std::invalid_argument("conjoin uses the joint-motion environment");
    AgentImage image;image.read(q.source);
    if (!image.binds || !image.recollects) throw std::invalid_argument("conjoin requires inherited visual and role contacts");
    Nursery lab("conjunction_live",image.recollection.genes,image.bindings.genes,q.sparse);
    auto initial=image.conjunctions.gates;
    bool hatch=q.generations || q.origin;
    if (q.origin==1) {initial={0,0,0};q.origin=0;}
    auto genes=lab.choose(q,initial);
    std::cout<<lab.name<<"  conjunction_bytes="<<sizeof(Conjunctions)<<"  replay_bytes="<<sizeof(SceneWave)
             <<"  generations="<<q.generations<<"  candidates="<<q.candidates<<"  lessons="<<(q.sparse ? "sparse" : "broad")<<"  gate="<<std::hex<<genes.low
             <<"  lateral="<<genes.high<<"  dynamics="<<genes.expression<<std::dec<<'\n';
    auto *j=journal::current();
    if (j) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;}
    Temporary parent(lab.d,sizeof(genes)),out(lab.d,Count(q.tests)*sizeof(RecallScore));
    lab.d.check(lab.d.upload(parent.ptr,&genes,sizeof(genes)),"present inherited conjunctive program");
    std::vector<RecallScore> rows(q.tests);
    constexpr std::array arms{"intact","fixed","no-recurrence","no-joint","one-tick"};
    constexpr std::array names{"arbitrary","sparse_role","completion","ambiguous","revision","unstructured_silence"};
    for (Word arm=0;arm<arms.size();++arm) {
        lab.evaluate(parent.ptr,out.ptr,1,q.tests,q.probe_seed,q.cycles,0,0,RecallCut(arm));
        lab.d.check(lab.d.download(rows.data(),out.ptr,rows.size()*sizeof(RecallScore)),"read joint-motion predictions");
        std::array<Count,6> counts{};
        for (const auto &r:rows) {counts[0]+=r.one;counts[1]+=r.delay;counts[2]+=r.noise;counts[3]+=r.revision;counts[4]+=r.faint;counts[5]+=r.empty;}
        Count n=Count(q.cycles)*q.tests;
        std::cout<<"conjunction="<<arms[arm]<<"  probes="<<n;
        for (Word k=0;k<counts.size();++k) {
            std::cout<<"  "<<names[k]<<'='<<counts[k];
            if (j && !arm) j->scores.push_back({names[k],counts[k],n});
        }
        std::cout<<'\n';
    }
    if (j) {
        Function snapshot{};lab.bind(snapshot,"conjunction_snapshot",Image::recollection);
        Word sample[4]{};
        void *args[]{&out.ptr,&q.probe_seed,&genes,&image.recollection.genes,&image.bindings.genes};
        lab.launch(snapshot,1,args);
        lab.d.check(lab.d.download(sample,out.ptr,sizeof(sample)),"record the held-out scene");
        j->line("conjunction_scene=probe  observed="+std::to_string(sample[0])+"  physical="+std::to_string(sample[1])
                +"  reported="+std::to_string(sample[2])+"  audible="+std::to_string(sample[3]));
    }
    if (q.replicas) benchmark_joint(lab,q,image.recollection.genes,image.bindings.genes,genes);
    if (q.census) certify_joint(lab,q,image.recollection.genes,image.bindings.genes,genes);
    if (!q.destination.empty()) {
        if (hatch) image.conjunctions={};
        image.conjoins=true;image.conjunctions.gates=genes;
        write_checked(q.destination,image);
    }
    return 0;
}
int binding_command(int argc,char **argv) {
    Conditions q;q.source="examples/dreamer.mind";q.generations=192;q.cycles=4;q.delay=0;q.origin=0;
    Word experience=2;bool newborn=false;
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];
        if (f=="--help") {
            std::cout<<"axion relate  |  learn from confounded observations and choose a causal experiment\n\n"
                     <<"--load FILE --save FILE --generations N --candidates N --worlds N --cycles N\n"
                     <<"--seed N --probe-seed N --probe-worlds N --origin inert|random|inherited --breeding select|drift\n"
                     <<"--effects aliased|distinct|independent|mixed --newborn --teach 0..4   rear exchanged, swapped, gathered and scattered fields\n";
            return 0;
        }
        if (f=="--newborn") {newborn=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") q.source=v;else if (f=="--save") q.destination=v;
        else if (f=="--generations") q.generations=quantity(v,0,65536);
        else if (f=="--candidates") q.candidates=quantity(v,1,65536);
        else if (f=="--worlds") q.worlds=quantity(v,1,128);
        else if (f=="--cycles") q.cycles=quantity(v,1,128);
        else if (f=="--probe-worlds") q.tests=quantity(v,1,65536);
        else if (f=="--seed") q.seed=quantity(v,0,~0u);else if (f=="--probe-seed") q.probe_seed=quantity(v,0,~0u);
        else if (f=="--origin" && (v=="inert" || v=="random" || v=="inherited")) q.origin=v=="inert" ? 1 : v=="random" ? 2 : 0;
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else if (f=="--effects" && (v=="aliased" || v=="distinct" || v=="independent" || v=="mixed"))
            q.delay=v=="distinct" ? 1 : v=="independent" ? 2 : v=="mixed" ? 3 : 0;
        else if (f=="--teach") experience=quantity(v,0,4);
        else throw std::invalid_argument("unknown object-binding option "+std::string(f));
    }
    AgentImage image;image.read(q.source);
    if (!image.recollects) throw std::invalid_argument("object binding requires a visual plasticity program");
    Nursery lab("binding_live",image.recollection.genes);
    auto g=image.bindings.genes;auto rule=lab.choose(q,{g.plastic,g.competition,g.exploration});
    std::cout<<lab.name<<"  binding_bytes="<<sizeof(Bindings)<<"  generations="<<q.generations<<"  candidate_lifetimes="<<Count(q.generations)*q.candidates*q.worlds
             <<"  plastic="<<std::hex<<rule.low<<"  competition="<<rule.high<<"  exploration="<<rule.expression<<std::dec<<'\n';
    if (auto *j=journal::current()) {j->source=q.source;j->device=lab.name;j->metrics["generations"]=q.generations;}
    Temporary parent(lab.d,sizeof(rule)),out(lab.d,Count(q.tests)*sizeof(RecallScore));
    lab.d.check(lab.d.upload(parent.ptr,&rule,sizeof(rule)),"present inherited binding program");
    std::vector<RecallScore> rows(q.tests);constexpr std::array arms{"intact","fixed-support","random-choice","confounded","crossed-arrivals"};
    constexpr std::array names{"uncertain","first","second","revised_first","revised_second","diagnostic"};
    constexpr std::array effects{"aliased","distinct","independent","mixed"};
    for (Word i=0;i<arms.size();++i) {
        lab.evaluate(parent.ptr,out.ptr,1,q.tests,q.probe_seed,q.cycles,q.delay,0,RecallCut(i));
        lab.d.check(lab.d.download(rows.data(),out.ptr,rows.size()*sizeof(RecallScore)),"read results of causal intervention");
        std::array<Count,6> counts{};
        for (const auto &r:rows) {counts[0]+=r.one;counts[1]+=r.delay;counts[2]+=r.noise;counts[3]+=r.revision;counts[4]+=r.faint;counts[5]+=r.empty;}
        Count n=Count(q.tests)*q.cycles;
        std::cout<<"binding="<<arms[i]<<"  effects="<<effects[q.delay]<<"  probes="<<n;
        for (Word k=0;k<counts.size();++k) {
            std::cout<<"  "<<names[k]<<'='<<counts[k];
            if (auto *j=journal::current();j && !i) j->scores.push_back({"Bindings / "+std::string(names[k]),counts[k],n});
        }
        std::cout<<'\n';
    }
    if (!q.destination.empty()) {
        if (q.generations || q.origin || !image.binds || newborn) {
            image.bindings={};
            for (auto &field:image.conjunctions.fields) field={};
        }
        image.binds=true;image.bindings.genes={rule.low,rule.high,rule.expression};
        std::array<Word,4> forms{word_digest("exchanged"),word_digest("swapped"),word_digest("gathered"),word_digest("scattered")};
        if (experience) {
            Temporary layer(lab.d,sizeof(Bindings)),sounds(lab.d,sizeof(forms));Function teach{};lab.bind(teach,"binding_school",Image::recollection);
            lab.d.check(lab.d.upload(layer.ptr,&image.bindings,sizeof(Bindings)),"restore coupled experience");
            lab.d.check(lab.d.upload(sounds.ptr,forms.data(),sizeof(forms)),"present actions' sounds");
            Temporary joint(lab.d,image.conjoins ? sizeof(Conjunctions) : 0);
            if (joint.ptr) lab.d.check(lab.d.upload(joint.ptr,&image.conjunctions,sizeof(Conjunctions)),"restore conjunctive contacts");
            void *args[]{&layer.ptr,&image.recollection.genes,&sounds.ptr,&q.seed,&experience,&joint.ptr};
            lab.d.check(lab.d.launch(teach,1,1,1,1,1,1,0,nullptr,args,nullptr),"observe the two objects in motion");
            lab.d.check(lab.d.download(&image.bindings,layer.ptr,sizeof(Bindings)),"read acquired role contacts");
            if (joint.ptr) lab.d.check(lab.d.download(&image.conjunctions,joint.ptr,sizeof(Conjunctions)),"read conjunctive contacts");
        }
        write_checked(q.destination,image);
    }
    return 0;
}
}
