#pragma once
// Host apparatus presents light, sound and motor events. Planning, speech,
// counterfactuals and physical actions all run inside the CUDA encounter.
class GardenConversation {
    Session &s;const ConceptConversation &access;
    DevicePtr a{},b{},c{},d{};Temporary world,result;std::optional<Temporary> audio;
    std::string focus;
    size_t capacity=0;Word enabled=63,reuse=1;Function advance{};bool captions=false;
    static bool number(std::istream &in,Word &out,Word maximum) {
        std::string text;if (!(in>>text)) return false;
        auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),out);
        return error==std::errc{} && end==text.data()+text.size() && out<=maximum;
    }
    static std::vector<Count> listen(std::istream &in) {
        std::vector<Count> words;std::string token;
        while (in>>token) {
            auto form=sound_pattern(token);if (form) words.push_back(form);
        }
        return words;
    }
    GardenReply present(GardenPort p,const std::vector<Count> &words) {
        if (words.size()>~0u) throw std::length_error("causal utterance exceeds device indices");
        p.eyes=access.visible();p.shared=access.audible();p.enabled=enabled;p.reuse=reuse;p.length=Word(words.size());
        DevicePtr sounds=0;
        if (!words.empty()) {
            if (words.size()>capacity) {capacity=std::max(size_t(16),words.size());audio.emplace(s.d,capacity*sizeof(Count));}
            sounds=audio->ptr;s.d.check(s.d.upload(sounds,words.data(),words.size()*sizeof(Count)),"present sound at the garden");
        }
        void *args[]{&a,&c,&b,&d,&world.ptr,&sounds,&p,&result.ptr};launch(s,advance,32,args);
        GardenReply r{};s.d.check(s.d.download(&r,result.ptr,sizeof(r)),"read the animal's speech and movements");return r;
    }
    static std::string sentence(const GardenReply &r) {
        std::string text;
        for (Word k=0;k<r.count;++k) {if (k) text+=' ';text+=transcribe(r.sounds[k]);}
        return text;
    }
    void record(const GardenReply &r,std::string_view phrase,Word event) {
        if (auto *j=journal::current()) {
            ++j->metrics["garden_events"];j->metrics["garden_actions"]+=r.actions;
            if (r.kind) ++j->metrics["garden_turns"];
            if (event==Word(GardenEvent::vocabulary)) captions=false;
            if (r.kind && !captions) {
                GroundedSignals speech{};s.d.check(s.d.download(&speech,a+offsetof(Agency,speech),sizeof(speech)),"read the acoustic labels for the journal");
                std::string names;
                for (Word property=0;property<traits;++property) {
                    if (property) names+=',';
                    std::string name="cell"+std::to_string(property);
                    for (Word k=0;k<32;++k) if (speech.forms[k] && speech.meanings[k]==(1u<<(property+6))) {name=transcribe(speech.forms[k]);break;}
                    names+=name;
                }
                j->line("garden_traits="+names);captions=true;
            }
            std::string path;for (Word i=0;i<r.samples;++i) {if (i) path+=',';path+=std::to_string(r.trajectory[i]);}
            j->line("garden_event="+std::to_string(event)+"  intent="+std::to_string(r.kind)+"  site="+std::to_string(r.subject)+"  before="+std::to_string(r.before)
                    +"  after="+std::to_string(r.after)+"  require="+std::to_string(r.require)+"  value="+std::to_string(r.value)+"  solved="+std::to_string(r.solved)
                    +"  branch="+std::to_string(r.branch)+"  repairs="+std::to_string(r.repairs)+"  values="+std::to_string(r.values)+"  motor="+std::to_string(r.selected)
                    +"  actions="+std::to_string(r.actions)+"  path="+path+"  speech="+sentence(r)+"  prompt="+std::string(phrase));
        }
    }
public:
    GardenConversation(Session &session,Partner &speaker,Partner &listener,Word pair,const ConceptConversation &views):
        s(session),access(views),world(s.d,speaker.agency.ptr ? sizeof(Apparatus) : 0),result(s.d,speaker.agency.ptr ? sizeof(GardenReply) : 0) {
        if (!world.ptr) return;
        a=speaker.agency.ptr+Count(pair)*sizeof(Agency);c=speaker.concepts.ptr+Count(pair)*sizeof(Concepts);
        if (listener.agency.ptr && listener.concepts.ptr) {b=listener.agency.ptr+Count(pair)*sizeof(Agency);d=listener.concepts.ptr+Count(pair)*sizeof(Concepts);}
        s.d.check(s.d.zero(world.ptr,0,sizeof(Apparatus)),"prepare the greenhouse");s.bind(advance,"agency_encounter",Image::causes);
    }
    bool procedure(procedure::MotorSpeech &motor,procedure::MotorSpeech &partner,const std::string &line) {
        if (!a || line.empty()) return false;
        std::istringstream text(line);std::string first,site,phrase; text>>first;
        if (first=="/focus-site") {
            std::string extra;if (!(text>>site) || (text>>extra)) {std::cout<<"Use /focus-site NAME, INDEX or off.\n";return true;}
            focus=site=="off" ? "" : site;std::cout<<"Focused site "<<site<<".\n";return true;
        }
        if (first=="/pass-score") {
            std::string wire,extra;text>>site>>wire;
            if (site.empty() || (text>>extra) || (!wire.empty() && wire!="open" && wire!="muted" && wire!="reversed")) {
                std::cout<<"Use /pass-score SITE open, muted or reversed.\n";return true;
            }
            auto waves=motor.pending();if (wire=="muted" || !access.audible()) waves.clear();
            if (wire=="reversed") std::reverse(waves.begin(),waves.end());
            Word eyes=((access.visible()&1)<<1)|(access.visible()>>1);
            if (waves.empty() || !partner.garden("",site,true,false,b,d,a,c,world.ptr,eyes,enabled,&waves)) std::cout<<"animal> ?"<<std::endl;
            return true;
        }
        bool explicit_use=first=="/perform" || first=="/preview" || first=="/devise";
        if (explicit_use) {
            if (first!="/devise") text>>site;
            std::getline(text,phrase);
        } else {if (line[0]=='/' || focus.empty()) return false;site=focus;phrase=line;}
        bool handled=motor.garden(phrase,site,first!="/preview",first=="/devise",a,c,b,d,world.ptr,access.visible(),enabled);
        if (explicit_use && !handled) std::cout<<"animal> ?"<<std::endl;
        return handled || explicit_use;
    }
    bool committed(procedure::MotorSpeech &motor,const std::vector<Count> &words) {
        if (focus.empty() || !motor.consent(words,a,c)) return false;
        return motor.garden("",focus,true,false,a,c,b,d,world.ptr,access.visible(),enabled);
    }
    bool control(const std::string &line) {
        if (line.empty() || line[0]!='/') return false;
        std::istringstream in(line);std::string name,extra;in>>name;
        GardenPort p{};bool valid=true;std::vector<Count> sounds;
        if (name=="/greenhouse" || name=="/lantern") {
            p.event=name=="/lantern" ? GardenEvent::lantern : GardenEvent::open;valid=number(in,p.a,32) && p.a && number(in,p.b,~0u);
        } else if (name=="/seed-bank") {
            std::string path;in>>std::quoted(path);if (!a || path.empty()) {std::cout<<"Invalid seed bank request.\n"<<std::flush;return true;}
            storage::SeedBank bank;bank.read(path);Word island=bank.header.focus;
            if (in>>std::ws && !in.eof()) valid=number(in,island,bank.header.count-1);
            if (!valid || (in>>extra)) {std::cout<<"Invalid seed bank island.\n"<<std::flush;return true;}
            s.d.check(s.d.upload(world.ptr,&bank.beds[island].field,sizeof(Apparatus)),"transplant the archived physical plot");
            p.event=GardenEvent::survey;
        } else if (name=="/plot" || name=="/read-plot") {
            p.event=name=="/plot" ? GardenEvent::place : GardenEvent::inspect;
            valid=number(in,p.a,31) && (name=="/read-plot" || number(in,p.b,63));
        } else if (name=="/diet") {
            p.event=GardenEvent::diet;Word x=0,y=0,lut=0;valid=number(in,x,5) && number(in,y,5) && number(in,lut,15);p.c=1+x+(y<<3)+(lut<<6);
        } else if (name=="/taste-plot") {
            p.event=GardenEvent::taste;valid=number(in,p.a,31);
        } else if (name=="/mechanism") {
            p.event=GardenEvent::change;Word left=0,right=0,lut=0;
            valid=number(in,p.a,5) && number(in,p.b,5) && number(in,left,5) && number(in,right,5) && number(in,lut,15);
            p.c=1+left+(right<<3)+(lut<<6);
        } else if (name=="/apply") {
            p.event=GardenEvent::contact;valid=number(in,p.a,5) && number(in,p.b,31);
        } else if (name=="/trial") {
            p.event=GardenEvent::transition;valid=number(in,p.a,5) && number(in,p.b,63) && number(in,p.c,63);
            if (in>>std::ws && !in.eof()) valid=valid && number(in,p.known_before,63) && number(in,p.known_after,63);
        } else if (name=="/practice") {
            p.event=GardenEvent::experience;valid=number(in,p.a,256) && number(in,p.b,~0u);
        } else if (name=="/cohear") {
            p.event=GardenEvent::vocabulary;valid=number(in,p.a,(1u<<26)-1) && p.a;sounds=listen(in);valid&=!sounds.empty();
        } else if (name=="/try-motor" || name=="/try-random") {
            p.event=GardenEvent::probe;p.b=name=="/try-random";valid=number(in,p.a,~0u) && number(in,p.c,63);
        } else if (name=="/goal-reuse") {
            std::string mode;in>>mode;if ((mode!="open" && mode!="cut") || (in>>extra)) std::cout<<"Use /goal-reuse open or cut.\n";
            else {reuse=mode=="open";std::cout<<"Goal inputs changed.\n";}std::cout<<std::flush;return true;
        } else if (name=="/allow-motors") {
            Word mask=0;if (number(in,mask,63) && !(in>>extra)) enabled=mask;
            else {std::cout<<"Use /allow-motors MASK from 0 through 63.\n"<<std::flush;return true;}
            std::cout<<"Available motors changed.\n"<<std::flush;return true;
        } else if (name=="/teach-reason") {
            p.event=GardenEvent::imitate;valid=number(in,p.a,63);sounds=listen(in);valid&=!sounds.empty() && sounds.size()<=8;
        } else if (name=="/teach-branch") {
            p.event=GardenEvent::branch;sounds=listen(in);valid=!sounds.empty() && sounds.size()<=8;
        } else if (name=="/show-choice") {
            p.event=GardenEvent::contrast;valid=number(in,p.a,63) && number(in,p.b,6) && number(in,p.c,63) && number(in,p.d,6);
            sounds=listen(in);valid&=!sounds.empty() && sounds.size()<=8;
        } else if (name=="/approve") {
            p.event=GardenEvent::category;std::string word;in>>word;sounds.push_back(sound_pattern(word));
            valid=sounds[0] && number(in,p.a,31) && number(in,p.b,1);
        } else if (name=="/pass-plan") {
            p.event=GardenEvent::relay;valid=number(in,p.a,31);std::string wire;in>>wire;
            valid&=wire=="open" || wire=="muted" || wire=="reversed";p.b=wire=="muted" ? 1 : wire=="reversed" ? 2 : 0;
        } else return false;
        if (!a || !valid || (in>>extra)) {std::cout<<"Invalid garden stimulus or missing agency.\n"<<std::flush;return true;}
        auto r=present(p,sounds);std::cout<<"garden> "<<r.handled;
        if (r.handled && r.subject<32) std::cout<<" site="<<r.subject<<" state="<<r.after<<" actions="<<r.actions;
        if (r.count) std::cout<<" speech="<<sentence(r);
        std::cout<<'\n'<<std::flush;
        if (r.handled) record(r,line,Word(p.event));
        return true;
    }
    bool heard(const std::vector<Count> &sounds,bool assertion,const std::string &line) {
        if (!a || sounds.empty()) return false;
        GardenPort p{};p.event=GardenEvent::speech;p.a=assertion;auto r=present(p,sounds);if (!r.handled) return false;
        if (r.kind==25) std::cout<<"The witness learned the description.\n";
        else std::cout<<"animal> "<<(r.count ? sentence(r) : "(silence)")<<'\n';
        std::cout<<std::flush;record(r,line,Word(p.event));return true;
    }
    bool respond(const std::string &line) {
        if (!a || line.empty() || line[0]=='/') return false;
        std::string sentence_in=line;for (char &v:sentence_in) if (std::string_view("?!.,;:").find(v)!=std::string_view::npos) v=' ';
        std::istringstream in(sentence_in);return heard(listen(in),line.back()=='.',line);
    }

};
