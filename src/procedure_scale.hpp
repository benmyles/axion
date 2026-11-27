void family_scale(PolicyWorkspace &w,RecollectionGenes optics,RecollectionGenes priorities,Word families,Word actors,Word ticks,Word seed) {
    auto &s=w.lab;Word stride=w.ast_nodes+1;
    Word batch=std::min(families,std::max(1u,Word((8u<<20)/(sizeof(Transfer)*stride))));
    Temporary models(s.d,Count(families)*sizeof(MotorModel)),scores(s.d,Count(families)*4*operations*sizeof(Word)),
        roots(s.d,Count(families)*sizeof(Transfer)),rows(s.d,Count(batch)*stride*sizeof(Transfer)),staging(s.d,Count(families)*sizeof(CompactTransfer)),
        offsets(s.d,families*sizeof(Word)),counter(s.d,sizeof(Word)),verified(s.d,Count(families)*local_states*sizeof(Word));
    Word contacts=64,used=0;put(s,counter.ptr,used);
    run(s,"procedure_family_learn",families*operations,32,w.plant.ptr,optics,priorities,models.ptr,seed,contacts,scores.ptr);
    for (Word start=0;start<families;start+=batch)
        run(s,"procedure_family_compile",std::min(batch,families-start),64,w.mind,w.decoder,w.performance.ptr,w.nodes.ptr,models.ptr,
            rows.ptr,roots.ptr,staging.ptr,offsets.ptr,counter.ptr,stride,start);
    get(s,used,counter.ptr);if (used>Count(families)*sizeof(CompactTransfer)) throw std::logic_error("compiled cohort exceeded the transfer arena");
    Temporary packed(s.d,used);s.d.check(s.d.copy_device(packed.ptr,staging.ptr,used),"deploy the independently learned tables");
    run(s,"procedure_family_audit",families,64,w.mind,w.decoder,w.performance.ptr,w.nodes.ptr,models.ptr,roots.ptr,packed.ptr,offsets.ptr,seed,verified.ptr);
    std::vector<Word> checks(families*local_states);
    s.d.check(s.d.download(checks.data(),verified.ptr,checks.size()*sizeof(Word)),"audit every start in each physical room");
    if (!std::all_of(checks.begin(),checks.end(),[](Word value){return value==7;}))
        throw std::invalid_argument("independent room timing requires grounded, settled paths of at most 1024 physical actions");
    Temporary result(s.d,Count(actors)*sizeof(Trial));std::vector<Trial> baseline(actors),other(actors);
    cuda::Event begin{},end{};s.d.check(s.d.event_new(&begin,0),"create cohort clock");s.d.check(s.d.event_new(&end,0),"create cohort clock");
    for (Word mode=0;mode<3;++mode) {
        auto simulate=[&]{run(s,"procedure_family_travel",(actors+127)/128,128,w.mind,w.decoder,w.performance.ptr,w.nodes.ptr,models.ptr,roots.ptr,packed.ptr,offsets.ptr,
                             result.ptr,actors,families,ticks,seed,mode);};
        simulate();std::array<float,5> times{};
        for (auto &ms:times) {
            s.d.check(s.d.record(begin,nullptr),"start independent histories");simulate();s.d.check(s.d.record(end,nullptr),"stop independent histories");
            s.d.check(s.d.wait(end),"finish independent histories");s.d.check(s.d.elapsed(&ms,begin,end),"read cohort duration");
        }
        auto &sample=mode ? other : baseline;s.d.check(s.d.download(sample.data(),result.ptr,Count(actors)*sizeof(Trial)),"compare each private history");
        Count moves=0;for (Word i=0;i<actors;++i) {
            const auto &a=sample[i],&b=baseline[i];moves+=a.movements;
            if (a.state!=b.state || a.digest!=b.digest || a.movements!=b.movements) throw std::logic_error("compiled independent model diverged");
        }
        std::sort(times.begin(),times.end());
        std::cout<<"family_batch="<<(mode==2 ? "packed" : mode ? "compiled" : "interpreted")<<"  families="<<families<<"  animals="<<actors<<"  calls="<<Count(actors)*ticks
                 <<"  audited="<<checks.size()<<"  represented_moves="<<moves<<"  model_bytes="<<(mode==2 ? used+4*families : mode ? families*sizeof(Transfer) : families*sizeof(MotorModel))
                 <<"  median_ms="<<times[2]<<"  Mcalls_per_s="<<(double(actors)*ticks/(1000*times[2]))<<'\n';
    }
    s.d.event_free(begin);s.d.event_free(end);
}
