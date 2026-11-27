#pragma once
// Included beside the device nursery. This scorer reconstructs distances
// from physical truth tables supplied by the experimenter.
int certify_causal(Nursery &lab,const storage::AgentImage &pupil,const std::string &file) {
    if (!pupil.causal) throw std::invalid_argument("the action census needs acquired causal contacts");
    std::ifstream in(file);if (!in) throw std::invalid_argument("cannot read the physical reference law");
    std::array<std::array<Word,64>,6> transitions{};
    for (Word a=0;a<6;++a) for (Word j=0;j<6;++j) {
        Word left,right,lut;
        if (!(in>>left>>right>>lut) || left>=6 || right>=6 || lut>=16) throw std::invalid_argument("expected 36 pairs of receptors and four-bit physical truth tables");
        for (Word x=0;x<64;++x) {Word v=((x>>left)&1)+2*((x>>right)&1);transitions[a][x]|=((lut>>v)&1)<<j;}
    }
    std::string extra;if (in>>extra) throw std::invalid_argument("extra fields after the 36 physical gates");
    Word n=64*729;Temporary mind(lab.d,sizeof(CausalModel)),maps(lab.d,Count(n)*3*sizeof(Count));Function compile{};
    lab.bind(compile,"causal_census",Image::causes);lab.d.check(lab.d.upload(mind.ptr,&pupil.agency.model,sizeof(CausalModel)),"present the frozen action model");
    void *args[]{&mind.ptr,&maps.ptr,&n};lab.launch(compile,32*n,args);std::vector<Count> planes(3*n);
    lab.d.check(lab.d.download(planes.data(),maps.ptr,planes.size()*sizeof(Count)),"read policies for every goal and motor subset");
    struct Route {Word length=65,first=0;};Count right=0,reachable=0,missed=0,wrong=0,already=0,unreachable=0;
    Count attempted=0,completed=0,stalled=0,infeasible=0;
    for (Word menu=0;menu<64;++menu) for (Word ordinal=0;ordinal<729;++ordinal) {
        Word ternary=ordinal,require=0,value=0;
        for (Word k=0;k<6;++k) {Word digit=ternary%3;ternary/=3;if (digit) require|=1u<<k;if (digit==2) value|=1u<<k;}
        auto *policy=&planes[3*(menu*729+ordinal)];std::array<Word,64> decision{},resolution{},next{};
        for (Word x=0;x<64;++x) {
            Word motor=0;for (Word k=0;k<3;++k) motor|=Word((policy[k]>>x)&1)<<k;decision[x]=motor;
            if (motor<6 && (menu&(1u<<motor))) next[x]=transitions[motor][x];
            else resolution[x]=motor==6 && (x&require)==value ? 1 : motor==7 ? 2 : 3;
        }
        for (Word start=0;start<64;++start) if (decision[start]<6) {
            Word x=start;Count path=0;
            while (!resolution[x] && !(path&(1ull<<x))) {path|=1ull<<x;x=next[x];}
            Word result=resolution[x] ? resolution[x] : 3;
            while (path) {Word k=Word(__builtin_ctzll(path));path&=path-1;resolution[k]=result;}
            ++attempted;if (result==1) ++completed;else if (result==2) ++stalled;else ++infeasible;
        }
    }
    for (Word menu=0;menu<64;++menu) for (Word start=0;start<64;++start) {
        std::array<Route,64> exact{};std::array<Word,64> queue{};Word front=0,back=1;queue[0]=start;exact[start].length=0;
        while (front<back) {
            Word x=queue[front++];
            for (Word a=0;a<6;++a) if (menu&(1u<<a)) {
                Word y=transitions[a][x],distance=exact[x].length+1;
                if (exact[y].length>distance) {exact[y].length=distance;queue[back++]=y;}
                if (exact[y].length==distance) exact[y].first|=x==start ? 1u<<a : exact[x].first;
            }
        }
        std::array<Route,729> goals{};
        for (int g=728;g>=0;--g) {
            Word ternary=Word(g),state=0,free=0,power=1;
            for (Word j=0;j<6;++j) {Word v=ternary%3;ternary/=3;if (!v && !free) free=power;if (v==2) state|=1u<<j;power*=3;}
            auto &r=goals[g];
            if (!free) r=exact[state];
            else {
                auto left=goals[g+free],right=goals[g+2*free];r=left.length<right.length ? left : right;
                if (left.length==right.length) r.first|=left.first|right.first;
            }
            const Count *p=&planes[3*(menu*729+g)];Word motor=0;for (Word j=0;j<3;++j) motor|=Word((p[j]>>start)&1)<<j;
            if (r.length==65) {++unreachable;if (motor==7) ++right;else ++wrong;}
            else if (!r.length) {++already;if (motor==6) ++right;else ++wrong;}
            else {++reachable;if (r.first&(1u<<motor)) ++right;else if (motor==7) ++missed;else ++wrong;}
        }
    }
    std::cout<<"causal_census=all  goals=729  menus=64  starts=64  trials="<<64*n<<"  correct="<<right<<"  reachable="<<reachable
             <<"  already="<<already<<"  unreachable="<<unreachable<<"  missed="<<missed<<"  wrong="<<wrong<<"  attempted="<<attempted
             <<"  completed="<<completed<<"  stalled="<<stalled<<"  unsafe="<<infeasible<<"  policy_bytes=24\n";
    return 0;
}
