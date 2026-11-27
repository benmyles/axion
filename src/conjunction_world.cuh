#pragma once
#include "conjunction.cuh"

namespace axion::life::conjunction_world {
AX_FN Word label(Word permutation,Word i) {return (permutation>>(2*i))&3;}
struct Apparatus {
    Word left,right,arrival,mode;
    AX_FN Word scene(Word site) const {return label(left,site&3)|(label(right,site>>2)<<2);}
    AX_FN Word result(Word site) const {
        Word x=site&3,y=site>>2;
        return label(arrival,mode ? ((x+y)&3) : (x^y));
    }
};
struct Pupil {
    BindingField roles{};JointField joint{};Word support=0;
    BindingGenes binding;RecollectionGenes gates,visual;Word cut;
    AX_FN void see(Word scene,Word value) {
        binding::observe(roles,support,binding,visual,scene,value*5);
        if (cut!=1) conjunction::observe_output(joint,visual,scene,0,1u<<value);
    }
    AX_FN Word predict(Word scene) const {
        conjunction::Candidates p{};auto g=gates;
        if (cut==2) g.expression&=~7u;
        if (cut==3) g.low=0;
        if (cut==4) g.expression=(g.expression&~7u)|1;
        if (conjunction::readout(joint,0,visual,g,p)) {
            Word result=conjunction::candidates(p,scene);
            return result&(result-1) ? 0 : result;
        }
        auto paths=binding::readout(roles,support,binding,visual);
        return binding::answer(paths,0,1u<<(scene&3),1u<<(scene>>2),binding);
    }
};
AX_FN void present(Pupil &p,const Apparatus &law,Word mask,Word key) {
    for (Word n=0;n<16;++n) {
        Word site=(key+n*((key>>4)|1))&15;
        if (mask&(1u<<site)) p.see(law.scene(site),law.result(site));
    }
}
AX_FN RecallScore lifetime(RecollectionGenes g,RecollectionGenes visual,BindingGenes binding,Word key,Word cycles,Word cut,bool sparse) {
    RecallScore s{};
    for (Word t=0;t<cycles;++t) {
        Word draw=mix(key^t*0xc7159acbu);
        Apparatus law{imagery::permutation(draw),imagery::permutation(mix(draw+1)),imagery::permutation(mix(draw+2)),(draw>>8)&1};
        Pupil newborn{{},{},0,binding,g,visual,cut};
        {
            // Independently drawn consequences need have no spatial symmetry.
            auto p=newborn;Word sequence=mix(draw+5),target=draw&15;
            for (Word i=0;i<16;++i) if (i!=target) p.see(law.scene(i),label(sequence,i));
            s.empty+=!p.predict(law.scene(target));
            Word answer=label(sequence,target);p.see(law.scene(target),answer);
            s.one+=p.predict(law.scene(target))==(1u<<answer);
            Word changed=(answer+1+((draw>>8)%3))&3;p.see(law.scene(target),changed);
            s.faint+=p.predict(law.scene(target))==(1u<<changed);
        }
        {
            auto p=newborn;Word side=(draw>>12)&1;
            for (Word n=0;n<8;++n) {Word v=(n+draw)&3;p.see(v*5,label(law.arrival,v));}
            Word a=draw&3,b=(a+1)&3;
            p.see(a|(b<<2),label(law.arrival,side ? b : a));
            Word fresh=((a+2)&3)|(((b+2)&3)<<2);
            s.delay+=p.predict(fresh)==(1u<<label(law.arrival,binding::coordinate(fresh,side)));
        }
        {
            // Unknowns form a chain. Its end cells can constrain the centre
            // on later ticks; the animal only sees the uncovered arrangements.
            auto p=newborn;
            Word clues=law.mode ? 0x7662u : 0xd5e8u,site=law.mode ? 4 : 1;
            if (draw&16) {clues=0x8c63u^65535;site=5;}
            if (sparse) {
                Word exposure=(draw>>19)&3;
                if (exposure==1) {clues=0x0007;site=3;}
                if (exposure==2) {clues=0x0111;site=12;}
                if (exposure==3) {clues=0x0217;site=6;}
            }
            present(p,law,clues,draw);
            s.noise+=p.predict(law.scene(site))==(1u<<law.result(site));
        }
        {
            // Removing the four corners of this alternating rectangle permits
            // two opposite completions with precisely the same observations.
            auto p=newborn;law.mode=0;
            Word shown=65535^0x0033u,site=(draw&1)|((draw>>4)&4);
            Word lesson=(draw>>5)&3;
            if (lesson==1) {
                // Four equal observations along a permuted diagonal leave
                // each off-diagonal site with three possible bearings.
                shown=0x8421;site=1+4*(2+(draw&1));
            }
            if (lesson==2) {shown=0x000f;site=4+(draw&3);}
            if (lesson==3) {shown=0x1111;site=1+4*(draw&3);}
            if (sparse && (draw&0x1000)) {
                // A familiar value on one axis can be misleading when the
                // other is new. Several physical completions remain possible.
                if (draw&0x2000) {shown=(draw&0x4000) ? 0x0003 : 0x0007;site=4;}
                else {shown=(draw&0x4000) ? 0x0011 : 0x0111;site=1;}
            }
            present(p,law,shown,draw);
            s.revision+=p.predict(law.scene(site))==0;
        }
    }
    return s;
}
}
