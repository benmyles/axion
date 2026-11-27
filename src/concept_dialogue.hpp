#pragma once
// Included in the host dialogue implementation beside its CUDA session helpers.
class ConceptConversation {
    Session &s;DevicePtr witness,peer;Temporary output;
    std::optional<Temporary> sounds;size_t capacity=0;
    Function encounter{},speak{};Word eyes=3,shared=1;
    static bool number(std::istream &in,Word &v,Word upper=31) {
        std::string token;if (!(in>>token)) return false;
        auto [end,err]=std::from_chars(token.data(),token.data()+token.size(),v);
        return err==std::errc{} && end==token.data()+token.size() && v<=upper;
    }
    static Count form(std::istream &in) {std::string text;in>>text;return sound_pattern(text);}
public:
    Word visible() const {return eyes;}
    Word audible() const {return shared;}
    ConceptConversation(Session &session,Partner &speaker,Partner &listener,Word pair):s(session),
        witness(speaker.concepts.ptr ? speaker.concepts.ptr+Count(pair)*sizeof(Concepts) : 0),
        peer(listener.concepts.ptr ? listener.concepts.ptr+Count(pair)*sizeof(Concepts) : 0),output(s.d,witness ? sizeof(ConceptReply) : 0) {
        if (witness) {s.bind(encounter,"concept_encounter",Image::dialogue);s.bind(speak,"concept_speech",Image::dialogue);}
    }
    bool control(const std::string &line) {
        if (line.empty() || line[0]!='/') return false;
        std::istringstream in(line);std::string command,extra;in>>command;
        if (command=="/gaze" || command=="/earshot") {
            std::string view;in>>view;
            if (command=="/earshot" && (view=="shared" || view=="private") && !(in>>extra)) shared=view=="shared";
            else if (command=="/gaze" && (view=="both" || view=="witness" || view=="peer" || view=="neither") && !(in>>extra))
                eyes=view=="both" ? 3 : view=="witness" ? 1 : view=="peer" ? 2 : 0;
            else {std::cout<<"Use /gaze both, witness, peer or neither, or /earshot shared or private.\n"<<std::flush;return true;}
            std::cout<<"Board access changed.\n"<<std::flush;return true;
        }
        ConceptInput input{};input.eyes=eyes;bool valid=true;
        if (command=="/board") {
            input.event=ConceptEvent::board;valid=number(in,input.a,32) && input.a;
            if (in>>extra) {valid&=extra=="fresh";input.b=1;}
        } else if (command=="/name" || command=="/ask" || command=="/ask-random") {
            input.event=command=="/name" ? ConceptEvent::name : ConceptEvent::ask;input.form=form(in);valid=bool(input.form);
            if (input.event==ConceptEvent::ask) input.b=command=="/ask-random";
            for (std::string item;in>>item;) {
                std::istringstream val(item);Word n=0;if (!number(val,n)) {valid=false;break;}input.a|=1u<<n;
            }
            valid&=input.event==ConceptEvent::ask || input.a!=0;
        } else if (command=="/reply") {
            input.event=ConceptEvent::reply;valid=number(in,input.a);
        } else if (command=="/cancel-question") {
            input.event=ConceptEvent::cancel;
        } else if (command=="/link") {
            input.event=ConceptEvent::link;valid=number(in,input.r,3) && number(in,input.a) && number(in,input.b,empty_link);
        } else if (command=="/map") {
            input.event=ConceptEvent::sheet;valid=number(in,input.r,3);
            for (std::string item;in>>item;) {
                std::istringstream value(item);Word y=0;if (input.a==32 || !number(value,y,empty_link)) {valid=false;break;}
                input.targets[input.a++]=static_cast<unsigned char>(y);
            }
        } else if (command=="/example" || command=="/view-example" || command=="/watch-answer") {
            input.event=command=="/example" ? ConceptEvent::example : command=="/view-example" ? ConceptEvent::scope : ConceptEvent::peer_answer;
            input.form=form(in);if (command!="/example") input.other=form(in);
            valid=input.form && (command=="/example" || input.other) && number(in,input.a) && (command=="/watch-answer" || number(in,input.b));
        } else if (command=="/verdict") {
            input.event=ConceptEvent::verdict;valid=number(in,input.a,2) && (input.form=form(in));
        } else return false;
        if (!witness || !valid || (in>>extra)) {std::cout<<"Invalid board stimulus or missing concept circuit.\n"<<std::flush;return true;}
        void *args[]{&witness,&peer,&input,&output.ptr};launch(s,encounter,1,args);
        ConceptReply response{};s.d.check(s.d.download(&response,output.ptr,sizeof(response)),"observe recruitment and speech");
        if (input.event==ConceptEvent::ask && response.handled) std::cout<<"animal asks> "<<transcribe(input.form)<<' '<<(response.sound ? transcribe(response.sound) : std::to_string(std::countr_zero(response.start)))<<'?';
        else {std::cout<<"board> "<<response.handled;if (response.sound) std::cout<<' '<<transcribe(response.sound);}
        std::cout<<'\n'<<std::flush;
        if (auto *j=journal::current()) {
            ++j->metrics["board_stimuli"];
            if (response.handled && input.event==ConceptEvent::example) ++j->metrics["route_examples"];
            if (response.handled && input.event==ConceptEvent::ask) ++j->metrics["concept_questions"];
        }
        return true;
    }
    bool respond(const std::string &line,bool assertion) {
        if (!witness || line.empty() || line.front()=='/') return false;
        std::string text=line;for (char &c:text) if (std::string_view("?.!,;:").find(c)!=std::string_view::npos) c=' ';
        std::istringstream in(text);std::vector<Count> forms;
        for (std::string token;in>>token;) if (auto sound=sound_pattern(token)) forms.push_back(sound);
        if (forms.empty()) return false;
        if (forms.size()>~0u) throw std::length_error("utterance exceeds the device index");
        if (capacity<forms.size()) {capacity=std::max(size_t(32),forms.size());sounds.emplace(s.d,capacity*sizeof(Count));}
        Word n=Word(forms.size()),statement=assertion;DevicePtr stimuli=sounds->ptr;
        s.d.check(s.d.upload(stimuli,forms.data(),forms.size()*sizeof(Count)),"present word pulses");
        void *args[]{&witness,&peer,&stimuli,&n,&statement,&shared,&output.ptr};launch(s,speak,1,args);
        ConceptReply response{};s.d.check(s.d.download(&response,output.ptr,sizeof(response)),"read the activated referent");
        if (!response.handled) return false;
        std::string voice=transcribe(response.sound);
        if (assertion && response.handled==2) std::cout<<"The witness heard the relation.\n";
        else if (response.handled==3) std::cout<<"The witness learned from the reply.\n";
        else if (response.handled==4) std::cout<<"The witness learned from the example.\n";
        else if (assertion) std::cout<<"The witness could not ground the statement.\n";
        else std::cout<<"animal> "<<(voice.empty() ? "(silence)" : voice)<<'\n';
        if (response.question) std::cout<<"animal asks> "<<transcribe(response.question)<<' '<<transcribe(response.subject)<<"?\n";
        if (auto *j=journal::current()) {
            auto t=++j->metrics["concept_turns"];j->metrics["concept_voiced"]+=bool(response.sound);
            j->metrics["concept_questions"]+=bool(response.question);
            j->metrics["spoken_route_examples"]+=response.handled==4;
            j->metrics["concept_buffer_bytes"]=sizeof(ConceptReply)+capacity*sizeof(Count);
            std::string route;
            for (Word k=0;k<=std::min(8u,response.hops);++k) {if (k) route+=',';route+=std::to_string(response.trace[k]);}
            j->line("concept_turn="+std::to_string(t)+"  assertion="+std::to_string(statement)+"  start="+std::to_string(response.start)
                    +"  views="+std::to_string(response.context)+"  steps="+std::to_string(response.steps)+"  cells="+std::to_string(response.cells)
                    +"  missing="+std::to_string(response.gap)+"  hops="+std::to_string(response.hops)+"  path="+route+"  speech="+voice+"  prompt="+line);
        }
        std::cout<<std::flush;return true;
    }
};
