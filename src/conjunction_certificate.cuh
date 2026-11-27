#pragma once
// External scorer. The possible-world table is never an input to the animal.
KERNEL void joint_oracle_seed(Word *possible,const Word *references,const Word *apparatuses,Word count) {
    Word i=thread_index();if (i>=576*count) return;
    Word tid=i/576,world=apparatuses[i%576],actual=references[tid],agreement=0,planes[4]{};
    for (Word s=0;s<16;++s) {
        Word bearing=(world>>(2*s))&3;
        agreement|=Word(bearing==((actual>>(2*s))&3))<<s;
        planes[bearing]|=1u<<s;
    }
    Word *entry=possible+2*(65536*tid+agreement);
    for (Word k=0;k<2;++k) {
        Word packed=planes[2*k]|(planes[2*k+1]<<16);
        asm volatile("red.global.or.b32 [%0],%1;"::"l"(entry+k),"r"(packed):"memory");
    }
}
KERNEL void joint_oracle_closure(Word *possible,Word count,Word bit) {
    Word i=thread_index();if (i>=65536*count) return;
    Word pair=i>>1,low=(1u<<bit)-1;
    Word site=2*((pair&low)|((pair&~low)<<1))+(i&1);
    possible[site]|=possible[site+(2u<<bit)];
}
KERNEL void joint_certify(const Word *possible,const Word *references,Word count,Word seed,RecollectionGenes vision,
                          BindingGenes sources,RecollectionGenes gates,Word *totals) {
    __attribute__((shared)) static Word bins[17*7];
    Word i=thread_index(),lane=i&127;
    if (lane<17*7) bins[lane]=0;
    asm volatile("bar.sync 0;":::"memory");
    Word tid=i>>16,shown=i&65535;
    if (tid<count && shown && shown!=65535) {
        // Counterfactual worlds with the same visible cells receive exactly
        // the same presentation order, independent of concealed outcomes.
        Word actual=references[tid],order=mix(shown^seed);
        conjunction_world::Pupil animal{{},{},0,sources,gates,vision,0};
        for (Word j=0;j<16;++j) {
            Word scene=(order+j*((order>>8)|1))&15;
            if (shown&(1u<<scene)) animal.see(scene,(actual>>(2*scene))&3);
        }
        conjunction::Candidates p{};
        bool joint=conjunction::readout(animal.joint,0,vision,gates,p);
        auto roles=binding::readout(animal.roles,animal.support,sources,vision);
        Word a=possible[2*i],b=possible[2*i+1];
        conjunction::Candidates oracle{{a&65535,a>>16,b&65535,b>>16}};
        Word totals_here[7]{},untested=65535^shown;
        while (untested) {
            Word scene=Word(__builtin_ctz(untested));untested&=untested-1;
            Word reply=joint ? conjunction::candidates(p,scene) : binding::answer(roles,0,1u<<(scene&3),1u<<(scene>>2),sources);
            if (reply&(reply-1)) reply=0;
            Word alternatives=conjunction::candidates(oracle,scene),truth=1u<<((actual>>(2*scene))&3);
            if (!(alternatives&truth)) ++totals_here[6];
            Word group=(alternatives&(alternatives-1)) ? 3 : 0;
            ++totals_here[group+(!reply ? 1 : reply==truth ? 0 : 2)];
        }
        Word base=7*Word(__builtin_popcount(shown));
        for (Word k=0;k<7;++k) if (totals_here[k])
            __atomic_fetch_add(bins+base+k,totals_here[k],__ATOMIC_RELAXED);
    }
    asm volatile("bar.sync 0;":::"memory");
    if (lane<17*7 && bins[lane]) asm volatile("red.global.add.u32 [%0],%1;"::"l"(totals+lane),"r"(bins[lane]):"memory");
}
