// This is an environment and an independent shortest-path scorer. Only the
// current and desired sensory ensembles are inputs to the resident circuit.
namespace {
AX_FN Word exit_to(Word doors,Word action,Word rotation) { return (doors>>(8*((action+rotation)&3)))&31; }
AX_FN Word passage_to(const Word *doors,Word room,Word pins,Word rotation,Word gait=8) {
    return (pins&gait) ? exit_to(doors[room],pins,rotation) : room;
}
AX_FN Word maze_seed(Word seed,Word world) { return mix(seed^world*0x345c8a71u^0x3e42de17u); }
AX_FN Word door_word(Word room,Word size,Word seed,bool changed) {
    Word limit=size-1,noise=mix(seed^room*0x38567a93u),layout=permutation(mix(noise+13),0);
    Word shortcut=2+mix(noise+1)%(size-3);
    if (changed) shortcut=2+(shortcut-2+1+mix(noise+2)%(size-4))%(size-3);
    Word destinations[]{(room+1)&limit,(room-1)&limit,(room+shortcut)&limit,room};
    Word result=0;
    UNROLL
    for (Word m=0;m<4;++m) result|=destinations[(layout>>(2*m))&3]<<(8*m);
    return result;
}
}
ENTRY void lay_maze(Maze *worlds,Word count,Word size,Word seed,Word catchment) {
    Word id=index(),lane=id&31,target=(id>>5)%size,world=(id>>5)/size;
    if (world>=count) return;
    Word key=catchment ? mix(seed^world*0xa08bd2bdu) : maze_seed(seed,world);
    Word match=catchment ? (key>>19)&3 : mix(key^0x395cf57u)&3;
    bool junction=lane<size && (lane&3)==match;
    Word junctions=0;
    asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0;vote.sync.ballot.b32 %0,p,-1; }":"=r"(junctions):"r"(Word(junction)));
    auto &m=worlds[world];
    if (!lane && !target) m.junctions=junctions;
    for (Word epoch=0;epoch<2;++epoch) {
        Word doors=door_word(lane,size,key,epoch && junction);
        if (!target && lane<size) (epoch ? m.changed : m.original)[lane]=doors;
        Word distance=lane==target ? 0 : 63;
        // All edges have unit length. The ring makes every room reachable.
        for (Word k=0;k<size;++k) {
            Word best=distance;
            UNROLL
            for (Word a=0;a<4;++a) best=lesser(best,1+shuffle(distance,(doors>>(8*a))&31,~0u));
            Word changes;
            asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0;vote.sync.ballot.b32 %0,p,-1; }":"=r"(changes):"r"(Word(best!=distance)));
            distance=best;if (!changes) break;
        }
        if (lane<size) m.distances[epoch][target][lane]=static_cast<unsigned char>(distance);
    }
}
namespace {
AX_FN Word best_grips(const Maze &m,Word epoch,Word source,Word goal,Word rotation) {
    Word dist=m.distances[epoch][goal][source],moves=0,doors=(epoch ? m.changed : m.original)[source];
    UNROLL
    for (Word a=0;a<4;++a) moves|=Word(1+m.distances[epoch][goal][exit_to(doors,a,rotation)]==dist)<<a;
    return moves;
}

} // namespace
