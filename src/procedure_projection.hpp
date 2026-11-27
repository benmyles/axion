// Host transport, allocation and timing for the CUDA policy compiler.
struct PolicyWorkspace {
    Session &lab;DevicePtr mind,decoder;
    Temporary plant,nodes,summary,grid,performance,rows,status;
    DevicePtr root=0;Word ast_nodes=0;bool resting=false;
    PolicyWorkspace(Session &s,DevicePtr m,DevicePtr d,DevicePtr agency,Word allowed=63):lab(s),mind(m),decoder(d),
        plant(s.d,sizeof(MotorModel)),nodes(s.d,(performance_span+1)*sizeof(Node)),summary(s.d,(performance_span+1)*sizeof(Summary)),
        grid(s.d,4096*sizeof(Half)),performance(s.d,sizeof(Performance)),rows(s.d,(performance_span+1)*sizeof(Transfer)),status(s.d,sizeof(Word)) {
        run(lab,"procedure_plant",1,128,mind,agency,plant.ptr,allowed);
    }
    Word pack(DevicePtr packed,const Transfer &expected) {
        run(lab,"procedure_pack",1,1,root,packed,status.ptr);
        Word bytes=0;get(lab,bytes,status.ptr);if (bytes<offsetof(CompactTransfer,bits) || bytes>sizeof(CompactTransfer)) throw std::logic_error("invalid compiled effect extent");
        Temporary check(lab.d,sizeof(Transfer));run(lab,"procedure_unpack",1,local_states,packed,check.ptr);
        Transfer copy;get(lab,copy,check.ptr);
        for (Word state=0;state<local_states;++state) if (copy.destination[state]!=expected.destination[state] ||
            (copy.destination[state]<local_states && copy.pulses[state]!=expected.pulses[state])) throw std::logic_error("compact physical denotation differs");
        return bytes;
    }
    Word compile(const std::vector<Count> &sounds) {
        if (sounds.empty() || sounds.size()>performance_span) return capacity;
        Temporary waves(lab.d,sounds.size()*sizeof(Count));Word n=Word(sounds.size());
        lab.d.check(lab.d.upload(waves.ptr,sounds.data(),sounds.size()*sizeof(Count)),"hear the proposed motor score");
        run(lab,"procedure_situate",1,1,mind,decoder,waves.ptr,n,nodes.ptr,summary.ptr,grid.ptr,performance.ptr,status.ptr);
        Word st;get(lab,st,status.ptr);if (st!=recognized) return st;
        Performance script;get(lab,script,performance.ptr);ast_nodes=script.nodes;root=rows.ptr+Count(script.stack[0].node)*sizeof(Transfer);
        run(lab,"procedure_projection",1,local_states,mind,decoder,plant.ptr,nodes.ptr,performance.ptr,rows.ptr);return recognized;
    }
    std::vector<Count> backward(DevicePtr palette,Word size,Word join,Count goals,Count starts,Word horizon,Word width,Word senses) {
        Temporary a(lab.d,width*sizeof(Suffix)),b(lab.d,width*sizeof(Suffix)),bids(lab.d,width*sizeof(Bid)),top(lab.d,sizeof(Count)),
                  stats(lab.d,sizeof(BeamStats)),text(lab.d,sizeof(InferredPlan));
        Suffix initial;initial.accepts=goals;put(lab,a.ptr,initial);Suffix best;Count considered=0;Word good=0,layers=0,occupied=1;bool clipped=false;
        for (Word depth=1;size && occupied && depth<=horizon && (depth==1 || join!=255);++depth) {
            Word old=depth==1 ? 1 : width;layers=depth;considered+=Count(occupied)*size;
            lab.d.check(lab.d.zero(bids.ptr,0,width*sizeof(Bid)),"clear policy frontier");
            lab.d.check(lab.d.zero(top.ptr,0,sizeof(Count)),"clear policy vote");lab.d.check(lab.d.zero(stats.ptr,0,sizeof(BeamStats)),"clear policy breadth");
            run(lab,"procedure_preimages",std::min(256u,(old*size+3)/4),128,mind,palette,a.ptr,bids.ptr,starts,width,old,stats.ptr);
            run(lab,"procedure_suffixes",(width+3)/4,128,palette,a.ptr,bids.ptr,b.ptr,width,top.ptr,stats.ptr);
            BeamStats state;get(lab,state,stats.ptr);occupied=state.occupied;clipped|=bool(state.collision);
            Count rank=0;get(lab,rank,top.ptr);Word hit=Word(rank>>56);
            if (rank && hit>good) {good=hit;get(lab,best,b.ptr+Count(~Word(rank))*sizeof(Suffix));}
            std::swap(a.ptr,b.ptr);
            if (good==Word(std::popcount(starts))) break;
        }
        std::cout<<"policy_search="<<(senses ? "receptive" : "blind")<<"  search=preimage  repertoire="<<size<<"  candidates="<<considered<<"  depth="<<layers
                 <<"  clipped="<<clipped<<"  suffixes="<<occupied<<"  satisfied="<<good<<"  origins="<<std::popcount(starts)<<"  stages="<<Word(best.length)<<'\n';
        if (!best.length) return {};
        put(lab,a.ptr,best);run(lab,"procedure_utter_policy",1,1,mind,palette,a.ptr,text.ptr);InferredPlan out;get(lab,out,text.ptr);
        if (out.n!=best.cost || out.n>performance_span) throw std::logic_error("policy witness and acoustic cost differ");
        return {out.sound,out.sound+out.n};
    }
    std::vector<Count> devise(Count goals,Count starts,Word horizon,Word senses,Word width=512) {
        if (!(starts&~goals)) {
            resting=true;root=rows.ptr;run(lab,"procedure_rest",1,local_states,root,performance.ptr);
            std::cout<<"policy_search="<<(senses ? "receptive" : "blind")<<"  search=rest  repertoire=0  candidates=0  depth=0  clipped=0  satisfied="
                     <<std::popcount(starts)<<"  origins="<<std::popcount(starts)<<"  stages=0\n";return {};
        }
        Temporary palette(lab.d,sizeof(Palette)),winner(lab.d,sizeof(Count)),plan(lab.d,sizeof(InferredPlan));
        run(lab,"procedure_palette",1,local_states,mind,plant.ptr,palette.ptr,senses);
        Half size=0;get(lab,size,palette.ptr+offsetof(Palette,size));Byte join=255;get(lab,join,palette.ptr+offsetof(Palette,join));
        if (width) return backward(palette.ptr,size,join,goals,starts,horizon,width,senses);
        Count rank=0;put(lab,winner.ptr,rank);Word choices=1;Count tried=0;Word examined=0;bool clipped=false;
        for (Word length=1;size && length<=horizon && (length==1 || join!=255);++length) {
            if (choices>24000000u/size) {clipped=true;break;}
            choices*=size;tried+=choices;examined=length;
            run(lab,"procedure_search",std::min(256u,(choices+3)/4),128,mind,palette.ptr,length,choices,goals,starts,winner.ptr);
            get(lab,rank,winner.ptr);if ((rank>>56)==Word(std::popcount(starts))) break;
        }
        Word stages=rank ? 255-Word((rank>>48)&255) : 0;
        std::cout<<"policy_search="<<(senses ? "receptive" : "blind")<<"  search=exhaustive  repertoire="<<size<<"  candidates="<<tried<<"  depth="<<examined<<"  clipped="<<clipped
                 <<"  satisfied="<<(rank>>56)<<"  origins="<<std::popcount(starts)<<"  stages="<<stages<<'\n';
        if (!rank || !(rank>>56)) return {};
        Word id=~Word(rank);run(lab,"procedure_pronounce",1,1,mind,palette.ptr,id,stages,plan.ptr);InferredPlan out;get(lab,out,plan.ptr);
        if (out.n>performance_span) throw std::logic_error("generated motor utterance exceeds the scored length");
        return {out.sound,out.sound+out.n};
    }
};
void audit_world(PolicyWorkspace &w,const Transfer &projected,Word room,Word lantern,Count goal,Count origins) {
    Temporary result(w.lab.d,sizeof(Transfer));Word limit=4096;
    run(w.lab,"procedure_physical_oracle",1,local_states,w.mind,w.decoder,w.plant.ptr,w.nodes.ptr,w.performance.ptr,room,lantern,limit,result.ptr);
    Transfer measured;get(w.lab,measured,result.ptr);Word correct=0,observed=0,hit=0,determinate=0,promised=0,confirmed=0;
    for (Word x=0;x<local_states;++x) if (measured.destination[x]<local_states) {
        ++observed;determinate+=projected.destination[x]<local_states;
        if (((origins>>x)&1) && projected.destination[x]<local_states && ((goal>>projected.destination[x])&1)) {
            ++promised;confirmed+=measured.destination[x]==projected.destination[x] && measured.pulses[x]==projected.pulses[x];
        }
        correct+=measured.destination[x]==projected.destination[x] && measured.pulses[x]==projected.pulses[x];
        hit+=((origins>>x)&1) && ((goal>>measured.destination[x])&1);
    }
    std::cout<<"physical_audit="<<room<<"  agreement="<<correct<<"  completed="<<observed<<"  starts=64  determinate="<<determinate<<"  promised="<<promised<<"  confirmed="<<confirmed<<"  goals="<<hit<<"  requested="<<std::popcount(origins)<<'\n';
    if (auto *j=journal::current()) {
        j->scores.push_back({"Physical outcomes forecast correctly",correct,local_states});
        if (goal) j->scores.push_back({"Requested origins reaching the goal",hit,Word(std::popcount(origins))});
    }
}
void time_policies(PolicyWorkspace &w,const Transfer &projected,Word n,Word rounds,Word seed,DevicePtr encoded,Word bytes) {
    for (Word x=0;x<local_states;++x) if (projected.destination[x]>=local_states || projected.pulses[x]>1024)
        throw std::invalid_argument("timing the explicit motor interpreter requires a settled policy of at most 1024 moves per origin");
    auto &s=w.lab;Temporary out(s.d,Count(n)*sizeof(Trial));std::vector<Trial> baseline(n),measured(n);cuda::Event begin{},end{};
    s.d.check(s.d.event_new(&begin,0),"create policy clock");s.d.check(s.d.event_new(&end,0),"create policy clock");
    Temporary compact(s.d,bytes);
    s.d.check(s.d.copy_device(compact.ptr,encoded,bytes),"deploy the packed policy");
    Word blocks=(n+127)/128;
    for (Word compiled=0;compiled<4;++compiled) {
        auto rollout=[&]{if (compiled==3) {run(s,"procedure_warp_travel",blocks,128,compact.ptr,out.ptr,n,rounds,seed);return;}
            run(s,"procedure_travel",(n+127)/128,128,w.mind,w.decoder,w.plant.ptr,w.nodes.ptr,w.performance.ptr,w.root,compact.ptr,out.ptr,n,rounds,seed,compiled);};
        rollout();std::array<float,5> times{};
        for (auto &ms:times) {
            s.d.check(s.d.record(begin,nullptr),"start policy clock");rollout();s.d.check(s.d.record(end,nullptr),"stop policy clock");
            s.d.check(s.d.wait(end),"finish policy cohort");s.d.check(s.d.elapsed(&ms,begin,end),"read policy duration");
        }
        auto &sample=compiled ? measured : baseline;
        s.d.check(s.d.download(sample.data(),out.ptr,Count(n)*sizeof(Trial)),"observe complete policy histories");Count moves=0;
        for (Word i=0;i<n;++i) {
            const auto &a=sample[i],&b=baseline[i];moves+=a.movements;
            if (a.state!=b.state || a.movements!=b.movements || a.digest!=b.digest) throw std::logic_error("a compiled history diverged from the acquired motor interpreter");
        }
        std::sort(times.begin(),times.end());std::cout<<"policy_batch="<<(compiled==3 ? "warp" : compiled==2 ? "packed" : compiled ? "compiled" : "interpreted")<<"  animals="<<n<<"  calls="<<Count(n)*rounds
            <<"  represented_moves="<<moves<<"  shared_table_bytes="<<(compiled>=2 ? bytes : compiled ? sizeof(Transfer) : 0)<<"  median_ms="<<times[2]
            <<"  Mcalls_per_s="<<(double(n)*rounds/(1000*times[2]))<<'\n';
    }
    s.d.event_free(begin);s.d.event_free(end);
}
