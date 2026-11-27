#pragma once

void scale_population(Session &s,DevicePtr brains,DevicePtr language,Word models,Word replicas,Word ticks,Word seed,Word style) {
    models=std::min(models,replicas);
    Temporary decks(s.d,Count(models)*sizeof(Catalog)),layouts(s.d,Count(models)*sizeof(Atlas));
    launch(s,"compose_deck",models,1,brains,language,seed,style,decks.ptr,layouts.ptr);
    std::vector<Atlas> atlas(models);s.d.check(s.d.download(atlas.data(),layouts.ptr,atlas.size()*sizeof(Atlas)),"inspect calibrated input layouts");
    Word words=1,keys=0,gates=0;
    for (auto &t:atlas) {
        if (t.unsupported) throw std::runtime_error("the calibrated acoustic catalog exceeds the compiled address space");
        words=std::max(words,t.words());keys=std::max(keys,t.n);gates=std::max(gates,t.views);
    }
    Word audit=std::min(256u,replicas),audit_ticks=std::max(ticks,256u),blocks=(replicas+127)/128;
    Temporary general(s.d,Count(replicas)*sizeof(Connections<slots>)),values(s.d,4ull*words*replicas),digest(s.d,replicas*sizeof(Word)),history(s.d,Count(replicas)*sizeof(Discourse)),audit_result(s.d,3*audit*sizeof(Word));
    launch(s,"compose_equivalence",(audit+127)/128,128,brains,decks.ptr,layouts.ptr,values.ptr,general.ptr,audit_result.ptr,audit,models,audit_ticks);
    std::vector<Word> comparisons(3*audit);s.d.check(s.d.download(comparisons.data(),audit_result.ptr,comparisons.size()*sizeof(Word)),"compare live and remembered contact values");
    Count checks=0,voiced=0;
    for (Word i=0;i<audit;++i) {
        if (comparisons[3*i]) throw std::logic_error("compiled temporal memory diverged from the acquired graph");
        voiced+=comparisons[3*i+1];checks+=comparisons[3*i+2];
    }
    cuda::Event begin{},end{};s.d.check(s.d.event_new(&begin,0),"create population clock");s.d.check(s.d.event_new(&end,0),"create population clock");
    std::vector<Word> reference(replicas),measured(replicas);
    for (Word packed=0;packed<2;++packed) {
        Word start=0;
        auto advance=[&]() {
            if (packed) launch(s,"compose_compact",blocks,128,brains,decks.ptr,layouts.ptr,values.ptr,history.ptr,digest.ptr,replicas,models,start,ticks);
            else launch(s,"compose_general",blocks,128,brains,decks.ptr,general.ptr,digest.ptr,replicas,models,ticks);
        };
        advance();std::array<float,5> time{};
        for (auto &ms:time) {
            s.d.check(s.d.record(begin,nullptr),"start population clock");advance();
            s.d.check(s.d.record(end,nullptr),"stop population clock");s.d.check(s.d.wait(end),"finish population inference");
            s.d.check(s.d.elapsed(&ms,begin,end),"measure population inference");
        }
        s.d.check(s.d.download(measured.data(),digest.ptr,replicas*sizeof(Word)),"compare complete acoustic histories");
        if (!packed) reference=measured;else if (reference!=measured) throw std::logic_error("compiled population emitted a different history");
        if (packed && ticks>1) {
            Word part=ticks/2,rest=ticks-part;
            launch(s,"compose_compact",blocks,128,brains,decks.ptr,layouts.ptr,values.ptr,history.ptr,digest.ptr,replicas,models,start,part);
            launch(s,"compose_compact",blocks,128,brains,decks.ptr,layouts.ptr,values.ptr,history.ptr,digest.ptr,replicas,models,part,rest);
            s.d.check(s.d.download(measured.data(),digest.ptr,replicas*sizeof(Word)),"check split acquired histories");
            if (reference!=measured) throw std::logic_error("resuming a packed conversation changed its answers");
        }
        std::sort(time.begin(),time.end());
        std::cout<<"composition_batch="<<(packed ? "packed" : "temporal")<<"  replicas="<<replicas<<"  acquired_models="<<models<<"  events="<<ticks
                 <<"  addresses="<<keys<<"  shutters="<<gates<<"  plastic_state_bytes="<<(packed ? 4*words : sizeof(Connections<slots>))
                 <<"  discourse_bytes="<<(packed ? sizeof(Discourse) : 0)<<"  checked_values="<<checks<<"  checked_replies="<<voiced<<"  checked_events="<<Count(audit)*audit_ticks
                 <<"  median_ms="<<time[2]<<"  Mevents_per_s="<<double(replicas)*ticks/(1e3*time[2])<<'\n';
    }
    s.d.event_free(begin);s.d.event_free(end);
}
