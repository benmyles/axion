#include "development_io.hpp"
#include "composition_host.hpp"
#include "pulse_words.hpp"
#include "life_runtime.hpp"
#include "sequence_probe.cuh"
#include "sequence_world.cuh"
#include "sequence_audit.hpp"
#include "sequence_compact.cuh"
#include "sequence_colony_io.hpp"
#include <set>
#include <algorithm>
#include <bit>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace axion::life {
namespace {
using namespace runtime;
using namespace storage;
using sequence::Experiment;
using sequence::Inquiry;

Word number(std::string_view text,Word low,Word high) {
    Word n=0;auto [end,ec]=std::from_chars(text.data(),text.data()+text.size(),n);
    if (ec!=std::errc{} || end!=text.data()+text.size() || n<low || n>high)
        throw std::invalid_argument("quantity outside the supported range: "+std::string(text));
    return n;
}
using acoustic::pulse;
using acoustic::spell;
struct Options {
    DevelopmentLimits limits{};
    Word family=chambers::network,n=32,states=6,motors=3,sounds=3,seed=913,noise=0,walk=256;
    Word generations=8,candidates=32,origin=2,drift=0,replicas=0,ticks=128,colony=0,seasons=8,days=64,arm=0;
    std::string eggs,resume,method="probe",family_name="network",source="examples/learner.mind",save,gestures,peer;
};
Options options(int argc,char **argv) {
    Options q;
    for (int i=2;i<argc;++i) {
        std::string f=argv[i];
        if (f=="--external") {q.method="external";continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+f);
        std::string v=argv[i];
        if (f=="--family") {
            const std::vector<std::string> names{"ring","latches","network","hideout","rhythm"};
            auto k=std::find(names.begin(),names.end(),v);
            if (k==names.end()) throw std::invalid_argument("unknown physical apparatus");
            q.family=Word(k-names.begin());q.family_name=v;
        } else if (f=="--method" && (v=="probe" || v=="replay")) q.method=v;
        else if (f=="--worlds") q.n=number(v,1,4096);
        else if (f=="--states") q.states=number(v,1,64);
        else if (f=="--motors") q.motors=number(v,2,16);
        else if (f=="--sounds") q.sounds=number(v,2,32);
        else if (f=="--noise") q.noise=number(v,0,32);
        else if (f=="--seed") q.seed=number(v,0,~0u);
        else if (f=="--budget") q.limits.queries=number(v,1,100000);
        else if (f=="--patience") q.limits.patience=number(v,1,8192);
        else if (f=="--span") q.limits.span=number(v,1,sequence::horizon);
        else if (f=="--capacity") q.limits.cells=number(v,1,sequence_cells);
        else if (f=="--steps") q.walk=number(v,1,trace_slots);
        else if (f=="--generations") q.generations=number(v,0,2048);
        else if (f=="--candidates") q.candidates=number(v,1,2048);
        else if (f=="--origin" && (v=="random" || v=="inert" || v=="inherited")) q.origin=v=="random" ? 2 : v=="inert" ? 1 : 0;
        else if (f=="--breeding" && (v=="select" || v=="drift")) q.drift=v=="drift";
        else if (f=="--load") q.source=v;
        else if (f=="--save") q.save=v;
        else if (f=="--peer") q.peer=v;
        else if (f=="--gestures") q.gestures=v;
        else if (f=="--save-colony") q.eggs=v;
        else if (f=="--resume-colony") q.resume=v;
        else if (f=="--colony") q.colony=number(v,32,65536);
        else if (f=="--seasons") q.seasons=number(v,1,4096);
        else if (f=="--days") q.days=number(v,1,4096);
        else if (f=="--forage" && (v=="intact" || v=="deaf" || v=="wander")) q.arm=v=="deaf" ? 1 : v=="wander" ? 2 : 0;
        else if (f=="--replicas") q.replicas=number(v,1,1048576);
        else if (f=="--ticks") q.ticks=number(v,1,65536);
        else throw std::invalid_argument("unknown developmental option "+f);
    }
    if (q.method=="external" && (q.gestures.empty() || !q.peer.empty())) throw std::invalid_argument("external rearing requires --gestures and a single physical partner");
    if (q.colony%32) throw std::invalid_argument("a chorus island houses 32 residents");
    if (!q.eggs.empty() && q.resume.empty() && !q.colony) throw std::invalid_argument("saving eggs requires a colony");
    if ((q.colony || !q.resume.empty()) && (q.method=="external" || !q.peer.empty())) throw std::invalid_argument("a colony supplies its own living partners");
    return q;
}
class Lab : public Session {
public:
    Lab() {open();}
    template<class... A> void run(std::string_view symbol,Word blocks,Word threads,A&... a) {
        Function f{};bind(f,std::string(symbol),Image::development);
        void *args[]{static_cast<void *>(&a)...};
        d.check(d.launch(f,blocks,1,1,threads,1,1,0,nullptr,args,nullptr),std::string(symbol).c_str());
    }
    template<class... A> void breed(const char *symbol,Word n,A&... a) {
        Function f{};bind(f,symbol,Image::recollection);void *args[]{static_cast<void *>(&a)...};
        d.check(d.launch(f,(n+63)/64,1,1,64,1,1,0,nullptr,args,nullptr),symbol);
    }
    template<class T> void upload(DevicePtr dst,const T &x) {d.check(d.upload(dst,&x,sizeof(x)),"place physical data on device");}
    template<class T> void read(T &x,DevicePtr src) {d.check(d.download(&x,src,sizeof(x)),"read developmental outcome");}
    template<class T> std::vector<T> download(DevicePtr src,Word n) {
        std::vector<T> data(n);d.check(d.download(data.data(),src,data.size()*sizeof(T)),"read independent animals");return data;
    }
};
struct Cohort {
    Lab &s;Word n,blocks;
    Temporary m,b,w,cost,live;
    Cohort(Lab &s,Word n,bool probe):s(s),n(n),blocks((n+31)/32),m(s.d,Count(n)*sizeof(SequenceMind)),
        b(s.d,probe ? Count(n)*sizeof(Inquiry) : 0),w(s.d,Count(n)*sizeof(Chamber)),cost(s.d,Count(n)*sizeof(DevelopmentCost)),live(s.d,probe ? sizeof(Word) : 0) {}
    void worlds(Word family,Word seed,Word states,Word motors,Word sounds,Word unique) {
        s.run("sequence_world",blocks,32,w.ptr,n,family,seed,states,motors,sounds,unique);
    }
    void learn(DevicePtr genes,Word siblings,Word seed,DevelopmentLimits limits,Word noise,Word walk,DevicePtr peer=0) {
        s.run("sequence_born",blocks,32,m.ptr,b.ptr,w.ptr,genes,siblings,n,seed,limits);
        if (b.ptr) {
            Word batch=64;
            for (Word first=0;first<limits.queries;first+=batch) {
                bool survey=(first+batch)%512==0 && first+batch<limits.queries;
                DevicePtr ongoing=survey ? live.ptr : 0;
                if (survey) s.d.check(s.d.zero(ongoing,0,sizeof(Word)),"survey active learners");
                s.run("sequence_probe",blocks,32,m.ptr,b.ptr,w.ptr,cost.ptr,n,batch,noise,peer,ongoing);
                if (survey) {Word remaining=0;s.read(remaining,ongoing);if (!remaining) break;}
            }
        } else {
            s.run("sequence_roam",blocks,32,m.ptr,w.ptr,cost.ptr,n,walk,seed,noise);
            s.run("sequence_sleep",n,32,m.ptr,n,limits.cells);
        }
    }
};
RecollectionGenes select(Lab &lab,const Options &q,RecollectionGenes founder) {
    if (!q.generations && !q.origin) return founder;
    Word n=q.candidates,seed=q.seed,origin=q.origin,drift=q.drift,keep=1;
    Temporary parents(lab.d,n*sizeof(founder)),children(lab.d,n*sizeof(founder)),merit(lab.d,n*sizeof(Word));
    lab.breed("recollect_seed",n,parents.ptr,n,founder,seed,origin);
    constexpr Word siblings=2;
    if (q.generations) {
        Cohort litter(lab,n*siblings,true);
        for (Word g=0;g<q.generations;++g) {
            seed=mix(q.seed^mix(g+0x91ba22u));Word states=2+(g%3);
            litter.worlds(g%2 ? chambers::latches : chambers::network,seed,states,3,3,siblings);
            litter.learn(parents.ptr,siblings,seed,{8,96,8,6},g%3,0);
            Word k=siblings;lab.run("sequence_merit",(n+31)/32,32,litter.m.ptr,litter.w.ptr,litter.cost.ptr,merit.ptr,n,k,seed);
            if (auto *log=journal::current()) {
                auto f=lab.download<Word>(merit.ptr,n);Count sum=0;for (Word x:f) sum+=x;
                log->breeding.push_back({g,n,{sum,*std::max_element(f.begin(),f.end())}});
            }
            lab.breed("recollect_brood",n,parents.ptr,children.ptr,merit.ptr,n,seed,drift,keep);
            std::swap(parents.ptr,children.ptr);
        }
    }
    lab.read(founder,parents.ptr);return founder;
}
void draw_contacts(const SequenceMind &s) {
    if (auto *j=journal::current()) for (Word cell=0;cell<s.map.size;++cell) {
        std::ostringstream line;line<<"latent_cell="<<cell<<"  contacts=";
        for (Word a=0;a<s.motors;++a) {
            Word e=s.map.edge[cell][a];if (a) line<<',';
            if (e) line<<cell_of(e)<<'.'<<sense_of(e);else line<<'-';
        }
        j->line(line.str());
    }
}
void throughput(Lab &,const std::vector<SequenceMind> &,Word,Word,Word);
void report(Lab &lab,AgentImage &brain,const Options &q,Cohort &c,float ms) {
    auto bodies=lab.download<SequenceMind>(c.m.ptr,q.n);auto rooms=lab.download<Chamber>(c.w.ptr,q.n);
    auto costs=lab.download<DevelopmentCost>(c.cost.ptr,q.n);
    Count exact=0,minimal=0,pairs=0,contacts=0,correct=0,flips=0,trips=0,moves=0,splits=0,cells=0;
    Count pinned=0,pinned_hits=0,pinned_checks=0;std::vector<Word> witness;
    for (Word i=0;i<q.n;++i) {
        auto grade=sequence_audit::check(rooms[i],bodies[i]);exact+=grade.exact();minimal+=grade.exact() && grade.minimal==bodies[i].map.size;
        auto flat=bodies[i];flat.map.size=1;
        for (Word a=0;a<flat.motors;++a) if (auto edge=flat.map.edge[0][a]) flat.map.edge[0][a]=contact(0,sense_of(edge));
        auto control=sequence_audit::check(rooms[i],flat);pinned+=control.exact();pinned_hits+=control.correct;pinned_checks+=control.tests;
        pairs+=grade.states;contacts+=grade.tests;correct+=grade.correct;flips+=grade.bits;
        if (witness.empty() && !grade.witness.empty()) witness=grade.witness;
        trips+=costs[i].trips;moves+=costs[i].moves;splits+=costs[i].splits;cells+=bodies[i].map.size;
    }
    brain.sequence=bodies[0];brain.sequential=true;
    auto &s=brain.sequence;
    std::cout<<"sequence_family="<<q.family_name<<"  method="<<q.method<<"  worlds="<<q.n<<"  exact="<<exact
             <<"  minimal="<<minimal<<"  pairs="<<pairs<<"  transitions="<<contacts<<"  correct="<<correct<<"  bit_errors="<<flips
             <<"  pinned_exact="<<pinned<<"  pinned_correct="<<pinned_hits<<"  pinned_transitions="<<pinned_checks
             <<"  trips="<<trips<<"  moves="<<moves<<"  splits="<<splits<<"  cells="<<cells<<"  first_cells="<<s.map.size<<"  first_senses="<<s.senses<<"  learning_ms="<<ms<<"  Mmoves_per_s="<<(ms>0 ? moves/(1000.0*ms) : 0.0)<<'\n';
    if (!witness.empty()) {std::cout<<"first_counterexample=";for (Word a:witness) std::cout<<std::hex<<a;std::cout<<std::dec<<'\n';}
    if (auto *j=journal::current()) {
        j->device=lab.name;j->scores.push_back({"Every possible motor history",exact,q.n});
        j->scores.push_back({"Recurrent state pinned to home",pinned,q.n});j->scores.push_back({"Exact minimum circuit",minimal,q.n});j->scores.push_back({"Whole sensory waveforms",correct,contacts});
        j->metrics["animals"]=q.n;j->metrics["sequence_cells"]=cells;j->metrics["sequence_moves"]=moves;j->metrics["sequence_trips"]=trips;
        j->metrics["sequence_splits"]=splits;j->metrics["sequence_errors"]=flips;j->metrics["sequence_exact"]=exact;
        draw_contacts(s);
    }
    if (q.replicas) throughput(lab,bodies,q.replicas,q.ticks,q.seed);
}
void chorus_life(Lab &lab,AgentImage &brain,const Options &q,RecollectionGenes genes) {
    ChorusEggs nursery;
    auto &h=nursery.header;h.count=q.colony;h.seed=q.seed;h.days=q.days;h.play=q.walk;
    h.cells=q.limits.cells;h.arm=q.arm;h.drift=q.drift;
    if (!q.resume.empty()) nursery.read(q.resume);
    if (q.seasons>~0u-h.next) throw std::invalid_argument("season counter would overflow");
    Word n=h.count,blocks=n/32,seed=h.seed,days=h.days,steps=h.play,arm=h.arm,drift=h.drift,limit=h.cells,start=h.next;
    Temporary elders(lab.d,n*sizeof(chorus::Heredity)),young(lab.d,n*sizeof(chorus::Heredity)),bodies(lab.d,n*sizeof(SequenceMind)),residents(lab.d,n*sizeof(chorus::Season));
    Temporary phenotypes(lab.d,n*sizeof(chorus::Repertoire));
    if (q.resume.empty()) lab.run("chorus_founders",blocks,32,elders.ptr,n,genes,seed);
    else lab.d.check(lab.d.upload(elders.ptr,nursery.eggs.data(),n*sizeof(chorus::Heredity)),"restore eggs from the preceding season");
    for (Word season=0;season<q.seasons;++season) {
        seed=mix(h.seed+(start+season)*0xa7b9d24fu);lab.run("chorus_cradle",blocks,32,elders.ptr,bodies.ptr,residents.ptr,n,seed,steps,days,arm);
        lab.run("sequence_sleep",n,32,bodies.ptr,n,limit);
        for (Word first=0;first<days;first+=32) {
            Word part=std::min(32u,days-first);
            lab.run("chorus_feed",blocks,32,elders.ptr,bodies.ptr,residents.ptr,seed,first,part,arm);
            if (first+part<days) lab.run("sequence_sleep",n,32,bodies.ptr,n,limit);
        }
        lab.run("chorus_repertoire",blocks,32,elders.ptr,phenotypes.ptr,n);
        auto heard=lab.download<chorus::Repertoire>(phenotypes.ptr,n);
        std::set<std::vector<unsigned char>> distinct;Word remembering=0,largest=0;
        auto g=lab.download<chorus::Heredity>(elders.ptr,n);auto l=lab.download<chorus::Season>(residents.ptr,n);
        Count meals=0,attempts=0,aims=0,accuracy=0,energy=0,alive=0,edits=0;std::set<Word> founders;std::set<std::array<Word,3>> voices;
        Word best=0;
        for (Word j=0;j<n;++j) if (g[j].alive) {
            ++alive;meals+=l[j].meals;attempts+=l[j].attempts;aims+=l[j].aims;accuracy+=l[j].accurate;energy+=l[j].energy;edits+=g[j].edits;
            distinct.emplace(heard[j].response,heard[j].response+4*heard[j].size);
            remembering+=heard[j].size>1;largest=std::max(largest,heard[j].size);
            founders.insert(g[j].founder);voices.insert({g[j].circuit,g[j].receptors,g[j].larynx});
            if (l[j].energy>l[best].energy) best=j;
        }
        std::cout<<"chorus_season="<<(start+season)<<"  residents="<<alive<<"  meals="<<meals<<"  moves="<<attempts<<"  aimed="<<aims
                 <<"  predicted="<<accuracy<<"  offspring_energy="<<energy<<"  voices="<<voices.size()<<"  distinct_calls="<<distinct.size()<<"  context_sensitive="<<remembering<<"  largest_voice="<<largest<<"  founders="<<founders.size()<<"  inherited_edits="<<edits<<'\n';
        if (auto *log=journal::current()) {
            log->metrics["animals"]=n;log->metrics["chorus_meals"]+=meals;log->metrics["chorus_moves"]+=attempts;log->metrics["chorus_aims"]+=aims;
            if (season+1==q.seasons) for (Word j=0;j<std::min(n,32u);++j) {
                std::ostringstream r;r<<"chorus_voice="<<j<<"  rule="<<g[j].circuit<<"  founder="<<g[j].founder<<"  meals="<<l[j].meals<<"  cells="<<(3+(g[j].larynx&3))<<"  aimed="<<l[j].aims;log->line(r.str());
            }
        }
        if (season+1==q.seasons) {lab.read(brain.sequence,bodies.ptr+Count(best)*sizeof(SequenceMind));brain.sequential=true;}
        if (season+1<q.seasons || !q.eggs.empty()) {lab.run("chorus_brood",blocks,32,elders.ptr,young.ptr,residents.ptr,seed,drift);std::swap(elders.ptr,young.ptr);}
    }
    if (!q.eggs.empty()) {
        h.next=start+q.seasons;nursery.eggs=lab.download<chorus::Heredity>(elders.ptr,n);write_checked(q.eggs,nursery);
    }
}
void throughput(Lab &lab,const std::vector<SequenceMind> &population,Word replicas,Word ticks,Word seed) {
    Word models=std::min(replicas,Word(population.size()));
    sequence::PackedGraph g;g.motors=population[0].motors;
    for (Word i=0;i<models;++i) {
        const auto &m=population[i];g.states=std::max(g.states,m.map.size);g.senses=std::max(g.senses,m.senses);
        if (m.motors!=g.motors) throw std::invalid_argument("a packed population needs a common physical motor count");
    }
    if (!g.states || !g.senses || !g.motors) throw std::invalid_argument("a throughput run needs a recruited circuit");
    g.cell_bits=std::bit_width(g.states-1);g.sense_bits=std::bit_width(g.senses-1);
    Word contacts=g.states*g.motors;
    for (Word i=0;i<models && !g.known_words;++i) for (Word q=0;q<g.states;++q) for (Word a=0;a<g.motors;++a)
        if (q>=population[i].map.size || !population[i].map.edge[q][a]) g.known_words=(contacts+31)/32;
    g.payload_words=(contacts*g.width()+31)/32;
    Temporary brains(lab.d,models*sizeof(SequenceMind)),out(lab.d,replicas*sizeof(Word));
    lab.d.check(lab.d.upload(brains.ptr,population.data(),models*sizeof(SequenceMind)),"supply independent acquired graphs");
    Word blocks=(replicas+127)/128;
    cuda::Event begin{},end{};lab.d.check(lab.d.event_new(&begin,0),"create throughput clock");lab.d.check(lab.d.event_new(&end,0),"create throughput clock");
    std::vector<Word> reference;
    for (Word mode=0;mode<3;++mode) {
        Word stored=mode==2 ? replicas : models;
        Temporary bank(lab.d,mode ? std::max<Count>(4,4ull*stored*g.words()) : 0);
        if (mode) {
            Word groups=(stored+127)/128;
            lab.run("sequence_pack",groups,128,brains.ptr,g,bank.ptr,stored,models);
            lab.run("sequence_check_pack",groups,128,brains.ptr,g,bank.ptr,stored,models,out.ptr);
            for (Word wrong:lab.download<Word>(out.ptr,stored)) if (wrong) throw std::logic_error("compiled contact differs from its acquired circuit");
        }
        auto advance=[&]() {
            if (mode) lab.run("sequence_run_packed",blocks,128,g,bank.ptr,out.ptr,replicas,ticks,seed,stored);
            else lab.run("sequence_run_shared",blocks,128,brains.ptr,out.ptr,replicas,ticks,seed,models);
        };
        advance();std::array<float,5> times{};
        for (auto &ms:times) {
            lab.d.check(lab.d.record(begin,nullptr),"start frozen clock");advance();lab.d.check(lab.d.record(end,nullptr),"stop frozen clock");
            lab.d.check(lab.d.wait(end),"finish frozen motor stream");lab.d.check(lab.d.elapsed(&ms,begin,end),"measure inference time");
        }
        auto result=lab.download<Word>(out.ptr,replicas);
        if (mode && result!=reference) throw std::logic_error("acquired streams changed during compilation");
        reference=std::move(result);std::sort(times.begin(),times.end());
        const char *layout=mode==2 ? "private" : mode==1 ? "pooled" : "shared";
        std::cout<<"sequence_batch="<<layout<<"  replicas="<<replicas<<"  ticks="<<ticks<<"  learned_models="<<models
                 <<"  stored_graphs="<<stored<<"  graph_bytes="<<(mode ? 4*g.words() : sizeof(SequenceGraph))<<"  waveform_bytes="<<8*(g.senses+g.motors)
                 <<"  contact_bits="<<g.width()<<"  median_ms="<<times[2]<<"  Gtransitions_per_s="<<double(replicas)*ticks/(1e6*times[2])<<'\n';
    }
    lab.d.event_free(begin);lab.d.event_free(end);
}
void external(Lab &lab,AgentImage &brain,const Options &q,RecollectionGenes genes) {
    std::vector<Count> muscles;std::istringstream input(q.gestures);std::string name;
    while (std::getline(input,name,',')) {
        Count sound=pulse(name);
        if (muscles.size()==sequence_motors || std::find(muscles.begin(),muscles.end(),sound)!=muscles.end())
            throw std::invalid_argument("duplicate gesture or more than sixteen muscles");
        muscles.push_back(sound);
    }
    Word motors=Word(muscles.size());if (!motors) throw std::invalid_argument("empty gesture inventory");
    Inquiry b;auto limits=q.limits;Word seed=q.seed;
    Temporary mind(lab.d,sizeof(SequenceMind)),inquiry(lab.d,sizeof(b)),request(lab.d,sizeof(Experiment)),hear(lab.d,sequence::query_length*sizeof(Count));
    lab.d.check(lab.d.upload(hear.ptr,muscles.data(),motors*sizeof(Count)),"supply physical gestures");
    lab.run("sequence_awaken",1,1,mind.ptr,inquiry.ptr,genes,hear.ptr,motors,seed,limits);
    for (;;) {
        lab.run("sequence_ask",1,1,mind.ptr,inquiry.ptr,request.ptr);Experiment word;lab.read(word,request.ptr);
        if (!word.length) break;
        std::cout<<"experiment>";for (Word j=0;j<word.length;++j) std::cout<<' '<<spell(muscles[word.actions[j]]);
        std::cout<<std::endl;
        std::string line;if (!std::getline(std::cin,line)) throw std::invalid_argument("the external apparatus disconnected before returning a sensory trace");
        std::istringstream row(line);std::vector<Count> spikes;while (row>>name) spikes.push_back(pulse(name));
        if (spikes.size()!=word.length) throw std::invalid_argument("the apparatus must return one sensory wave per gesture");
        lab.d.check(lab.d.upload(hear.ptr,spikes.data(),spikes.size()*sizeof(Count)),"admit external sensory spikes");
        lab.run("sequence_hear",1,1,mind.ptr,inquiry.ptr,request.ptr,hear.ptr);
    }
    lab.read(brain.sequence,mind.ptr);lab.read(b,inquiry.ptr);brain.sequential=true;
    std::cout<<"sequence_external=1  first_cells="<<brain.sequence.map.size<<"  first_senses="<<brain.sequence.senses
             <<"  trips="<<b.questions<<"  moves="<<b.steps<<"  splits="<<b.confirmed<<'\n';
    draw_contacts(brain.sequence);
    if (auto *j=journal::current()) {
        j->metrics["animals"]=1;j->metrics["sequence_cells"]=brain.sequence.map.size;j->metrics["sequence_moves"]=b.steps;
        j->metrics["sequence_trips"]=b.questions;j->metrics["sequence_splits"]=b.confirmed;
    }
}
}
namespace {
template<class... Args> void sensory_kernel(Session &s,const char *name,Word threads,Args&... args) {
    Function f{};s.bind(f,name,Image::development);void *arguments[]{static_cast<void *>(&args)...};
    s.d.check(s.d.launch(f,1,1,1,threads,1,1,0,nullptr,arguments,nullptr),name);
}
}
DevelopmentalSpeech::DevelopmentalSpeech(Session &s,AgentImage &a,DevicePtr sequence,DevicePtr composition,DevicePtr muscle):
    lab(s),animal(a),mind(sequence),port(s.d,sequence ? 128*sizeof(PulsePort) : 0),
    compositional(s,a.composition,a.composes,composition),procedures(s,a.practice,a.procedural,muscle) {}
void DevelopmentalSpeech::transact(std::vector<PulsePort> &events) {
    for (size_t offset=0;offset<events.size();offset+=128) {
        Word count=Word(std::min<size_t>(128,events.size()-offset));size_t bytes=count*sizeof(PulsePort);
        lab.d.check(lab.d.upload(port.ptr,events.data()+offset,bytes),"offer auditory and motor events");
        sensory_kernel(lab,"sequence_port",1,mind,port.ptr,count);
        lab.d.check(lab.d.download(events.data()+offset,port.ptr,bytes),"hear the emitted waveform");
    }
}
bool DevelopmentalSpeech::hear(const std::string &line,bool fallthrough,std::vector<Count> *emitted,bool inner) {
    if (!mind && !animal.composes && !animal.procedural) return false;
    std::istringstream words(line);std::string first;if (!(words>>first)) return false;
    std::vector<PulsePort> events;bool raw=first=="/raw",spoken=first[0]!='/',invalid=false,known=bool(mind);
    if (spoken || raw) {
        std::istringstream input(line);std::string token;if (raw) input>>token;
        while (input>>token) {
            while (!token.empty() && (token.back()=='.' || token.back()==',' || token.back()=='?' || token.back()=='!')) token.pop_back();
            if (token.empty()) continue;
            try {
                PulsePort p;p.gesture=pulse(token);events.push_back(p);
                auto end=animal.sequence.gestures+animal.sequence.motors;known&=std::find(animal.sequence.gestures,end,p.gesture)!=end;
            } catch (const std::invalid_argument &) {if (raw) throw;invalid=true;known=false;}
        }
    }
    if (animal.procedural) {
        std::string voice;
        if (procedures.hear(line,voice,emitted)) {
            std::cout<<(inner ? "inner> " : "animal> ")<<voice<<std::endl;++turns;answers+=voice!="?" && voice!="_";
            if (auto *j=journal::current()) {++j->metrics["procedure_turns"];j->line("procedural_reply="+voice+"  heard="+line);}
            return true;
        }
    }
    std::string constructed;
    if (animal.composes && compositional.hear(line,constructed)) {
        if (constructed.ends_with('?') && known && spoken) compositional.cancel_question();
        else {
            std::cout<<(inner ? "inner> " : "animal> ")<<constructed<<std::endl;
            if (emitted && !constructed.ends_with('?')) {
                const auto &voice=compositional.output();
                for (Word i=0;i<voice.size;++i) emitted->push_back(voice.words[i]);
            }
            ++turns;answers+=constructed!="?" && constructed!="_";
            if (auto *j=journal::current()) {
                j->line("composed_reply="+constructed+"  heard="+line);++j->metrics["composition_turns"];
                j->metrics["composition_questions"]+=constructed.size()>1 && constructed.ends_with('?');
            }
            return true;
        }
    }
    if (spoken && fallthrough && !known) return false;
    Word one=1,limit=sequence_cells;
    if (mind && first=="/sleep") {sensory_kernel(lab,"sequence_sleep",32,mind,one,limit);std::cout<<"rested"<<std::endl;return true;}
    if (mind && (first=="/home" || first=="/lost")) {
        PulsePort p;p.kind=first=="/home" ? return_home : lose_position;events.push_back(p);transact(events);new_home=first=="/home";
        std::cout<<"possible="<<std::popcount(events[0].active)<<std::endl;return true;
    }
    if (mind && (first=="/show" || first=="/seek")) {
        std::string a,o,mask,extra;PulsePort p;
        if (!(words>>a)) throw std::invalid_argument("missing physical signal");
        if (first=="/show") {
            if (!(words>>o)) throw std::invalid_argument("missing observed echo");
            p.kind=demonstrate;p.gesture=pulse(a);p.sensation=pulse(o);p.home=new_home;new_home=false;
            if (words>>mask) p.visible=pulse(mask);
        } else {p.kind=pursue;p.sensation=pulse(a);}
        if (words>>extra) throw std::invalid_argument("too many signals");
        events.push_back(p);transact(events);
        if (p.kind==demonstrate) {
            if (events[0].options) sensory_kernel(lab,"sequence_sleep",32,mind,one,limit);
            lab.d.check(lab.d.download(&animal.sequence,mind,sizeof(SequenceMind)),"update the acquired gesture inventory");
            std::cout<<(events[0].answered ? "observed" : "?")<<std::endl;
        } else std::cout<<"motor> "<<(events[0].answered ? spell(events[0].heard) : "?")<<std::endl;
        return true;
    }
    if (!(spoken || raw)) return false;
    if (invalid || !mind) {std::cout<<"animal> ?"<<std::endl;++turns;return true;}
    if (events.empty()) return true;
    transact(events);new_home=false;++turns;
    bool quiet=false,has_listen=false;
    for (Word a=0;a<animal.sequence.motors;++a) has_listen|=animal.sequence.gestures[a]==0;
    std::string reply;
    auto collect=[&](const PulsePort &e) {
        quiet=e.answered && !e.heard;
        if (raw || !quiet) {if (!reply.empty()) reply+=' ';reply+=e.answered ? spell(e.heard) : "?";}
        if (emitted && e.answered && e.heard) emitted->push_back(e.heard);
    };
    for (const auto &e:events) collect(e);
    if (spoken && !quiet && has_listen && events.back().answered) for (int i=0;i<16;++i) {
        std::vector<PulsePort> wait(1);transact(wait);collect(wait[0]);if (quiet || !wait[0].answered) break;
    }
    if (reply.empty()) reply="_";
    if (emitted && reply.find('?')!=std::string::npos) emitted->clear();
    answers+=reply.find('?')==std::string::npos && reply!="_";
    std::cout<<(inner ? "inner> " : raw ? "echo> " : "animal> ")<<reply<<std::endl;
    if (auto *log=journal::current()) log->line("auditory_reply="+reply+"  heard="+line);
    return true;
}
void DevelopmentalSpeech::report() {
    if (auto *j=journal::current();j && turns) {
        j->metrics["sequence_turns"]+=turns;j->metrics["sequence_replies"]+=answers;j->metrics["sequence_cells"]=animal.sequence.map.size;
        j->scores.push_back({"Turns with an acquired response",answers,turns});
    }
}
int developmental_interview(AgentImage &animal,const std::string &source,const std::string &save) {
    Lab lab;Temporary sequence(lab.d,animal.sequential ? sizeof(SequenceMind) : 0),composition(lab.d,animal.composes ? sizeof(composition::Mind) : 0);
    if (sequence.ptr) lab.upload(sequence.ptr,animal.sequence);
    if (composition.ptr) lab.upload(composition.ptr,animal.composition);
    Temporary practice(lab.d,animal.procedural ? sizeof(procedure::Mind) : 0);if (practice.ptr) lab.upload(practice.ptr,animal.practice);
    DevelopmentalSpeech speaker(lab,animal,sequence.ptr,composition.ptr,practice.ptr);
    if (auto *j=journal::current()) {j->device=lab.name;j->source=source;j->metrics["animals"]=1;j->metrics["sequence_body"]=animal.brain.genes.body;j->metrics["sequence_tape"]=1+(animal.brain.genes.expression&7);}
    std::cout<<"Raw auditory circuit on "<<lab.name<<".  /home  /lost  /raw GESTURES  /show GESTURE ECHO  /seek ECHO  /sleep  /quit"<<std::endl;
    for (std::string line;std::getline(std::cin,line);) {
        if (line=="/quit") break;
        if (!speaker.hear(line) && !line.empty()) std::cout<<"?"<<std::endl;
    }
    speaker.report();
    if (sequence.ptr) lab.read(animal.sequence,sequence.ptr);
    if (composition.ptr) lab.read(animal.composition,composition.ptr);
    if (practice.ptr) lab.read(animal.practice,practice.ptr);
    if (!save.empty()) write_checked(save,animal);
    return 0;
}
int develop_command(int argc,char **argv) {
    if (argc==3 && std::string_view(argv[2])=="--help") {
        std::cout<<"axion develop [--family network|ring|latches|rhythm|hideout] [--method probe|replay] [--worlds N] [--states N] [--motors N] [--sounds N]\n"
                 <<"  --budget N --span N --patience N --capacity N --noise N --steps N --seed N\n"
                 <<"  --origin random|inherited|inert --generations N --candidates N --breeding select|drift --load MIND --save MIND\n"
                 <<"  --colony N --seasons N --days N --forage intact|deaf|wander   Rear and breed living callers on 32-resident islands.\n"
                 <<"  --save-colony FILE --resume-colony FILE   Save or restore a complete brood between seasons.\n"
                 <<"  --replicas N --ticks N   Time pooled and private learned graphs across independent motor histories.\n"
                 <<"  --peer MIND   Rear through the acoustic emissions of an acquired circuit.\n"
                 <<"  --external --gestures WORD,WORD,...   Read one sensory pulse per motor, starting at home each experiment.\n";return 0;
    }
    auto q=options(argc,argv);AgentImage brain;
    if (q.resume.empty() || !q.save.empty()) brain.read(q.source);
    Lab lab;auto founder=brain.sequential ? brain.sequence.genes : brain.recollection.genes;
    auto genes=q.resume.empty() ? select(lab,q,founder) : founder;
    if (auto *j=journal::current()) {j->device=lab.name;j->source=q.resume.empty() ? q.source : q.resume;j->metrics["sequence_body"]=brain.brain.genes.body;j->metrics["sequence_tape"]=1+(brain.brain.genes.expression&7);}
    std::cout<<lab.name<<"  developmental_bytes="<<sizeof(SequenceMind);
    if (q.resume.empty()) std::cout<<"  learning_rule="<<std::hex<<genes.low<<'.'<<genes.high<<'.'<<genes.expression<<std::dec;
    else std::cout<<"  resumed_brood="<<q.resume;
    std::cout<<'\n';
    if (q.colony || !q.resume.empty()) chorus_life(lab,brain,q,genes);
    else if (q.method=="external") external(lab,brain,q,genes);
    else {
        Temporary inherited(lab.d,sizeof(genes));lab.upload(inherited.ptr,genes);
        Cohort c(lab,q.n,q.method=="probe");
        Temporary teacher(lab.d,q.peer.empty() ? 0 : sizeof(SequenceMind));
        if (!q.peer.empty()) {
            if (q.method!="probe") throw std::invalid_argument("acoustic tutoring requires active rearing");
            AgentImage adult;adult.read(q.peer);const auto &p=adult.sequence;
            if (!adult.sequential || !p.anchored || !p.motors || !p.map.size) throw std::invalid_argument("peer needs an anchored, acquired sensory circuit");
            for (Word k=0;k<p.map.size;++k) for (Word a=0;a<p.motors;++a) if (!p.map.edge[k][a])
                throw std::invalid_argument("peer has an unobserved response");
            lab.upload(teacher.ptr,p);lab.run("sequence_peer_world",c.blocks,32,c.w.ptr,q.n,teacher.ptr);q.family_name="peer";
        } else if (q.family==chambers::hideout) {
            Chamber channels;channels.motors=12;channels.senses=5;
            const char *actions[]{"_","left","right","outside","inside","where","it","look","fox","is","will","wait"};
            const char *sounds[]{"_","hidden","expects","left","right"};
            for (Word a=0;a<channels.motors;++a) channels.gestures[a]=pulse(actions[a]);
            for (Word a=0;a<channels.senses;++a) channels.sounds[a]=pulse(sounds[a]);
            Temporary ports(lab.d,sizeof(channels));lab.upload(ports.ptr,channels);lab.run("sequence_den",c.blocks,32,c.w.ptr,q.n,ports.ptr,q.seed);
        } else c.worlds(q.family,q.seed^0x2d38a5b1u,q.states,q.motors,q.sounds,q.n);
        cuda::Event begin{},end{};float ms=0;
        lab.d.check(lab.d.event_new(&begin,0),"create rearing clock");lab.d.check(lab.d.event_new(&end,0),"create rearing clock");
        lab.d.check(lab.d.record(begin,nullptr),"start rearing clock");
        c.learn(inherited.ptr,q.n,q.seed,q.limits,q.noise,q.walk,teacher.ptr);
        lab.d.check(lab.d.record(end,nullptr),"stop rearing clock");lab.d.check(lab.d.wait(end),"finish rearing");
        lab.d.check(lab.d.elapsed(&ms,begin,end),"measure acquisition");lab.d.event_free(begin);lab.d.event_free(end);
        report(lab,brain,q,c,ms);
    }
    if (q.replicas && (q.colony || !q.resume.empty() || q.method=="external")) throughput(lab,{brain.sequence},q.replicas,q.ticks,q.seed);
    if (!q.save.empty()) write_checked(q.save,brain);
    return 0;
}
}
