#pragma once
#include "composition.cuh"

namespace axion::life::shelter {
using namespace composition;
enum Speech : Word { put,choose,veil,give,where,holds,search,believe,chosen,offered,follow,count };
enum Lexeme : Word { put_word,inside,chooses,is,offers,to,where_word,what,holds_word,should,search_word,will,look,for_word,did,choose_word,offer,and_word,its,container,rests,at,try_word,expects,because,rides,awake,asleep };
constexpr Byte X=64,Y=65,Z=66;
struct Cadence {Byte length=0,pin[width]{};};
struct Dialect {Count words[32]{},names[96]{};Cadence input[count]{},output[count]{};};
AX_FN Count scattered(Word key) {return Count(mix(key))|(Count(mix(key+0xbb67ae85u))<<32);}
AX_FN void vary(Dialect &d,Word seed,Word style) {
    if (!style) return;
    for (Word k=0;k<32;++k) d.words[k]=scattered(seed^(k+1)*0x45d9f3bu);
    for (Word k=0;k<96;++k) d.names[k]=scattered(seed^0x68e31da4u^(k+1)*0x85ebca6bu);
    if (style>=2) for (Word i=0;i<count;++i) for (Word y=0;y<2;++y) {
        auto &f=y ? d.output[i] : d.input[i];
        for (Word j=f.length;j>1;--j) {
            Word a=mix(seed^(i+41*y)*0x91ba65bfu^j*853u)%j;
            auto t=f.pin[j-1];f.pin[j-1]=f.pin[a];f.pin[a]=t;
        }
    }
}
struct Frame {Word kind=0,x=0,y=0,z=0;};
struct Physical {
    Byte parent[96]{},taste[96]{},gifts[12][12]{},shut[12]{},view[12][96]{};
    Word focus=0;
};
AX_FN void empty(Physical &w) {
    for (auto &p:w.parent) p=absent;
    for (auto &p:w.taste) p=absent;
    for (auto &r:w.gifts) for (auto &p:r) p=absent;
    for (auto &v:w.shut) v=0;
    w.focus=absent;
}
AX_FN Word root(const Byte *edges,Word x) {
    if (x==absent || edges[x]==absent) return absent;
    for (Word j=0;j<96;++j) {Word y=edges[x];if (y==absent) return x;x=y;}
    return absent;
}
AX_FN Mouth pronounce(const Dialect &d,const Cadence &f,Count x,Count y,Count z) {
    Mouth m;m.size=f.length;
    for (Word j=0;j<m.size;++j) {Word k=f.pin[j];m.words[j]=k==X ? x : k==Y ? y : k==Z ? z : d.words[k];}
    return m;
}
// Private physical state belongs to the caregiver. The pupil gets the two
// pulse sequences through a port and has no address into this structure.
AX_FN void encounter(Physical &w,const Dialect &d,Frame f,Mouth &input,Mouth &reply) {
    Word x=f.x,y=f.y,z=f.z,k=f.kind,answer=absent,subject=x;reply={};
    input=pronounce(d,d.input[k],d.names[x],k==veil ? d.words[y ? asleep : awake] : d.names[y],d.names[z]);
    if (k==put) w.parent[x]=static_cast<Byte>(y);
    else if (k==choose) w.taste[x]=static_cast<Byte>(y);
    else if (k==give) w.gifts[x%12][y%12]=static_cast<Byte>(z);
    else if (k==veil) {
        if (y && !w.shut[x%12]) for (Word j=0;j<96;++j) w.view[x%12][j]=w.parent[j];
        w.shut[x%12]=static_cast<Byte>(y);
    } else {
        if (k==where) {answer=root(w.parent,x);w.focus=x;}
        if (k==holds) answer=w.parent[x];
        if (k==search) answer=root(w.parent,w.taste[x]);
        if (k==believe) answer=root(w.shut[x%12] ? w.view[x%12] : w.parent,y);
        if (k==chosen) answer=w.taste[x];
        if (k==offered) answer=w.gifts[x%12][y%12];
        if (k==follow && w.focus!=absent) {subject=w.focus;answer=w.parent[w.focus];}
        if (answer!=absent) reply=pronounce(d,d.output[k],d.names[subject],d.names[answer],d.names[z]);
    }
}
AX_FN void retain(Mind &m,const Mouth &x,const Mouth &y) {
    auto stimulus=perceive(m.lexicon,x,m.genes,true),response=perceive(m.lexicon,y,m.genes,true);
    if (stimulus.size!=x.size || response.size!=y.size || !x.size) return;
    if (m.lessons.n==episodes) {
        for (Word j=1;j<episodes;++j) m.lessons.event[j-1]=m.lessons.event[j];
        --m.lessons.n;
    }
    if (m.lessons.seen==~0u) {Word gap=m.lessons.seen-m.consolidated;m.lessons.seen=m.lessons.n;m.consolidated=gap<m.lessons.n ? m.lessons.n-gap : 0;}
    ++m.lessons.seen;m.lessons.event[m.lessons.n++]={stimulus,response};
}
template<class Port> AX_FN void play(Port &port,Word seed,Word rounds,Word deep,bool novel) {
    Word base=novel ? 48 : 0;
    for (Word t=0;t<rounds;++t) {
        Word key=mix(seed+t*0x9e3779b9u);
        Word who=base+(t%4),other=base+4+(key%4),third=base+4+((key+1)%4),stranger=base+((t+1)%4);
        Word item=base+8+t%6,second=base+8+(t+1)%6,room=base+40+(key%4),away=base+40+((key+1)%4);
        Word depth=1+(t%deep),carrier=item;
        port({veil,who,0});
        // Each chain may use the same containers in a new order. Overwriting
        // its terminal edge keeps the current arrangement acyclic.
        for (Word j=0;j<depth;++j) {
            Word next=j+1==depth ? room : base+16+(j+(key%8))%24;
            port({put,carrier,next});if (next!=room) carrier=next;
        }
        Word jewel=base+8+(t+2)%6,choice=key&1 ? item : second;
        port({put,second,base+40+((key+2)%4)});port({choose,who,choice});
        Frame swaps[]{{give,who,other,item},{give,who,third,second},{give,stranger,other,jewel}};
        Word rotation=(key>>7)%3;for (Word j=0;j<3;++j) port(swaps[(j+rotation)%3]);
        Frame visit=swaps[(t+(key>>14))%3];visit.kind=offered;
        Frame requests[]{{where,item},{holds,item},{search,who},{chosen,who},visit,{believe,who,item}};
        for (Word k=6;k>1;--k) {Word j=mix(key+k*797u)%k;auto tmp=requests[k-1];requests[k-1]=requests[j];requests[j]=tmp;}
        for (auto request:requests) {port(request);if (request.kind==where) port({follow});}
        port({veil,who,1});port({put,carrier,away});port({where,second});port({believe,who,item});
        port({veil,who,0});port({believe,who,item});
    }
}
// The evaluation habitat uses a separately scheduled population and keeps
// many witnesses, gifts and chains alive simultaneously.
template<class Port> AX_FN void roam(Port &port,Word seed,Word questions,Word depth,Word base=48) {
    Word people=base,things=base+8,boxes=base+16,places=base+40;
    for (Word j=0;j<8;++j) {
        port({veil,people+j,0});port({choose,people+j,things+(j+3)%8});
        for (Word k=0;k<8;++k) port({give,people+j,people+k,things+mix(seed+j*83u+k)%8});
    }
    for (Word k=0;k<24;++k) {
        Word y=k+1<depth ? boxes+k+1 : places+k%4;port({put,boxes+k,y});
    }
    for (Word j=0;j<8;++j) port({put,things+j,boxes+j%depth});
    for (Word t=0;t<questions;++t) {
        Word key=mix(seed^t*0x9e3779b9u),person=people+(key&7),object=things+((key>>4)&7);
        Word box=boxes+((key>>8)%depth),end=places+((key>>13)&3),other=people+((key>>16)&7);
        switch ((key>>20)%6) {
        case 0:port({veil,person,(key>>30)&1});break;
        case 1:port({put,object,box});break;
        case 2:port({put,boxes+depth-1,end});break;
        case 3:port({choose,person,object});break;
        case 4:port({give,person,other,object});break;
        default:port({put,object,end});break;
        }
        Word r=mix(key+47),a=people+(r&7),b=things+((r>>4)&7),c=people+((r>>8)&7);
        switch ((r>>16)%7) {
        case 0:port({holds,b});break;
        case 1:port({where,b});port({follow});break;
        case 2:port({chosen,a});break;
        case 3:port({search,a});break;
        case 4:port({offered,a,c});break;
        default:port({believe,a,b});break;
        }
    }
}

}
