#pragma once
#include "procedure_guard.cuh"

namespace axion::life::procedure {
constexpr Word stack_limit=128;
constexpr Half uncertain=0x8000;
struct Shape {Byte front,middle,back,link;};
struct Decoder {Shape shape[productions]{};Byte head[letters+1]{};Word direction=0;};
AX_FN bool unique_heads(const Circuit &p,Decoder &d,bool backwards) {
    for (auto &h:d.head) h=0;
    for (Word i=0;i<p.count;++i) {
        auto x=cue(p,i);auto &s=d.shape[i];Word length=p.cell[i].arity ? (backwards ? s.back : s.front) : x.n;
        if (!length || p.cell[i].raw) return false;
        Word lead=x.p[backwards ? x.n-1 : 0];
        for (Word j=d.head[lead];j;j=d.shape[j-1].link) {
            auto y=cue(p,j-1);Word m=p.cell[j-1].arity ? (backwards ? d.shape[j-1].back : d.shape[j-1].front) : y.n;
            Word common=length<m ? length : m,k=0;
            while (k<common && x.p[backwards ? x.n-1-k : k]==y.p[backwards ? y.n-1-k : k]) ++k;
            if (k==common) return false;
        }
        s.link=d.head[lead];d.head[lead]=static_cast<Byte>(i+1);
    }
    return p.count!=0;
}
AX_FN void compile(const Circuit &p,Decoder &d) {
    for (Word i=0;i<p.count;++i) {
        auto r=p.cell[i];auto x=cue(p,i);Layout s=r.arity ? layout(x,r.arity) : Layout{x.n,0,0};
        d.shape[i]={static_cast<Byte>(s.start),static_cast<Byte>(s.between),static_cast<Byte>(s.end),0};
    }
    d.direction=unique_heads(p,d,false) ? 1 : unique_heads(p,d,true) ? 2 : 0;
}
constexpr Count radix=0x100000001b3ull;
AX_FN bool combine(Summary &a,Summary b) {
    if (~0ull-a.length<b.length) return false;
    a.hash=a.hash*b.power+b.hash;a.power*=b.power;a.length+=b.length;return true;
}
AX_FN void discharge(Summary &s,Count pulse) {s.hash=s.hash*radix+pulse;s.power*=radix;++s.length;}
struct Tissue {
    Node *node;Summary *action;Half *grid;Word limit,n;bool structure=false,override_eyes=false;Word retina=0,seeing=0;
};
AX_FN Slice bound(Slice text,const Node &n,const Shape &s,Word which) {
    Word left=n.begin+s.front,end=n.end-s.back;
    if (!which) return part(text,left,n.split-left);
    Word right=n.split+s.middle;return part(text,right,end-right);
}
AX_FN bool measure(const Mind &m,const Decoder &d,const Tissue &t,Slice x,const Node &node,Summary &s) {
    int choice=switch_to(m.circuit,node.rule,t.override_eyes ? t.retina : m.retina,t.override_eyes ? t.seeing : m.observing);
    if (choice<0) {s={0,0,0};return true;}
    auto pattern=part_action(m.circuit,node.rule,choice>0);s={};
    for (Word k=0;k<pattern.n;++k) {
        Byte op=pattern.p[k];
        if (op<call0) {if (s.length==~0ull) return false;discharge(s,m.ear.model[op]);}
        else if (op<=call1) {Word child=op==call0 ? node.left : node.right;if (!t.action[child].power) {s={0,0,0};return true;}if (!combine(s,t.action[child])) return false;}
        else {
            auto raw=bound(x,node,d.shape[node.rule],slot(op));Summary spoken;
            for (Word j=0;j<raw.n;++j) discharge(spoken,m.ear.model[raw.p[j]]);
            if (!combine(s,spoken)) return false;
        }
    }
    return true;
}
AX_FN Word sound_at(const Mind &m,const Decoder &d,const Tissue &t,Slice x,Word root,Count index) {
    while (root && index<t.action[root].length) {
        const auto &n=t.node[root];auto plan=part_action(m.circuit,n.rule,switch_to(m.circuit,n.rule,t.override_eyes ? t.retina : m.retina,t.override_eyes ? t.seeing : m.observing)>0);Word next=0;
        for (Word k=0;k<plan.n;++k) {
            Byte op=plan.p[k];
            if (op<call0) {if (!index) return op;--index;continue;}
            Word which=slot(op),child=which ? n.right : n.left;
            if (op>=quote0) {auto span=bound(x,n,d.shape[n.rule],which);if (index<span.n) return span.p[Word(index)];index-=span.n;}
            else if (index<t.action[child].length) {next=child;break;}
            else index-=t.action[child].length;
        }
        root=next;
    }
    return 0;
}
AX_FN Word grow(const Mind &m,const Decoder &d,Tissue &t,Slice x,Node n,Word &status) {
    if (t.n>=t.limit) {status=capacity;return 0;}
    Word i=t.n+1;
    if (t.structure) t.action[i]={};
    else if (!measure(m,d,t,x,n,t.action[i])) {status=capacity;return 0;}
    t.node[i]=n;t.n=i;return i;
}
struct Frame {Half rule,start,left,right,split;Byte children;};
AX_FN bool consume(Slice x,Word &pos,Slice expected,bool backwards) {
    if (backwards) {
        if (pos<expected.n || !at(x,pos-expected.n,expected)) return false;
        pos-=expected.n;
    } else {
        if (!at(x,pos,expected)) return false;
        pos+=expected.n;
    }
    return true;
}
AX_FN Word deterministic(const Mind &m,const Decoder &d,Tissue &t,Slice x,Report &result) {
    const auto &p=m.circuit;bool back=d.direction==2;Word pos=back ? x.n : 0,level=0,done=0;Frame stack[stack_limit];
    for (;;) {
        if (!done) {
            if ((back && !pos) || (!back && pos==x.n)) return 0;
            Word start=pos,id=d.head[x.p[back ? pos-1 : pos]];
            for (;id;id=d.shape[id-1].link) {
                auto s=d.shape[id-1];auto pat=cue(p,id-1);
                Word n=p.cell[id-1].arity ? (back ? s.back : s.front) : pat.n;
                if (consume(x,pos,part(pat,back ? pat.n-n : 0,n),back)) break;
            }
            if (!id) return 0;
            auto r=p.cell[id-1];
            if (r.arity) {
                if (level==stack_limit) {result.status=capacity;return 0;}
                stack[level++]={static_cast<Half>(id-1),static_cast<Half>(start),0,0,0,0};
                if (level>result.depth) result.depth=level;
                continue;
            }
            Node leaf{static_cast<Half>(id-1),0,0,static_cast<Half>(back ? pos : start),static_cast<Half>(back ? start : pos),0};
            done=grow(m,d,t,x,leaf,result.status);if (!done) return 0;
        }
        if (!level) return pos==(back ? 0u : x.n) ? done : 0;
        auto &f=stack[level-1];auto r=p.cell[f.rule];auto s=d.shape[f.rule];auto pat=cue(p,f.rule);
        Word child=back ? r.arity-1-f.children : f.children;
        if (child) f.right=static_cast<Half>(done);else f.left=static_cast<Half>(done);
        ++f.children;done=0;
        if (f.children<r.arity) {
            Word middle=pos;if (!consume(x,pos,part(pat,s.front+1,s.middle),back)) return 0;
            f.split=static_cast<Half>(back ? pos : middle);continue;
        }
        if (!consume(x,pos,back ? part(pat,0,s.front) : part(pat,pat.n-s.back,s.back),back)) return 0;
        Node n{f.rule,f.left,f.right,static_cast<Half>(back ? pos : f.start),static_cast<Half>(back ? f.start : pos),f.split};
        if (r.arity==1) n.split=static_cast<Half>(n.end-s.back);
        --level;done=grow(m,d,t,x,n,result.status);if (!done) return 0;
    }
}
AX_FN Half &edge(Tissue &t,Word stride,Word lo,Word hi) {return t.grid[(hi-lo-1)*stride+lo];}
AX_FN bool equivalent(const Mind &m,const Decoder &d,Tissue &t,Slice x,Word a,Word b) {
    auto one=t.action[a],two=t.action[b];if (one.length!=two.length || one.hash!=two.hash) return false;
    const auto &p=t.node[a],&q=t.node[b];
    if (p.rule==q.rule && p.left==q.left && p.right==q.right && p.split==q.split) return true;
    if (t.structure || !one.power || !two.power || one.length>512) return false;
    for (Word k=0;k<one.length;++k) if (sound_at(m,d,t,x,a,k)!=sound_at(m,d,t,x,b,k)) return false;
    return true;
}
AX_FN Word general(const Mind &m,const Decoder &d,Tissue &t,Slice x,Report &result) {
    const auto &p=m.circuit;
    for (Word n=1;n<=x.n;++n) for (Word begin=0;begin+n<=x.n;++begin) {
        Word hi=begin+n;Slice chunk=part(x,begin,n);
        Half &cell=edge(t,x.n,begin,hi);cell=0;bool literal=false;
        for (Word j=0;j<p.count;++j) if (!p.cell[j].arity && same(cue(p,j),chunk)) {
            Node node{static_cast<Half>(j),0,0,static_cast<Half>(begin),static_cast<Half>(hi),0};
            Word h=grow(m,d,t,x,node,result.status);if (!h) return 0;
            if (!literal) cell=static_cast<Half>(h);
            else {if (!equivalent(m,d,t,x,cell&~uncertain,h)) cell|=uncertain;--t.n;}
            literal=true;
        }
        if (literal) continue;
        for (Word j=0;j<p.count && !(cell&uncertain);++j) {
            auto r=p.cell[j];auto pat=cue(p,j);auto s=d.shape[j];
            if (!r.arity || pat.n>n || !at(chunk,0,part(pat,0,s.front)) || !at(chunk,n-s.back,part(pat,pat.n-s.back,s.back))) continue;
            Word lo=begin+s.front,end=hi-s.back;
            for (Word cut=r.arity==1 ? end : lo+1;cut<=end;++cut) {
                Word l=r.raw&1 ? 0 : edge(t,x.n,lo,cut);
                if (!l && !(r.raw&1)) continue;
                Word rr=0;
                if (r.arity==2) {
                    if (cut+s.middle>=end || !at(x,cut,part(pat,s.front+1,s.middle))) continue;
                    rr=r.raw&2 ? 0 : edge(t,x.n,cut+s.middle,end);
                    if (!rr && !(r.raw&2)) continue;
                }
                if ((l|rr)&uncertain) {cell|=uncertain;break;}
                Node node{static_cast<Half>(j),static_cast<Half>(l),static_cast<Half>(rr),static_cast<Half>(begin),static_cast<Half>(hi),static_cast<Half>(cut)};
                Word h=grow(m,d,t,x,node,result.status);if (!h) return 0;
                if (!cell) cell=static_cast<Half>(h);
                else {if (!equivalent(m,d,t,x,cell,h)) cell|=uncertain;--t.n;}
                if (cell&uncertain) break;
            }
        }
    }
    Word h=edge(t,x.n,0,x.n);if (h&uncertain) {result.status=ambiguous;return 0;}return h;
}
AX_FN Word read(const Mind &m,const Decoder &d,Tissue &t,Slice x,Report &out) {
    t.n=0;out={};if (!x.n) return 0;
    Word h=d.direction ? deterministic(m,d,t,x,out) : general(m,d,t,x,out);
    out.nodes=t.n;
    if (h) {out.voice=t.action[h];out.status=!t.structure && !out.voice.power ? needs_observation : recognized;if (out.voice.length) out.first=m.ear.model[sound_at(m,d,t,x,h,0)];}
    return h;
}
}
