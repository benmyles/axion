#include "axion/journal.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
extern "C" const char axion_journal_script[];

namespace axion::journal {
namespace {
thread_local Report *active=nullptr;
namespace fs=std::filesystem;
void quoted(std::ostream &out,std::string_view s) {
    out<<'"';
    for (unsigned char c:s) {
        if (c=='"' || c=='\\') out<<'\\'<<c;
        else if (c<32) out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c)<<std::dec;
        else out<<c;
    }
    out<<'"';
}
template<class T,class F> void array(std::ostream &s,const T &items,F put) {
    bool first=true;s<<'[';
    for (const auto &value:items) { if (!first) s<<',';put(value);first=false; }
    s<<']';
}
void moment(std::ostream &s,const Moment &m) {
    s<<'['<<m.elapsed<<','<<m.tick<<','<<m.passage<<','<<m.alive<<','<<m.families<<','<<m.generation
     <<','<<m.births<<','<<m.deaths<<','<<m.moves<<','<<m.edits<<','<<m.grazed<<','<<m.bitten<<','<<m.signals<<']';
}
std::string encode(const Report &r) {
    std::ostringstream s;s<<"{\"version\":1,\"command\":";quoted(s,r.command);
    for (auto pair:{std::pair{"source",&r.source},{"habitat",&r.habitat},{"metabolism",&r.metabolism},{"device",&r.device},{"archive",&r.archive}}) {
        s<<',';quoted(s,pair.first);s<<':';quoted(s,*pair.second);
    }
    s<<",\"interviews\":";array(s,r.interviews,[&](Word turn){s<<turn;});
    s<<",\"perceptions\":";array(s,r.perceptions,[&](Word trace){s<<trace;});
    if (!r.commons_residents.empty()) {s<<",\"commons_residents\":";array(s,r.commons_residents,[&](const auto &a){array(s,a,[&](Word v){s<<v;});});}
    if (!r.viewpoints.empty()) {
        s<<",\"habitat_views\":";array(s,r.viewpoints,[&](const Viewpoint &v){
            s<<"{\"after\":"<<v.after<<",\"tick\":"<<v.tick<<",\"x\":"<<v.x<<",\"y\":"<<v.y<<",\"side\":"<<v.side<<",\"food\":";
            array(s,v.food,[&](Word x){s<<x;});s<<",\"water\":";array(s,v.water,[&](Word x){s<<x;});
            if (v.slot!=~0u) s<<",\"resident\":"<<v.slot<<",\"founder\":"<<v.founder<<",\"born\":"<<v.born<<",\"generation\":"<<v.generation;
            s<<'}';
        });
    }
    s<<",\"args\":";array(s,r.args,[&](const auto &v){quoted(s,v);});
    if (!r.specimens.empty()) {
        s<<",\"specimens\":";array(s,r.specimens,[&](const Specimen &p){
            s<<"{\"path\":";quoted(s,p.path);s<<",\"role\":";quoted(s,p.role);
            s<<",\"checksum\":\""<<std::hex<<p.checksum<<std::dec<<"\",\"senses\":";array(s,p.senses,[&](const auto &v){quoted(s,v);});
            s<<",\"circuits\":";array(s,p.circuits,[&](const auto &v){quoted(s,v);});s<<",\"measures\":{";
            bool first=true;for (const auto &[k,v]:p.measures) {if (!first) s<<',';first=false;quoted(s,k);s<<':'<<v;}
            s<<"}}";
        });
    }
    s<<",\"checkpoints\":";array(s,r.checkpoints,[&](const Checkpoint &file) {
        s<<"{\"path\":";quoted(s,file.path);
        s<<",\"saved\":"<<(file.saved ? "true" : "false")<<",\"checksum\":\""
         <<std::hex<<std::setw(16)<<std::setfill('0')<<file.checksum<<std::dec<<"\"}";
    });
    s<<",\"lines\":";array(s,r.lines,[&](const auto &v){quoted(s,v);});
    s<<",\"metrics\":{";bool first=true;
    for (const auto &[name,v]:r.metrics) { if (!first) s<<',';first=false;quoted(s,name);s<<':'<<v; }
    s<<"},\"side\":"<<r.side<<",\"resolution\":"<<r.resolution<<",\"points\":";array(s,r.points,[&](const auto &p){moment(s,p);});
    s<<",\"frames\":";array(s,r.frames,[&](const auto &f){
        s<<"{\"t\":";moment(s,f.time);s<<",\"residents\":";array(s,f.residents,[&](const auto &a){
            s<<'['<<a.slot<<','<<a.position<<','<<a.founder<<','<<a.born<<','<<a.generation<<','<<a.energy<<','<<a.body<<','<<a.muscles<<','<<a.heading<<','<<a.tape
             <<','<<a.expression<<','<<a.effectors_low<<','<<a.effectors_high<<','<<a.navigation_rule
             <<','<<a.temporal_rule.plastic<<','<<a.temporal_rule.charge<<','<<a.temporal_rule.attention<<']';
        });s<<'}';
    });
    s<<",\"tiles\":";array(s,r.tiles,[&](const auto &p){s<<'['<<p.food<<','<<p.alive<<','<<p.lineage<<']';});
    s<<",\"scores\":";array(s,r.scores,[&](const auto &v){s<<'[';quoted(s,v.name);s<<','<<v.correct<<','<<v.total<<']';});
    s<<",\"motors\":";array(s,r.motors,[&](auto v){s<<v;});s<<",\"actions\":";array(s,r.actions,[&](auto v){s<<v;});
    s<<",\"transitions\":";array(s,r.transitions,[&](auto v){s<<v;});
    s<<",\"conditional\":";array(s,r.conditional,[&](auto v){s<<v;});
    s<<",\"journeys\":";array(s,r.journeys,[&](const auto &v){s<<'['<<v.first<<','<<v.second<<']';});
    s<<",\"breeding\":";array(s,r.breeding,[&](const auto &v){s<<'['<<v.generation<<','<<v.animals<<','<<v.score.total<<','<<v.score.best<<']';});
    s<<'}';return s.str();
}
class Transcript : public std::streambuf {
    std::streambuf *original;Report &r;std::string pending;
    void take(const char *data,std::streamsize n) {
        while (n) {
            const auto *end=static_cast<const char *>(std::memchr(data,'\n',size_t(n)));
            auto length=end ? end-data : n;
            pending.append(data,size_t(std::min<std::streamsize>(length,4096-pending.size())));
            if (!end) return;
            r.line(std::move(pending));pending.clear();n-=length+1;data=end+1;
        }
    }
protected:
    std::streamsize xsputn(const char *p,std::streamsize n) override { auto written=original->sputn(p,n);take(p,written);return written; }
    int overflow(int c) override {
        if (c==traits_type::eof()) return traits_type::not_eof(c);
        char b=char(c);return xsputn(&b,1)==1 ? c : traits_type::eof();
    }
    int sync() override { return original->pubsync(); }
public:
    explicit Transcript(Report &report):original(std::cout.rdbuf()),r(report){std::cout.rdbuf(this);}
    ~Transcript(){std::cout.rdbuf(original);if (!pending.empty()) r.line(std::move(pending));}
};
fs::path root_dir() {
    if (const char *p=std::getenv("AXION_JOURNAL_DIR");p && *p) return fs::absolute(p);
    std::error_code ec;auto binary=fs::read_symlink("/proc/self/exe",ec);
    if (!ec && fs::exists(binary.parent_path()/".git") && fs::exists(binary.parent_path()/"include/axion")) return binary.parent_path()/"journal";
    return fs::absolute("journal");
}
int renderer(const fs::path &root,std::vector<std::string> args,const std::string *payload) {
    fs::create_directories(root);
    fs::path receipt;
    struct QueueInput {int fd=-1;~QueueInput(){if (fd>=0) close(fd);}} source;
    if (payload) {
        std::string path=(root/".write-XXXXXX").string();int fd=mkstemp(path.data());
        if (fd<0) throw std::runtime_error("cannot allocate a journal receipt: "+std::string(std::strerror(errno)));
        std::FILE *file=fdopen(fd,"wb");
        if (!file) {close(fd);unlink(path.c_str());throw std::runtime_error("cannot open journal receipt");}
        bool written=std::fwrite(payload->data(),1,payload->size(),file)==payload->size() && std::fflush(file)==0 && fsync(fd)==0;
        if (std::fclose(file)!=0 || !written) {
            unlink(path.c_str());throw std::runtime_error("cannot write journal evidence");
        }
        source.fd=open(path.c_str(),O_RDONLY|O_CLOEXEC);
        if (source.fd<0) {unlink(path.c_str());throw std::runtime_error("cannot read the completed journal receipt");}
        // The renderer only reads receipts whose entire payload is on disk.
        // A hard link publishes the evidence without replacing a queued run.
        receipt=root/(".receipt-"+std::to_string(getpid())+"-"+fs::path(path).filename().string().substr(7));
        for (unsigned i=0;link(path.c_str(),receipt.c_str())<0;++i) {
            if (errno!=EEXIST) {unlink(path.c_str());throw std::runtime_error("cannot queue journal evidence");}
            receipt=root/(".receipt-"+std::to_string(getpid())+"-"+fs::path(path).filename().string().substr(7)+"-"+std::to_string(i));
        }
        unlink(path.c_str());
        // Another renderer can publish this run before our child starts.
        // The open file retains the exact bytes and identity of our receipt.
        args.push_back(receipt.filename().string());args.push_back(std::to_string(source.fd));
    }
    std::string folder=root.string(),binary=fs::read_symlink("/proc/self/exe").string();
    // Linux also limits each individual exec argument. Present the embedded
    // renderer as an anonymous readable file, independent of its source size.
    QueueInput script;script.fd=memfd_create("axion-journal",MFD_CLOEXEC);
    if (script.fd<0) throw std::runtime_error("cannot open embedded journal renderer");
    const char *part=axion_journal_script;size_t remaining=std::strlen(part);
    while (remaining) {
        ssize_t sent=write(script.fd,part,remaining);
        if (sent<0 && errno==EINTR) continue;
        if (sent<=0) throw std::runtime_error("cannot prepare embedded journal renderer");
        part+=sent;remaining-=size_t(sent);
    }
    std::string program="/proc/self/fd/"+std::to_string(script.fd);
    std::vector<char *> words{const_cast<char *>("python3"),program.data(),folder.data(),binary.data()};
    for (auto &arg:args) words.push_back(arg.data());
    words.push_back(nullptr);
    std::cout.flush();
    pid_t child=fork();
    if (child==0) {
        if (fcntl(script.fd,F_SETFD,0)<0) _exit(127);
        if (source.fd>=0 && fcntl(source.fd,F_SETFD,0)<0) _exit(127);
        execvp(words[0],words.data());_exit(127);
    }
    if (child<0) throw std::runtime_error("cannot start the journal renderer");
    int status=0;while (waitpid(child,&status,0)<0) if (errno!=EINTR) throw std::runtime_error("cannot wait for journal renderer");
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0)
        throw std::runtime_error("journal rendering failed in "+folder+(payload ? ". Run axion journal rebuild to publish queued observations." : ""));
    return 0;
}
}
Report *current(){return active;}
void checkpoint(const std::string &path,Count checksum,bool saved) {
    if (!active) return;
    for (const auto &file:active->checkpoints)
        if (file.path==path && file.checksum==checksum && file.saved==saved) return;
    active->checkpoints.push_back({path,checksum,saved});
}
void Report::observe(Moment p) {
    metrics["peak_alive"]=std::max(metrics["peak_alive"],p.alive);
    metrics["peak_depth"]=std::max(metrics["peak_depth"],p.generation);
    if (!points.empty() && points.back().passage==p.passage) {
        const auto &last=points.back();
        metrics["interval_births"]+=p.births-last.births;
        metrics["interval_deaths"]+=p.deaths-last.deaths;
        metrics["interval_moves"]+=p.moves-last.moves;
        metrics["interval_edits"]+=p.edits-last.edits;
        metrics["interval_grazed"]+=p.grazed-last.grazed;
    }
    if (!points.empty() && points.back().elapsed==p.elapsed && points.back().passage==p.passage) {points.back()=p;return;}
    if (points.size()>=256) {
        size_t count=1;for (size_t i=2;i<points.size();i+=2) points[count++]=points[i];points.resize(count);
    }
    points.push_back(p);
}
void Report::action(Word motor,Word pins,Word event) {
    ++metrics["events"];if (motor<8) ++motors[motor];
    Word record=(motor&15)|((pins&255)<<4)|(event<<12);
    if (actions.size()<192) actions.push_back(record);
    else actions[96+((metrics["events"]-1)%96)]=record;
}
void Report::turn(Word words,Word first,Word second,Word request,Word view,Word sound,Word shutters,Word dim,Word retinal,Word attention) {
    metrics["human_words"]+=words;auto ordinal=++metrics["human_turns"];
    interviews.push_back(first|(second<<3)|(request<<6)|(view<<9)|(sound<<11)|(std::min(words,0x3ffffu)<<14));
    perceptions.push_back((shutters&3)|((dim&3)<<2)|((retinal&15)<<4)|(attention<<8));
    line("interview=turn  turn="+std::to_string(ordinal)+"  words="+std::to_string(words)+"  first="+std::to_string(first)
         +"  second="+std::to_string(second)+"  request="+std::to_string(request)+"  view="+std::to_string(view)+"  sound="+std::to_string(sound));
}
void Report::line(std::string entry) {
    if (entry.empty()) return;
    auto pos=entry.find('=');auto label=entry.substr(0,pos);
    if (command=="agent" && pos!=std::string::npos && (label=="probe" || label=="orient" || label=="travel" || label=="sample" || label=="seek" || label=="taste" || label=="savour" || label=="step" || label=="trial" || label=="stride")) return;
    if (lines.size()>=(command=="mind"?2048u:(command=="floodplain"?512u:160u))) lines.erase(lines.begin()+32);
    lines.push_back(std::move(entry));
}
int execute(int argc,char **argv,const std::function<int(int,char **)> &command) {
    if (argc<2) return command(argc,argv);
    fs::path folder=root_dir();std::vector<char *> words{argv[0]};Report r;
    for (int i=1;i<argc;++i) {
        if (std::string_view(argv[i])=="--journal-dir") {
            if (++i>=argc) throw std::invalid_argument("--journal-dir needs a directory");
            folder=fs::absolute(argv[i]);
        } else words.push_back(argv[i]);
    }
    if (words.size()<2) return command(int(words.size()),words.data());
    int n=int(words.size());words.push_back(nullptr);r.command=words[1];
    for (int i=1;i<n;++i) r.args.emplace_back(words[i]);
    if (r.command=="journal") return renderer(folder,std::vector<std::string>(r.args.begin()+1,r.args.end()),nullptr);
    if (std::find(r.args.begin(),r.args.end(),"--help")!=r.args.end() || std::find(r.args.begin(),r.args.end(),"--inspect")!=r.args.end() || r.command=="--help") return command(n,words.data());
    if (const char *archive=std::getenv("AXION_JOURNAL_ARCHIVE_KEY");archive && *archive) r.archive=archive;
    for (int i=2;i+1<n;++i) if (std::string_view(words[i])=="--load" || std::string_view(words[i])=="--from" || std::string_view(words[i])=="--resident" || std::string_view(words[i])=="--founders") r.source=words[i+1];
    struct Scope { Scope(Report &v){active=&v;}~Scope(){active=nullptr;} };
    int result;
    auto start=std::chrono::steady_clock::now();
    { Scope scope(r);Transcript tee(r);result=command(n,words.data()); }
    r.metrics["command_wall_ns"]=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
    if (r.command=="talk" || std::find(r.args.begin(),r.args.end(),"-")!=r.args.end()) r.metrics["interactive_wall"]=1;
    if (!result) { auto data=encode(r);renderer(folder,{"record"},&data); }
    return result;
}
}
