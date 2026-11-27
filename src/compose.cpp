#include "composition_host.hpp"
#include "composition_island.cuh"
#include "composition_compact.cuh"
#include "composition_census.cuh"
#include "composition_nest.hpp"
#include <set>
#include "individual_io.hpp"
#include "pulse_words.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace axion::life::composition {
namespace {
using namespace runtime;
using namespace storage;
template<class... T> void launch(Session &s,const char *symbol,Word blocks,Word threads,T&... arguments) {
    Function f{};s.bind(f,symbol,Image::composition);void *args[]{static_cast<void *>(&arguments)...};
    s.d.check(s.d.launch(f,blocks,1,1,threads,1,1,0,nullptr,args,nullptr),symbol);
}
template<class T> void upload(Session &s,DevicePtr ptr,const T &x) {s.d.check(s.d.upload(ptr,&x,sizeof(x)),"upload sensory data");}
template<class T> void download(Session &s,T &x,DevicePtr ptr) {s.d.check(s.d.download(&x,ptr,sizeof(x)),"read compositional outcome");}
runtime::DevicePtr offset(DevicePtr data,Word i) {return data+Count(i)*sizeof(Mind);}
Mouth sounds(std::string_view text) {
    Mouth x;std::istringstream in{std::string(text)};std::string token;
    while (in>>token) {
        while (!token.empty() && (token.back()=='.' || token.back()==',' || token.back()=='?' || token.back()=='!')) token.pop_back();
        if (token.empty() || token=="_") continue;
        if (x.size==width) throw std::invalid_argument("an acoustic phrase holds up to eight waves");
        x.words[x.size++]=acoustic::pulse(token);
    }
    return x;
}
std::string print(const Mouth &x) {
    std::string out;
    for (Word k=0;k<x.size;++k) {if (k) out+=' ';out+=acoustic::spell(x.words[k]);}
    return out;
}
shelter::Dialect english() {
    using namespace shelter;Dialect d;
    const char *words[]{"put","inside","chooses","is","offers","to","where","what","holds","should","search","will","look","for","did","choose","offer","and","its","container","rests","at","try","expects","because","rides","awake","asleep"};
    for (Word j=0;j<std::size(words);++j) d.words[j]=acoustic::pulse(words[j]);
    for (Word i=0;i<96;++i) {
        std::string name="na";name+=char('a'+i/26);name+=char('a'+i%26);d.names[i]=acoustic::pulse(name);
    }
    auto set=[](Cadence &c,std::initializer_list<unsigned> wave) {
        c.length=Word(wave.size());Word i=0;for (auto w:wave) c.pin[i++]=static_cast<Byte>(w);
    };
    set(d.input[put],{put_word,X,inside,Y});set(d.input[choose],{X,chooses,Y});
    set(d.input[veil],{X,is,Y});set(d.input[give],{X,offers,Z,to,Y});
    set(d.input[where],{where_word,is,X});set(d.output[where],{X,rests,at,Y});
    set(d.input[holds],{what,holds_word,X});set(d.output[holds],{Y});
    set(d.input[search],{where_word,should,X,search_word});set(d.output[search],{try_word,Y});
    set(d.input[believe],{where_word,will,X,look,for_word,Y});set(d.output[believe],{X,expects,Y});
    set(d.input[chosen],{what,did,X,choose_word});set(d.output[chosen],{Y});
    set(d.input[offered],{what,did,X,offer,Y});set(d.output[offered],{Y});
    set(d.input[follow],{and_word,its,container});set(d.output[follow],{because,X,rides,Y});
    return d;
}
void fit(Session &s,DevicePtr m,DevicePtr scratch,Word n) {
    launch(s,"compose_patterns",n,128,m,scratch);
    launch(s,"compose_induce",n*queries*width,128,m,scratch);
    launch(s,"compose_settle",(n+31)/32,32,m,n);
}
void consolidate(Session &s,DevicePtr brain) {
    Temporary apprentice(s.d,sizeof(Mind)),tissue(s.d,sizeof(Detections)),status(s.d,sizeof(Word));
    launch(s,"compose_apprentice",1,1,brain,apprentice.ptr);fit(s,apprentice.ptr,tissue.ptr,1);
    launch(s,"compose_absorb",1,1,brain,apprentice.ptr,status.ptr);
    Word error=0;download(s,error,status.ptr);if (error) throw std::runtime_error("insufficient free construction or projection cells for this lesson");
}
Word integer(std::string_view v,Word low,Word high,int radix=10) {
    Word n=0;auto [end,err]=std::from_chars(v.data(),v.data()+v.size(),n,radix);
    if (err!=std::errc{} || end!=v.data()+v.size() || n<low || n>high) throw std::invalid_argument("invalid composition quantity "+std::string(v));
    return n;
}
struct Options {
    std::string source="examples/learner.mind",save,lesson,mentor,catalog,stream,nest,resume;
    Word worlds=16,seed=2717,style=0,rounds=10,trials=24,depth=12,controls=1;
    Word origin=2,generations=12,population=48,drift=0,noise=0,residents=0,seasons=4,days=32,wire=0,replicas=0,ticks=128,inquiries=0,wander=0;
    bool inspect=false,fresh=false,census=false,specified_origin=false,specified_generations=false;
};
Options parse(int argc,char **argv) {
    Options o;
    for (int i=2;i<argc;++i) {
        std::string f=argv[i];if (f=="--census") {o.census=true;continue;}if (f=="--circuit") {o.inspect=true;continue;}if (f=="--fresh") {o.fresh=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+f);
        std::string v=argv[i];
        if (f=="--load") o.source=v;else if (f=="--save") o.save=v;else if (f=="--lesson") o.lesson=v;
        else if (f=="--catalog") o.catalog=v;
        else if (f=="--stream") o.stream=v;
        else if (f=="--inquiries") o.inquiries=integer(v,0,4096);
        else if (f=="--attention" && (v=="disagreement" || v=="random" || v=="yoked")) o.wander=v=="yoked" ? 2 : v=="random";
        else if (f=="--replicas") o.replicas=integer(v,1,1048576);
        else if (f=="--ticks") o.ticks=integer(v,1,32768);
        else if (f=="--mentor") o.mentor=v;
        else if (f=="--save-nest") o.nest=v;
        else if (f=="--resume-nest") o.resume=v;
        else if (f=="--society") o.residents=integer(v,32,8192);
        else if (f=="--seasons") o.seasons=integer(v,1,512);
        else if (f=="--days") o.days=integer(v,1,2048);
        else if (f=="--wire" && (v=="open" || v=="muted" || v=="shifted" || v=="solitary" || v=="no-repair"))
            o.wire=v=="muted" ? 1 : v=="shifted" ? 2 : v=="solitary" ? 3 : v=="no-repair" ? 4 : 0;
        else if (f=="--worlds") o.worlds=integer(v,1,4096);else if (f=="--seed") o.seed=integer(v,0,~0u);
        else if (f=="--generations") {o.generations=integer(v,0,512);o.specified_generations=true;}
        else if (f=="--population") o.population=integer(v,1,1024);
        else if (f=="--noise") o.noise=integer(v,0,16);
        else if (f=="--selection" && (v=="select" || v=="drift")) o.drift=v=="drift";
        else if (f=="--rounds") o.rounds=integer(v,2,12);else if (f=="--trials") o.trials=integer(v,1,2048);
        else if (f=="--depth") o.depth=integer(v,1,24);else if (f=="--controls") o.controls=integer(v,0,1);
        else if (f=="--dialect" && (v=="english" || v=="novel" || v=="permuted")) o.style=v=="english" ? 0 : v=="novel" ? 1 : 2;
        else if (f=="--origin" && (v=="inherited" || v=="inert" || v=="permissive" || v=="random")) {
            o.specified_origin=true;o.origin=v=="inherited" ? 0 : v=="inert" ? 1 : v=="random" ? 2 : 3;
        } else throw std::invalid_argument("unknown composition option "+f);
    }
    if (o.inquiries && (!o.lesson.empty() || o.residents || !o.resume.empty() || !o.catalog.empty())) throw std::invalid_argument("active tuition needs a rearing world or a live mentor");
    if (o.catalog.empty()!=o.stream.empty() || (o.catalog=="-" && o.stream=="-")) throw std::invalid_argument("a compiled session needs a catalog and a distinct input stream");
    if (!o.nest.empty() && o.resume.empty() && !o.residents) throw std::invalid_argument("saving a brood requires an island population");
    if (!o.catalog.empty() && (!o.lesson.empty() || !o.mentor.empty() || o.residents || !o.resume.empty() || o.census || o.replicas))
        throw std::invalid_argument("a compiled stream runs one learned animal from its checkpoint");
    if ((o.residents || !o.resume.empty()) && (o.census || o.replicas || !o.mentor.empty() || !o.lesson.empty()))
        throw std::invalid_argument("choose a society or an independent rearing cohort");
    if (o.residents%32) throw std::invalid_argument("a social island houses 32 residents");
    if ((!o.mentor.empty() && !o.lesson.empty()) || (o.residents && (!o.mentor.empty() || !o.lesson.empty())))
        throw std::invalid_argument("choose one source of rearing experience");
    return o;
}
template<class... T> void breed(Session &s,const char *name,Word count,T&... arguments) {
    Function f{};s.bind(f,name,Image::recollection);void *args[]{static_cast<void *>(&arguments)...};
    s.d.check(s.d.launch(f,(count+63)/64,1,1,64,1,1,0,nullptr,args,nullptr),name);
}
RecollectionGenes evolve(Session &s,const Options &o,RecollectionGenes parent,DevicePtr language) {
    Word n=o.population,origin=o.origin,seed=o.seed,elites=1,drift=o.drift,siblings=1;
    if (origin==3) {parent={0xffffu,~0u,0};origin=0;}
    if (!o.generations && !origin) return parent;
    Temporary parents(s.d,n*sizeof(parent)),eggs(s.d,n*sizeof(parent)),merit(s.d,n*sizeof(Word));
    breed(s,"recollect_seed",n,parents.ptr,n,parent,seed,origin);
    if (o.generations) {
        Temporary pupils(s.d,n*sizeof(Mind)),scores(s.d,n*sizeof(Measures)),d(s.d,std::min(n,64u)*sizeof(Detections));
        Word style=2,rounds=7,turns=96,depth=7,cut=0;
        for (Word g=0;g<o.generations;++g) {
            seed=mix(o.seed^mix(0x45181abu+g));Word noise=g%3;
            launch(s,"compose_rear",(n+31)/32,32,pupils.ptr,language,n,parents.ptr,siblings,seed,style,rounds);
            for (Word i=0;i<n;i+=64) {Word k=std::min(64u,n-i);fit(s,offset(pupils.ptr,i),d.ptr,k);}
            launch(s,"compose_trials",(n+31)/32,32,pupils.ptr,language,scores.ptr,n,seed,style,turns,depth,cut,siblings,noise);
            launch(s,"compose_merit",(n+31)/32,32,scores.ptr,merit.ptr,n,siblings);
            if (auto *j=journal::current()) {
                std::vector<Word> values(n);s.d.check(s.d.download(values.data(),merit.ptr,n*sizeof(Word)),"record reproductive outcomes");
                Count sum=0;for (Word v:values) sum+=v;
                j->breeding.push_back({g,n,{sum,*std::max_element(values.begin(),values.end())}});
            }
            breed(s,"recollect_brood",n,parents.ptr,eggs.ptr,merit.ptr,n,seed,drift,elites);
            std::swap(parents.ptr,eggs.ptr);
        }
    }
    download(s,parent,parents.ptr);return parent;
}
void describe(const Mind &m) {
    const auto &a=m.anatomy;Word ready=0;for (Word i=0;i<a.phrases_n;++i) ready+=a.phrases[i].ready;
    std::cout<<"learned_facts="<<a.facts_n<<"  projections="<<a.fields_n<<"  used_projections="<<__builtin_popcountll(a.used)
             <<"  constructions="<<a.phrases_n<<"  settled="<<ready<<"  vocabulary="<<m.lexicon.n<<"  observations="<<m.lessons.n
             <<"  active_versions="<<m.memory.n<<"  saturated="<<m.memory.full<<"  organ_bytes="<<sizeof(m)<<'\n';
}
std::string frame(const Mind &m,const Pattern &p) {
    std::string line;
    for (Word k=0;k<p.prototype.size;++k) {if (k) line+=' ';line+=(p.fixed&(1u<<k)) ? acoustic::spell(m.lexicon.pulse[p.prototype.words[k]]) : "*";}
    return line;
}
void illustrate(const Mind &m) {
    auto *log=journal::current();if (!log) return;
    const auto &a=m.anatomy;log->metrics["formed_fields"]=__builtin_popcountll(a.used);log->metrics["formed_speech"]=a.phrases_n;
    log->metrics["forms"]=m.lexicon.n;log->metrics["lessons"]=m.lessons.seen;log->metrics["private_contacts"]=m.memory.n;
    for (Word t=0;t<a.facts_n;++t) log->line("role_frame="+std::to_string(t)+"  form="+frame(m,a.facts[t]));
    for (Word t=0;t<a.phrases_n;++t) {
        const auto &q=a.phrases[t];Word inputs=0,shutters=0,cycles=0;
        for (Word j=0;j<q.length;++j) {
            auto &f=q.voice[j];if (f.first) inputs|=1u<<a.fields[f.first-1].type;
            if (f.second) inputs|=1u<<a.fields[f.second-1].type;
            if (f.gate) shutters|=1u<<a.fields[f.gate-1].type;
            cycles+=f.repeat;
        }
        log->line("vocal_frame="+std::to_string(t)+"  inputs="+std::to_string(inputs)+"  shutters="+std::to_string(shutters)+
                  "  recursion="+std::to_string(cycles)+"  ready="+std::to_string(q.ready)+"  form="+frame(m,q.cue));
    }
}
void inspect(const Mind &m) {
    const auto &a=m.anatomy;
    for (Word k=0;k<a.facts_n;++k) std::cout<<"acquired_frame="<<k<<"  stimulus="<<frame(m,a.facts[k])<<'\n';
    for (Word k=0;k<a.phrases_n;++k) {
        const auto &q=a.phrases[k];std::cout<<"acquired_question="<<k<<"  settled="<<Word(q.ready)<<"  supported="<<Word(q.support)<<"  input="<<frame(m,q.cue)<<"  output=";
        for (Word j=0;j<q.length;++j) {
            auto f=q.voice[j];if (j) std::cout<<',';
            if (f.literal) std::cout<<acoustic::spell(m.lexicon.pulse[f.literal]);
            else if (f.source==absent) std::cout<<'?';
            else std::cout<<Word(f.source)<<'.'<<Word(f.first)<<'.'<<Word(f.second)<<'.'<<Word(f.repeat)<<'.'<<Word(f.gate)<<'.'<<Word(f.other);
        }
        std::cout<<'\n';
    }
}
#include "composition_scale.hpp"
void society(Session &s,const Options &o,AgentImage &image,RecollectionGenes founder,DevicePtr language) {
    ShelterNest nest;auto &h=nest.header;h.count=o.residents;h.seed=o.seed;h.dialect=o.style;h.rounds=o.rounds;h.days=o.days;h.wire=o.wire;h.drift=o.drift;
    if (!o.resume.empty()) nest.read(o.resume);
    if (o.seasons>~0u-h.season) throw std::invalid_argument("season counter overflow");
    Word n=h.count,islands=n/32,seed=h.seed,style=h.dialect,rounds=h.rounds,days=h.days,arm=h.wire,drift=h.drift;
    Temporary adults(s.d,Count(n)*sizeof(market::Genome)),eggs(s.d,Count(n)*sizeof(market::Genome)),minds(s.d,Count(n)*sizeof(Mind)),
        lives(s.d,Count(n/2)*sizeof(market::Life)),d(s.d,std::min(64u,n)*sizeof(Detections));
    if (o.resume.empty()) launch(s,"market_seed",islands,32,adults.ptr,n,founder,seed);
    else s.d.check(s.d.upload(adults.ptr,nest.eggs.data(),n*sizeof(market::Genome)),"hatch the preceding season’s brood");
    for (Word epoch=0;epoch<o.seasons;++epoch) {
        seed=mix(h.seed+(h.season+epoch)*0x6a09e667u);
        launch(s,"market_rear",islands,32,minds.ptr,adults.ptr,language,n,seed,style,rounds);
        for (Word i=0;i<n;i+=64) {Word count=std::min(64u,n-i);fit(s,offset(minds.ptr,i),d.ptr,count);}
        launch(s,"market_forage",islands,16,minds.ptr,adults.ptr,language,lives.ptr,n,seed,style,days,arm);
        std::vector<market::Life> results(n/2);std::vector<market::Genome> genotypes(n);
        s.d.check(s.d.download(results.data(),lives.ptr,results.size()*sizeof(results[0])),"read foraging consequences");
        s.d.check(s.d.download(genotypes.data(),adults.ptr,genotypes.size()*sizeof(genotypes[0])),"trace island pedigrees");
        Count total=0,won=0,fed=0,private_days=0,inferred=0,asked=0,heard=0,fixes=0,checks=0,energy=0,alive=0;Word best=0,high=0;
        std::set<Word> lineages;std::set<std::array<Word,3>> genomes;
        for (Word i=0;i<n;++i) {
            auto &g=genotypes[i];if (!g.alive) continue;
            auto &l=i&1 ? results[i/2].second : results[i/2].first;
            ++alive;total+=l.queries;won+=l.correct;fed+=l.meals;private_days+=l.private_days;inferred+=l.private_correct;
            asked+=l.asked;heard+=l.heard;fixes+=l.repaired;checks+=l.checks;energy+=l.energy;
            lineages.insert(g.founder);genomes.insert({g.learner.low,g.learner.high,g.learner.expression});
            if (l.energy>high) {best=i;high=l.energy;}
            if (auto *j=journal::current();j && i<32 && epoch+1==o.seasons) j->line("shelter_resident="+std::to_string(i)+"  founder="+std::to_string(g.founder)+
                "  body="+std::to_string(g.learner.high)+"  asked="+std::to_string(l.asked)+"  fed="+std::to_string(l.meals));
        }
        std::cout<<"shelter_season="<<(h.season+epoch)<<"  residents="<<alive<<"  opportunities="<<total<<"  reached="<<won<<"  meals="<<fed
                 <<"  private_days="<<private_days<<"  informed_choices="<<inferred<<"  inquiries="<<asked<<"  replies="<<heard<<"  repairs="<<fixes
                 <<"  failed_first="<<checks<<"  offspring_energy="<<energy<<"  founders="<<lineages.size()<<"  genotypes="<<genomes.size()<<'\n';
        if (auto *j=journal::current()) {
            j->metrics["animals"]=n;j->metrics["market_meals"]+=fed;j->metrics["market_inquiries"]+=asked;j->metrics["market_private"]+=private_days;j->metrics["market_informed"]+=inferred;
        }
        if (epoch+1==o.seasons) {download(s,image.composition,offset(minds.ptr,best));image.composes=true;}
        if (epoch+1<o.seasons || !o.nest.empty()) {launch(s,"market_brood",islands,32,adults.ptr,eggs.ptr,lives.ptr,seed,drift);std::swap(adults.ptr,eggs.ptr);}
    }
    if (!o.nest.empty()) {
        h.season+=o.seasons;nest.eggs.resize(n);s.d.check(s.d.download(nest.eggs.data(),adults.ptr,n*sizeof(market::Genome)),"collect newborn shelter genomes");
        write_checked(o.nest,nest);
    }
}
int stream(Session &s,const Options &o,AgentImage &image) {
    if (!image.composes) throw std::invalid_argument("a compiled session needs a reared animal");
    std::ifstream catalog_file(o.catalog),live_file;std::istream *catalog=o.catalog=="-" ? &std::cin : &catalog_file,*input=&std::cin;
    if (!*catalog) throw std::runtime_error("cannot read the acoustic catalog");
    if (o.stream!="-") {live_file.open(o.stream);if (!live_file) throw std::runtime_error("cannot read the acoustic stream");input=&live_file;}
    std::vector<Mouth> domain;std::string line;
    while (std::getline(*catalog,line)) {
        auto m=sounds(line);if (!m.size) continue;
        if (domain.size()==catalog_size) throw std::invalid_argument("a catalog holds up to 256 calibration utterances");
        domain.push_back(m);
    }
    if (domain.empty()) throw std::invalid_argument("empty acoustic catalog");
    Word n=Word(domain.size());Temporary m(s.d,sizeof(Mind)),forms(s.d,n*sizeof(Mouth)),d(s.d,sizeof(Catalog)),t(s.d,sizeof(Atlas));
    upload(s,m.ptr,image.composition);s.d.check(s.d.upload(forms.ptr,domain.data(),domain.size()*sizeof(Mouth)),"present calibration inventory");
    launch(s,"compose_calibrate",1,1,m.ptr,forms.ptr,n,d.ptr,t.ptr);Atlas layout;download(s,layout,t.ptr);
    if (layout.unsupported) throw std::invalid_argument("the calibration inventory exceeds the circuit capacity");
    Temporary state(s.d,std::max(4u,4*layout.words())),discourse(s.d,sizeof(Discourse)),port(s.d,sizeof(Exchange));
    Word export_state=!o.save.empty();
    auto reset=[&](){if (export_state) {s.d.check(s.d.zero(m.ptr+offsetof(Mind,memory),0,sizeof(Connections<slots>)),"empty exportable memory");s.d.check(s.d.zero(m.ptr+offsetof(Mind,pending),0,sizeof(Question)),"empty pending testimony");}
        s.d.check(s.d.zero(state.ptr,0,std::max(4u,4*layout.words())),"empty the private views");s.d.check(s.d.zero(discourse.ptr,0,sizeof(Discourse)),"empty the conversational context");};
    reset();std::cout<<"compiled_addresses="<<layout.n<<"  plastic_state_bytes="<<4*layout.words()<<"  discourse_bytes="<<sizeof(Discourse)<<std::endl;
    Word events=0,answers=0,outside=0;
    while (std::getline(*input,line)) {
        if (line=="/quit") break;
        if (line=="/new-scene") {reset();std::cout<<"animal> _"<<std::endl;continue;}
        Exchange e;e.heard=sounds(line);if (!e.heard.size) continue;
        upload(s,port.ptr,e);launch(s,"compose_stream",1,1,m.ptr,t.ptr,state.ptr,discourse.ptr,port.ptr,export_state);download(s,e,port.ptr);
        if (e.status==5) throw std::runtime_error("the current catalog history exceeds the capacity of an exportable plastic mind");
        std::string voice=e.reply.size ? print(e.reply) : e.status==3 ? "[outside catalog]" : e.status==1 ? "_" : "?";
        std::cout<<"animal> "<<voice<<std::endl;++events;answers+=bool(e.reply.size);outside+=e.status==3;
        if (auto *j=journal::current()) j->line("composed_reply="+voice+"  heard="+line);
    }
    if (auto *j=journal::current()) {j->source=o.source;j->device=s.name;j->metrics["compiled_events"]=events;j->metrics["compiled_answers"]=answers;j->metrics["outside_catalog"]=outside;}
    if (export_state) {download(s,image.composition,m.ptr);write_checked(o.save,image);}
    return 0;
}
void enumerate_all(Session &s,DevicePtr brain,DevicePtr language,Word seed,Word style) {
    constexpr Word configurations=65536;
    Temporary cases(s.d,sizeof(Square)),results(s.d,configurations*sizeof(Enumeration));
    launch(s,"compose_census_inputs",1,1,brain,language,cases.ptr,seed,style);
    for (Word first=0;first<configurations;first+=4096) {
        Word count=4096;launch(s,"compose_census",count/128,128,brain,cases.ptr,results.ptr,first,count);
    }
    std::vector<Enumeration> scores(configurations);s.d.check(s.d.download(scores.data(),results.ptr,scores.size()*sizeof(scores[0])),"count all distinguishable scenes");
    Count exact=0,answers=0,replies=0,transitions=0;
    for (const auto &r:scores) {exact+=r.exact;answers+=r.answers;replies+=r.replies;transitions+=r.transitions;}
    std::cout<<"binding_census="<<configurations<<"  exact="<<exact<<"  initial_answers="<<answers<<"  initial_total="<<8*configurations
             <<"  exact_updates="<<transitions<<"  updates="<<32*configurations<<"  answers_after_update="<<replies<<"  update_total="<<256*configurations<<'\n';
    if (auto *j=journal::current()) {j->scores.push_back({"Every object after a single move",replies,256*configurations});j->scores.push_back({"Distinct exact arrangements",exact,configurations});}
}
void expose(Session &s,DevicePtr brain,const std::vector<Exchange> &examples) {
    if (examples.empty()) return;
    Temporary port(s.d,examples.size()*sizeof(Exchange));Word n=Word(examples.size());
    s.d.check(s.d.upload(port.ptr,examples.data(),n*sizeof(Exchange)),"present external caregiver observations");
    launch(s,"compose_expose",1,1,brain,port.ptr,n);
}
void lesson(Session &s,DevicePtr brain,const std::string &path) {
    std::ifstream file;std::istream *input=&std::cin;
    if (path!="-") {file.open(path);if (!file) throw std::runtime_error("cannot open lesson "+path);input=&file;}
    std::vector<Exchange> examples;std::string line;
    while (std::getline(*input,line)) {
        if (line.empty()) continue;
        if (examples.size()==episodes) throw std::invalid_argument("lesson exceeds 240 retained exchanges");
        auto divider=line.find('|');auto x=sounds(line.substr(0,divider));
        if (!x.size) continue;
        Exchange e;e.heard=x;if (divider!=std::string::npos) e.reply=sounds(line.substr(divider+1));examples.push_back(e);
    }
    expose(s,brain,examples);
}
}
CircuitPort::CircuitPort(runtime::Session &session,const Mind &mind,bool present,DevicePtr shared):s(session),body(s.d,present && !shared ? sizeof(Mind) : 0),port(s.d,present ? sizeof(Exchange) : 0),address(shared ? shared : body.ptr) {
    if (present) {
        if (!shared) upload(s,address,mind);
        lessons_changed=mind.lessons.seen!=mind.consolidated;
        awaiting=mind.pending.field!=0;last_question=emit(mind.lexicon,awaiting ? mind.pending.original : mind.memory.prior);
    }
}
bool CircuitPort::hear(const std::string &line,std::string &answer) {
    emission={};if (!address || line.empty()) return false;
    if (line=="/new-scene") {
        s.d.check(s.d.zero(address+offsetof(Mind,memory),0,sizeof(Connections<slots>)),"clear the current event scene");
        s.d.check(s.d.zero(address+offsetof(Mind,pending),0,sizeof(Question)),"clear pending social inquiry");
        previous={};last_question={};awaiting=false;answer="_";return true;
    }
    if (line.starts_with("/want ")) {
        auto split=line.find('|');if (split==std::string::npos) throw std::invalid_argument("use /want QUESTION | DESIRED REPLY");
        Mouth input[3]{sounds(std::string_view(line).substr(6,split-6)),sounds(std::string_view(line).substr(split+1)),{}};
        Temporary signals(s.d,sizeof(input)),count(s.d,sizeof(Word));upload(s,signals.ptr,input);
        DevicePtr cue=signals.ptr,wish=cue+sizeof(Mouth),result=wish+sizeof(Mouth);
        launch(s,"compose_purpose",1,1,address,cue,wish,result,count.ptr);download(s,emission,result);
        Word tested=0;download(s,tested,count.ptr);answer=emission.size ? print(emission) : "?";
        if (auto *j=journal::current()) {j->metrics["imagined_edits"]+=tested;j->metrics["articulated_edits"]+=bool(emission.size);}
        return true;
    }
    if (line=="/cancel") {cancel_question();answer="_";return true;}
    if (line=="/miss") {
        if (!last_question.size) {answer="?";return true;}
        Exchange e;e.heard=last_question;e.evidence=1;upload(s,port.ptr,e);Word one=1;
        launch(s,"compose_turn",1,1,address,port.ptr,one);download(s,e,port.ptr);
        awaiting=e.status==4;answer=e.reply.size ? print(e.reply)+(e.status==4 ? "?" : "") : "?";return true;
    }
    if (line.starts_with("/example ") || line.starts_with("/demonstrate ")) {
        auto start=line.find(' ')+1,bar=line.find('|');auto input=std::string_view(line).substr(start);
        Exchange e;e.heard=sounds(bar==std::string::npos ? input : input.substr(0,bar-start));
        if (bar!=std::string::npos) e.reply=sounds(std::string_view(line).substr(bar+1));
        if (e.heard.size) {expose(s,address,{e});lessons_changed=true;}
        answer="_";return true;
    }
    if (line=="/learn") {
        if (lessons_changed) {consolidate(s,address);lessons_changed=false;}
        answer="_";return true;
    }
    if (line.starts_with("/heard ")) {
        if (!previous.heard.size) {answer="?";return true;}
        previous.reply=sounds(std::string_view(line).substr(7));expose(s,address,{previous});
        lessons_changed=true;answer="_";return true;
    }
    if (line[0]=='/') return false;
    Exchange e;
    try {e.heard=sounds(line);} catch (const std::invalid_argument &) {return false;}
    if (!e.heard.size) return false;
    previous=e;upload(s,port.ptr,e);Word one=1;
    launch(s,"compose_turn",1,1,address,port.ptr,one);download(s,e,port.ptr);
    emission=e.reply;if (!e.status) return false;
    if (e.status==2 || e.status==4) {if (!awaiting) last_question=previous.heard;awaiting=e.status==4;}
    answer=e.reply.size ? print(e.reply)+(e.status==4 ? "?" : "") : e.status==1 ? "_" : "?";return true;
}
void CircuitPort::cancel_question() {awaiting=false;s.d.check(s.d.zero(address+offsetof(Mind,pending),0,sizeof(Question)),"release the tentative vocal request");}
void CircuitPort::save(Mind &m) {if (address) download(s,m,address);}
int command(int argc,char **argv) {
    if (argc==3 && std::string_view(argv[2])=="--help") {
        std::cout<<"axion compose [--load MIND] [--save MIND] [--worlds N] [--dialect english|novel|permuted]\n"
                 <<"  --seed N --rounds N --trials N --depth N --controls 0|1 --circuit --census\n"
                 <<"  --generations N --population N --origin random|inherited|inert|permissive --selection select|drift --noise N\n"
                 <<"  --catalog FILE --stream FILE|-   Serve an acquired circuit on a finite acoustic inventory.\n"
                 <<"  --replicas N --ticks N   Compare temporal contacts with a compiled population of private views.\n"
                 <<"  --inquiries N --attention disagreement|random|yoked   Request feedback on competing acquired projections.\n"
                 <<"  --mentor MIND   Learn through a mature spiking animal’s emitted waveforms.\n"
                 <<"  --society N --seasons N --days N --wire open|muted|shifted|solitary|no-repair\n"
                 <<"  --save-nest FILE --resume-nest FILE   Continue a complete population at the next birth season.\n"
                 <<"  --lesson FILE|-   Each line contains a stimulus and an optional | followed by the caregiver reply.\n";return 0;
    }
    auto o=parse(argc,argv);AgentImage image;if (o.resume.empty() || !o.save.empty()) image.read(o.source);
    Session s;s.open();if (!o.catalog.empty()) return stream(s,o,image);Word n=o.lesson.empty() ? o.worlds : 1;const Word chunk=std::min(n,64u);

    Temporary language(s.d,sizeof(shelter::Dialect));
    auto dialect=english();upload(s,language.ptr,dialect);
    bool append=image.composes && !o.lesson.empty() && !o.fresh;
    if (append && !o.specified_origin) o.origin=0;
    if (append && !o.specified_generations) o.generations=0;
    auto generation_start=std::chrono::steady_clock::now();
    auto genes=o.resume.empty() ? evolve(s,o,image.composes ? image.composition.genes : image.recollection.genes,language.ptr) : RecollectionGenes{};

    auto rearing_start=std::chrono::steady_clock::now();
    if (o.resume.empty()) std::cout<<s.name<<"  compositional_rule="<<std::hex<<genes.low<<'.'<<genes.high<<'.'<<genes.expression<<std::dec<<'\n';
    else std::cout<<s.name<<"  restored_brood="<<o.resume<<'\n';
    if (auto *j=journal::current()) {j->source=o.source;j->device=s.name;j->metrics["composition_body"]=image.brain.genes.body;j->metrics["composition_tape"]=1+(image.brain.genes.expression&7);}
    if (o.residents || !o.resume.empty()) {
        if (!o.resume.empty() && journal::current()) journal::current()->source=o.resume;
        society(s,o,image,genes,language.ptr);if (!o.save.empty()) write_checked(o.save,image);return 0;
    }
    Temporary minds(s.d,Count(n)*sizeof(Mind)),scratch(s.d,Count(chunk)*sizeof(Detections)),germ(s.d,sizeof(genes));upload(s,germ.ptr,genes);
    Temporary adult(s.d,o.mentor.empty() ? 0 : sizeof(Mind));
    if (!o.mentor.empty()) {
        AgentImage mentor;mentor.read(o.mentor);if (!mentor.composes) throw std::invalid_argument("the mentor needs an acquired compositional circuit");
        Temporary teachers(s.d,Count(n)*sizeof(Mind));upload(s,adult.ptr,mentor.composition);
        launch(s,"compose_mentor",(n+31)/32,32,minds.ptr,teachers.ptr,adult.ptr,language.ptr,n,genes,o.seed,o.style,o.rounds);
    } else if (o.lesson.empty()) launch(s,"compose_rear",(n+31)/32,32,minds.ptr,language.ptr,n,germ.ptr,n,o.seed,o.style,o.rounds);
    else {Mind base=append ? image.composition : Mind{};base.genes=genes;upload(s,minds.ptr,base);lesson(s,minds.ptr,o.lesson);}
    if (append) consolidate(s,minds.ptr);
    else {
        Temporary rivals(s.d,o.inquiries ? Count(chunk)*sizeof(Competition) : 0),fees(s.d,o.inquiries ? Count(chunk)*sizeof(Tuition) : 0),
            teachers(s.d,o.inquiries && adult.ptr ? Count(chunk)*sizeof(Mind) : 0),
            twin(s.d,o.inquiries && o.wander==2 ? Count(chunk)*sizeof(Mind) : 0),competing(s.d,twin.ptr ? Count(chunk)*sizeof(Competition) : 0),
            quota(s.d,twin.ptr ? Count(chunk)*sizeof(Tuition) : 0);
        Count heard=0,asked=0,disagreed=0,removed=0,changed=0,unexplained=0;
        for (Word i=0;i<n;i+=chunk) {
            Word k=std::min(chunk,n-i);auto limb=offset(minds.ptr,i);fit(s,limb,scratch.ptr,k);
            if (!o.inquiries) continue;
            launch(s,"compose_contest",k*queries*width,128,limb,scratch.ptr,rivals.ptr);
            if (twin.ptr) {
                Word active=0;DevicePtr none=0;
                s.d.check(s.d.copy_device(twin.ptr,limb,Count(k)*sizeof(Mind)),"pair animals before selective listening");
                s.d.check(s.d.copy_device(competing.ptr,rivals.ptr,Count(k)*sizeof(Competition)),"pair the surviving predictions");
                launch(s,"compose_question",(k+31)/32,32,twin.ptr,scratch.ptr,competing.ptr,quota.ptr,language.ptr,teachers.ptr,adult.ptr,k,i,o.seed,o.style,o.inquiries,active,none);
            }
            launch(s,"compose_question",(k+31)/32,32,limb,scratch.ptr,rivals.ptr,fees.ptr,language.ptr,teachers.ptr,adult.ptr,k,i,o.seed,o.style,o.inquiries,o.wander,quota.ptr);
            std::vector<Tuition> result(k);s.d.check(s.d.download(result.data(),fees.ptr,k*sizeof(Tuition)),"count requested feedback");
            for (const auto &r:result) {heard+=r.heard;asked+=r.asked;disagreed+=r.disagreements;removed+=r.removed;changed+=r.changed;unexplained+=r.unexplained;}
        }
        if (o.inquiries) std::cout<<"tuition="<<(o.wander==2 ? "yoked" : o.wander ? "random" : "disagreement")<<"  opportunities="<<heard<<"  requested="<<asked
            <<"  contested="<<disagreed<<"  excluded="<<removed<<"  revised_columns="<<changed<<"  uncovered="<<unexplained<<'\n';
    }
    image.composes=true;download(s,image.composition,minds.ptr);validate(image.composition);describe(image.composition);illustrate(image.composition);
    auto rearing_end=std::chrono::steady_clock::now();
    std::cout<<"composition_rearing="<<n<<"  evolution_ms="<<std::chrono::duration<double,std::milli>(rearing_start-generation_start).count()
             <<"  acquisition_ms="<<std::chrono::duration<double,std::milli>(rearing_end-rearing_start).count()<<'\n';
    if (o.inspect) inspect(image.composition);
    if (auto *j=journal::current()) {j->source=o.source;j->device=s.name;j->metrics["animals"]=n;}
    if (o.lesson.empty()) {
        Temporary testing(s.d,Count(n)*sizeof(Mind)),scores(s.d,Count(n)*sizeof(Measures));
        for (Word cut=0;cut<(o.controls ? 5u : 1u);++cut) {
            s.d.check(s.d.copy_device(testing.ptr,minds.ptr,Count(n)*sizeof(Mind)),"restore learned animals before a new scene");
            launch(s,"compose_trials",(n+31)/32,32,testing.ptr,language.ptr,scores.ptr,n,o.seed,o.style,o.trials,o.depth,cut,n,o.noise);
            std::vector<Measures> results(n);s.d.check(s.d.download(results.data(),scores.ptr,n*sizeof(Measures)),"read novel-scene probes");
            Count correct=0,total=0,voiced=0,old=0,old_total=0,deep=0,deep_total=0,unique=0,quiet=0,unknown=0,perfect=0,actions=0;
            for (auto &r:results) {correct+=r.correct;total+=r.queries;voiced+=r.voiced;old+=r.displaced_correct;old_total+=r.displaced;deep+=r.deep_correct;deep_total+=r.deep;
                unique+=r.fresh;quiet+=r.unknown_quiet;unknown+=r.unknown;perfect+=r.correct==r.queries && r.unknown_quiet==r.unknown;actions+=r.actions;}
            const char *arm[]{"intact","one-hop","present","unbound","frozen"};
            std::cout<<"composition="<<arm[cut]<<"  worlds="<<n<<"  exact="<<perfect<<"  correct="<<correct<<"  turns="<<total
                     <<"  voiced="<<voiced<<"  stale="<<old<<"  stale_total="<<old_total<<"  deep="<<deep<<"  deep_total="<<deep_total
                     <<"  quiet="<<quiet<<"  unknown="<<unknown<<"  fresh_forms="<<unique<<"  moves="<<actions<<'\n';
            if (!cut) for (Word i=0;i<n;++i) if (results[i].missed_at) {
                const auto &r=results[i];std::cout<<"composition_failure="<<i<<"  question_number="<<r.missed_at<<"  heard="<<print(r.first_input)
                    <<"  expected="<<print(r.expected)<<"  emitted="<<(r.produced.size ? print(r.produced) : "_")<<'\n';break;
            }
            if (auto *j=journal::current();j && !cut) {
                j->scores.push_back({"Exact utterances in new scenes",correct,total});j->scores.push_back({"An absent observer",old,old_total});
                j->scores.push_back({"Chains deeper than training",deep,deep_total});
            }
        }
    }
    if (o.census) enumerate_all(s,minds.ptr,language.ptr,o.seed,o.style);
    if (o.replicas) scale_population(s,minds.ptr,language.ptr,n,o.replicas,o.ticks,o.seed,o.style);
    if (!o.save.empty()) write_checked(o.save,image);
    return 0;
}
}
