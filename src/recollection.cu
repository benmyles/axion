#include "recollection.cuh"
#include "memory_proof.cuh"
#include "attention_school.cuh"
#include "attention_batch.cuh"
#include "imagery_world.cuh"
#include "binding_world.cuh"
#include "conjunction_world.cuh"
#include "conjunction_batch.cuh"
#include "report_world.cuh"
#include "concept_world.cuh"
using namespace axion;
using namespace axion::life;
using namespace axion::life::engram;
#define KERNEL extern "C" __attribute__((global))
namespace {
AX_FN Word thread_index() {
    Word block,lane,width;
    asm("mov.u32 %0, %%ctaid.x;":"=r"(block));
    asm("mov.u32 %0, %%tid.x;":"=r"(lane));
    asm("mov.u32 %0, %%ntid.x;":"=r"(width));
    return block*width+lane;
}
struct Pupil {
    Recollection memory;
    RecallCut cut;
    Word last=0,by_cue[4]{};
    AX_FN void observe(Word cue,Word bearing,bool clear) {
        last=1u<<bearing;by_cue[cue]=last;
        if (cut!=RecallCut::fixed) see(memory,cue,last,cut==RecallCut::source ? !clear : clear);
    }
    AX_FN Word recall(Word cue) {
        if (cut==RecallCut::last) return by_cue[cue];
        if (cut==RecallCut::immediate) return last;
        if (cut==RecallCut::erased) return 0;
        Word result=reinstate(memory,cut==RecallCut::crossed ? (cue+1)&3 : cue);
        if (cut!=RecallCut::fixed) see(memory,cue,0,false);
        return result;
    }
};
AX_FN RecallScore lifetime(RecollectionGenes genes,Word seed,Word cycles,Word delay,Word varied,RecallCut cut) {
    Pupil brain{{genes,0},cut};
    RecallScore score{};
    for (Word v=0;v<4;++v) score.empty+=brain.recall(v)==0;
    for (Word turn=0;turn<cycles;++turn) {
        // All candidates meet the same individual habitat in a generation.
        // Every scoring read occurs with the retina shuttered.
        Word draw=mix(seed^turn*0xb854ac6du),cue=draw&3,bearing=(draw>>5)&3;
        Word changed=(bearing+1+((draw>>10)%3))&3,dim=(changed+1+((draw>>16)%3))&3;
        brain.observe(cue,bearing,true);
        score.one+=brain.recall(cue)==(1u<<bearing);
        Word interval=varied ? mix(draw^0x574aa3fbu)%(delay+1) : delay;
        for (Word k=0;k<interval;++k) {
            Word other=mix(draw^k*0xf27da28bu^0x91c53db9u);
            brain.observe((cue+1+other%3)&3,(other>>10)&3,(other&8)!=0);
            if (varied && (other&16)) brain.recall(cue);
        }
        score.delay+=brain.recall(cue)==(1u<<bearing);
        brain.observe(cue,(bearing+1+((draw>>13)%3))&3,false);
        score.noise+=brain.recall(cue)==(1u<<bearing);
        brain.observe(cue,changed,true);
        score.revision+=brain.recall(cue)==(1u<<changed);
        for (Word k=0;k<4;++k) brain.observe(cue,dim,false);
        score.faint+=brain.recall(cue)==(1u<<dim);
    }
    return score;
}
AX_FN Word merit(RecallScore s) {
    Word sum=s.one+s.delay+s.noise+s.revision+s.faint,minimum=s.one;
    Word others[]{s.delay,s.noise,s.revision,s.faint};
    for (Word x:others) minimum=x<minimum ? x : minimum;
    return 8*minimum+sum;
}
AX_FN Word pick(const Word *fitness,Word count,Word key,bool drift) {
    Word best=mix(key)%count;
    if (!drift) for (Word j=1;j<6;++j) {
        Word candidate=mix(key+j*0x8d327a63u)%count;
        if (fitness[candidate]>fitness[best]) best=candidate;
    }
    return best;
}
}
KERNEL void recollect_seed(RecollectionGenes *pool,Word n,RecollectionGenes parent,Word seed,Word origin) {
    Word i=thread_index();if (i>=n) return;
    auto g=parent;
    if (origin==1) g=RecollectionGenes{};
    if (origin==2) {Word key=mix(seed^i);g={mix(key),mix(key+1),mix(key+2)};}
    pool[i]=g;
}
KERNEL void conjunction_live(const RecollectionGenes *pool,RecallScore *out,Word count,Word worlds,Word seed,Word cycles,
                             Word,Word,Word cut,Word first,Word batch,RecollectionGenes vision,BindingGenes binding,Word sparse) {
    Word local=thread_index();if (local>=batch) return;
    Word i=first+local;if (i>=count*worlds) return;
    out[i]=conjunction_world::lifetime(pool[i/worlds],vision,binding,mix(seed^(i%worlds)*0xda3e39cbu),cycles,cut,sparse!=0);
}
KERNEL void joint_batch_seed(Word *bank,Word n,Word seed,RecollectionGenes vision,BindingGenes sources) {
    Word i=thread_index();if (i>=n) return;
    auto a=conjunction_batch::rear(mix(seed^i*0xb755403bu),i&3,vision,sources);
    conjunction_batch::write(a,bank,n,i);
}
KERNEL void conjunction_snapshot(Word *out,Word key,RecollectionGenes gates,RecollectionGenes vision,BindingGenes sources) {
    if (thread_index()) return;
    using namespace conjunction_world;
    Apparatus law{imagery::permutation(key),imagery::permutation(mix(key+1)),imagery::permutation(mix(key+2)),key&1};
    conjunction_world::Pupil p{{},{},0,sources,gates,vision,0};Word clues=law.mode ? 0x7662u : 0xd5e8u;
    present(p,law,clues,key);
    Word observed=0,physical=0,reported=0,audible=0;
    for (Word site=0;site<16;++site) {
        Word scene=law.scene(site),m=p.predict(scene);
        observed|=((clues>>site)&1)<<scene;
        physical|=law.result(site)<<(2*scene);
        if (m) {audible|=1u<<scene;reported|=Word(__builtin_ctz(m))<<(2*scene);}
    }
    out[0]=observed;out[1]=physical;out[2]=reported;out[3]=audible;
}
KERNEL void conjunction_merit(const RecallScore *scores,Word *fitness,Word n,Word worlds) {
    Word i=thread_index();if (i>=n) return;
    Word total=0;
    for (Word w=0;w<worlds;++w) {
        auto s=scores[i*worlds+w];Word sums[6]{s.one,s.delay,s.noise,s.revision,s.faint,s.empty},weakest=s.one;
        for (Word v:sums) {weakest=v<weakest ? v : weakest;total+=v;}
        total+=8*weakest;
    }
    fitness[i]=total;
}
KERNEL void report_live(const RecollectionGenes *pool,RecallScore *out,Word count,Word worlds,Word salt,Word cycles,
                        Word lessons,Word,Word cut,Word first,Word batch,RecollectionGenes,BindingGenes,Word setting) {
    Word local=thread_index();if (local>=batch) return;
    Word i=first+local;if (i>=count*worlds) return;
    out[i]=report_world::lifetime(pool[i/worlds],mix(salt^(i%worlds)*0x24757bd5u),cycles,lessons,setting,cut);
}
KERNEL void report_merit(const RecallScore *scores,Word *fitness,Word count,Word worlds) {
    Word i=thread_index();if (i>=count) return;Word total=0;
    for (Word j=0;j<worlds;++j) {
        auto s=scores[i*worlds+j];
        total+=s.one+8*s.delay+10*s.noise+2*s.revision+2*s.faint+4*s.empty;
    }
    fitness[i]=total;
}
KERNEL void concept_live(const RecollectionGenes *pool,RecallScore *out,Word count,Word worlds,Word salt,Word cycles,
                         Word lessons,Word,Word cut,Word first,Word batch,RecollectionGenes,BindingGenes,Word) {
    Word local=thread_index();if (local>=batch) return;
    Word i=first+local;if (i>=count*worlds) return;
    auto g=pool[i/worlds];if (cut==4) g.high=0;
    out[i]=concept_world::lifetime(g,mix(salt^(i%worlds)*0xd67110d5u),cycles,lessons,cut);
}
KERNEL void concept_merit(const RecallScore *scores,Word *fitness,Word count,Word worlds) {
    Word i=thread_index();if (i>=count) return;Word total=0;
    for (Word j=0;j<worlds;++j) {
        auto s=scores[i*worlds+j];Word components[6]{s.one,s.delay,8*s.noise,8*s.revision,8*s.faint,8*s.empty},weakest=s.one;
        for (Word v:components) {total+=v;if (v<weakest) weakest=v;}
        total+=4*weakest;
    }
    fitness[i]=total;
}
KERNEL void joint_batch_compile(const Word *bank,Word *packed,Word n,RecollectionGenes vision,BindingGenes sources,RecollectionGenes gates) {
    Word i=thread_index();if (i>=n) return;
    auto a=conjunction_batch::read(bank,n,i);auto p=conjunction_batch::compile(a,vision,sources,gates);
    packed[i]=p.bearings[0];packed[n+i]=p.bearings[1];packed[2*n+i]=p.valid;
}
KERNEL void joint_batch_predict(const Word *bank,Word n,Word salt,Word steps,Word packed,Word *out,
                                RecollectionGenes vision,BindingGenes sources,RecollectionGenes gates) {
    Word i=thread_index();if (i>=n) return;
    conjunction::Candidates p[2]{};RoleProjection roles{};Word active=0;
    conjunction_batch::Frozen frozen{};
    if (packed) frozen={{bank[i],bank[n+i]},bank[2*n+i]};
    else {
        auto a=conjunction_batch::read(bank,n,i);
        roles=binding::readout(a.roles,a.strength,sources,vision);
        for (Word o=0;o<2;++o) active|=Word(conjunction::readout(a.joint,o,vision,gates,p[o]))<<o;
    }
    Word checksum=0,spoken=0,start=mix(salt^i);
    conjunction_batch::Frozen reported{};
    for (Word k=0;k<steps;++k) {
        // Each block of 32 questions visits both outputs of all sixteen
        // arrangements, in an independently phased order for every animal.
        Word draw=start+k*((start>>8)|1),scene=draw&15,side=(draw>>4)&1;
        Word m=packed ? conjunction_batch::query(frozen,scene,side) : (active&(1u<<side)) ? conjunction::candidates(p[side],scene) :
                    binding::answer(roles,side,1u<<(scene&3),1u<<(scene>>2),sources);
        if (m&(m-1)) m=0;
        if (m) {
            reported.valid|=1u<<(16*side+scene);
            reported.bearings[side]|=Word(__builtin_ctz(m))<<(2*scene);
            ++spoken;
        }
        checksum=checksum*0x9e3779b1u+(m^k);
    }
    out[i]=checksum;out[n+i]=spoken;
    out[2*n+i]=reported.bearings[0];out[3*n+i]=reported.bearings[1];out[4*n+i]=reported.valid;
}
KERNEL void recollect_live(const RecollectionGenes *pool,RecallScore *out,Word candidates,Word worlds,Word seed,Word cycles,Word delay,Word varied,RecallCut cut,Word first,Word batch) {
    Word local=thread_index();if (local>=batch) return;
    Word i=first+local;if (i>=candidates*worlds) return;
    out[i]=lifetime(pool[i/worlds],mix(seed^(i%worlds)*0x94b32a71u),cycles,delay,varied,cut);
}
KERNEL void recollect_merit(const RecallScore *out,Word *fitness,Word count,Word worlds) {
    Word i=thread_index();if (i>=count) return;
    RecallScore s{};
    for (Word k=0;k<worlds;++k) {
        const auto &v=out[i*worlds+k];
        s.one+=v.one;s.delay+=v.delay;s.noise+=v.noise;s.revision+=v.revision;s.faint+=v.faint;
    }
    fitness[i]=merit(s);
}
KERNEL void focus_live(const RecollectionGenes *pool,RecallScore *out,Word candidates,Word worlds,Word seed,Word cycles,Word length,Word,Word cut,Word first,Word batch) {
    Word local=thread_index();if (local>=batch) return;
    Word i=first+local;if (i>=candidates*worlds) return;
    out[i]=school::course(pool[i/worlds],mix(seed^(i%worlds)*0x94b32a71u),cycles,length,cut);
}
KERNEL void recollect_proof_merit(const MemoryProof *proofs,Word *fitness,Word count) {
    Word i=thread_index();if (i>=count) return;
    const auto &p=proofs[i];Word total=0,weakest=4096;
    for (Word k=0;k<4;++k) {
        Word good=4096*(p.checked[k]-p.failed[k])/p.checked[k];
        total+=good;weakest=good<weakest ? good : weakest;
    }
    fitness[i]=8*weakest+total;
}
KERNEL void recollect_brood(const RecollectionGenes *parents,RecollectionGenes *young,const Word *fitness,Word n,Word seed,Word drift,Word keep) {
    Word i=thread_index();if (i>=n) return;
    if (!i && keep && !drift) {
        Word elite=0;
        for (Word k=1;k<n;++k) if (fitness[k]>fitness[elite]) elite=k;
        young[0]=parents[elite];return;
    }
    Word key=mix(seed^i*0x7d213a97u);
    auto a=parents[pick(fitness,n,key,drift!=0)],b=parents[pick(fitness,n,key+1,drift!=0)];
    Word genes[3]{a.low,a.high,a.expression},other[3]{b.low,b.high,b.expression};
    for (Word k=0;k<3;++k) {
        Word draw=mix(key^(0xbd739a53u*(k+1)));
        if ((key&7)==0) genes[k]=select_bits(mix(draw+1),genes[k],other[k]);
        if ((draw&3)!=0) genes[k]^=1u<<((draw>>8)&31);
    }
    young[i]={genes[0],genes[1],genes[2]};
}
#include "conjunction_certificate.cuh"
#include "concept_batch.cuh"
// A byte contains three destination spikes and one unresolved-branch flag.
// Every worker sees one complete three-object graph; no reference law is supplied.
KERNEL void concept_census(const Concepts *mind,Word op,unsigned char *answers,Word count) {
    Word i=thread_index();if (i>=count) return;
    RelationalView scene{};Word code=i,gate=(mind->genes.low>>12)&3;
    for (Word r=0;r<4;++r) for (Word x=0;x<3;++x) {
        Word target=code%3;code/=3;
        if (gate==1 || gate==3) meaning::store(scene.relation[r],x,target);
    }
    for (Word x=0;x<3;++x) {
        auto pulse=meaning::infer_view(*mind,op,scene,7,{1u<<x,0});
        answers[3*i+x]=static_cast<unsigned char>(pulse.cells|(pulse.gap<<3));
    }
}
