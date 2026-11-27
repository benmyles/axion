#pragma once
class Orchard {
    Session &s;Word n,islands;
    Temporary beds,seeds,policies,decisions,sounds,counts;
    std::optional<Temporary> donors;Word donor_count=0;
    Function found{},grow{},prepare{},perceive{},choose{},settle{},disperse{},measure{},transplant{};
public:
    Orchard(Session &session,Word residents,const std::string &source):s(session),n(residents),islands(n/32),beds(s.d,Count(islands)*sizeof(SeedBed)),
        seeds(s.d,Count(islands)*sizeof(SeedBed)),policies(s.d,Count(n)*sizeof(CropPolicy)),decisions(s.d,Count(n)*sizeof(Word)),
        sounds(s.d,sizeof(GardenSpeech)),counts(s.d,4*sizeof(Count)) {
        for (auto [f,name]:{std::pair{&found,"garden_seed"},{&grow,"garden_rear"},{&prepare,"garden_prepare"},{&perceive,"garden_sense"},
                {&choose,"garden_choose"},{&settle,"garden_settle"},{&disperse,"garden_disperse"},{&measure,"garden_census"}}) s.bind(*f,name,Image::causes);
        GardenSpeech voice{};auto hear=[](auto &forms,const char *text){
            std::istringstream in(text);for (auto &form:forms) {std::string word;in>>word;form=sound_pattern(word);}
        };
        hear(voice.names,"pearl acorn shell flint pouch basket crate chest tern ibis swift wren cove quay inlet delta reed lark dune cape ford kelp fern moss sand gull bay salt pine wave rock shoal");
        hear(voice.words,"soak heat sow tend pollinate ripen moist warm sown green flowering fruiting dry cold unsown dormant budless bare how suppose why make describe edible forage");
        hear(voice.verdicts,"no yes unsure");
        s.d.check(s.d.upload(sounds.ptr,&voice,sizeof(voice)),"introduce calls in the common garden");
        if (!source.empty()) {
            storage::SeedBank bank;bank.read(source);donor_count=bank.header.count;donors.emplace(s.d,Count(donor_count)*sizeof(SeedBed));
            s.d.check(s.d.upload(donors->ptr,bank.beds.data(),Count(donor_count)*sizeof(SeedBed)),"introduce plant donors");
            s.bind(transplant,"garden_transplant",Image::causes);
        }
    }
    Count bytes() const {return Count(2*islands+donor_count)*sizeof(SeedBed)+Count(n)*(sizeof(CropPolicy)+sizeof(Word))+sizeof(GardenSpeech)+4*sizeof(Count);}
    void birth(DevicePtr settings,Partner &animal,Word generation) {
        if (generation==1) {
            if (donors) {void *args[]{&settings,&donors->ptr,&donor_count,&beds.ptr};launch(s,transplant,islands,args);}
            else {void *plant[]{&settings,&beds.ptr};launch(s,found,islands,plant);}
        }
        void *rear[]{&settings,&beds.ptr,&animal.agency.ptr,&animal.concepts.ptr,&policies.ptr,&sounds.ptr};launch(s,grow,Count(n)*32,rear);
    }
    void day(DevicePtr q,DevicePtr bodies,DevicePtr water,Partner &animal,DevicePtr events,DevicePtr muscles,Stream stream) {
        void *goals[]{&q,&animal.agency.ptr,&policies.ptr,&bodies};launch(s,prepare,Count(n)*32,goals,stream);
        void *retina[]{&q,&beds.ptr,&bodies,&events};launch(s,perceive,n,retina,stream);
        animal.act(events,muscles,stream);
        void *act[]{&q,&beds.ptr,&bodies,&animal.agency.ptr,&policies.ptr,&muscles,&animal.bindings.ptr,&decisions.ptr};launch(s,choose,n,act,stream);
        void *step[]{&q,&beds.ptr,&bodies,&water,&animal.agency.ptr,&policies.ptr,&decisions.ptr};launch(s,settle,n,step,stream);
    }
    void pollen(DevicePtr q,DevicePtr water,Word generation) {
        void *args[]{&q,&beds.ptr,&water,&seeds.ptr,&generation};launch(s,disperse,islands,args);
        s.d.check(s.d.copy_device(beds.ptr,seeds.ptr,Count(islands)*sizeof(SeedBed)),"plant the next generation of seeds");
    }
    void save(const std::string &path,Word focus) {
        storage::SeedBank bank;bank.header.focus=focus;bank.beds.resize(islands);
        s.d.check(s.d.download(bank.beds.data(),beds.ptr,Count(islands)*sizeof(SeedBed)),"archive the physical plants");
        storage::write_checked(path,bank);
    }
    void census(DevicePtr q,Partner &animal,Word generation,const CommonsOptions &options,const Count *economy) {
        void *args[]{&q,&animal.agency.ptr,&policies.ptr,&counts.ptr};launch(s,measure,128,args);Count v[4];
        s.d.check(s.d.download(v,counts.ptr,sizeof(v)),"read causal contacts and appetites");
        std::vector<SeedBed> plants(islands);s.d.check(s.d.download(plants.data(),beds.ptr,Count(islands)*sizeof(SeedBed)),"sample current plant genomes");
        std::set<std::array<unsigned short,38>> laws;std::set<Word> origins;Count edits=0,moved=0,ready=0,regrazed=0;
        for (const auto &bed:plants) {
            std::array<unsigned short,38> code{};Word k=0;for (auto &r:bed.field.gates) for (auto gene:r) code[k++]=gene;
            code[36]=static_cast<unsigned short>(bed.field.food);code[37]=static_cast<unsigned short>(bed.field.dormant);
            laws.insert(code);origins.insert(bed.ancestor);edits+=bed.edits;moved+=bed.migrated;
            Word g=bed.field.food-1,x=bed.field.dormant,k0=((x>>(g&7))&1)+2*((x>>((g>>3)&7))&1);
            ready+=(g>>(6+k0))&1;regrazed+=bed.dormant_meals;
        }
        std::cout<<"greenhouse_generation="<<generation<<"  residents="<<n<<"  islands="<<islands<<"  appetites="<<v[1]<<"  tastings="<<v[0]
                 <<"  policy_builds="<<v[2]<<"  settled_fields="<<v[3]<<"  planned_work="<<economy[4]<<"  work="<<economy[3]
                 <<"  plant_genomes="<<laws.size()<<"  plant_founders="<<origins.size()<<"  inherited_edits="<<edits<<"  immigrants="<<moved
                 <<"  edible_at_reset="<<ready<<"  reset_meals="<<regrazed<<"  flora="<<options.flora<<"  crop_model="<<options.garden_model<<"  cached="<<options.garden_cache<<'\n';
        if (auto *j=journal::current()) for (Word k=0;k<(islands>1 ? 2u : 1u);++k) {
            Word i=k ? islands-1 : 0;auto &bed=plants[i];std::string states;
            for (auto x:bed.field.plots) {if (!states.empty()) states+=',';states+=std::to_string(x);}
            j->line("crop_island="+std::to_string(i)+"  generation="+std::to_string(generation)+"  ancestor="+std::to_string(bed.ancestor)
                    +"  edits="+std::to_string(bed.edits)+"  food="+std::to_string(bed.field.food)+"  plots="+states);
        }
    }
};
