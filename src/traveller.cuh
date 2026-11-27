// The room index, its physical exits, and the test answer stay in the arena.
// The animal observes sensory patterns, its chosen motor and the next pattern.
namespace {
enum class TravelArm : Word { intact,silent,fixed,one_pulse,erased,misbound,wrong_goal,old_exits,escorted,tonic,no_appetite,unpaired_meals,raw,trace_fixed,immediate,no_hold,no_novelty };
struct CrossingWorld {
    Word key,depth,rotation,movement=8;
    AX_FN Word cue(Word room) const {
        Word x=(room*((key>>13)|1)+key)&31;
        x^=x>>3; x=(x*((key>>21)|1)+(key>>7))&31;
        return x^(x>>2);
    }
    AX_FN Word corridor(Word room,bool changed) const {
        Word band=room/4, code=permutation(mix(key^band*0x65a2b3edu^0x34754aefu),0);
        Word next=(code>>(2*(room&3)))&3;
        if (changed && band+1==depth) next=(next+1+(key>>18)%3)&3;
        return 4*(band+1)+next;
    }
    AX_FN Word passage(Word room,Word pins,bool changed) const {
        if (!(pins&movement) || room>=4*depth) return room;
        if (room<4) {
            Word table=permutation(mix(key^room*0x254b890du^0xd96e5f27u),0);
            Word lane=(table>>(2*((pins+rotation)&3)))&3;
            if (changed && depth==1) lane=(lane+1+(key>>18)%3)&3;
            return 4+lane;
        }
        Word open=mix(key^room*0x804e1869u^0xc3211bcdu)&3;
        return (pins&3)==open ? corridor(room,changed) : room;
    }
    AX_FN Word terminal(Word root,Word pins,bool changed) const {
        if (!(pins&movement)) return ~0u;
        Word next=passage(root,pins,changed);
        while (next<4*depth) next=corridor(next,changed);
        return next;
    }
    AX_FN Word nutrient(Word place,bool changed) const {
        Word recipe=permutation(mix(key^0xba78e361u),0);
        return 1+(((recipe>>(2*(place&3)))+Word(changed)*(1+(key>>21)%3))&3);
    }
    AX_FN Word food_site(Word goal,bool changed) const {
        Word place=0;
        UNROLL
        for (Word i=0;i<4;++i) if (nutrient(i,changed)==goal+1) place=i;
        return 4*depth+place;
    }
};

template<int Tiles,int Level> struct Traveller {
    const Genome &g;
    Tissue soma;
    Dendrites<Tiles,Level> banks;
    MapCircuit<Tiles> map;
    Word tile,base,mask;
    Settings c;
    Word motivated=0,change_food=0;
    AX_FN Word choose(Tissue &state,Word source,Word goal,Word tie,Word *stamp,Word need=0,Word event=0) const {
        Word raw=1u<<source;
        Word ambient=goal|(need<<20)|(food_spike(tasted(event))<<24)|(food_spike(absorbed(event))<<28);
        neural<Tiles,false>(g,state,raw^ambient,tile,base,mask,c.plastic);
        auto votes=motor_counts<Tiles>(g,state,tile,base,mask);
        Word basal=banks.fire(g,source,tile,base,mask,false,nullptr,ambient,stamp,&votes,tie);
        if (c.navigation) basal=map.drive(g,raw,goal,need,basal,tile,base,mask,c.navigation,tie,&votes);
        return effector(g,c.random_actions ? tie&7 : select_motor<true>(g,votes,tie,basal));
    }
    AX_FN void taste(Word tag,Word food) {
        Word raw=1u<<tag;map.sense_event(raw,tile,base,mask);
        if (c.navigation && c.plastic) map.associate(map.address(raw),food,tile);
        neural<Tiles,false>(g,soma,raw^food_field(0,0,receipt(0,0,food)),tile,base,mask,c.plastic);
    }
    AX_FN void meals(const CrossingWorld &world,bool changed,TravelArm arm) {
        for (Word n=0;n<4;++n) {
            Word site=(n+world.key)&3,source=4*world.depth+site;
            Word food=world.nutrient(site+Word(arm==TravelArm::unpaired_meals),changed && arm!=TravelArm::old_exits);
            taste(world.cue(source),food);
        }
    }
    AX_FN void earlier_life(Word seed) {
        Word recent=0;
        for (Word t=0;t<192;++t) {
            Word draw=mix(seed^t*0x6e3658abu),period=t/64;
            Word i=draw&15,j=(draw>>7)&15,tag=panel_tag(seed,i);
            if (draw&256) tag=pair_cue(attribute(seed,i,0),attribute(mix(seed),j,1));
            Word target=(mix(seed^i*0x396fd47du^period*0x34891a37u)>>13)&3,stamp=0;
            Word motor=take_action<Tiles>(g,soma,banks,tag,recent,draw,tile,base,mask,c,false,nullptr,0x01070000u,&stamp);
            bool success=consumes(g,motor,target);
            banks.remember(stamp);
            if (c.plastic) banks.update(g,g.expression>>16,delayed(tag,motor),success,tile);
            recent=success ? 1 : 2;
        }
    }
    AX_FN void explore(const CrossingWorld &world,Word rounds,bool changed,TravelArm arm,Word &observed,Word &tried,Word &visited) {
        Word n=changed ? 4 : world.depth*4,start=changed ? (world.depth-1)*4 : 0;
        for (Word r=0;r<rounds;++r) {
            Word draw=mix(world.key^r*0xb25498a1u^Word(changed)*0x753f1391u);
            for (Word step=0;step<n;++step) {
                // Rotation and reversal shuffle the lesson without giving the policy a room number.
                Word room=start+((draw%n+((draw&256) ? n-1 : 1)*step)%n);
                Word tag=world.cue(room),stamp=0;
                Word pins=choose(soma,tag,0,mix(draw^step*0x4fd53339u),&stamp);
                if (changed && arm==TravelArm::escorted) pins=world.movement|((room+r)&3);
                Word dest=world.passage(room,pins,changed && arm!=TravelArm::old_exits);
                if (changed && dest!=room) visited|=1u<<(room&3);
                banks.remember(stamp);map.sense_event(1u<<tag,tile,base,mask);
                Word outcome=world.cue(dest);
                if (arm==TravelArm::misbound) outcome=world.cue((dest+1)%(4*(world.depth+1)));
                if (c.navigation && c.plastic) map.encounter(map.note(1u<<tag,pins),1u<<outcome,0,tile);
                map.sense_event(1u<<outcome,tile,base,mask);
                neural<Tiles,false>(g,soma,1u<<outcome,tile,base,mask,c.plastic);
                ++tried; observed+=dest!=room;
            }
        }
    }
    AX_FN Word probe(const CrossingWorld &world,bool changed,TravelArm arm,Word *informed=nullptr,Word visited=0) const {
        Word total=0;
        for (Word root=0;root<4;++root) for (Word goal=0;goal<4;++goal) {
            Word desired=motivated ? world.food_site(goal,changed && change_food) : 4*world.depth+goal;
            Word shown=(goal+Word(arm==TravelArm::wrong_goal))&3,presented=4*world.depth+shown;
            Tissue copy=soma;
            Word p=choose(copy,world.cue(root),motivated ? 0 : 1u<<world.cue(presented),mix(world.key^root*0xa14237u),nullptr,motivated ? 1u<<shown : 0);
            bool hit=world.terminal(root,p,changed && change_food!=1)==desired;total+=hit;
            if (informed) {
                UNROLL
                for (Word j=0;j<4;++j) if (visited&(1u<<j)) {
                    Word terminal=world.depth==1 ? 4+j : world.corridor(4*(world.depth-1)+j,changed && change_food!=1);
                    *informed+=Word(hit && terminal==desired);
                }
            }
        }
        return total;
    }
    AX_FN Word walk(const CrossingWorld &world,bool changed,TravelArm arm,Word &moves) const {
        Word reached=0;
        for (Word route=0;route<4;++route) {
            Word place=route,kind=(route+(world.key>>17))&3;
            Word goal=motivated ? world.food_site(kind,changed && change_food) : 4*world.depth+kind;
            Word shown=(kind+Word(arm==TravelArm::wrong_goal))&3,presented=4*world.depth+shown;
            Tissue copy=soma;
            for (Word t=0;t<2*world.depth && place<4*world.depth;++t) {
                Word pins=choose(copy,world.cue(place),motivated ? 0 : 1u<<world.cue(presented),mix(world.key^place^t*0x57137693u),nullptr,motivated ? 1u<<shown : 0);
                place=world.passage(place,pins,changed && change_food!=1); ++moves;
            }
            reached+=place==goal;
        }
        return reached;
    }
};
} // namespace
