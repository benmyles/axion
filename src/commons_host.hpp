#pragma once
// Included beside the shared CUDA implementation of a detached spiking animal.
struct CommonsOptions {
    std::string source="examples/naturalist.mind",saved,newborn,saved_peer;
    Word families=128,copies=8,days=64,generations=1,seed=36931,rations=6,volatility=14,price=1;
    Word policy=0,choice=0,credit=1,soma=1,variation=0,drift=0,exposures=8,reserve=12,receipts=0,channel=0,hearing=1,kin=0,focused=0,optics=1511;
    Word geometry=2,joint=0,calories=3,shuffled=0,audit=0,projection=0;bool graphs=true;
    Word culture=0,tutoring=96,report_wire=0,report_cost=0,event_age=0,teachers=0;
    std::string recent="happened",earlier="earlier";
    std::string teacher,other_teacher;Word report_cache=1;bool local_tuition=true;
    Word route_depth=2,route_reuse=1,tidal_feedback=1;bool warp_routes=true;
    Word mentors=0,mentoring=16;bool renewing=false;
    Word flora=1,pollen=4,observers=1,garden_cache=1,garden_model=0,mixed_plots=1;
    std::string seed_bank,save_seeds;
};
CommonsOptions commons_options(int argc,char **argv) {
    CommonsOptions q;bool specified_source=false,specified_exposures=false,calories_set=false,reserve_set=false;
    for (int i=2;i<argc;++i) {
        std::string_view f=argv[i];if (++i>=argc) throw std::invalid_argument("missing value for "+std::string(f));
        std::string_view v=argv[i];
        if (f=="--load") {q.source=v;specified_source=true;}
        else if (f=="--save") q.saved=v;
        else if (f=="--save-newborn") q.newborn=v;
        else if (f=="--save-neighbor") q.saved_peer=v;
        else if (f=="--families") q.families=integer(v,1,65536);
        else if (f=="--replicates") q.copies=integer(v,1,4096);
        else if (f=="--days") q.days=integer(v,1,8192);
        else if (f=="--generations") q.generations=integer(v,1,4096);
        else if (f=="--seed") q.seed=integer(v,0,~0u);
        else if (f=="--rations") q.rations=integer(v,1,32);
        else if (f=="--volatility") q.volatility=integer(v,0,65536);
        else if (f=="--price") q.price=integer(v,0,16);
        else if (f=="--exposures") {q.exposures=integer(v,0,64);specified_exposures=true;}
        else if (f=="--reserve") {q.reserve=integer(v,1,128);reserve_set=true;}
        else if (f=="--receipts" && (v=="none" || v=="learned")) q.receipts=v=="learned";
        else if (f=="--channel" && (v=="private" || v=="open" || v=="muted" || v=="permuted" || v=="reports" || v=="estuary" || v=="garden")) q.channel=v=="private" ? 0 : v=="open" ? 1 : v=="muted" ? 2 : v=="permuted" ? 3 : v=="reports" ? 4 : v=="estuary" ? 5 : 6;
        else if (f=="--flora" && (v=="fixed" || v=="heritable" || v=="drift")) q.flora=v=="fixed" ? 0 : v=="heritable" ? 1 : 2;
        else if (f=="--pollen") q.pollen=integer(v,0,65536);
        else if (f=="--observe-workers" && (v=="yes" || v=="no")) q.observers=v=="yes";
        else if (f=="--crop-policy" && (v=="cached" || v=="rebuild")) q.garden_cache=v=="cached";
        else if (f=="--crop-model" && (v=="intact" || v=="episodic" || v=="one-beat" || v=="blind")) q.garden_model=v=="intact" ? 0 : v=="episodic" ? 1 : v=="one-beat" ? 2 : 3;
        else if (f=="--plots" && (v=="mixed" || v=="dormant")) q.mixed_plots=v=="mixed";
        else if (f=="--plant-seeds") q.seed_bank=v;
        else if (f=="--save-seeds") q.save_seeds=v;
        else if (f=="--route-depth") q.route_depth=integer(v,1,3);
        else if (f=="--reuse" && (v=="open" || v=="cut")) q.route_reuse=v=="open";
        else if (f=="--tides" && (v=="harvest" || v=="clock")) q.tidal_feedback=v=="harvest";
        else if (f=="--route-layout" && (v=="warp" || v=="direct")) q.warp_routes=v=="warp";
        else if (f=="--mentors" && (v=="silent" || v=="local" || v=="displaced")) q.mentors=v=="silent" ? 0 : v=="local" ? 1 : 2;
        else if (f=="--mentoring") q.mentoring=integer(v,0,64);
        else if (f=="--currents" && (v=="local" || v=="novel")) q.renewing=v=="novel";
        else if (f=="--culture" && (v=="adult" || v=="invent" || v=="none")) q.culture=v=="adult" ? 0 : v=="invent" ? 1 : 2;
        else if (f=="--teachers" && (v=="local" || v=="fixed" || v=="random")) q.teachers=v=="local" ? 0 : v=="fixed" ? 1 : 2;
        else if (f=="--tutoring") q.tutoring=integer(v,0,512);
        else if (f=="--teacher") q.teacher=v;
        else if (f=="--other-teacher") q.other_teacher=v;
        else if (f=="--report-cache" && (v=="on" || v=="off")) q.report_cache=v=="on";
        else if (f=="--tuition-layout" && (v=="direct" || v=="local")) q.local_tuition=v=="local";
        else if (f=="--report-cost") q.report_cost=integer(v,0,16);
        else if (f=="--event-age" && (v=="recent" || v=="earlier" || v=="mixed")) q.event_age=v=="recent" ? 0 : v=="earlier" ? 1 : 2;
        else if (f=="--report-wire" && (v=="open" || v=="muted" || v=="reversed" || v=="displaced")) q.report_wire=v=="open" ? 0 : v=="muted" ? 1 : v=="reversed" ? 2 : 3;
        else if (f=="--recent-cue") q.recent=v;
        else if (f=="--earlier-cue") q.earlier=v;
        else if (f=="--hearing" && (v=="plastic" || v=="fixed")) q.hearing=v=="plastic";
        else if (f=="--neighbors" && (v=="mixed" || v=="kin")) q.kin=v=="kin";
        else if (f=="--retina" && (v=="mixed" || v=="focused")) q.focused=v=="focused";
        else if (f=="--optical-seed") q.optics=integer(v,0,~0u);
        else if (f=="--optics" && (v=="shared" || v=="shuffled")) q.shuffled=v=="shuffled";
        else if (f=="--audit-calls" && (v=="on" || v=="off")) q.audit=v=="on";
        else if (f=="--landmarks" && (v=="aliased" || v=="distinct" || v=="coupled")) q.geometry=v=="distinct" ? 1 : v=="coupled" ? 3 : 2;
        else if (f=="--joint-readout" && (v=="intact" || v=="observed" || v=="muted")) q.projection=v=="observed" ? 1 : v=="muted" ? 2 : 0;
        else if (f=="--pods" && (v=="single" || v=="tandem")) q.joint=v=="tandem";
        else if (f=="--calories") {q.calories=integer(v,1,32);calories_set=true;}
        else if (f=="--policy" && (v=="evolved" || v=="harvest" || v=="investigate")) q.policy=v=="harvest" ? 1 : v=="investigate" ? 2 : 0;
        else if (f=="--choice" && (v=="learned" || v=="random")) q.choice=v=="random";
        else if (f=="--credit" && (v=="intact" || v=="frozen")) q.credit=v=="intact";
        else if (f=="--soma" && (v=="intact" || v=="muted")) q.soma=v=="intact";
        else if (f=="--founders" && (v=="identical" || v=="varied")) q.variation=v=="varied";
        else if (f=="--breeding" && (v=="mass" || v=="drift")) q.drift=v=="drift";
        else if (f=="--dispatch" && (v=="direct" || v=="graph")) q.graphs=v=="graph";
        else throw std::invalid_argument("invalid commons option "+std::string(f));
    }
    if (q.geometry==3 && !specified_source) q.source="examples/marsh-newborn.mind";
    if (q.channel==4 && !specified_source) q.source="examples/storyteller.mind";
    if (q.channel==5 && !specified_source) q.source="examples/cartographer.mind";
    if (q.channel==5 && !specified_exposures) q.exposures=0;
    if (q.channel==6) {
        if (!specified_source) q.source="examples/gardener.mind";
        if (!specified_exposures) q.exposures=0;
        if (!calories_set) q.calories=24;
        if (!reserve_set) q.reserve=48;
        if (q.joint) throw std::invalid_argument("a garden plot accepts one winning motor at a time");
    }
    Count n=Count(q.families)*q.copies;
    if (n%32 || n>1048576) throw std::invalid_argument("the commons needs full 32-resident islands, up to 1048576 animals");
    if (q.joint && !q.channel) throw std::invalid_argument("a tandem pod needs the split-view pair apparatus");
    std::set<std::filesystem::path> destinations;
    if (q.channel!=6 && (!q.seed_bank.empty() || !q.save_seeds.empty())) throw std::invalid_argument("seed banks belong to the greenhouse habitat");
    for (const auto &p:{q.saved,q.saved_peer,q.newborn,q.save_seeds}) if (!p.empty() && !destinations.insert(std::filesystem::absolute(p).lexically_normal()).second)
        throw std::invalid_argument("save experienced residents, neighbours and newborns to different files");
    return q;
}
struct CommonsGraph {
    GraphApi api{};Stream stream{};Graph graph{};GraphExec executable{};Event begin{},end{},school_begin{},school_end{};Driver &d;
    explicit CommonsGraph(Driver &driver):d(driver) {}
    ~CommonsGraph() {
        if (school_begin) d.event_free(school_begin);
        if (school_end) d.event_free(school_end);
        if (executable) api.destroy_exec(executable);
        if (graph) api.destroy(graph);
        if (stream) api.destroy_stream(stream);
        if (begin) d.event_free(begin);
        if (end) d.event_free(end);
    }
};
#include "orchard_host.hpp"
int commons_command(int argc,char **argv) {
    for (int i=2;i<argc;++i) if (std::string_view(argv[i])=="--help") {
        std::cout<<"axion commons  |  costly experiments and food competition among independently acting spiking residents\n\n"
                 <<"--load FILE --save FILE --save-newborn FILE --seed N\n"
                 <<"--families N --replicates N --days N --generations N --rations 1..32 --price 0..16 --volatility N\n"
                 <<"--policy evolved|harvest|investigate --choice learned|random --credit intact|frozen --soma intact|muted\n"
                 <<"--exposures N --reserve N --receipts none|learned  natal observations, initial energy and local trap feedback\n"
                 <<"--channel private|open|muted|permuted --hearing plastic|fixed --neighbors mixed|kin\n"
                 <<"--retina mixed|focused --optical-seed N --optics shared|shuffled   input during the first vocal beat\n"
                 <<"--landmarks aliased|distinct|coupled --pods single|tandem --calories N   mechanics of a food packet\n"
                 <<"--joint-readout intact|observed|muted   compare recurrent, observation-only and separate-object predictions\n"
                 <<"--audit-calls on|off   read the same acquired contacts with the actual, delayed and displaced sounds\n"
                 <<"--channel reports --culture adult|invent|none --teachers local|fixed|random --tutoring N\n"
                 <<"--channel estuary --route-depth 1..3 --reuse open|cut --tides harvest|clock --route-layout warp|direct\n"
                 <<"--mentors silent|local|displaced --mentoring 0..64 --currents local|novel   speech across tidal seasons\n"
                 <<"--channel garden --flora fixed|heritable|drift --pollen N --plots mixed|dormant --observe-workers yes|no\n"
                 <<"--crop-model intact|episodic|one-beat|blind --crop-policy cached|rebuild\n"
                 <<"--plant-seeds FILE --save-seeds FILE   transplant inherited physical rules and archive the final plots\n"
                 <<"--report-wire open|muted|reversed|displaced --report-cost N --event-age recent|earlier|mixed\n"
                 <<"--recent-cue WORD --earlier-cue WORD   sounds that request a report from a witness\n"
                 <<"--teacher MIND --other-teacher MIND   rear identical newborns under alternating local tutors\n"
                 <<"--save-neighbor MIND   export the selected resident's actual, independently reared partner\n"
                 <<"--report-cache on|off   use compiled afferent masks inside the existing contact words\n"
                 <<"--tuition-layout local|direct   update a private working copy or the resident's global contacts\n"
                 <<"--founders identical|varied --breeding mass|drift --dispatch graph|direct\n";
        return 0;
    }
    auto q=commons_options(argc,argv);auto image=read_brain(q.source,{});
    if (!image.binds || !image.recollects) throw std::invalid_argument("the commons requires an individual with visual and object-binding organs");
    if (q.channel && q.channel<4 && !image.imagines) throw std::invalid_argument("split sensory access requires auditory action fields");
    if (q.channel==4 && !image.reports) throw std::invalid_argument("the report channel requires a reporting individual");
    bool estuary=q.channel==5;if (estuary && !image.conceptual) throw std::invalid_argument("the estuary needs relational contacts");
    if (q.channel==6 && !image.causal) throw std::invalid_argument("the greenhouse requires an individual with causal contacts");
    Word n=q.families*q.copies,patches=n/32;Session s;s.open();
    Partner animal(s,std::move(image),n,0x6a98df13u,false);
    auto form=animal.form();auto donor=animal.germline();
    Temporary animals(s.d,Count(n)*sizeof(Forager)),water(s.d,Count(patches)*sizeof(Reservoir)),settings(s.d,sizeof(CommonsSettings));
    Temporary social(s.d,q.channel && q.channel<=4 ? Count(n)*sizeof(AcousticState) : 0),acoustics(s.d,q.channel && q.channel<4 ? Count(patches)*sizeof(AcousticIsland) : 0);
    std::optional<Orchard> crops;if (q.channel==6) crops.emplace(s,n,q.seed_bank);
    Temporary charts(s.d,estuary ? Count(patches)*sizeof(RelationalView) : 0),decisions(s.d,estuary ? Count(n)*sizeof(Word) : 0),
              port_sounds(s.d,estuary ? sizeof(EstuarySpeech) : 0),route_counts(s.d,estuary ? 4*sizeof(Word) : 0),
              mentors(s.d,estuary && q.mentors ? Count(patches)*sizeof(Concepts) : 0);
    Function chart{},place{},gather{},inspect{},listen{},succeed{};
    if (estuary) {
        EstuarySpeech shore{};std::istringstream names("pearl acorn shell flint pouch basket crate chest tern ibis swift wren cove quay inlet delta reed lark dune cape ford kelp fern moss sand gull bay salt pine wave rock shoal");
        for (auto &place:shore.places) {std::string name;names>>name;place=sound_pattern(name);}
        shore.routes[0]=sound_pattern("drifts");shore.routes[1]=sound_pattern("feeds");shore.routes[2]=sound_pattern("nests");
        shore.verdicts[0]=sound_pattern("no");shore.verdicts[1]=sound_pattern("yes");shore.verdicts[2]=sound_pattern("unsure");
        s.d.check(s.d.upload(port_sounds.ptr,&shore,sizeof(shore)),"introduce calls for places and tidal events");
        s.bind(chart,"estuary_chart",Image::vocal_brood);s.bind(place,"estuary_birth",Image::vocal_brood);
        s.bind(gather,q.warp_routes ? "estuary_warp" : "estuary_direct",Image::vocal_brood);s.bind(inspect,"estuary_census",Image::vocal_brood);
        if (mentors.ptr) {
            s.d.check(s.d.zero(mentors.ptr,0,Count(patches)*sizeof(Concepts)),"start without an adult teacher");
            s.bind(listen,"estuary_elders",Image::vocal_brood);s.bind(succeed,"estuary_succession",Image::vocal_brood);
        }
    }
    Word narrative=q.channel==4;
    Temporary elders(s.d,narrative ? Count(patches)*sizeof(CulturalIsland) : 0),sampled(s.d,narrative ? Count(patches)*sizeof(CulturalSample) : 0);
    Word varieties=1+Word(!q.other_teacher.empty());
    Temporary founding(s.d,narrative ? varieties*sizeof(Reports) : 0);Function origin{},copy{},exchange{},handoff{};
    if (narrative) {
        std::array<Reports,2> tutors{animal.image.report,{}};
        for (Word j=0;j<varieties;++j) {
            const auto &path=j ? q.other_teacher : q.teacher;
            if (!path.empty()) {AgentImage tutor;tutor.read(path);if (!tutor.reports) throw std::invalid_argument("the cultural tutor needs a reporting organ");tutors[j]=tutor.report;}
        }
        s.d.check(s.d.upload(founding.ptr,tutors.data(),varieties*sizeof(Reports)),"introduce the cultural witnesses");
        s.bind(origin,"culture_origin",Image::vocal_brood);s.bind(copy,q.local_tuition ? "culture_rear_local" : "culture_rear",Image::vocal_brood);
        s.bind(exchange,"culture_exchange",Image::vocal_brood);s.bind(handoff,"culture_handoff",Image::vocal_brood);
    }
    Temporary events(s.d,Count(n)*sizeof(Encounter)),outputs(s.d,Count(n)*sizeof(Response));
    Temporary stock(s.d,Count(q.families)*sizeof(VoiceGermline)),young(s.d,Count(q.families)*sizeof(VoiceGermline));
    Temporary fitness(s.d,Count(q.families)*sizeof(Word)),cdf(s.d,Count(q.families)*sizeof(Count)),summary(s.d,3*sizeof(Count)),report(s.d,sizeof(CommonsTotals));
    Function found{},hatch{},open{},sense{},hear{},settle{},grade{},census{},prefix{},recruit{},survey{};
    for (auto [ptr,name]:{std::pair{&found,"commons_found"},{&hatch,"voice_hatch"},{&open,"commons_begin"},{&sense,"commons_sense"},
                          {&hear,"commons_hear"},{&settle,"commons_settle"},{&grade,"commons_grade"},{&census,"commons_census"},{&prefix,"commons_prefix"},
                          {&recruit,"commons_recruit"},{&survey,"voice_survey"}}) s.bind(*ptr,name,Image::vocal_brood);
    CommonsSettings config{n,0,q.rations,q.volatility,q.price,q.policy,q.choice,q.credit,q.soma,word_digest("rustled"),q.exposures,q.reserve,q.receipts,
                           q.channel,q.hearing,word_digest("/peer-pitch"),q.kin,q.focused,q.optics,q.geometry,q.joint,q.calories,q.shuffled,q.audit,q.projection};
    DevicePtr parents=stock.ptr,children=young.ptr;
    config.culture=q.culture;config.tutoring=q.tutoring;config.report_wire=q.report_wire;config.report_cost=q.report_cost;
    config.event_age=q.event_age;config.cues[0]=word_digest(q.recent);config.cues[1]=word_digest(q.earlier);config.teachers=q.teachers;
    config.report_cache=q.report_cache;config.alternating=varieties-1;
    config.route_depth=q.route_depth;config.route_reuse=q.route_reuse;config.tidal_feedback=q.tidal_feedback;config.mentoring=q.mentoring;
    config.flora=q.flora;config.pollen=q.pollen;config.observers=q.observers;config.garden_cache=q.garden_cache;config.garden_model=q.garden_model;config.mixed_plots=q.mixed_plots;
    void *seed[]{&parents,&donor,&q.families,&q.seed,&q.variation,&narrative};launch(s,found,q.families,seed);
    CommonsGraph g(s.d);g.api.bind(s.d);s.d.check(g.api.create_stream(&g.stream,0),"open commons stream");
    s.d.check(s.d.event_new(&g.begin,0),"open commons timer");s.d.check(s.d.event_new(&g.end,0),"open commons timer");
    if (narrative) {s.d.check(s.d.event_new(&g.school_begin,0),"open rearing timer");s.d.check(s.d.event_new(&g.school_end,0),"open rearing timer");}
    auto day=[&](){
        if (crops) {crops->day(settings.ptr,animals.ptr,water.ptr,animal,events.ptr,outputs.ptr,g.stream);return;}
        if (estuary) {void *tide[]{&settings.ptr,&animals.ptr,&water.ptr,&charts.ptr};launch(s,chart,n,tide,g.stream);}
        if (mentors.ptr) {void *show[]{&settings.ptr,&charts.ptr,&mentors.ptr};launch(s,listen,patches,show,g.stream);}
        void *show[]{&settings.ptr,&animals.ptr,&acoustics.ptr,&events.ptr,&water.ptr};launch(s,sense,n,show,g.stream);
        animal.act(events.ptr,outputs.ptr,g.stream);
        if (estuary) {
            void *visit[]{&settings.ptr,&animal.concepts.ptr,&port_sounds.ptr,&animals.ptr,&water.ptr,&charts.ptr,&animal.bindings.ptr,&outputs.ptr,&decisions.ptr,&mentors.ptr};
            launch(s,gather,Count(n)*(q.warp_routes ? 32 : 1),visit,g.stream);
        } else if (narrative) {
            void *speak[]{&settings.ptr,&animals.ptr,&animal.reports.ptr,&animal.bindings.ptr,&outputs.ptr,&events.ptr,&social.ptr,&elders.ptr,&water.ptr};
            launch(s,exchange,n,speak,g.stream);animal.act(events.ptr,outputs.ptr,g.stream);
        } else if (q.channel) {
            void *listen[]{&settings.ptr,&animals.ptr,&animal.memory.ptr,&animal.imagery.ptr,&outputs.ptr,&events.ptr,&acoustics.ptr,&social.ptr};launch(s,hear,n,listen,g.stream);
            animal.act(events.ptr,outputs.ptr,g.stream);
        }
        void *act[]{&settings.ptr,&animals.ptr,&water.ptr,&animal.bindings.ptr,&animal.memory.ptr,&animal.imagery.ptr,&outputs.ptr,&social.ptr,&animal.joint.ptr,&charts.ptr,&decisions.ptr};launch(s,settle,n,act,g.stream);
    };
    if (q.graphs && q.days>=8) {
        s.d.check(g.api.begin(g.stream,1),"capture eight days in the commons");
        try {for (Word i=0;i<8;++i) day();} catch (...) {g.api.end(g.stream,&g.graph);throw;}
        s.d.check(g.api.end(g.stream,&g.graph),"finish commons graph");
        s.d.check(g.api.instantiate(&g.executable,g.graph,0),"instantiate commons graph");
    }
    Count device_bytes=Count(n)*(animal.bytes()+sizeof(Forager)+sizeof(Encounter)+sizeof(Response))+Count(patches)*sizeof(Reservoir)
                      +Count(q.families)*(2*sizeof(VoiceGermline)+sizeof(Word)+sizeof(Count))+sizeof(CommonsSettings)+sizeof(CommonsTotals)+3*sizeof(Count);
    if (social.ptr) device_bytes+=Count(n)*sizeof(AcousticState);
    if (estuary) device_bytes+=Count(patches)*sizeof(RelationalView)+Count(n)*sizeof(Word)+sizeof(EstuarySpeech)+4*sizeof(Word);
    if (mentors.ptr) device_bytes+=Count(patches)*sizeof(Concepts);
    if (crops) device_bytes+=crops->bytes();
    if (acoustics.ptr) device_bytes+=Count(patches)*sizeof(AcousticIsland);
    if (narrative) device_bytes+=Count(patches)*(sizeof(CulturalIsland)+sizeof(CulturalSample))+varieties*sizeof(Reports);
    std::cout<<s.name<<"  resident_bytes="<<animal.bytes()<<"  simulation_bytes="<<device_bytes<<"  neurons="<<32*animal.image.brain.tiles<<"  islands="<<patches
             <<"  residents="<<n<<"  policy="<<q.policy<<"  rations="<<q.rations<<"  volatility="<<q.volatility<<"  price="<<q.price
             <<"  exposures="<<q.exposures<<"  reserve="<<q.reserve<<"  receipts="<<q.receipts<<"  channel="<<q.channel<<"  kin="<<q.kin
             <<"  calories="<<q.calories<<"  tandem="<<q.joint<<"  geometry="<<(q.geometry==1 ? "distinct" : q.geometry==3 ? "coupled" : "aliased")
             <<"  joint_readout="<<q.projection<<'\n';
    if (auto *j=journal::current()) {j->source=q.source;j->device=s.name;j->metrics["residents"]=n;j->metrics["islands"]=patches;j->metrics["resident_bytes"]=animal.bytes();}
    Word best=0;Count final_mass=0,opened=0;bool extinct=false;
    for (Word gen=1;gen<=q.generations;++gen) {
        config.seed=mix(q.seed^gen*0x38ad59cfu);
        config.current_seed=q.renewing ? config.seed : mix(q.seed^0x38ad59cfu);
        s.d.check(s.d.upload(settings.ptr,&config,sizeof(config)),"start a season");
        animal.hatch(hatch,parents,q.copies,config.seed,1-q.kin);
        void *init[]{&settings.ptr,&animals.ptr,&water.ptr,&animal.bindings.ptr,&animal.memory.ptr,&social.ptr,&acoustics.ptr,&animal.joint.ptr};launch(s,open,n,init);
        if (crops) crops->birth(settings.ptr,animal,gen);
        if (estuary) {
            s.d.check(s.d.zero(decisions.ptr,0,Count(n)*sizeof(Word)),"clear seasonal tidal decisions");
            void *start[]{&settings.ptr,&animal.concepts.ptr,&port_sounds.ptr};launch(s,place,n,start);
        }
        if (narrative) {
            s.d.check(s.d.record(g.school_begin,g.stream),"start timing acoustic rearing");
            if (gen==1) {void *start[]{&settings.ptr,&elders.ptr,&animal.reports.ptr,&founding.ptr};launch(s,origin,patches,start,g.stream);}
            for (Word first=0;first<n;first+=32768) {
                Word batch=std::min(32768u,n-first);
                void *rear[]{&settings.ptr,&elders.ptr,&animal.reports.ptr,&first,&batch};launch(s,copy,batch,rear,g.stream);
            }
            s.d.check(s.d.record(g.school_end,g.stream),"finish timing acoustic rearing");
        }
        Word remaining=q.days;s.d.check(s.d.record(g.begin,g.stream),"start the commons clock");
        while (g.executable && remaining>=8) {s.d.check(g.api.launch(g.executable,g.stream),"advance eight days");remaining-=8;}
        while (remaining) {day();--remaining;}
        s.d.check(s.d.record(g.end,g.stream),"stop the commons clock");s.d.check(s.d.wait(g.end),"complete the season");
        float ms{};s.d.check(s.d.elapsed(&ms,g.begin,g.end),"time the foraging season");
        void *assess[]{&animals.ptr,&fitness.ptr,&q.families,&q.copies,&q.kin};launch(s,grade,q.families,assess);
        void *measure[]{&fitness.ptr,&q.families,&summary.ptr};launch(s,survey,128,measure);
        Count stats[3];s.d.check(s.d.download(stats,summary.ptr,sizeof(stats)),"read deposited reproductive material");
        best=Word(stats[2]);final_mass=stats[0];extinct=!final_mass;opened=gen;
        if (mentors.ptr && gen<q.generations && !extinct) {
            Word displaced=q.mentors==2;void *next[]{&settings.ptr,&animals.ptr,&animal.concepts.ptr,&mentors.ptr,&displaced};launch(s,succeed,patches,next);
        }
        if (narrative) {
            Word renew=gen<q.generations && !extinct;
            void *transfer[]{&settings.ptr,&elders.ptr,&animal.reports.ptr,&animals.ptr,&sampled.ptr,&renew};launch(s,handoff,patches,transfer);
        }
        if (gen==1 || gen==q.generations || extinct || gen%std::max(1u,q.generations/8)==0) {
            void *observe[]{&animals.ptr,&social.ptr,&n,&report.ptr};launch(s,census,128,observe);
            CommonsTotals t{};s.d.check(s.d.download(&t,report.ptr,sizeof(t)),"read the energy budget");
            auto *v=t.count;if (Count(q.reserve)*n+q.calories*v[2]!=v[1]+v[9]+v[10] || v[1]!=stats[0]) throw std::runtime_error("commons energy budget diverged");
            if (v[8]!=v[2]+v[3]+v[6]+v[7]+v[18]) throw std::runtime_error("commons daily choices diverged");
            std::vector<Reservoir> reserves(patches);s.d.check(s.d.download(reserves.data(),water.ptr,reserves.size()*sizeof(Reservoir)),"read shared food reservoirs");
            Count supply=8ull*q.rations*patches,available=0,eaten=0,changes=0,speech_events=0,audit[4]{};double information=0;
            for (const auto &r:reserves) {
                supply+=r.regrown;eaten+=r.consumed;changes+=r.switches;for (Word x:r.stock) available+=x;
            }
            std::vector<AcousticIsland> voices(acoustics.ptr ? patches : 0);
            if (!voices.empty()) s.d.check(s.d.download(voices.data(),acoustics.ptr,voices.size()*sizeof(AcousticIsland)),"read observed vocal activity");
            for (const auto &r:voices) for (Word i=0;i<4;++i) audit[i]+=r.audit[i];
            for (const auto &r:voices) for (const auto &role:r.speech) {
                Count stimulus[4]{},sound[5]{},all=0;
                for (Word b=0;b<4;++b) for (Word m=0;m<5;++m) {Word v=role[b][m];stimulus[b]+=v;sound[m]+=v;all+=v;}
                speech_events+=all;
                for (Word b=0;b<4;++b) for (Word m=0;m<5;++m) if (Word v=role[b][m])
                    information+=v*std::log2(double(v)*all/(double(stimulus[b])*sound[m]));
            }
            if (eaten!=v[2] || available+eaten!=supply) throw std::runtime_error("commons food budget diverged");
            if (acoustics.ptr && speech_events!=v[8]) throw std::runtime_error("commons voice census lost a living resident");
            if (crops) crops->census(settings.ptr,animal,gen,q,v);
            if (estuary) {
                Word learned[4]{};s.d.check(s.d.upload(route_counts.ptr,learned,sizeof(learned)),"open acquired-route census");
                void *inspect_args[]{&settings.ptr,&animal.concepts.ptr,&port_sounds.ptr,&decisions.ptr,&route_counts.ptr};launch(s,inspect,n,inspect_args);
                s.d.check(s.d.download(learned,route_counts.ptr,sizeof(learned)),"read acquired pathways");
                std::cout<<"estuary_generation="<<gen<<"  depth="<<q.route_depth<<"  reuse="<<q.route_reuse<<"  harvest_tides="<<q.tidal_feedback
                         <<"  warp="<<q.warp_routes<<"  two_hops="<<learned[0]<<"  four_hops="<<learned[1]<<"  eight_hops="<<learned[2]
                         <<"  correct_traps="<<v[2]+v[7]+v[18]<<"  placements="<<v[8]-v[3]<<"  lessons="<<v[11]<<"  social_lessons="<<learned[3]
                         <<"  mentors="<<q.mentors<<"  novel_currents="<<q.renewing<<'\n';
            }
            if (narrative) {
                std::vector<CulturalSample> samples(patches);s.d.check(s.d.download(samples.data(),sampled.ptr,samples.size()*sizeof(samples[0])),"read reports of a common event");
                Count accepted=0,innovations=0,words=0,studied=0,lexicon=0;std::set<std::string> distinct;
                for (const auto &v:samples) {
                    accepted+=v.accepted;innovations+=v.improvised;words+=v.words;studied+=v.investigated;lexicon+=v.forms;
                    std::string heard;for (Word k=0;k<v.voice.count;++k) {if (k) heard+=' ';heard+=transcribe(v.voice.sound[k]);}
                    if (!heard.empty()) distinct.insert(heard);
                }
                float tuition_ms{};s.d.check(s.d.elapsed(&tuition_ms,g.school_begin,g.school_end),"read acoustic rearing time");
                std::cout<<"culture_generation="<<gen<<"  tuition="<<accepted<<"  improvised="<<innovations<<"  words="<<words<<"  learned_visits="<<studied
                         <<"  sampled_forms="<<lexicon<<"  sampled_islands="<<patches<<"  distinct_reports="<<distinct.size()
                         <<"  event_age="<<q.event_age<<"  culture="<<q.culture<<"  teachers="<<q.teachers<<"  report_wire="<<q.report_wire<<"  word_cost="<<q.report_cost
                         <<"  tuition_layout="<<(q.local_tuition ? "local" : "direct")<<"  tuition_ms="<<std::fixed<<std::setprecision(3)<<tuition_ms<<'\n';
                Word shown=0;for (const auto &line:distinct) {if (shown==4) break;std::cout<<"island_report="<<++shown<<"  generation="<<gen<<"  speech="<<line<<'\n';}
            }
            std::cout<<"commons_generation="<<gen<<"  residents="<<n<<"  alive="<<v[0]<<"  deposited="<<v[1]<<"  meals="<<v[2]<<"  experiments="<<v[3]
                     <<"  diagnostic="<<v[4]<<"  after_miss="<<v[5]<<"  wrong_traps="<<v[6]<<"  lost_contests="<<v[7]<<"  lived_days="<<v[8]
                     <<"  energy="<<v[9]<<"  spent="<<v[10]<<"  meal_updates="<<v[11]<<"  switches="<<changes<<"  food_supplied="<<supply<<"  food_left="<<available
                     <<"  calls="<<v[12]<<"  heard="<<v[13]<<"  decoded="<<v[14]<<"  correctly_decoded="<<v[15]<<"  hidden_traps="<<v[16]<<"  hidden_hits="<<v[17]
                     <<"  unopened="<<v[18]<<"  bits_per_emission="<<std::fixed<<std::setprecision(4)<<(speech_events ? information/speech_events : 0)
                     <<"  matched_calls="<<audit[0]<<"  actual_call="<<audit[1]<<"  delayed_call="<<audit[2]<<"  displaced_call="<<audit[3]
                     <<"  best_mass="<<stats[1]<<"  GPU_ms="<<std::setprecision(3)<<ms<<'\n';
            if (auto *j=journal::current()) {j->breeding.push_back({gen,q.families,{v[1],stats[1]}});j->metrics["commons_generations"]=gen;}
        }
        if (gen==q.generations || extinct) break;
        if (crops) crops->pollen(settings.ptr,water.ptr,gen);
        void *scan[]{&fitness.ptr,&q.families,&cdf.ptr};launch(s,prefix,128,scan);
        void *breed[]{&parents,&children,&cdf.ptr,&q.families,&config.seed,&q.drift,&form};launch(s,recruit,q.families,breed);
        std::swap(parents,children);
    }
    if (!q.saved.empty() || !q.saved_peer.empty() || !q.save_seeds.empty()) {
        Word resident=best,mass=0;
        for (Word i=0;i<q.copies;++i) {
            Word site=q.kin ? best*q.copies+i : best+i*q.families;Forager a{};
            s.d.check(s.d.download(&a,animals.ptr+Count(site)*sizeof(a),sizeof(a)),"inspect selected relatives");
            if (!i || a.offspring>mass) {resident=site;mass=a.offspring;}
        }
        animal.save(q.saved,resident);
        animal.save(q.saved_peer,resident^1);
        if (crops && !q.save_seeds.empty()) crops->save(q.save_seeds,resident/32);
    }
    if (auto *j=journal::current()) {
        std::vector<journal::CensusMember> roster(n);Temporary packed(s.d,Count(n)*sizeof(roster[0]));Function survey{};s.bind(survey,"commons_card_census",Image::vocal_brood);
        void *all[]{&animals.ptr,&animal.bodies.ptr,&animal.shape.body_bytes,&n,&q.families,&q.kin,&packed.ptr};launch(s,survey,n,all);
        s.d.check(s.d.download(roster.data(),packed.ptr,Count(n)*sizeof(roster[0])),"read every living body form");journal::ranked_forms(*j,std::move(roster));
        Word take=std::min(64u,n);j->commons_residents.resize(take);
        Temporary sample(s.d,Count(take)*sizeof(j->commons_residents[0]));Function view{};
        s.bind(view,"commons_sample",Image::vocal_brood);
        void *observe[]{&animals.ptr,&animal.bodies.ptr,&animal.shape.body_bytes,&n,&q.families,&q.kin,&sample.ptr};launch(s,view,take,observe);
        s.d.check(s.d.download(j->commons_residents.data(),sample.ptr,Count(take)*sizeof(j->commons_residents[0])),"record resident morphology and foraging");
    }
    if (!q.newborn.empty() && !extinct) {
        animal.hatch(hatch,parents+Count(best)*sizeof(VoiceGermline),n,mix(q.seed^0x7f25bb29u));
        animal.save(q.newborn,0);
    }
    std::cout<<"commons="<<(extinct ? "no-offspring" : "completed")<<"  generations="<<opened<<"  reproductive_mass="<<final_mass<<"  selected_family="<<best<<'\n';
    return 0;
}
