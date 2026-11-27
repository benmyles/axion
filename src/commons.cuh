#pragma once
#include "axion/commons.hpp"
#include "binding_apparatus.cuh"
#include "conjunction.cuh"
#include "culture.cuh"
#include "estuary.cuh"

namespace axion::life::commons {
AX_FN Word stream(Word seed,Word i,Word day) {return mix(seed^i*0x94b32a71u^day*0x8207ab39u);}
AX_FN binding_world::Law unpack(Word packed,Word phase) {
    return {(packed>>16)^(phase ? 3u : 0u),{packed&255,(packed>>8)&255},0};
}
AX_FN Word arrival(Word packed,Word phase,Word geometry,Word scene) {
    if (geometry!=3) return binding_world::effect(unpack(packed,phase),scene);
    Word x=(packed>>(2*(scene&3)))&3,y=((packed>>(8+2*(scene>>2)))+phase)&3;
    Word first=(packed>>(16+2*((x+y)&3)))&3,second=(packed>>(24+2*(x^y)))&3;
    return first|(second<<2);
}
AX_FN Word local_prediction(const BindingField &f,Word strength,BindingGenes g,RecollectionGenes visual,Word before,Word o,Word available) {
    Word gates=binding::select(strength>>(4*o),g.competition&3,(g.competition>>2)&3),result=0;
    for (Word k=0;k<2;++k) if (gates&(1u<<k)) {
        if (!(available&(1u<<k))) return 0;
        Word current=binding::prediction(f.contacts[2*o+k],visual,binding::coordinate(before,k));
        if (!current) return 0;
        result|=current;
    }
    if (result&(result-1)) return (g.competition&16) ? result&-result : 0;
    return result;
}
AX_FN Word invent(const BindingField &f,BindingGenes g,RecollectionGenes visual,Word random,bool blind) {
    Word chosen=random&15,best=0;if (blind) return chosen;
    Word routes[4];for (Word j=0;j<4;++j) routes[j]=imagery::readout(visual,f.contacts[j]);
    for (Word k=0;k<16;++k) {
        Word state=(k*((random>>8)|1)+random)&15;
        Word pattern=0;
        for (Word o=0;o<2;++o) {
            Word a=(routes[2*o]>>(4*(state&3)))&15,b=(routes[2*o+1]>>(4*(state>>2)))&15;
            if (!a || !b) pattern|=4;else if (a!=b) pattern|=1u<<o;
        }
        Word drive=(g.exploration>>(4*pattern))&15;
        if (!k || drive>best) {best=drive;chosen=state;}
    }
    return chosen;
}
// A trapped meal reveals one actual arrival. An empty trap supplies only a
// negative observation at the attempted location, including during competition.
AX_FN bool receipt(Bindings &b,RecollectionGenes visual,Word form,Word before,Word need,Word aim,bool fed,bool credit,Word available,Conjunctions *joint) {
    bool allowed=(b.genes.competition>>(fed ? 16 : 17))&1;
    if (!allowed) return false;
    Word k=binding::field(b,form,true),shift=8*k,scores=(b.support>>shift)&255;
    if (b.forms[k]!=form) {b.fields[k]={};scores=0;b.forms[k]=form;if (joint) joint->fields[k]={};}
    auto &f=b.fields[k];Word arrival=1u<<aim;
    if (joint && available==3 && fed && (b.genes.competition&(1u<<18)))
        conjunction::observe_output(joint->fields[k],visual,before,need,arrival);
    for (Word role=0;role<2;++role) {
        if (!(available&(1u<<role))) continue;
        Word j=2*need+role,from=binding::coordinate(before,role);
        Recollection tissue{visual,f.contacts[j]};Word predicted=engram::reinstate(tissue,from);
        if (credit && (fed || predicted==arrival)) binding::reinforce(scores,j,b.genes.plastic,fed ? binding::evidence(predicted,arrival) : 2);
        if (fed && (b.genes.competition&(1u<<18))) {engram::see(tissue,from,arrival,true);f.contacts[j]=tissue.weights;}
    }
    b.support=(b.support&~(255u<<shift))|(scores<<shift);return true;
}
}
ENTRY void commons_begin(const CommonsSettings *settings,Forager *animals,Reservoir *pools,Bindings *links,const Recollection *memory,AcousticState *social,AcousticIsland *acoustics,Conjunctions *joint) {
    Word i=index();auto c=*settings;if (i>=c.count) return;
    Forager a{};a.energy=c.reserve;animals[i]=a;
    Word island=i/32,packed=0;
    if (social) social[i]={};
    if (!(i&31)) {
        Reservoir r{};for (Word &v:r.stock) v=2*c.rations;
        auto law=binding_world::physical(mix(c.seed^island*0x884b5ae9u),c.geometry);
        packed=law.map[0]|(law.map[1]<<8)|(law.sources<<16);
        if (c.geometry==3) {
            Word key=mix(c.seed^island*0x7620a17bu);packed=0;
            for (Word j=0;j<4;++j) packed|=imagery::permutation(mix(key+j))<<(8*j);
        }
        r.law=packed;pools[island]=r;
        if (acoustics) acoustics[island]={};
        if (acoustics && c.focused) for (Word o=0;o<2;++o) {
            Word used=0,state=mix(c.seed^c.optics^island*0xa32de04du^o*0x89925fc7u);
            for (Word b=0;b<4;++b) {
                Word pin=Word(__builtin_ctz(sensory_address(mix(c.optics),b)));
                if (c.shuffled) {
                    // Draw without replacement; each optical receptor has
                    // its own afferent even when the organ is realigned.
                    state=mix(state+0x9e3779b9u);Word available=~used,rank=state%(32-b);
                    for (Word k=0;k<rank;++k) available&=available-1;
                    pin=Word(__builtin_ctz(available));
                    used|=1u<<pin;
                }
                acoustics[island].retinas[o]|=pin<<(5*b);
            }
        }
    }
    asm("shfl.sync.idx.b32 %0, %1, 0, 31, 0xffffffff;":"=r"(packed):"r"(packed));
    for (Word n=0;n<c.exposures && c.channel<4;++n) {
        Word start=5*((n+c.seed+i)&3);
        if (c.geometry==3) {
            Word order=mix(c.seed^i*0xe29d74b5u);
            start=(order+n*((order>>8)|1))&15;
        }
        conjunction::experience(links[i],joint ? joint+i : nullptr,memory[i].genes,c.form,start,commons::arrival(packed,0,c.geometry,start),c.credit!=0);
    }
}
ENTRY void commons_sense(const CommonsSettings *settings,const Forager *animals,const AcousticIsland *pools,Encounter *events,const Reservoir *water) {
    Word i=index();auto c=*settings;if (i>=c.count) return;
    const auto &a=animals[i];Word draw=commons::stream(c.seed,c.channel ? i/2 : i,a.day),pair=draw&15,need=(draw>>4)&1;
    Word sight=(1u<<(pair&3))|(16u<<(pair>>2));
    if (c.channel==5) {
        sight=1u<<(estuary::draw(c.seed,i,a.day)&31);
    } else if (c.channel==4) {
        auto p=culture::day(c.seed,i,a.day,water[i/32].phase);sight=report::sight(p.positions[2],0);
        need=culture::target_age(c,i,a.day);
    } else if (c.channel) {sight&=(i&1) ? 240u : 15u;need^=i&1;}
    Word external=sight|(1u<<(8+need))|(1u<<(10+(a.energy<8)))|(1u<<(12+a.receipt));
    if (pools && c.focused) external=1u<<((pools[i/32].retinas[i&1]>>(5*binding::coordinate(pair,i&1)))&31);
    Word feedback=a.receipt==1 ? 1u : a.receipt==2 ? 2u : 0u;
    events[i]={external,0x01070000u,0,0,feedback,a.energy ? EventKind::step : EventKind::probe};
}
ENTRY void commons_hear(const CommonsSettings *settings,Forager *animals,const Recollection *memory,const Imagery *ears,
                       const Response *speakers,Encounter *events,AcousticIsland *pools,AcousticState *social) {
    Word i=index();auto c=*settings;if (i>=c.count) return;const auto &a=animals[i];auto &call=social[i];
    Word peer=i^1;
    if (c.channel==3) peer=(peer+(c.count>32 ? 32u : 2u))%c.count;
    Word own=speakers[i].ports,other=speakers[peer].ports;
    call.calls+=a.energy && c.soma && (own&8);
    call.heard=(c.soma && c.channel!=2 && animals[peer].energy && (other&8)) ? other&3 : 4;
    call.decoded=0;
    if (a.energy && call.heard<4) {
        ++call.heard_calls;Word k=imagery::field(ears[i],c.auditory_form,false);
        if (k<6) {
            Word signal=binding::prediction(ears[i].weights[k],memory[i].genes,call.heard);
            if (signal && !(signal&(signal-1))) call.decoded=signal;
        }
    }
    Word draw=commons::stream(c.seed,i/2,a.day),pair=draw&15,private_side=(i&1)^1;
    if (c.audit) {
        Word neighbor=i^1,stranger=(neighbor+(c.count>32 ? 32u : 2u))%c.count;
        Word local=animals[neighbor].energy && c.soma && (speakers[neighbor].ports&8) ? speakers[neighbor].ports&3 : 4;
        Word distant=animals[stranger].energy && c.soma && (speakers[stranger].ports&8) ? speakers[stranger].ports&3 : 4;
        Word late=call.prior_call ? call.prior_call-1 : 4;call.prior_call=local+1;
        Word site=imagery::field(ears[i],c.auditory_form,false),test[4]{};
        if (a.energy && site<6 && local<4 && distant<4 && late<4) {
            Word w=ears[i].weights[site],target=1u<<binding::coordinate(pair,private_side);
            Word sound[]{local,late,distant},fields[3]{};bool matched=true;
            for (Word k=0;k<3;++k) {fields[k]=binding::prediction(w,memory[i].genes,sound[k]);matched&=fields[k] && !(fields[k]&(fields[k]-1));}
            if (matched) {test[0]=1;for (Word k=0;k<3;++k) test[k+1]=fields[k]==target;}
        }
        for (Word k=0;k<4;++k) {
            Word votes;asm("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, 0xffffffff; }":"=r"(votes):"r"(test[k]));
            if (!(i&31)) pools[i/32].audit[k]+=Word(__builtin_popcount(votes));
        }
    }
    if (a.energy) {
        Word seen=binding::coordinate(pair,i&1),sound=c.soma && (own&8) ? own&3 : 4;
        asm volatile("red.global.add.u32 [%0], 1;"::"l"(&pools[i/32].speech[i&1][seen][sound]):"memory");
    }
    if (call.decoded) {++call.readings;call.accurate+=call.decoded==(1u<<binding::coordinate(pair,private_side));}
    Word visible=(1u<<binding::coordinate(pair,i&1))<<(4*(i&1)),need=((draw>>4)&1)^(i&1);
    Word external=visible|(1u<<(8+need))|(1u<<(10+(a.energy<8)))|(1u<<(12+a.receipt))|(call.decoded<<(17+4*private_side))|(1u<<(25+call.heard));
    events[i]={external,0x01070000u,0,0,0,a.energy ? EventKind::step : EventKind::probe};
}
ENTRY void commons_settle(const CommonsSettings *settings,Forager *animals,Reservoir *pools,Bindings *links,const Recollection *memory,Imagery *ears,const Response *muscles,AcousticState *social,Conjunctions *joint,const RelationalView *charts,const Word *decisions) {
    Word i=index();auto c=*settings;if (i>=c.count) return;
    Word lane=i&31,island=i/32;auto &water=pools[island];auto &a=animals[i];
    Word day=a.day++,key=commons::stream(c.seed,c.channel ? i/2 : i,day),before=key&15,need=((key>>4)&1)^(c.channel ? i&1 : 0);
    Word perceived=before,available=3;
    if (c.channel && c.channel<4) {
        Word role=i&1,hidden=role^1;available=1u<<role;
        perceived=before&(3u<<(2*role));
        if (social[i].decoded) {perceived|=Word(__builtin_ctz(social[i].decoded))<<(2*hidden);available|=1u<<hidden;}
    }
    auto law=commons::unpack(water.law,water.phase);
    Word supply[4];for (Word bearing=0;bearing<4;++bearing) supply[bearing]=lesser(3*c.rations,water.stock[bearing]+c.rations);
    bool hungry=a.energy!=0,experiment=false,eligible=false;Word aim=4,price=1;
    if (hungry && c.channel==5) {
        ++a.lived;Word d=decisions[i],motor=d&31;experiment=(d&estuary::studying)!=0;
        a.meal_updates+=(d&estuary::learned)!=0;
        if (experiment) {
            ++a.experiments;a.after_miss+=a.receipt==2;a.diagnostic+=(d&estuary::diagnostic)!=0;
            price+=c.price;a.receipt=4;
        } else {
            Word subject=estuary::draw(c.seed,i,day)&31;
            Word food=estuary::drift(charts[island],estuary::passage(c.current_seed,island,water.phase,c.route_depth-1),subject);
            aim=motor>>3;eligible=motor==food;
            if (!eligible) {++a.missed;a.receipt=2;}
        }
    } else if (hungry && c.channel==4) {
        ++a.lived;auto &call=social[i];price+=c.report_cost*call.heard;
        experiment=call.prior_call!=0;
        if (experiment) {
            ++a.experiments;a.after_miss+=a.receipt==2;price+=c.price;a.receipt=4;
        } else {
            Word action=call.decoded ? Word(__builtin_ctz(call.decoded)) : (c.soma ? muscles[i].ports&31 : 0u);
            auto passage=culture::day(c.seed,i^1,day,water.phase);Word age=culture::target_age(c,i,day);
            eligible=action==(passage.events[1-age]&31);aim=action>>3;
            ++call.private_traps;call.private_hits+=eligible;
            if (!eligible) {++a.missed;a.receipt=2;}
        }
    } else if (hungry) {
        ++a.lived;
        auto &b=links[i];auto visual=memory[i].genes;Word k=binding::field(b,c.form,false);
        if (k>=4) k=binding::field(b,c.form,true);
        auto &f=b.fields[k];Word score=(b.support>>(8*k))&255;
        Word predicted=commons::local_prediction(f,score,b.genes,visual,perceived,need,available);
        if (joint && c.projection!=2 && b.forms[k]==c.form) {
            conjunction::Candidates model{};
            auto gates=joint[i].gates;if (c.projection==1) gates.expression&=~7u;
            if (conjunction::readout(joint[i].fields[k],need,visual,gates,model)) {
                Word options=conjunction::pairs((available&1) ? 1u<<(perceived&3) : 15,(available&2) ? 1u<<(perceived>>2) : 15);
                predicted=0;
                for (Word v=0;v<4;++v) if (model.planes[v]&options) predicted|=1u<<v;
                if (predicted&(predicted-1)) predicted=0;
            }
        }
        Word motor=c.soma ? muscles[i].ports : 0;
        Word pulse=(motor>>((b.genes.competition>>24)&7))&1;
        Word address=Word(!predicted)+2*Word(a.receipt==2)+4*pulse;
        experiment=c.policy==1 ? false : c.policy==2 ? true : ((b.genes.competition>>(8+address))&1)!=0;
        if (experiment) {
            ++a.experiments;a.after_miss+=a.receipt==2;price+=c.price;
            if (c.channel && c.ear_learning && social[i].heard<4)
                imagery::experience(ears[i],visual,c.auditory_form,social[i].heard,binding::coordinate(before,(i&1)^1));
            Word motor_noise=c.channel ? commons::stream(c.seed^0x4e20b3d1u,i,day) : key;
            Word trial=commons::invent(f,b.genes,visual,mix(motor_noise^0x8ac92b63u),c.choice!=0);
            Word left=trial&3,right=trial>>2;
            bool separates=true;
            Word observed=commons::arrival(water.law,water.phase,c.geometry,trial);
            if (c.geometry==3) {
                Word a=commons::arrival(water.law,water.phase,c.geometry,left*5),b=commons::arrival(water.law,water.phase,c.geometry,right*5);
                for (Word o=0;o<2;++o) {
                    Word value=binding::coordinate(observed,o);
                    separates&=value!=binding::coordinate(a,o) && value!=binding::coordinate(b,o);
                }
            } else for (Word o=0;o<2;++o) separates&=((law.map[o]>>(2*left))&3)!=((law.map[o]>>(2*right))&3);
            a.diagnostic+=separates;
            conjunction::experience(b,joint ? joint+i : nullptr,visual,c.form,trial,observed,c.credit!=0);
            a.receipt=4;
        } else {
            aim=predicted ? Word(__builtin_ctz(predicted)) : motor&3;
            eligible=aim==binding::coordinate(commons::arrival(water.law,water.phase,c.geometry,before),need);
            if (c.channel && (c.geometry==3 || ((law.sources>>need)&1)!=(i&1))) {++social[i].private_traps;social[i].private_hits+=eligible;}
            if (!eligible) {++a.missed;a.receipt=2;}
        }
    }
    if (c.joint) {
        Word partner,placed=Word(eligible);asm("shfl.sync.bfly.b32 %0, %1, 1, 31, 0xffffffff;":"=r"(partner):"r"(placed));
        if (eligible && !partner) {eligible=false;++a.unopened;a.receipt=2;}
    }
    Word gained=0,eaten=0,recovered=0,start=mix(c.seed^island^day*0x9de583b1u)&31;
    for (Word bearing=0;bearing<4;++bearing) {
        Word vote=Word(eligible && aim==bearing),mask;
        asm(" { .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, 0xffffffff; }":"=r"(mask):"r"(vote));
        Word rotated=(mask>>start)|(mask<<((32-start)&31));
        Word preceding=rotated&((1u<<((lane+32-start)&31))-1);
        Word taken=lesser(Word(__builtin_popcount(mask)),supply[bearing]);
        if (vote) {
            if (Word(__builtin_popcount(preceding))<supply[bearing]) {++a.meals;gained=c.calories;a.receipt=1;}
            else {++a.contested;a.receipt=3;}
        }
        eaten+=taken;
        if (!lane) {recovered+=supply[bearing]-water.stock[bearing];water.stock[bearing]=supply[bearing]-taken;}
    }
    if (hungry) {
        if (!experiment && c.receipts && c.channel<4)
            a.meal_updates+=commons::receipt(links[i],memory[i].genes,c.form,perceived,need,aim,gained!=0,c.credit!=0,available,joint ? joint+i : nullptr);
        a.energy+=gained;Word used=lesser(a.energy,price);a.energy-=used;a.expense+=used;
        if (a.energy>=c.reserve+12) {Word clutch=12*((a.energy-c.reserve)/12);a.energy-=clutch;a.offspring+=clutch;}
    }
    if (!lane) {
        water.regrown+=recovered;water.consumed+=eaten;
        if (c.volatility && mix(c.seed^island*0xce761f93u^day*0x38d2c571u)%c.volatility==0) {water.phase^=1;++water.switches;}
    }
}
ENTRY void commons_grade(const Forager *animals,Word *fitness,Word n,Word copies,Word kin) {
    Word i=index();if (i>=n) return;
    Word mass=0;for (Word q=0;q<copies;++q) mass+=animals[kin ? i*copies+q : q*n+i].offspring;
    fitness[i]=mass;
}
ENTRY void commons_census(const Forager *animals,const AcousticState *social,Word n,CommonsTotals *totals) {
    Word i=index(),lane=i&127;Count values[19]{};
    for (Word j=i;j<n;j+=128) {
        const auto &a=animals[j];
        values[0]+=a.energy!=0;values[1]+=a.offspring;values[2]+=a.meals;values[3]+=a.experiments;
        values[4]+=a.diagnostic;values[5]+=a.after_miss;values[6]+=a.missed;values[7]+=a.contested;
        values[8]+=a.lived;values[9]+=a.energy;values[10]+=a.expense;values[11]+=a.meal_updates;
        if (social) {
            const auto &c=social[j];
            values[12]+=c.calls;values[13]+=c.heard_calls;values[14]+=c.readings;values[15]+=c.accurate;values[16]+=c.private_traps;values[17]+=c.private_hits;
        }
        values[18]+=a.unopened;
    }
    __attribute__((shared)) static Count sums[128];
    for (Word k=0;k<19;++k) {
        sums[lane]=values[k];asm volatile("bar.sync 0;":::"memory");
        for (Word step=64;step;step>>=1) {if (lane<step) sums[lane]+=sums[lane+step];asm volatile("bar.sync 0;":::"memory");}
        if (!lane) totals->count[k]=sums[0];asm volatile("bar.sync 0;":::"memory");
    }
}
ENTRY void commons_prefix(const Word *fitness,Word n,Count *distribution) {
    Word lane=index();__attribute__((shared)) static Count s[128];Count carry=0;
    for (Word block=0;block<n;block+=128) {
        Word i=block+lane;s[lane]=i<n ? fitness[i] : 0;
        for (Word step=1;step<128;step<<=1) {
            asm volatile("bar.sync 0;":::"memory");Count other=lane>=step ? s[lane-step] : 0;
            asm volatile("bar.sync 0;":::"memory");s[lane]+=other;
        }
        asm volatile("bar.sync 0;":::"memory");if (i<n) distribution[i]=carry+s[lane];carry+=s[127];
        asm volatile("bar.sync 0;":::"memory");
    }
}
ENTRY void commons_recruit(const VoiceGermline *parents,VoiceGermline *young,const Count *distribution,Word n,Word seed,Word drift,VoiceBody form) {
    Word i=index();if (i>=n) return;Word key=mix(seed^i*0x52a3bc6fu),winner=key%n;
    if (!drift) {
        Count target=((Count(mix(key+13))<<32)|mix(key+91))%distribution[n-1];Word low=0,high=n;
        while (low<high) {Word mid=low+(high-low)/2;if (distribution[mid]>target) high=mid;else low=mid+1;}
        winner=low;
    }
    auto p=parents[winner];mutate_voice_bundle(p,form,mix(key+7));young[i]=p;
}
ENTRY void commons_found(VoiceGermline *out,VoiceGermline donor,Word n,Word seed,Word variation,Word narrative) {
    Word i=index();if (i>=n) return;
    if (i && variation) donor.binding.competition=(donor.binding.competition&~0xff00u)|((mix(seed^i)&255)<<8);
    if (i && variation && narrative) {
        Word key=mix(seed^i*0x84db86adu);
        donor.report.high=(donor.report.high&0xfffu)|(key&0xfffff000u);
    }
    out[i]=donor;
}
ENTRY void commons_sample(const Forager *animals,const unsigned char *brains,Word stride,Word n,Word families,Word kin,Word *out) {
    Word j=index();if (j>=lesser(n,64u)) return;Word i=j<32 ? j : n-64+j;
    const auto &a=animals[i];const auto *b=reinterpret_cast<const Individual *>(brains+Count(i)*stride);
    Word row[8]{i,kin ? i/(n/families) : i%families,a.energy,a.meals,a.experiments,a.offspring,b->genes.body,1+(b->genes.expression&7)};
    for (Word k=0;k<8;++k) out[8*j+k]=row[k];
}

ENTRY void commons_card_census(const Forager *animals,const unsigned char *brains,Word stride,Word n,Word families,Word kin,axion::journal::CensusMember *out) {
    Word i=index();if (i>=n) return;
    const auto &a=animals[i];const auto &b=*reinterpret_cast<const Individual *>(brains+Count(i)*stride);
    out[i]={b.genes.body,i,a.energy,kin ? i/(n/families) : i%families,32*living_tiles(b.tiles,b.genes.development),1+(b.genes.expression&7),b.genes.effectors[0],b.genes.effectors[1]};
}
