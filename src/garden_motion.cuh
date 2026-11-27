#pragma once
#include "agency.cuh"
namespace axion::life::volition {
AX_FN void sight(Agency *a,Concepts *c,Agency *b,Concepts *d,Word site,Word state,Word eyes) {
    if (a && c) look(*a,*c,site,state,(eyes&1)!=0,(eyes&2)!=0);
    if (b && d) look(*b,*d,site,state,(eyes&2)!=0,(eyes&1)!=0);
}
AX_FN void demonstrate(Agency *a,Agency *b,Intervention e,Word eyes) {
    if (a && (eyes&1)) causal::learn(a->model,e);
    if (b && (eyes&2)) causal::learn(b->model,e);
}
AX_FN void move(Agency *a,Concepts *c,Agency *b,Concepts *d,Apparatus &w,Word site,Word action,Word eyes,GardenReply &out) {
    Word x=w.plots[site],y=causal::physical(w,action,x);
    w.plots[site]=static_cast<unsigned char>(y);
    demonstrate(a,b,{x,action,y},eyes);sight(a,c,b,d,site,y,eyes);
    if (!out.actions) out.before=x;
    if (!out.samples) out.trajectory[out.samples++]=static_cast<unsigned char>(x);
    if (out.samples<utterance_limit+1) out.trajectory[out.samples++]=static_cast<unsigned char>(y);
    out.after=y;++out.actions;out.subject=site;out.selected=action;
}
}
