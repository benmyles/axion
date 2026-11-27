#include "procedure_host.hpp"
#include "journal_population.hpp"
#include "procedure_probe.cuh"
#include "procedure_express.cuh"
#include "procedure_nest.hpp"
#include "procedure_pack.cuh"
#include <set>
#include "individual_io.hpp"
#include "pulse_words.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace axion::life::procedure {
namespace {
using namespace runtime;
using namespace storage;
template<class... A> void run(Session &s,const char *name,Word blocks,Word threads,A&... a) {
    Function f{};s.bind(f,name,Image::procedure);void *args[]{static_cast<void *>(&a)...};
    s.d.check(s.d.launch(f,blocks,1,1,threads,1,1,0,nullptr,args,nullptr),name);
}
template<class T> void put(Session &s,DevicePtr dst,const T &x) {s.d.check(s.d.upload(dst,&x,sizeof(x)),"present procedure state");}
template<class T> void get(Session &s,T &x,DevicePtr src) {s.d.check(s.d.download(&x,src,sizeof(x)),"read procedure state");}
Count quantity(std::string_view x,Count low,Count high,int radix=10) {
    Count n=0;auto [end,ec]=std::from_chars(x.data(),x.data()+x.size(),n,radix);
    if (ec!=std::errc{} || end!=x.data()+x.size() || n<low || n>high) throw std::invalid_argument("invalid procedural quantity "+std::string(x));
    return n;
}
std::string trim(std::string_view x) {
    auto start=x.find_first_not_of(" \t\r\n");if (start==x.npos) return {};
    return std::string(x.substr(start,x.find_last_not_of(" \t\r\n")-start+1));
}
std::vector<Count> transmit(std::string_view text,bool input,bool raw=false) {
    std::vector<Count> waves;
    if (raw) {
        std::istringstream in{std::string(text)};std::string s;
        while (in>>s) {if (s[0]=='@') s.erase(0,1);waves.push_back(quantity(s,1,~0ull,16));}
    } else for (unsigned char c:text) {
        if (input && (c==' ' || c=='\t' || c=='\r' || c=='\n')) continue;
        if (input && c>='A' && c<='Z') c=static_cast<unsigned char>(c+32);
        if (c<' ' || c>126) throw std::invalid_argument("the text port accepts printable ASCII or explicit raw spike waves");
        waves.push_back(signal(c));
    }
    return waves;
}
std::vector<Count> motors(std::string_view text,bool raw=false) {
    if (!text.starts_with("@motors ")) return transmit(text,false,raw);
    std::istringstream in{std::string(text.substr(8))};std::string token;std::vector<Count> result;
    while (in>>token) {
        Count wave=sound_pattern(token);if (!wave) throw std::invalid_argument("a grounded motor form holds one through twelve letters");
        result.push_back(acoustic::wave(wave));
    }
    return result;
}
std::string decode(Count wave) {
    for (Word j=32;j<127;++j) if (signal(j)==wave) return std::string(1,char(j));
    auto raw=acoustic::unwave(wave);auto form=transcribe(raw);
    if (wave && !form.empty() && form.size()<=12 && sound_pattern(form)==raw) return "<"+form+">";
    std::ostringstream s;s<<'@'<<std::hex<<wave;return s.str();
}
Word mark(Session &s,DevicePtr body,Word n,bool strict=true) {
    Word batch=std::min(n,128u);
    Temporary t(s.d,Count(batch)*sizeof(Fit)),errors(s.d,batch*sizeof(Word));Word failures=0;
    for (Word i=0;i<n;i+=batch) {
        Word k=std::min(batch,n-i);DevicePtr m=body+Count(i)*sizeof(Mind);
        run(s,"procedure_recursion",(k+31)/32,32,m,t.ptr,k);run(s,"procedure_sites",k,128,m,t.ptr);run(s,"procedure_align",k,128,m,t.ptr);
        run(s,"procedure_connect",(k+31)/32,32,m,t.ptr,errors.ptr,k);
        std::vector<Word> clipped(k);s.d.check(s.d.download(clipped.data(),errors.ptr,k*sizeof(Word)),"count exhausted procedural circuits");
        for (Word v:clipped) failures+=v;
    }
    if (strict && failures) throw std::runtime_error("the acquired procedure exceeds the available circuit capacity");
    return failures;
}
bool teach(Session &s,DevicePtr m,std::string_view line,bool raw,bool online=false) {
    Word retina=0,visible=0;if (line.starts_with("@view=")) {
        auto space=line.find(' ');if (space==line.npos) throw std::invalid_argument("a visual observation needs its acoustic demonstration");
        retina=Word(quantity(line.substr(6,space-6),0,255,16));visible=1;line.remove_prefix(space+1);
    }
    auto bar=line.find('|');if (bar==line.npos) throw std::invalid_argument("a demonstration needs the heard signal | emitted signal");
    auto x=transmit(trim(line.substr(0,bar)),true,raw),y=motors(trim(line.substr(bar+1)),raw);
    if (x.empty() || x.size()>255 || y.size()>255) throw std::invalid_argument("each side of a demonstration holds at most 255 spikes and its stimulus must be present");
    Word a=Word(x.size()),b=Word(y.size());x.insert(x.end(),y.begin(),y.end());Temporary signals(s.d,x.size()*sizeof(Count));
    s.d.check(s.d.upload(signals.ptr,x.data(),x.size()*sizeof(Count)),"deliver a motor demonstration");Temporary signal(s.d,online ? sizeof(Word) : 0);
    run(s,"procedure_trace",1,1,m,signals.ptr,a,b,retina,visible,signal.ptr);Word change=0;if (online) get(s,change,signal.ptr);return change!=0;
}
struct Response {Report stats;std::string text;std::vector<Count> waves;};
Response query_device(Session &s,DevicePtr m,DevicePtr d,Word direction,DevicePtr heard,Word n,Count from,Word requested,bool chart) {
    Response answer;if (!n) return answer;
    if (n>32766) {answer.stats.status=capacity;return answer;}
    Word force=chart;
    bool use_chart=(chart || !direction) && n<=512;
    Word extra=use_chart ? std::min(32765u,n*(n+1)/2+1) : n+1;
    Temporary ids(s.d,n),nodes(s.d,(extra+1)*sizeof(Node)),plans(s.d,(extra+1)*sizeof(Summary)),
        grid(s.d,use_chart ? n*n*sizeof(Half) : 0),out(s.d,sizeof(Report)),sound(s.d,requested*sizeof(Count));
    run(s,"procedure_query",1,1,m,d,heard,n,ids.ptr,nodes.ptr,plans.ptr,grid.ptr,extra,out.ptr,sound.ptr,from,requested,force);
    get(s,answer.stats,out.ptr);answer.waves.resize(answer.stats.spoken);
    if (!answer.waves.empty()) s.d.check(s.d.download(answer.waves.data(),sound.ptr,answer.waves.size()*sizeof(Count)),"read the articulated motor stream");
    for (auto v:answer.waves) answer.text+=decode(v);
    return answer;
}
Response query(Session &s,DevicePtr m,DevicePtr d,Word direction,const std::vector<Count> &x,Count from,Word requested,bool chart=false) {
    if (x.empty()) return {};
    if (x.size()>32766) {Response response;response.stats.status=capacity;return response;}
    Temporary heard(s.d,x.size()*sizeof(Count));
    s.d.check(s.d.upload(heard.ptr,x.data(),x.size()*sizeof(Count)),"deliver an uninterrupted acoustic phrase");
    return query_device(s,m,d,direction,heard.ptr,Word(x.size()),from,requested,chart);
}
Response express(Session &s,DevicePtr m,DevicePtr decoder,Word direction,const std::vector<Count> &waves) {
    Response answer;if (waves.empty()) return answer;
    if (waves.size()>384) {answer.stats.status=capacity;return answer;}
    Word n=Word(waves.size()),cap=4096;
    Temporary y(s.d,n*sizeof(Count)),target(s.d,n),chart(s.d,Count(n)*n*sizeof(Derivation)),ids(s.d,cap),text(s.d,cap*sizeof(Count)),result(s.d,sizeof(Description));
    s.d.check(s.d.upload(y.ptr,waves.data(),n*sizeof(Count)),"offer a witnessed motor trajectory");
    run(s,"procedure_express",1,128,m,y.ptr,n,target.ptr,chart.ptr,ids.ptr,text.ptr,cap,result.ptr);
    Description emitted;get(s,emitted,result.ptr);answer.stats.status=emitted.status;if (emitted.status!=recognized) return answer;
    answer.waves.resize(emitted.length);s.d.check(s.d.download(answer.waves.data(),text.ptr,answer.waves.size()*sizeof(Count)),"hear the inferred procedure");
    auto checked=query(s,m,decoder,direction,answer.waves,0,n);
    if (checked.stats.status!=recognized || checked.stats.voice.length!=n || checked.waves!=waves) {answer={};return answer;}
    for (auto v:answer.waves) answer.text+=decode(v);
    answer.stats.voice.length=answer.waves.size();answer.stats.spoken=emitted.length;return answer;
}
apprenticeship::Convention english() {
    using namespace apprenticeship;Convention v;
    const char *names[]{"hop","veer","dip","reach","nudge","sip","twice","rim","join","after","braid","echo"};
    for (Word j=0;j<std::size(names);++j) {auto &f=v.name[j];f.size=Byte(std::char_traits<char>::length(names[j]));std::copy_n(names[j],f.size,f.bytes);}
    for (Word j=0;j<actions;++j) v.motor[j]=Byte('a'+j);
    auto law=[&](Word i,Word arity,std::initializer_list<Byte> script) {
        v.arity[i]=static_cast<Byte>(arity);v.length[i]=static_cast<Byte>(script.size());std::copy(script.begin(),script.end(),v.law[i]);
    };
    law(0,1,{call0,call0});law(1,1,{'l',call0,'r'});law(2,2,{call0,call1});
    law(3,2,{call1,call0});law(4,2,{call0,call1,call0});law(5,2,{call1,call0,call1,call1});
    return v;
}
#include "procedure_projection.hpp"
#include "procedure_scale.hpp"
struct Options {
    std::string source="examples/learner.mind",save,lesson,said,expressed,sampled,mentor,brood,resume,forecast;
    Count target=0,starts=~0ull;Word horizon=6,senses=1,beam=512,families=0,replicas=0,episodes=16,room=0;bool exhaustive=false,audit=false,calibrate_lantern=false,verify_lantern=false;Word contacts=24,lamp_seed=0,lamp_attention=0,select_questions=0,survey=0;
    Word worlds=16,rounds=6,seed=7231,style=0,noise=0,limit=256,journeys=1,inquiries=0,random=0,probes=16,depth=16,distortion=0;
    Word coins=0,chunk=8,drills=256,tempo=0,colony=0,seasons=6,days=32,wire=0,culture=0,innovation=1,storm=0,guards=32,cases=256;
    Word generations=12,population=48,origin=2,drift=0;
    Count start=0;bool inspect=false,raw=false,chart=false,fresh=false,explicit_genes=false,explicit_generations=false;
    bool asked() const {return !said.empty() || !expressed.empty() || !sampled.empty() || !forecast.empty() || target || survey;}

};
Options parse(int argc,char **argv) {
    Options o;for (int i=2;i<argc;++i) {
        std::string f=argv[i];
        if (f=="--fresh") {o.fresh=true;continue;}if (f=="--inspect-code") {o.inspect=true;continue;}if (f=="--raw") {o.raw=true;continue;}if (f=="--chart") {o.chart=true;continue;}
        if (++i>=argc) throw std::invalid_argument("missing value for "+f);
        std::string v=argv[i];
        if (f=="--forecast") o.forecast=v;
        else if (f=="--goal-states") o.target=quantity(v,1,~0ull,16);else if (f=="--start-states") o.starts=quantity(v,1,~0ull,16);
        else if (f=="--search" && (v=="preimage" || v=="exhaustive")) o.exhaustive=v=="exhaustive";
        else if (f=="--beam") {o.beam=Word(quantity(v,1,8192));if (!std::has_single_bit(o.beam)) throw std::invalid_argument("the policy beam width must be a power of two");}
        else if (f=="--horizon") o.horizon=Word(quantity(v,1,max_steps));else if (f=="--senses" && (v=="open" || v=="closed")) o.senses=v=="open";
        else if (f=="--families") o.families=Word(quantity(v,1,32768));
        else if (f=="--replicas") o.replicas=Word(quantity(v,1,16777216));else if (f=="--episodes") o.episodes=Word(quantity(v,1,4096));
        else if (f=="--lantern") {o.lamp_seed=Word(quantity(v,0,~0u));o.calibrate_lantern=true;}
        else if (f=="--survey-rooms") o.survey=Word(quantity(v,1,131072));
        else if (f=="--select-questions") o.select_questions=Word(quantity(v,0,1));
        else if (f=="--contact-policy" && (v=="active" || v=="sweep")) o.lamp_attention=v=="active";
        else if (f=="--contacts") o.contacts=Word(quantity(v,0,64));
        else if (f=="--verify-lantern") {o.room=Word(quantity(v,0,~0u));o.audit=o.verify_lantern=true;}
        else if (f=="--verify-room") {o.room=Word(quantity(v,0,~0u));o.audit=true;}
        else if (f=="--guards") o.guards=Word(quantity(v,0,48));else if (f=="--cases") o.cases=Word(quantity(v,0,65536));
        else if (f=="--mentor") o.mentor=v;else if (f=="--save-roost") o.brood=v;else if (f=="--resume-roost") o.resume=v;
        else if (f=="--colony") o.colony=Word(quantity(v,32,16384));else if (f=="--seasons") o.seasons=Word(quantity(v,1,4096));
        else if (f=="--days") o.days=Word(quantity(v,1,4096));else if (f=="--storm") o.storm=Word(quantity(v,0,4096));
        else if (f=="--innovation") o.innovation=Word(quantity(v,0,1));
        else if (f=="--culture" && (v=="local" || v=="isolated" || v=="apparatus")) o.culture=v=="local" ? 0 : v=="isolated" ? 1 : 2;
        else if (f=="--wire" && (v=="open" || v=="mute" || v=="reversed" || v=="stale" || v=="fixed-gait" || v=="fixed-attention" || v=="immediate-replay" || v=="rote")) {
            const std::array<std::string,8> modes{"open","mute","reversed","stale","fixed-gait","rote","fixed-attention","immediate-replay"};
            o.wire=Word(std::find(modes.begin(),modes.end(),v)-modes.begin());
        } else if (f=="--samples") o.sampled=v;else if (f=="--tempo") o.tempo=Word(quantity(v,0,16));
        else if (f=="--express") o.expressed=v;else if (f=="--invent") o.coins=Word(quantity(v,0,12));else if (f=="--chunk") o.chunk=Word(quantity(v,2,16));else if (f=="--drills") o.drills=Word(quantity(v,0,roost::practice_limit));
        else if (f=="--load") o.source=v;else if (f=="--save") o.save=v;else if (f=="--lesson") o.lesson=v;
        else if (f=="--say") o.said=v;else if (f=="--from-spike") o.start=quantity(v,0,~0ull);else if (f=="--limit") o.limit=Word(quantity(v,0,65536));
        else if (f=="--probes") o.probes=Word(quantity(v,0,4096));
        else if (f=="--depth") o.depth=Word(quantity(v,1,96));
        else if (f=="--receptor-noise") o.distortion=Word(quantity(v,0,16));
        else if (f=="--generations") {o.generations=Word(quantity(v,0,512));o.explicit_generations=true;}
        else if (f=="--population") o.population=Word(quantity(v,1,2048));
        else if (f=="--selection" && (v=="select" || v=="drift")) o.drift=v=="drift";
        else if (f=="--worlds") o.worlds=Word(quantity(v,1,8192));else if (f=="--rounds") o.rounds=Word(quantity(v,2,14));
        else if (f=="--seed") o.seed=Word(quantity(v,0,~0u));else if (f=="--noise") o.noise=Word(quantity(v,0,16));
        else if (f=="--journeys" && (v=="none" || v=="mixed" || v=="sparse")) o.journeys=v=="none" ? 0 : v=="sparse" ? 2 : 1;
        else if (f=="--inquiries") o.inquiries=Word(quantity(v,0,64));
        else if (f=="--attention" && (v=="random" || v=="disagreement" || v=="yoked")) o.random=v=="yoked" ? 2 : v=="random";
        else if (f=="--dialect" && (v=="english" || v=="prefix" || v=="suffix")) o.style=v=="english" ? 0 : v=="prefix" ? 1 : 2;
        else if (f=="--origin" && (v=="random" || v=="inherited" || v=="inert" || v=="permissive")) {o.explicit_genes=true;o.origin=v=="inherited" ? 0 : v=="inert" ? 1 : v=="permissive" ? 3 : 2;}
        else throw std::invalid_argument("unknown procedure option "+f);
    }
    if (o.colony%32) throw std::invalid_argument("the roost has 32 residents per island");
    if (!o.brood.empty() && o.resume.empty() && !o.colony) throw std::invalid_argument("a saved brood needs a roost population");
    if ((!o.resume.empty() || o.colony) && (!o.mentor.empty() || !o.lesson.empty() || o.asked()))
        throw std::invalid_argument("a roost supplies its own learning partners");
    if (!o.mentor.empty() && !o.lesson.empty()) throw std::invalid_argument("choose a live mentor or an external lesson");
    if ((o.audit || o.replicas || o.calibrate_lantern) && !o.target && o.forecast.empty()) throw std::invalid_argument("supply a forecast or goal for the policy assay");
    if (o.families && (!o.replicas || o.forecast.empty())) throw std::invalid_argument("a cohort needs an explicit forecast and replica count");
    if (o.target && !o.forecast.empty()) throw std::invalid_argument("choose a goal or an explicit procedure for the forecast");
    return o;
}
template<class... Args> void breed(Session &s,const char *name,Word n,Args&... values) {
    Function f{};s.bind(f,name,Image::recollection);void *a[]{static_cast<void *>(&values)...};
    s.d.check(s.d.launch(f,(n+63)/64,1,1,64,1,1,0,nullptr,a,nullptr),name);
}
class Examination {
    Session &s;Word batch,limit;
    Temporary nodes,plans,chart;
public:
    Examination(Session &s,Word n,Word depth):s(s),batch(std::min(n,512u)),limit(std::max(512u,2*depth+4)),
        nodes(s.d,Count(batch)*(limit+1)*sizeof(Node)),plans(s.d,Count(batch)*(limit+1)*sizeof(Summary)),chart(s.d,Count(batch)*4096*sizeof(Half)) {}
    void probe(DevicePtr brains,DevicePtr d,DevicePtr world,DevicePtr scores,Word n,Word siblings,Word seed,Word style,Word events,Word depth,Word noise) {
        for (Word i=0;i<n;i+=batch) {
            Word k=std::min(batch,n-i);
            run(s,"procedure_probes",(k+31)/32,32,brains,d,world,scores,nodes.ptr,plans.ptr,chart.ptr,k,i,siblings,seed,style,events,depth,noise,limit);
        }
    }
};
void calibrate(Session &,DevicePtr,DevicePtr,DevicePtr,Word,Word,Word,Word,Word,Word);
RecollectionGenes evolve(Session &s,const Options &o,DevicePtr physical,RecollectionGenes founder,RecollectionGenes optics) {
    Word n=o.population,seed=o.seed,origin=o.origin,drift=o.drift,keep=1;
    if (origin==3) {origin=0;founder={0xff90000fu,0x080801ffu,0x40000000u};}
    if (!o.generations && !origin) return founder;
    Temporary parents(s.d,n*sizeof(founder)),children(s.d,n*sizeof(founder));
    breed(s,"recollect_seed",n,parents.ptr,n,founder,seed,origin);
    if (o.generations) {
        Word siblings=1,total=n,rounds=5,journeys=1,tests=8,depth=4;
        Temporary brains(s.d,Count(total)*sizeof(Mind)),decoders(s.d,Count(total)*sizeof(Decoder)),scores(s.d,Count(total)*sizeof(apprenticeship::Probes)),fitness(s.d,n*sizeof(Word));
        Examination check(s,total,depth);Temporary gait(s.d,Count(total)*sizeof(roost::GaitScore)),episodes(s.d,Count(total)*sizeof(apprenticeship::PerformanceCase)),
            activity(s.d,total*sizeof(SceneCheck)),calls(s.d,total*sizeof(Invention)),gaze(s.d,o.select_questions ? n*operations*4*sizeof(Word) : 0);
        for (Word g=0;g<o.generations;++g) {
            seed=mix(o.seed^mix(0x9935c915u+g));Word style=1+g%2,noise=0,dust=g%3,tempo=o.tempo && (g&1) ? 4 : 0,guards=o.guards ? 24 : 0;
            run(s,"procedure_rear",(total+31)/32,32,brains.ptr,physical,parents.ptr,siblings,total,seed,style,rounds,noise,journeys,tempo,guards);
            mark(s,brains.ptr,total,false);run(s,"procedure_compile",(total+31)/32,32,brains.ptr,decoders.ptr,total);
            check.probe(brains.ptr,decoders.ptr,physical,scores.ptr,total,siblings,seed,style,tests,depth,dust);
            run(s,"procedure_merit",(n+31)/32,32,scores.ptr,fitness.ptr,n,siblings);
            if (o.drills) {Word exercises=128,attempts=96;calibrate(s,brains.ptr,physical,gait.ptr,total,siblings,seed,style,exercises,attempts);
                run(s,"procedure_motor_merit",(n+31)/32,32,fitness.ptr,gait.ptr,n,siblings);}
            if (guards) {
                Word scenes=96,cut=0;run(s,"procedure_concerts",(total+31)/32,32,brains.ptr,decoders.ptr,physical,episodes.ptr,total,siblings,seed,style);
                run(s,"procedure_contingencies",total,128,brains.ptr,decoders.ptr,physical,episodes.ptr,activity.ptr,total,siblings,seed,style,scenes,cut);
                run(s,"procedure_scene_merit",(n+31)/32,32,fitness.ptr,activity.ptr,n,siblings);
            }
            if (g%3==2) {
                Word chunk=6;run(s,"procedure_innovate",(total+31)/32,32,brains.ptr,calls.ptr,total,seed,chunk);
                run(s,"procedure_call_merit",(n+31)/32,32,fitness.ptr,calls.ptr,brains.ptr,n);
            }
            if (o.select_questions) {
                Word visits=8+g%5;
                run(s,"procedure_lantern_merit",n*operations,32,parents.ptr,optics,seed,visits,gaze.ptr);
                run(s,"procedure_value_questions",(n+31)/32,32,gaze.ptr,fitness.ptr,n);
            }
            if (auto *j=journal::current()) {
                std::vector<Word> values(n);s.d.check(s.d.download(values.data(),fitness.ptr,n*sizeof(Word)),"measure developmental fitness");
                Count sum=0;for (Word v:values) sum+=v;j->breeding.push_back({g,n,{sum,*std::max_element(values.begin(),values.end())}});
            }
            breed(s,"recollect_brood",n,parents.ptr,children.ptr,fitness.ptr,n,seed,drift,keep);std::swap(parents.ptr,children.ptr);
        }
    }
    get(s,founder,parents.ptr);return founder;
}
void calibrate(Session &s,DevicePtr minds,DevicePtr base,DevicePtr scores,Word n,Word siblings,Word seed,Word style,Word steps,Word trials) {
    Temporary history(s.d,Count(n)*sizeof(roost::Kinetics));
    run(s,"procedure_explore",(n+31)/32,32,minds,history.ptr,base,n,siblings,seed,style,steps);
    run(s,"procedure_attend",n,32,minds,history.ptr);
    run(s,"procedure_gait_score",(n+31)/32,32,minds,base,scores,n,siblings,seed,style,trials);
}
void examine(Session &s,const Options &o,DevicePtr brains,DevicePtr d,DevicePtr world,Word n) {
    Temporary results(s.d,Count(n)*sizeof(apprenticeship::Probes));Examination lab(s,n,o.depth);
    lab.probe(brains,d,world,results.ptr,n,n,o.seed,o.style,o.probes,o.depth,o.distortion);
    std::vector<apprenticeship::Probes> scores(n);s.d.check(s.d.download(scores.data(),results.ptr,n*sizeof(scores[0])),"read held-out procedures");
    Count passed=0,total=0,samples=0,matching=0,grammar=0,recursion=0,trips=0,returned=0,unresolved=0,longest=0,exact=0,small=0,measured=0,overflow=0;
    for (auto &v:scores) {
        passed+=v.perfect;total+=v.paths;samples+=v.samples;matching+=v.samples_ok;grammar+=v.grammar;recursion+=v.recursion;trips+=v.trips;returned+=v.returned;
        unresolved+=v.absent;longest=std::max(longest,v.longest);exact+=v.perfect==v.paths && v.paths!=0;small+=v.performed;measured+=v.performed_ok;overflow+=v.overflow;
    }
    std::cout<<"motor_transfer="<<n<<"  exact_animals="<<exact<<"  full_programs="<<passed<<"  programs="<<total<<"  checked_symbols="<<samples<<"  correct_symbols="<<matching
             <<"  fully_expanded="<<small<<"  fully_correct="<<measured<<"  certified_grammars="<<grammar<<"  certified_returns="<<recursion
             <<"  return_plans="<<returned<<"  journeys="<<trips<<"  unsettled="<<unresolved<<"  longest="<<longest<<"  overflow="<<overflow<<'\n';
    if (auto *j=journal::current()) {
        j->scores.push_back({"Held-out action procedures",passed,total});j->scores.push_back({"Entire recursive grammar",grammar,n});
        j->scores.push_back({"Exact return trajectories",returned,trips});
    }
}
void roost_life(Session &s,const Options &o,AgentImage &image,DevicePtr habitat,RecollectionGenes genes) {
    RoostNest nest;auto &h=nest.header;h.n=o.colony;h.days=o.days;h.rounds=o.rounds;h.steps=o.drills;h.seed=o.seed;h.style=o.style;
    h.cut=o.wire;h.coins=o.innovation;h.culture=o.culture;h.storm=o.storm;h.drift=o.drift;
    if (!o.resume.empty()) nest.read(o.resume);
    if (o.seasons>~0u-h.epoch || Count(h.epoch+o.seasons)*h.days>~0u) throw std::invalid_argument("roost season counter overflow");
    Word n=h.n,islands=n/32;Temporary parents(s.d,n*sizeof(roost::Genome)),young(s.d,n*sizeof(roost::Genome)),adults(s.d,islands*sizeof(Mind)),
        minds(s.d,Count(n)*sizeof(Mind)),terrain(s.d,islands*sizeof(roost::Terrain)),day(s.d,n*sizeof(roost::Day)),exercise(s.d,n*sizeof(roost::Kinetics));
    auto load=[&](DevicePtr data,const auto &v) {s.d.check(s.d.upload(data,v.data(),v.size()*sizeof(v[0])),"hatch the saved roost");};
    auto read=[&](auto &v,DevicePtr data) {s.d.check(s.d.download(v.data(),data,v.size()*sizeof(v[0])),"observe the roost");};
    if (!o.resume.empty()) {load(parents.ptr,nest.eggs);load(adults.ptr,nest.mentors);load(terrain.ptr,nest.field);}
    else {run(s,"roost_founders",islands,32,parents.ptr,terrain.ptr,n,genes,h.seed);s.d.check(s.d.zero(adults.ptr,0,islands*sizeof(Mind)),"clear the first nursery");}
    std::vector<roost::Day> lives(n);std::vector<roost::Genome> family(n);
    for (Word elapsed=0;elapsed<o.seasons;++elapsed) {
        Word epoch=h.epoch+elapsed,established=epoch!=0;
        run(s,"roost_rear",islands,32,minds.ptr,adults.ptr,parents.ptr,habitat,n,h.seed,h.style,h.rounds,established,h.culture);
        Word clipped=mark(s,minds.ptr,n,false);
        run(s,"roost_body",islands,32,minds.ptr,exercise.ptr,habitat,n,h.seed,epoch,h.style,h.steps);
        run(s,"procedure_attend",n,32,minds.ptr,exercise.ptr);
        run(s,"roost_forage",islands,32,minds.ptr,parents.ptr,terrain.ptr,habitat,day.ptr,h.seed,epoch,h.style,h.days,h.cut,h.coins,h.storm,exercise.ptr,h.version);
        read(lives,day.ptr);read(family,parents.ptr);Count alive=0,sorties=0,landings=0,meals=0,moves=0,calls=0,pulses=0,heard=0,guided=0,
            learned=0,inventions=0,shortcalls=0,energy=0,charges=0,matched=0,reactive=0,replays=0,rewired=0,adopted=0,declined=0,total_rules=0,total_receptors=0;std::array<Count,3> successes{},attempts{};Word best=0;
        std::set<Word> founders;std::set<std::array<Word,3>> phenotypes;
        for (Word i=0;i<n;++i) if (family[i].alive) {
            auto &v=lives[i];++alive;sorties+=v.sorties;landings+=v.landings;meals+=v.meals;moves+=v.moves;calls+=v.utterances;pulses+=v.spikes;
            heard+=v.receptions;guided+=v.understood;learned+=v.imitations;inventions+=v.innovations;shortcalls+=v.abbreviated;
            charges+=v.charged;energy+=v.energy;matched+=v.matched;reactive+=v.reactive;replays+=v.replays;rewired+=v.rewired;adopted+=v.adopted;declined+=v.declined;total_rules+=v.productions;total_receptors+=v.receptors;
            for (Word type=0;type<3;++type) {successes[type]+=v.success[type];attempts[type]+=v.attempts[type];}
            if (v.energy>lives[best].energy) best=i;
            const auto &g=family[i];founders.insert(g.founder);phenotypes.insert({g.learning.low,g.learning.high,g.learning.expression});
            if (i<32 && elapsed+1==o.seasons && journal::current() && v.path_n) {
                std::string path;for (Word k=0;k<v.path_n;++k) {if (k) path+=',';path+=std::to_string(v.path[k]);}
                journal::current()->line("roost_flight="+std::to_string(i)+"  tick="+std::to_string(v.path_tick)+"  found="+std::to_string(v.fruit)
                    +"  signal="+(v.call ? decode(v.call) : "_")+"  path="+path);
            }
            if (i<32 && elapsed+1==o.seasons && journal::current()) journal::current()->line("roost_resident="+std::to_string(i)+"  founder="+std::to_string(g.founder)+
                "  form="+std::to_string(g.roaming)+"  site="+std::to_string(v.site)+"  calls="+std::to_string(v.utterances)+"  interpreted="+std::to_string(v.understood)+"  food="+std::to_string(v.meals)+"  inventions="+std::to_string(v.innovations));
        }
        std::cout<<"roost_season="<<epoch<<"  residents="<<alive<<"  flights="<<sorties<<"  homed="<<landings<<"  meals="<<meals<<"  moves="<<moves<<"  attained="<<reactive
                 <<"  calls="<<calls<<"  acoustic_spikes="<<pulses<<"  heard="<<heard<<"  guided="<<guided<<"  exact_reports="<<matched<<"  invented="<<inventions
                 <<"  learned="<<learned<<"  brief_calls="<<shortcalls<<"  expenditure="<<charges<<"  offspring_energy="<<energy<<"  founders="<<founders.size()
                 <<"  production_sum="<<total_rules<<"  receptor_sum="<<total_receptors<<"  replays="<<replays<<"  rewired="<<rewired<<"  adopted="<<adopted<<"  declined="<<declined<<"  learning_genotypes="<<phenotypes.size()<<"  clipped="<<clipped<<'\n';
        if (h.storm) for (Word age=0;age<3;++age) std::cout<<"wind_window="<<age<<"  season="<<epoch<<"  attained="<<successes[age]<<"  moves="<<attempts[age]<<'\n';
        if (auto *log=journal::current()) {log->metrics["animals"]=n;log->metrics["roost_days"]=h.days;log->metrics["roost_food"]+=meals;log->metrics["roost_motions"]+=moves;log->metrics["roost_coined"]+=inventions;}
        if (elapsed+1==o.seasons && journal::current()) {
            std::vector<journal::CensusMember> roster;roster.reserve(n);
            for (Word i=0;i<n;++i) if (family[i].alive && lives[i].energy) {
                const auto &g=family[i];const auto &v=lives[i];roster.push_back({g.roaming,i,v.energy,g.founder,0,0,v.productions,v.receptors});
            }
            journal::ranked_forms(*journal::current(),std::move(roster),true);
            roost::Terrain ground;get(s,ground,terrain.ptr);journal::current()->line("roost_terrain=0  buds="+std::to_string(ground.buds));
        }
        if (elapsed+1==o.seasons) {
            get(s,image.practice,minds.ptr+Count(best)*sizeof(Mind));image.practice.extent=sizeof(Mind);image.procedural=true;
            if (auto *log=journal::current();log && alive) {
                const auto &mind=image.practice;const auto &flight=lives[best];journal::Specimen p;
                p.role="roost";p.path="Resident "+std::to_string(best);p.circuits={"Motor control"};
                if (mind.circuit.count) p.circuits.emplace_back("Acquired motor procedures");
                if (mind.recursion.n) p.circuits.emplace_back("Recursive route memory");
                if (mind.muscle.forms) p.senses={"Tidal light","Muscle feedback"};
                if (mind.ear.size) p.senses.emplace_back("Acoustic receptors");
                p.measures={{"roaming",family[best].roaming},{"founder",family[best].founder},{"season",epoch},{"meals",flight.meals},{"calls",flight.utterances},
                    {"receptors",mind.ear.size},{"productions",mind.circuit.count},{"muscle_forms",mind.muscle.forms},{"recursions",mind.recursion.n},
                    {"gates",mind.circuit.guard_n},{"demonstrations",mind.memory.observed},{"replays",flight.replays},{"rewired",flight.rewired}};
                log->specimens.emplace_back(std::move(p));
            }
        }
        if (elapsed+1<o.seasons || !o.brood.empty()) {
            Word seed=mix(h.seed+epoch*0x1397a2ddu);run(s,"roost_brood",islands,32,parents.ptr,young.ptr,day.ptr,minds.ptr,adults.ptr,seed,h.drift);std::swap(parents.ptr,young.ptr);
        }
    }
    if (!o.brood.empty()) {
        h.epoch+=o.seasons;nest.eggs.resize(n);nest.mentors.resize(islands);nest.field.resize(islands);
        read(nest.eggs,parents.ptr);read(nest.mentors,adults.ptr);read(nest.field,terrain.ptr);
        for (auto &adult:nest.mentors) {adult.extent=sizeof(Mind);validate(adult);}
        write_checked(o.brood,nest);
    }
}
std::string pattern(const Mind &m,Slice x) {
    std::string text;for (Word i=0;i<x.n;++i) {
        Byte c=x.p[i];if (var(c)) text+=c==call0 ? "[A]" : c==call1 ? "[B]" : c==quote0 ? "{A}" : "{B}";
        else text+=decode(m.ear.model[c]);
    }return text;
}
void describe(const Mind &m,Word direction,bool inspect) {
    Word rules=0,raw=0;
    for (Word i=0;i<m.circuit.count;++i) {auto r=m.circuit.cell[i];rules+=r.arity!=0;raw+=r.raw!=0;
        auto line="motor_cell="+std::to_string(i)+"  support="+std::to_string(r.support)+"  input="+pattern(m,cue(m.circuit,i))+"  action="+pattern(m,output(m.circuit,i));
        if (Word gate=r.flags>>2) {
            auto &g=m.circuit.guards[gate-1];Word known_views=0,inputs=0;
            for (Word mask:g.known) known_views+=std::popcount(mask);
            for (Word eye=0;eye<8;++eye) for (Word v=0;v<256;++v) {
                int a=switch_to(m.circuit,i,v,true),b=switch_to(m.circuit,i,v^(1u<<eye),true);
                if (a>=0 && b>=0 && a!=b) {inputs|=1u<<eye;break;}
            }
            line+="  alternative="+pattern(m,{g.effect,g.length})+"  resolved_views="+std::to_string(known_views)+"  sensitive_channels="+std::to_string(inputs);
        }
        if (inspect) std::cout<<line<<'\n';else if (auto *j=journal::current()) j->line(line);
    }
    for (Word j=0;j<m.recursion.n;++j) {
        const auto &p=m.recursion.state[j];
        for (Word k=0;k<p.n;++k) {
            auto a=p.arm[k];auto motor=p.wire+a.at+a.n;
            Word left=__builtin_ctz(a.cuts);
            std::string line="recurrent_state="+std::to_string(j)+"  label="+pattern(m,{p.prefix,p.head})+"  terminal="+pattern(m,token(p,k))
                +"  first="+pattern(m,{motor,left})+"  deferred="+pattern(m,{motor+left+p.center,a.m-left-p.center})+"  alternatives="+std::to_string(std::popcount(a.cuts));
            if (inspect) std::cout<<line<<'\n';else if (auto *log=journal::current()) log->line(line);
        }
    }
    std::cout<<"procedural_cells="<<m.circuit.count<<"  recursive_productions="<<rules<<"  raw_bindings="<<raw<<"  recurrent_states="<<m.recursion.n<<"  acoustic_cells="<<m.ear.size
             <<"  observations="<<m.memory.observed<<"  code_bytes="<<m.circuit.bytes+m.circuit.count*sizeof(Cell)
             <<"  sensory_gates="<<m.circuit.guard_n<<"  decoder="<<(direction==1 ? "prefix" : direction==2 ? "suffix" : "chart")<<"  organ_bytes="<<sizeof(Mind)<<'\n';
}
}
MotorSpeech::MotorSpeech(Session &lab,const Mind &initial,bool present,DevicePtr shared):s(lab),memory(s.d,present && !shared ? sizeof(Mind) : 0),
    decoder(s.d,present ? sizeof(Decoder) : 0),body(shared ? shared : memory.ptr),populated(present) {
    if (present) {if (!shared) put(s,body,initial);Word n=1;run(s,"procedure_compile",1,1,body,decoder.ptr,n);}
}
bool MotorSpeech::hear(const std::string &line,std::string &answer,std::vector<Count> *echo) {
    if (!populated || line.empty()) return false;
    if (line.starts_with("/begin ") || line.starts_with("/step ")) {
        Temporary nodes(s.d,(performance_span+1)*sizeof(Node)),plans(s.d,(performance_span+1)*sizeof(Summary)),grid(s.d,4096*sizeof(Half));
        if (line.starts_with("/begin ")) {
            auto in=transmit(std::string_view(line).substr(7),true);if (in.empty() || in.size()>performance_span) {Word idle=0;put(s,body+offsetof(Mind,performance)+offsetof(Performance,active),idle);answer="[capacity]";return true;}
            Temporary waves(s.d,in.size()*sizeof(Count)),state(s.d,sizeof(Word));Word n=Word(in.size());
            s.d.check(s.d.upload(waves.ptr,in.data(),in.size()*sizeof(Count)),"hear a closed-loop command");
            run(s,"procedure_begin",1,1,body,decoder.ptr,waves.ptr,n,nodes.ptr,plans.ptr,grid.ptr,state.ptr);Word status=0;get(s,status,state.ptr);
            answer=status==recognized ? "[ready]" : status==capacity ? "[capacity]" : "?";return true;
        }
        auto value=trim(std::string_view(line).substr(6));Word open=value!="closed",v=open ? Word(quantity(value,0,255,16)) : 0;
        Temporary state(s.d,sizeof(Motion));run(s,"procedure_step",1,1,body,decoder.ptr,nodes.ptr,plans.ptr,grid.ptr,v,open,state.ptr);
        Motion a;get(s,a,state.ptr);answer=a.status==finished ? "[done]" : a.status==waiting ? "[observation needed]" : a.status==broken ? "[capacity]" : decode(a.emitted);
        if (auto *log=journal::current()) {log->metrics["procedural_moves"]+=a.status==moved;log->line("retinal_tick="+std::to_string(a.tick)+"  view="+std::to_string(v)+"  open="+std::to_string(open)+"  motor="+answer);}
        return true;
    }
    if (line.starts_with("/retina ")) {
        auto value=std::string_view(line).substr(8);Word retina=0,visible=value!="closed";
        if (visible) retina=Word(quantity(value,0,255,16));
        put(s,body+offsetof(Mind,retina),retina);put(s,body+offsetof(Mind,observing),visible);answer="_";return true;
    }
    if (line=="/ask-procedure") {
        Temporary question(s.d,sizeof(Inquiry));Word seed=0,random=0;Mind child;get(s,child,body);seed=mix(child.memory.observed+child.conversations);
        run(s,"procedure_ask",1,1,body,question.ptr,seed,random);Inquiry q;get(s,q,question.ptr);
        answer=q.length ? pattern(child,{q.signal,q.length})+"?" : "_";return true;
    }
    if (line.starts_with("/express ")) {
        Decoder d;get(s,d,decoder.ptr);auto r=express(s,body,decoder.ptr,d.direction,motors(std::string_view(line).substr(9)));
        answer=r.stats.status==recognized ? r.text : "?";return true;
    }
    if (line.starts_with("/imitate ")) {
        if (teach(s,body,std::string_view(line).substr(9),false,true) && !mark(s,body,1,false)) {
            Word one=1;run(s,"procedure_compile",1,1,body,decoder.ptr,one);
            if (auto *j=journal::current()) ++j->metrics["online_reconsolidations"];
        }
        answer="_";return true;
    }
    if (line=="/rehearse") {mark(s,body,1);Word n=1;run(s,"procedure_compile",1,1,body,decoder.ptr,n);answer="_";return true;}
    std::string utterance=line;Count from=0;Word wanted=1024;bool sample=line.starts_with("/excerpt ");
    if (sample) {
        std::istringstream fields(line.substr(9));std::string offset,n;
        if (!(fields>>offset>>n)) throw std::invalid_argument("/excerpt needs a start spike, count and utterance");
        from=quantity(offset,0,~0ull);wanted=Word(quantity(n,0,65536));std::getline(fields,utterance);
    } else if (line[0]=='/') return false;
    Decoder d;get(s,d,decoder.ptr);std::vector<Count> input;
    try {input=transmit(utterance,true);} catch (const std::invalid_argument &) {return false;}
    auto r=query(s,body,decoder.ptr,d.direction,input,from,wanted);
    if (r.stats.status==unrecognized) return false;
    if (r.stats.status!=recognized) {answer=r.stats.status==ambiguous ? "[ambiguous]" : r.stats.status==needs_observation ? "[unresolved view]" : "[capacity]";return true;}
    if (!sample && r.stats.voice.length>wanted) {answer="[motor sequence of "+std::to_string(r.stats.voice.length)+" spikes]";return true;}
    answer=r.text.empty() ? "_" : r.text;
    if (echo && !sample) {
        bool grounded=!r.waves.empty();for (Count wave:r.waves) {auto raw=acoustic::unwave(wave);auto spoken=transcribe(raw);grounded&=!spoken.empty() && sound_pattern(spoken)==raw;}
        if (grounded) {*echo=r.waves;return true;}
        std::istringstream words(r.text);std::string token;std::vector<Count> projected;
        try {while (words>>token) projected.push_back(acoustic::pulse(token));} catch (const std::invalid_argument &) {return true;}
        *echo=std::move(projected);
    }
    if (auto *j=journal::current()) {j->metrics["procedure_output"]+=r.stats.spoken;j->metrics["procedure_depth"]=std::max<Count>(j->metrics["procedure_depth"],r.stats.depth);}
    return true;
}
std::vector<Count> MotorSpeech::pending() {
    if (!populated) return {};
    Temporary voice(s.d,performance_span*sizeof(Count)),size(s.d,sizeof(Word));run(s,"procedure_pending",1,128,body,voice.ptr,size.ptr);
    Word n=0;get(s,n,size.ptr);std::vector<Count> result(n);
    if (n) s.d.check(s.d.download(result.data(),voice.ptr,n*sizeof(Count)),"hear the speaker’s retained procedure");
    return result;
}
bool MotorSpeech::consent(const std::vector<Count> &words,DevicePtr agency,DevicePtr concepts) {
    if (!populated || words.empty() || words.size()>512 || !agency || !concepts) return false;
    Temporary voice(s.d,words.size()*sizeof(Count)),result(s.d,sizeof(Word));Word n=Word(words.size());
    s.d.check(s.d.upload(voice.ptr,words.data(),words.size()*sizeof(Count)),"hear a request to carry out the score");
    run(s,"procedure_consent",1,1,body,agency,concepts,voice.ptr,n,result.ptr);Word ok=0;get(s,ok,result.ptr);return ok;
}
bool MotorSpeech::garden(const std::string &utterance,const std::string &site,bool execute,bool solve,DevicePtr agency,DevicePtr concepts,
                       DevicePtr peer,DevicePtr peer_concepts,DevicePtr plots,Word eyes,Word allowed,const std::vector<Count> *incoming) {
    if (!populated || !agency) return false;
    PolicyWorkspace w(s,body,decoder.ptr,agency,allowed);
    if (solve) {
        std::istringstream words(utterance);std::string word,source,span,extra;words>>word>>source>>span;
        if (word.empty() || (words>>extra)) {std::cout<<"animal> ?\n";return true;}
        Count name=sound_pattern(word),starts=source.empty() ? ~0ull : quantity(source,1,~0ull,16);
        Word horizon=span.empty() ? 3 : Word(quantity(span,1,max_steps));
        Temporary target(s.d,sizeof(Count));run(s,"procedure_desire",1,1,agency,name,target.ptr);Count goals=0;get(s,goals,target.ptr);
        if (!goals) {std::cout<<"animal> ?\n";return true;}
        auto phrase=w.devise(goals,starts,horizon,1);std::string sound;for (auto v:phrase) sound+=decode(v);
        if (phrase.empty() || w.compile(phrase)!=recognized) {Word idle=0;put(s,body+offsetof(Mind,performance)+offsetof(Performance,active),idle);std::cout<<"animal> "<<(w.resting ? "_" : "?")<<std::endl;return true;}
        s.d.check(s.d.copy_device(body+offsetof(Mind,performance),w.performance.ptr,sizeof(Performance)),"retain the self-generated score");
        std::cout<<"animal> "<<sound<<std::endl;return true;
    }
    GardenFocus f;f.execute=execute;f.eyes=eyes;f.enabled=allowed;
    if (site.empty()) return false;
    if (site[0]>='0' && site[0]<='9') f.site=Word(quantity(site,0,31));
    else {f.name=sound_pattern(site);if (!f.name) return false;}
    bool recalled=!incoming && trim(utterance).empty();
    std::vector<Count> waves;try {waves=incoming ? *incoming : recalled ? pending() : transmit(utterance,true);} catch (const std::invalid_argument &) {return false;}
    Word status=w.compile(waves);if (status!=recognized) return false;
    Temporary result(s.d,sizeof(GroundReply));run(s,"procedure_settle",1,1,body,decoder.ptr,w.plant.ptr,w.nodes.ptr,w.performance.ptr,w.root,agency,concepts,
                                                 peer,peer_concepts,plots,f,result.ptr);
    GroundReply r;get(s,r,result.ptr);
    if (recalled && execute && !r.stop) {Word idle=0;put(s,body+offsetof(Mind,performance)+offsetof(Performance,active),idle);}
    std::string path;for (Word j=0;j<r.motion.samples;++j) {if (j) path+=',';path+=std::to_string(r.motion.trajectory[j]);}
    std::cout<<"grounded_procedure="<<(execute ? "performed" : "imagined")<<"  site="<<r.motion.subject<<"  start="<<r.start<<"  expected="<<r.expected
             <<"  predicted_moves="<<r.predicted_moves<<"  actions="<<r.motion.actions<<"  final="<<(r.motion.actions ? r.motion.after : r.start)<<"  stopped="<<r.stop
             <<"  recalled="<<recalled<<"  heard_from_peer="<<bool(incoming)<<"  path="<<path<<"  phrase="<<utterance<<std::endl;
    if (auto *j=journal::current()) {j->metrics["procedural_moves"]+=r.motion.actions;j->metrics["procedural_forecasts"]++;}return true;
}
void MotorSpeech::save(Mind &image) {if (body) get(s,image,body);}
int command(int argc,char **argv) {
    if (argc==3 && std::string_view(argv[2])=="--help") {
        std::cout<<"axion imitate [--load MIND] [--save MIND] [--lesson FILE|-] [--say UTTERANCE] [--raw] [--inspect-code]\n"
                   "  --generations N --population N --origin inherited|random|inert|permissive --selection select|drift\n"
                   "  --journeys none|mixed|sparse --inquiries N --attention disagreement|random|yoked --probes N --depth N --receptor-noise N\n"
                   "  --forecast PHRASE --goal-states HEX --start-states HEX --horizon N --senses open|closed --search preimage|exhaustive --beam N\n"
                   "  --lantern SEED --contacts N --contact-policy sweep|active --select-questions 0|1 --survey-rooms N --verify-lantern SEED --verify-room SEED --replicas N --families N --episodes N --guards N --cases N\n"
                   "  --mentor MIND --express MOTORS --invent N --chunk N --samples FILE|- --tempo N\n"
                   "  --colony N --seasons N --days N --wire open|mute|reversed|stale|fixed-gait|rote|fixed-attention|immediate-replay --culture local|isolated|apparatus\n"
                   "  --drills N --storm DAY --innovation 0|1 --save-roost FILE --resume-roost FILE\n"
                   "  --worlds N --rounds N --seed N --dialect english|prefix|suffix --noise N --from-spike N --limit N --chart\n";return 0;
    }
    auto o=parse(argc,argv);AgentImage image;if (o.resume.empty() || !o.save.empty()) image.read(o.source);
    if ((o.select_questions || o.survey) && !image.causal) throw std::invalid_argument("selecting causal question priorities requires a learned intervention model");
    Session lab;lab.open();auto v=english();
    Word n=o.lesson.empty() && (!o.asked() || !image.procedural) ? o.worlds : 1;
    Temporary brain(lab.d,Count(n)*sizeof(Mind)),decode_table(lab.d,Count(n)*sizeof(Decoder)),world(lab.d,sizeof(v)),g(lab.d,sizeof(RecollectionGenes));
    put(lab,world.ptr,v);
    if (image.procedural && (!o.lesson.empty() || o.asked())) {
        if (!o.explicit_genes) o.origin=0;
        if (!o.explicit_generations) o.generations=0;
    }
    auto genes=o.resume.empty() ? evolve(lab,o,world.ptr,image.procedural ? image.practice.genes : image.recollection.genes,image.agency.model.genes) : RecollectionGenes{};
    auto began=std::chrono::steady_clock::now();
    std::cout<<lab.name<<"  procedural_rule="<<std::hex<<genes.low<<'.'<<genes.high<<'.'<<genes.expression<<std::dec<<'\n';
    if (auto *j=journal::current()) {j->source=o.resume.empty() ? o.source : o.resume;j->device=lab.name;}
    if (o.colony || !o.resume.empty()) {
        roost_life(lab,o,image,world.ptr,genes);if (!o.save.empty()) write_checked(o.save,image);return 0;
    }
    put(lab,g.ptr,genes);
    if (!o.mentor.empty()) {
        AgentImage elder;elder.read(o.mentor);if (!elder.procedural) throw std::invalid_argument("the mentor has no acquired motor circuit");
        Temporary adult(lab.d,sizeof(Mind));put(lab,adult.ptr,elder.practice);Word cut=0;
        run(lab,"procedure_mentor",(n+31)/32,32,brain.ptr,adult.ptr,genes,n,o.seed,cut);mark(lab,brain.ptr,n,false);
        Temporary report(lab.d,n*2*sizeof(Word));run(lab,"procedure_transmitted",(n+31)/32,32,brain.ptr,adult.ptr,n,report.ptr);
        std::vector<Word> result(n*2);lab.d.check(lab.d.download(result.data(),report.ptr,result.size()*sizeof(Word)),"compare acquired cultural procedures");
        Word faithful=0;Count cells=0,states=0;for (Word i=0;i<n;++i) {cells+=result[2*i];states+=result[2*i+1];faithful+=result[2*i]==elder.practice.circuit.count && result[2*i+1]==elder.practice.recursion.n;}
        std::cout<<"mentored="<<n<<"  faithful="<<faithful<<"  matched_productions="<<cells<<"  available="<<Count(n)*elder.practice.circuit.count
                 <<"  recurrent="<<states<<"  available_states="<<Count(n)*elder.practice.recursion.n<<'\n';
    } else if (!o.lesson.empty()) {
        Mind child=image.procedural && !o.fresh ? image.practice : Mind{};child.genes=genes;put(lab,brain.ptr,child);
        std::ifstream file;std::istream *in=&std::cin;if (o.lesson!="-") {file.open(o.lesson);if (!file) throw std::runtime_error("cannot open acoustic lesson "+o.lesson);in=&file;}
        for (std::string line;std::getline(*in,line);) if (!trim(line).empty()) teach(lab,brain.ptr,line,o.raw);
        mark(lab,brain.ptr,n);
    } else if ((o.asked()) && image.procedural) {image.practice.genes=genes;put(lab,brain.ptr,image.practice);}
    else {
        run(lab,"procedure_rear",(n+31)/32,32,brain.ptr,world.ptr,g.ptr,n,n,o.seed,o.style,o.rounds,o.noise,o.journeys,o.tempo,o.guards);
        Word clipped=mark(lab,brain.ptr,n,false);
        if (clipped) std::cout<<"clipped_circuits="<<clipped<<"  learners="<<n<<'\n';
        if (o.inquiries) {
            Temporary counts(lab.d,n*sizeof(Word));Count twin_cost=0;
            if (o.random==2) {
                Temporary twins(lab.d,Count(n)*sizeof(Mind));lab.d.check(lab.d.copy_device(twins.ptr,brain.ptr,Count(n)*sizeof(Mind)),"duplicate the pre-question learners");
                Word directed=0;run(lab,"procedure_tuition",(n+31)/32,32,twins.ptr,world.ptr,n,o.seed,o.style,o.inquiries,directed,counts.ptr);
                std::vector<Word> quota(n);lab.d.check(lab.d.download(quota.data(),counts.ptr,n*sizeof(Word)),"read paired question costs");
                for (Word v:quota) twin_cost+=v;
            }
            run(lab,"procedure_tuition",(n+31)/32,32,brain.ptr,world.ptr,n,o.seed,o.style,o.inquiries,o.random,counts.ptr);
            std::vector<Word> spent(n);lab.d.check(lab.d.download(spent.data(),counts.ptr,n*sizeof(Word)),"count requested demonstrations");
            Count paid=0;for (Word x:spent) paid+=x;std::cout<<"procedure_tuition="<<(o.random==2 ? "yoked" : o.random ? "random" : "disagreement")<<"  requested="<<paid<<"  matched_budget="<<twin_cost<<"  worlds="<<n<<'\n';
        }
    }
    if (o.drills && o.lesson.empty() && !o.asked()) {
        Temporary gait(lab.d,Count(n)*sizeof(roost::GaitScore));Word test=3072;
        calibrate(lab,brain.ptr,world.ptr,gait.ptr,n,n,o.seed,o.style,o.drills,test);
        std::vector<roost::GaitScore> sizes(n);lab.d.check(lab.d.download(sizes.data(),gait.ptr,n*sizeof(sizes[0])),"read reactive motor tests");
        Count correct=0,perfect=0,features=0;for (auto a:sizes) {correct+=a.correct;perfect+=a.correct==test;features+=a.sensor_pair;}
        std::cout<<"motor_calibration="<<n<<"  explorations="<<Count(n)*o.drills<<"  correct="<<correct<<"  choices="<<Count(n)*test<<"  exact="<<perfect<<"  inferred_inputs="<<features<<'\n';
        if (auto *j=journal::current()) j->scores.push_back({"Reactive motor choices",correct,Count(n)*test});
    }
    if (o.coins) {
        Temporary proposals(lab.d,Count(n)*sizeof(Invention));
        for (Word k=0;k<o.coins;++k) {
            Word seed=mix(o.seed+k);run(lab,"procedure_innovate",(n+31)/32,32,brain.ptr,proposals.ptr,n,seed,o.chunk);
            Invention proposal;get(lab,proposal,proposals.ptr);Mind animal;get(lab,animal,brain.ptr);
            if (proposal.length) std::cout<<"invented_call="<<pattern(animal,{proposal.sign,proposal.length})<<"  movement="<<pattern(animal,{proposal.movement,proposal.actions})
                <<"  recurrences="<<proposal.exposures<<"  description_saving="<<proposal.saving<<'\n';
        }
    }
    run(lab,"procedure_compile",(n+31)/32,32,brain.ptr,decode_table.ptr,n);Decoder d;get(lab,d,decode_table.ptr);
    image.procedural=true;get(lab,image.practice,brain.ptr);image.practice.extent=sizeof(Mind);validate(image.practice);
    auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
    std::cout<<"acquired_ms="<<elapsed<<'\n';describe(image.practice,d.direction,o.inspect);
    if (auto *j=journal::current()) {j->source=o.source;j->device=lab.name;j->metrics["animals"]=n;}
    if (o.mentor.empty() && o.lesson.empty() && !o.asked() && o.probes) examine(lab,o,brain.ptr,decode_table.ptr,world.ptr,n);
    if (o.mentor.empty() && o.guards && o.cases && o.lesson.empty() && !o.asked()) {
        Temporary episodes(lab.d,Count(n)*sizeof(apprenticeship::PerformanceCase)),scores(lab.d,n*sizeof(SceneCheck));
        run(lab,"procedure_concerts",(n+31)/32,32,brain.ptr,decode_table.ptr,world.ptr,episodes.ptr,n,n,o.seed,o.style);
        for (Word cut=0;cut<2;++cut) {
            run(lab,"procedure_contingencies",n,128,brain.ptr,decode_table.ptr,world.ptr,episodes.ptr,scores.ptr,n,n,o.seed,o.style,o.cases,cut);
            std::vector<SceneCheck> measured(n);lab.d.check(lab.d.download(measured.data(),scores.ptr,n*sizeof(SceneCheck)),"read closed-loop motor trajectories");
            Count correct=0,total=0,pulses=0,expected=0,changed=0,sensitive=0,exact=0;
            for (auto s:measured) {correct+=s.all;total+=s.trials;pulses+=s.pulses;expected+=s.expected;changed+=s.changes;sensitive+=s.sensitive;exact+=s.all==s.trials;}
            std::cout<<"sensorimotor="<<(cut ? "held-view" : "live")<<"  exact_worlds="<<exact<<"  paths="<<total<<"  correct="<<correct
                     <<"  pulses="<<pulses<<"  expected="<<expected<<"  switched="<<sensitive<<"  correct_switches="<<changed<<'\n';
            if (auto *j=journal::current()) j->scores.push_back({cut ? "Held initial view" : "Live recurrent decisions",correct,total});
        }
    }
    if (!o.said.empty()) {
        auto answer=query(lab,brain.ptr,decode_table.ptr,d.direction,transmit(o.said,true,o.raw),o.start,o.limit,o.chart);
        std::cout<<"interpretation="<<answer.stats.status<<"  length="<<answer.stats.voice.length<<"  depth="<<answer.stats.depth
                 <<"  digest="<<std::hex<<answer.stats.voice.hash<<std::dec<<"  speech="<<answer.text<<'\n';
    }
    if (!o.sampled.empty()) {
        std::ifstream source;std::istream *in=&std::cin;
        if (o.sampled!="-") {source.open(o.sampled);if (!source) throw std::runtime_error("cannot open acoustic sample stream");in=&source;}
        Temporary state(lab.d,sizeof(Hearing)),frames(lab.d,256*sizeof(Count)),symbols(lab.d,32766*sizeof(Count)),received(lab.d,sizeof(Word));
        Hearing hearing;put(lab,state.ptr,hearing);std::vector<Count> chunk;std::string word;Count ticks=0;Word events=0;put(lab,received.ptr,events);
        auto consume=[&](Word final) {
            Word n=Word(chunk.size());if (n) lab.d.check(lab.d.upload(frames.ptr,chunk.data(),n*sizeof(Count)),"admit acoustic samples");
            run(lab,"procedure_listen",1,1,state.ptr,frames.ptr,n,image.practice.genes,symbols.ptr,received.ptr,final);
            chunk.clear();
        };
        while (*in>>word) {if (word.starts_with('@')) word.erase(0,1);chunk.push_back(quantity(word,0,~0ull,16));++ticks;if (chunk.size()==256) consume(0);}
        consume(1);get(lab,events,received.ptr);auto r=query_device(lab,brain.ptr,decode_table.ptr,d.direction,symbols.ptr,events,o.start,o.limit,o.chart);
        std::cout<<"continuous_frames="<<ticks<<"  detected_crests="<<events<<"  interpretation="<<r.stats.status<<"  length="<<r.stats.voice.length<<"  speech="<<r.text<<'\n';
    }
    if (!o.expressed.empty()) {
        auto answer=express(lab,brain.ptr,decode_table.ptr,d.direction,motors(o.expressed,o.raw));
        std::cout<<"description="<<answer.stats.status<<"  length="<<answer.stats.spoken<<"  speech="<<answer.text<<'\n';
    }
    if (o.survey) {
        constexpr Word stride=4*operations;Temporary records(lab.d,Count(o.survey)*stride*sizeof(Word));
        run(lab,"procedure_survey",o.survey*operations,32,image.practice.genes,image.agency.model.genes,o.seed,o.contacts,o.lamp_attention,records.ptr);
        std::vector<Word> scores(size_t(o.survey)*stride);lab.d.check(lab.d.download(scores.data(),records.ptr,scores.size()*sizeof(Word)),"measure withheld physical transitions");
        Count correct=0,held=0,settled=0,early=0;Word perfect=0;
        for (Word k=0;k<o.survey;++k) {
            bool exact=true;for (Word a=0;a<operations;++a) {auto v=scores.data()+k*stride+4*a;
                correct+=v[0];held+=v[1];settled+=v[2];early+=v[3]<o.contacts;exact&=v[0]==v[1] && v[1]!=0;}
            perfect+=exact;
        }
        std::cout<<"motor_survey="<<o.survey<<"  attention="<<(o.lamp_attention ? "active" : "sweep")<<"  contacts="<<Count(o.survey)*operations*o.contacts
                 <<"  inferred="<<correct<<"  withheld="<<held<<"  settled="<<settled<<"  perfect="<<perfect<<"  early_actuators="<<early<<'\n';
        if (auto *journal=journal::current()) journal->scores.push_back({"Unseen intervention outcomes",correct,held});
    }
    if (!o.forecast.empty() || o.target) {
        if (!image.causal) throw std::invalid_argument("forecasting motor effects needs a creature with acquired causal experience");
        Temporary agency(lab.d,sizeof(Agency));put(lab,agency.ptr,image.agency);
        if (o.calibrate_lantern) {
            Temporary record(lab.d,4*operations*sizeof(Word));run(lab,"procedure_lantern",operations,32,agency.ptr,o.lamp_seed,o.contacts,o.lamp_attention,image.practice.genes.expression,record.ptr);
            std::array<Word,4*operations> answers{};get(lab,answers,record.ptr);Word hits=0,total=0,settled=0;
            for (Word k=0;k<operations;++k) {hits+=answers[4*k];total+=answers[4*k+1];settled+=answers[4*k+2];}
            std::cout<<"lantern="<<o.lamp_seed<<"  attention="<<(o.lamp_attention ? "active" : "sweep")<<"  physical_contacts="<<operations*o.contacts<<"  unseen="<<total<<"  forecast="<<hits<<"  settled="<<settled<<'\n';
            get(lab,image.agency,agency.ptr);
        }
        PolicyWorkspace w(lab,brain.ptr,decode_table.ptr,agency.ptr);
        auto waves=o.target ? w.devise(o.target,o.starts,o.horizon,o.senses,o.exhaustive ? 0 : o.beam) : transmit(o.forecast,true,o.raw);
        std::string label;for (Count pulse:waves) label+=decode(pulse);
        Word status=w.resting ? recognized : waves.empty() ? unrecognized : w.compile(waves);
        std::cout<<"motor_forecast="<<status<<"  utterance="<<label<<'\n';
        if (status==recognized) {
            if (o.target) {lab.d.check(lab.d.copy_device(brain.ptr+offsetof(Mind,performance),w.performance.ptr,sizeof(Performance)),"keep the generated goal-directed score");
                get(lab,image.practice,brain.ptr);image.practice.extent=sizeof(Mind);}
            Transfer v;get(lab,v,w.root);Temporary encoded(lab.d,sizeof(CompactTransfer));Word packed=w.pack(encoded.ptr,v);Count supported=0,lo=~0ull,hi=0;std::string states;
            for (Word x=0;x<local_states;++x) {
                if (x) states+=',';
                states+=v.destination[x]<local_states ? std::to_string(v.destination[x]) : "?";
                if (v.destination[x]<local_states) {++supported;lo=std::min(lo,v.pulses[x]);hi=std::max(hi,v.pulses[x]);}
            }
            std::cout<<"projected_states="<<supported<<"  origins=64  minimum_moves="<<(supported ? lo : 0)<<"  maximum_moves="<<hi<<"  nodes="<<w.ast_nodes
                     <<"  root_bytes="<<sizeof(Transfer)<<"  packed_bytes="<<packed<<"  destinations="<<states<<'\n';
            if (o.audit) audit_world(w,v,o.room,o.verify_lantern,o.target,o.starts);
            if (o.families) family_scale(w,image.agency.model.genes,image.practice.genes,o.families,o.replicas,o.episodes,o.seed);
            else if (o.replicas) time_policies(w,v,o.replicas,o.episodes,o.seed,encoded.ptr,packed);
        }
    }
    image.specimen(o.save.empty() ? o.source : o.save,"focal");
    if (!o.save.empty()) write_checked(o.save,image);
    return 0;
}
}
