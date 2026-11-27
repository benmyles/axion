#include "axion/journal.hpp"
#include "axion/census.hpp"
using namespace axion;
using namespace axion::life;
extern "C" __attribute__((global)) void journal_field(const Site *world,const Genome *genes,const Contacts *contacts,
        Settings c,Word edge,Word limit,journal::FieldTile *tiles,journal::Animal *samples,const OutcomeLayer *organs) {
    Word block,lane,width;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(block));asm("mov.u32 %0, %%tid.x;" : "=r"(lane));asm("mov.u32 %0, %%ntid.x;" : "=r"(width));
    Word cell=block*width+lane;
    if (cell>=edge*edge) return;
    Word span=c.side/edge,x=(cell%edge)*span,y=(cell/edge)*span,food=0,alive=0,founder=0,least=~0u;
    for (Word dy=0;dy<span;++dy) for (Word dx=0;dx<span;++dx) {
        Word pos=(y+dy)*c.side+x+dx; const auto &s=world[pos];const auto &b=s.body;food+=s.food;
        if (b.energy) { ++alive;Word key=mix(b.slot^c.seed);if (key<least) {least=key;founder=b.founder;} }
        // The odd multiplier permutes slots. A sampled slot has one writer even after relocation.
        Word watch=(b.slot*0x9e3779b1u+c.seed)&(c.side*c.side-1);
        if (watch>=limit) continue;
        if (!b.energy) { samples[watch]={};continue; }
        const auto &g=genes[b.slot];Word issued=contacts ? contacts[b.slot].previous : 0,motor=(issued>>5)&7;
        Word pins=issued ? (g.effectors[motor>>2]>>(8*(motor&3)))&255 : 0;
        Word rule=c.navigation ? map_tail(organs,c.side*c.side)[b.slot].genes.plastic : 0;
        TraceGenes temporal=c.navigation>=4 ? trace_tail(organs,c.side*c.side)[b.slot].genes : TraceGenes{};
        samples[watch]={b.slot,pos,b.founder,b.born,b.generation,b.energy,g.body,pins,b.heading,1+(g.expression&7),
                        g.expression,g.effectors[0],g.effectors[1],rule,temporal};
    }
    tiles[cell]={static_cast<unsigned short>(food),static_cast<unsigned short>(alive),founder};
}

// Sample the scores already assigned to candidates. The breeding kernel owns
// selection. These counters have no connection back to the simulation.
extern "C" __attribute__((global)) void journal_brood(const Aptitude *scores,Word animals,journal::BroodScore *record) {
    Word block,lane;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(block));
    asm("mov.u32 %0, %%tid.x;" : "=r"(lane));
    Word id=block*128+lane;
    if (id>=animals) return;
    Count points=0;
    for (Word value:scores[id].score) points+=value;
    asm volatile("red.global.add.u64 [%0],%1;" :: "l"(&record->total),"l"(points):"memory");
    asm volatile("red.global.max.u64 [%0],%1;" :: "l"(&record->best),"l"(points):"memory");
}

extern "C" __attribute__((global)) void journal_population(const Site *world,const Genome *genes,Settings c,journal::CensusMember *out) {
    Word lane,block,width;asm("mov.u32 %0, %%tid.x;":"=r"(lane));asm("mov.u32 %0, %%ctaid.x;":"=r"(block));asm("mov.u32 %0, %%ntid.x;":"=r"(width));
    Word i=block*width+lane;if (i>=c.side*c.side) return;
    auto b=world[i].body;
    if (!b.energy) {out[i]={};return;}
    const auto &g=genes[b.slot];
    out[i]={g.body,b.slot,b.energy,b.founder,32*living_tiles(c.tiles,g.development),1+(g.expression&7),g.effectors[0],g.effectors[1]};
}
