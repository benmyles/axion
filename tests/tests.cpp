#include "axion/evolution.hpp"
#include "axion/tutor.hpp"
#include "axion/actor.hpp"
#include "axion/planning.hpp"
#include "../src/imagery.cuh"
#include "../src/binding.cuh"
#include "../src/conjunction.cuh"
#include "../src/report.cuh"
#include "../src/concept.cuh"
#include "../src/causal.cuh"
#include <bit>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unistd.h>

namespace {
using namespace axion;

void check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
void relational_fibres() {
    using namespace life;
    // Simultaneous and serial retinal writes must agree for every inherited
    // opcode, including completely hidden, empty and many-to-one displays.
    for (Word gate=0;gate<16;++gate) for (Word seen=0;seen<4;++seen) {
        Concepts slow{},fast{};meaning::arrange(slow,32,true);slow.genes.low=gate*0x11111111u;
        Fibres field{};
        for (Word j=0;j<32;++j) {
            meaning::store(slow.views[0].relation[1],j,(j+3)&31);meaning::store(slow.views[1].relation[1],j,31-j);
            meaning::store(field,j,j%7 ? j%11 : empty_link);
        }
        fast=slow;meaning::observe_field(fast,1,field,seen&1,seen&2);
        for (Word j=0;j<32;++j) meaning::observe(slow,1,j,(seen&1) ? (j%7 ? j%11 : empty_link) : hidden_link,seen&2);
        check(!std::memcmp(slow.views,fast.views,sizeof(slow.views)),"a tiled retinal write changed an inherited gate");
    }
    RelationalView graph{};
    for (Word j=0;j<32;++j) meaning::store(graph.relation[0],j,j%5 ? j%3 : empty_link);
    auto inverse=meaning::step(graph,~0u,4,{1,0});Word expected=0;
    for (Word j=0;j<32;++j) if (j%5 && j%3==0) expected|=1u<<j;
    check(!inverse.gap && inverse.cells==expected,"lost an alternative predecessor");
    meaning::store(graph.relation[0],31,hidden_link);
    check(meaning::step(graph,~0u,4,{1,0}).cells==(expected|1u<<31),"an unseen link was treated as absent");
    check(meaning::identifier(meaning::step(graph,~0u,0,{1u<<31,0}))==hidden_link,"a hidden destination leaked to the readout");
    Count walk=1;for (Word j=0;j<21;++j) walk=meaning::join(walk,8+(j&7));
    check(walk && !meaning::join(walk,9) && !meaning::join(9,walk),"a long pathway wrapped around the stop bit");
    Concepts c{};c.genes.expression=1u<<24;meaning::arrange(c,32,true);
    for (Word j=0;j<8;++j) {meaning::recruit(c,j+1,true);c.words[j].walk=8+j;}
    c.words[7].inputs|=1u<<8;c.words[7].threshold=3;c.words[7].low[7]=c.words[7].high[7]=1u<<24;
    check(meaning::depends_on(c.words[7],8),"active first input missing from reuse graph");
    c.words[7].inputs+=20u<<16;
    check(meaning::recruit(c,901,true)==6 && c.words[0].form==1 && c.words[7].form==8,"a ninth word erased an active prerequisite");
}
void causal_contacts() {
    using namespace life;
    for (Word x=0;x<64;++x) {
        auto groups=causal::patterns(x);Word all=0;
        for (Word pattern:groups.group) {check(!(all&pattern),"causal input regions overlap");all|=pattern;}
        check(all==all_pairs,"a dendrite fell outside the retinal cover");
        for (Word k=0;k<pairs_of_traits;++k) {
            Word a=causal::from(k),b=causal::other(k),input=((x>>a)&1)+2*((x>>b)&1);
            check(groups.group[input]&(1u<<k),"retinal factor sampled another receptor");
            for (Word lut=0;lut<16;++lut)
                check(causal::evaluate(causal::canonical(a,b,lut),x)==((lut>>input)&1),"compiled causal field changed an output");
        }
    }
    CausalModel m{};m.genes={0x0000afaf,0x18d,0};causal::learn(m,{0,0,63,3,4});
    for (Word j=0;j<traits;++j) for (Word k=0;k<4;++k) {
        Word active=m.fields[0][j].low[k]|m.fields[0][j].high[k];
        check(active==Word(j==2 && k==0),"hidden causal afferents or efferents received credit");
    }
    check(!m.visited[0],"a partially seen state was counted as fully experienced");
    for (Word yes=0;yes<64;++yes) for (Word unknown=0;unknown<64;++unknown) {
        Word fixed=yes&~unknown;Count set=causal::successors({fixed,unknown});
        for (Word x=0;x<64;++x) check(bool(set&(1ull<<x))==((x&~unknown)==fixed),"forecast discarded or invented a possible state");
    }
}
void predictive_relations() {
    using namespace life;
    for (Word who=0;who<4;++who) for (Word from=0;from<16;++from) for (Word to=0;to<16;++to) {
        Word bits=report::possibilities(who,from,to),slow=0,marginals[3]{};
        for (Word e=0;e<32;++e) {
            Word r=1u<<(e&1),a=1u<<((e>>1)&3),b=1u<<(e>>3);
            if ((who&r) && (from&a) && (to&b) && a!=b) {slow|=1u<<e;marginals[0]|=r;marginals[1]|=a;marginals[2]|=b;}
        }
        check(bits==slow,"hearing implicated an impossible movement");
        for (Word j=0;j<3;++j) check(report::values(bits,j)==marginals[j],"retelling fabricated an event component");
    }
    VocalFrame circuit{};Word syllables[3]{1,2,3};
    for (Word k=0;k<1024;++k) {
        Word key=mix(k);RecollectionGenes g{mix(key),mix(key+1),mix(key+2)};
        Episodes h{0,report::event(key&1,(key>>2)&3,(key>>4)&3),report::event((key>>8)&1,(key>>10)&3,(key>>12)&3)};
        report::model(circuit,h,g,syllables,3);
        for (Word j=0;j<=3;++j) check(report::pathways(circuit.phases[j],g)==report::direct_pathways(circuit.phases[j],g),"compiled speech gating diverged after a local update");
    }
    Reports crowded{};crowded.history.recent=report::event(0,0,1);
    for (Word i=0;i<report_words;++i) {
        crowded.syllables[i]=i+1;crowded.frames[1].phases[i/4].contacts[0]|=(i+1)<<(5*(i%4));
    }
    crowded.frames[1].cue=21;
    Utterance unfamiliar{};unfamiliar.count=2;unfamiliar.sound[0]=1000;unfamiliar.sound[1]=2000;
    const auto before=crowded;
    check(!report::learn(crowded,9,unfamiliar) && !std::memcmp(&before,&crowded,sizeof(crowded)),"failed lexical admission altered existing contacts");
    namespace joint=life::conjunction;
    for (Word scene_set=0;scene_set<65536;++scene_set) {
        Word left=0,right=0,repeat_left=0,repeat_right=0;
        for (Word i=0;i<4;++i) {
            Word row=15u<<(i*4),col=0x1111u<<i;
            if (scene_set&col) left|=1u<<i;
            if (scene_set&row) right|=1u<<i;
            if (std::popcount(scene_set&row)>1) repeat_left|=row;
            if (std::popcount(scene_set&col)>1) repeat_right|=col;
        }
        check(joint::marginal(scene_set,0)==left && joint::marginal(scene_set,1)==right,"scene replay mixed up its bearings");
        check(joint::repeated_rows(scene_set)==repeat_left && joint::repeated_columns(scene_set)==repeat_right,"lateral address crossed a retinal boundary");
    }
    life::SceneWave divergence{},collapse{};
    for (Word i=0;i<16;++i) {divergence.scenes[i]=0x21;collapse.scenes[i]=1u<<((i+1)&15);}
    joint::compose(divergence,collapse);
    check(!joint::answer(divergence,0,1,1),"ambiguous intermediate scene disappeared in replay");
    for (auto &s:collapse.scenes) s=1u<<13;
    joint::compose(divergence,collapse);
    check(joint::answer(divergence,0,0,0)==2 && joint::answer(divergence,1,0,0)==8,"converging futures retained spurious uncertainty");
    for (Word edges=0;edges<65536;++edges) for (Word active=0;active<16;++active) {
        Word paths=active|(((active+3)&15)<<4)|(((active+7)&15)<<8)|(((active+11)&15)<<12);
        Word correct=0;
        for (Word row=0;row<4;++row) correct|=life::imagery::propagate(edges,(paths>>(4*row))&15)<<(4*row);
        check(life::imagery::compose(paths,edges)==correct,"packed replay mixed activity between rows");
    }
    using life::RoleProjection;
    RoleProjection partially_seen{};
    partially_seen.routes[0]&=~1u;partially_seen.missing=1;
    RoleProjection alternatives{};
    alternatives.routes[0]=0x84218421u;alternatives.apertures|=2;
    auto ambiguous=life::binding::compose(partially_seen,alternatives);
    check(!life::binding::answer(ambiguous,0,1,1,{}),"an unobserved alternative acquired a fabricated prediction");
    RoleProjection swap{};swap.routes[0]=0x84210000u;swap.routes[1]=0x8421u;swap.apertures=6;
    auto involution=life::binding::compose(swap,swap);
    for (Word k=0;k<16;++k) {
        Word a=1u<<(k&3),b=1u<<(k>>2);
        check(life::binding::answer(involution,0,a,b,{})==a && life::binding::answer(involution,1,a,b,{})==b,"two physical exchanges lost the object identity");
    }
}

bool identical(const Model &a, const Model &b) {
    auto bytes_equal = [](const auto &x, const auto &y) {
        return x.size() == y.size() && (x.empty() || !std::memcmp(x.data(), y.data(), x.size() * sizeof(x[0])));
    };
    return bytes_equal(a.cells, b.cells) && bytes_equal(a.memory, b.memory) && bytes_equal(a.maps, b.maps) && bytes_equal(a.drawings, b.drawings)
        && bytes_equal(a.archive, b.archive) && bytes_equal(a.interventions, b.interventions);
}

void same_counts(const Run &a, const Run &b) {
    check(a.animals.size() == b.animals.size(), "population count mismatch");
    for (size_t i = 0; i < a.animals.size(); ++i) {
        check(a.animals[i].ticks == b.animals[i].ticks, "neuron clocks differ");
        check(a.animals[i].spikes == b.animals[i].spikes, "spike counts differ");
        check(a.animals[i].wins == b.animals[i].wins, "behavior differs");
        check(a.animals[i].matches == b.animals[i].matches && a.animals[i].image == b.animals[i].image,
              "generated sensory outputs differ");
    }
}

void neuron() {
    for (unsigned k = 0; k < 1000; ++k) {
        Cell<Learning::full> c{};
        c.weights = initial_weights(3, 7, int(k));
        c.reset(k, k);
        Word input = mix(k), near = mix(k + 71), far = mix(k + 290);
        int drive = -(population(near) + population(far)) / 4;
        for (int i = 0; i < 32; ++i) {
            Word b = 1u << i;
            if (input & b) drive += (c.weights.sensory & b) ? 3 : -3;
            if (near & b) drive += (c.weights.local & b) ? 1 : -1;
            if (far & b) drive += (c.weights.remote & b) ? 1 : -1;
        }
        int threshold = 7 + (k & 3);
        bool fired = c.tick(int(k), input, near, far, active_cost(population(input), near, far));
        check(fired == (drive >= threshold), "packed synaptic drive differs from scalar sum");
        check(c.state.voltage == bound(drive - (fired ? threshold : 0), -16, 24), "soft reset");
        check(c.state.trace == (fired ? 24 : 0), "activity trace");
    }
    Cell<Learning::full> cell{};
    cell.weights.motor[0] = 64;
    cell.tag.sample = 37;
    cell.tag.age = 5;
    cell.state.trace = 32;
    cell.commit(0, 0);
    cell.tag.age = 0;
    cell.reinforce(256);
    check(cell.weights.motor[0] == 128 && !cell.weights.motor[1], "delayed motor credit");
    check(cell.weights.remote == 32 && !cell.weights.sensory, "sampled recurrent potentiation");
    cell.reinforce(-256);
    check(cell.weights.remote == 0, "sampled recurrent depression");
    Cell<Learning::full> silent{};
    silent.tag.sample = 1;
    silent.reinforce(0);
    check(silent.weights.sensory == 2, "silent cell failed to recruit a sensory input");
    silent.tag.motor[0] = 32;
    silent.reinforce(0);
    check(!silent.weights.sensory, "overactive cell failed to attenuate input");
    Cell<Learning::no_scaling> control{};
    control.tag.sample = 1;
    control.reinforce(0);
    check(!control.weights.sensory, "homeostatic adjustment leaked into the ablation");
    cell.reset(91, 0);
    cell.reinforce(256);
    check(cell.weights.motor[0] == 64 && !cell.weights.remote, "trial boundary cleared eligibility");
}

void worlds() {
    for (Word k = 0; k < 256; ++k) {
        auto a = Arena::enter({Task::delay, 0, 0, 0}, k);
        auto b = a;
        b.course.reversed = 1;
        auto changed = a;
        changed.odor ^= 1;
        for (int t = 0; t <= a.finish; ++t) {
            Word x = a.sense(t);
            check(x == b.sense(t), "hidden reward rule leaked into receptors");
            Word y = changed.sense(t);
            if (t >= 3) check(x == y && !(x & 0xffffu), "delay leaked sample identity");
            if (t == a.fork) { a.take(t, k & 1); b.take(t, k & 1); }
        }
        check(a.on_path != b.on_path, "reversal must exchange outcomes");
        check(a.transit >= 2, "reward was immediate");

        auto left = Arena::enter({Task::trail, 0, 0, 0}, k);
        auto right = left;
        int fork = left.fork;
        left.take(fork, 0);
        right.take(fork, 1);
        check(left.odor != right.odor && left.fork > fork, "action must change the second patch");
        int target = food_arm(left.odor);
        bool first_correct = left.on_path;
        left.take(left.fork, target);
        right.take(right.fork, food_arm(right.odor));
        check(left.on_path == first_correct && left.on_path != right.on_path, "terminal route reward");
    }
}

std::uint64_t wins(const Run &r) {
    std::uint64_t n = 0;
    for (auto x : r.animals) n += x.wins;
    return n;
}

void behavior() {
    auto m = hatch(16, 128, 12, {Task::context, 3442, 0, 0});
    auto cpu = make_cpu(m, 4);
    Course probe = m.course;
    probe.seed ^= 0x876231abu;
    auto before = cpu->run(probe, 0, 128, Learning::frozen);
    cpu->run(m.course, 0, 256, Learning::full);
    auto after = cpu->run(probe, 0, 128, Learning::frozen);
    check(wins(after) > 0.78 * 2048 && wins(after) > wins(before) + 500, "conditional learning");
    auto trained = m;
    cpu->download(trained);
    bool sensory = false, recurrent = false;
    for (size_t i = 0; i < m.cells.size(); ++i) {
        sensory |= trained.cells[i].sensory != m.cells[i].sensory;
        recurrent |= trained.cells[i].remote != m.cells[i].remote;
        check(trained.cells[i].local == m.cells[i].local, "local scaffold mutated in a lifetime");
    }
    check(sensory && recurrent, "hidden synapses did not learn");
    probe.mute_odor = 1;
    auto blind = cpu->run(probe, 0, 128, Learning::frozen);
    check(wins(blind) > 0.44 * 2048 && wins(blind) < 0.56 * 2048, "odor intervention above chance");
    auto again = m;
    cpu->download(again);
    check(identical(again, trained), "evaluation changed synapses");

    cpu->restore(m);
    Course delay{Task::delay, 5902, 0, 0};
    cpu->run(delay, 0, 128, Learning::motor);
    cpu->download(again);
    for (size_t i = 0; i < m.cells.size(); ++i) {
        check(again.cells[i].sensory == m.cells[i].sensory && again.cells[i].remote == m.cells[i].remote,
              "motor-only learning changed the core");
    }
    delay.reversed = 1;
    check(wins(cpu->run(delay, 256, 128, Learning::frozen)) < 0.1 * 2048, "unannounced reversal");
    cpu->run(delay, 384, 256, Learning::full);
    check(wins(cpu->run(delay, 640, 128, Learning::frozen)) > 0.9 * 2048, "reversal learning");
}

void persistence(const std::filesystem::path &dir) {
    auto m = hatch(5, 64, 41, {Task::trail, 39239, 0, 0});
    m.learning = Learning::no_scaling;
    auto uninterrupted = make_cpu(m, 2), split = make_cpu(m, 1);
    uninterrupted->run(m.course, 0, 73, m.learning);
    auto expected = m;
    uninterrupted->download(expected);
    split->run(m.course, 0, 19, m.learning);
    split->download(m);
    m.cursor = 19;
    auto path = dir / "resume.axn";
    save_model(m, path);
    auto restored = load_model(path);
    check(identical(m, restored) && restored.cursor == 19 && restored.learning == m.learning && restored.course.seed == m.course.seed,
          "checkpoint round-trip");
    auto resumed = make_cpu(restored, 3);
    resumed->run(restored.course, restored.cursor, 54, restored.learning);
    resumed->download(restored);
    check(identical(expected, restored), "checkpoint continuation diverged");

    auto reject = [&] {
        try { load_model(path); } catch (const std::runtime_error &) { return; }
        throw std::runtime_error("corrupt checkpoint accepted");
    };
    { std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary);
      f.seekg(73); char c = char(f.get() ^ 1); f.seekp(73); f.put(c); }
    reject();
    save_model(m, path);
    std::filesystem::resize_file(path, 3);
    reject();
}

void selection() {
    auto m = hatch(24, 64, 9, {Task::scent, 432, 0, 0});
    auto cpu = make_cpu(m, 2);
    auto backup = m;
    auto scores = lifetime_scores(*cpu, m, m.course, 8, 16, Learning::full);
    check(identical(m, backup), "lifetime changes leaked into inherited weights");
    check(scores == lifetime_scores(*cpu, m, m.course, 8, 16, Learning::full), "fitness repeatability");
    std::iota(scores.begin(), scores.end(), 0u);
    breed(m, scores);
    check(m.generation == 1 && m.cursor == 0, "generation boundary");
    for (Word i = 0; i < 3; ++i) {
        check(!std::memcmp(&m.cells[i * 64], &backup.cells[(23 - i) * 64], 64 * sizeof(Weights)),
              "elite genome was overwritten");
    }
    breed(backup, scores);
    check(identical(m, backup), "nondeterministic reproduction");
}

void lessons(const std::filesystem::path &dir) {
    for (Word key = 0; key < 64; ++key) {
        Maze world = Maze::make(key, 4);
        for (Word start = 0; start < 16; ++start) {
            Word near = world.expand(world.expand(1u << start));
            Word reach = world.expand(world.expand(world.expand(near)));
            Word goal = 1u << world.far_goal(start, mix(key + start));
            check((reach & goal) && !(near & goal), "route probe has invalid shortest distance");
        }
        Course c{Task::pattern, 222, 0, 0};
        c.lesson = key;
        auto support = example(c, 4, key);
        c.reversed = 1;
        auto reversed = example(c, 4, key);
        check(!std::memcmp(&support.visible, &reversed.visible, sizeof(Scene)), "reward leaked into scene");
        check(support.rewarded != reversed.rewarded, "reversal label");
        c.novel = 1;
        auto query = example(c, 4, key);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                check(support.visible.objects[i] != query.visible.objects[j], "probe alphabet overlaps support");
    }
    auto m = hatch(16, 128, 17, {Task::errand, 8274, 0, 0});
    m.course.lesson = 39819;
    auto engine = make_cpu(m, 4);
    auto c = m.course;
    auto probe = [&](Task task, Word scrambled = 0) {
        auto q = c;
        q.task = task;
        q.novel = 1;
        q.seed ^= 0xe62f3a51u;
        q.swap_words = scrambled;
        return wins(engine->run(q, 0, 64, Learning::frozen)) / 1024.0;
    };
    // Teach the lexicon and doors separately, across a checkpoint, then ask
    // for compound behavior the agent has never performed during training.
    c.task = Task::landmarks;
    engine->run(c, 0, 16, Learning::full);
    auto labels = m;
    engine->download(labels);
    check(labels.maps.empty() && labels.memory[0].used == 16, "labels recruited unrelated storage");
    auto path = dir / "lessons.axn";
    save_model(labels, path);
    auto reloaded = load_model(path);
    check(identical(labels, reloaded), "lexical checkpoint");
    engine->restore(reloaded);
    c.task = Task::routes;
    c.survey_steps = 64;
    engine->run(c, 0, 1, Learning::full);
    engine->download(m);
    check(probe(Task::routes) > 0.85, "failed to plan after continuous exploration");
    check(probe(Task::errand) > 0.78, "failed to compose lexicon and learned transitions");
    check(probe(Task::errand, 1) < 0.35, "command order ignored");
    auto frozen = m;
    engine->download(frozen);
    check(identical(frozen, m), "probes rewrote experience");
    save_model(m, path);
    auto copy = load_model(path);
    check(identical(copy, m) && copy.course.lesson == m.course.lesson, "combined checkpoint");
    engine->restore(labels);
    check(probe(Task::errand) < 0.35, "identity memory leaked a map after restore");
    // Externally supplied examples. There is no hidden task or vocabulary
    // oracle in the input stream. Frozen queries must leave the cursor alone.
    auto solo = hatch(1, 64, 7, c);
    auto pupil = make_cpu(solo, 1);
    std::istringstream examples("ask a z a cat dog\nreward left\nask a z z dog cat\nreward left\n"
                                "ask q w q dog cat\nreward right\nask q w w dog cat\nreward left\n"
                                "pick lid box lid cat dog\nend\n");
    std::ostringstream answers;
    tutor(*pupil, solo, examples, answers);
    check(solo.cursor == 4 && answers.str().ends_with("choice left cat\n"), "external transfer failed");
}

void consolidation(const std::filesystem::path &dir) {
    Course c{Task::naming, 51278, 0, 0};
    c.vocabulary = 256;
    c.archive_slots = 1024;
    auto m = hatch(8, 64, 13, c);
    auto cpu = make_cpu(m, 3);
    cpu->run(c, 0, 256, Learning::full);
    c.novel = 1;
    c.seed ^= 0xe62f3a51u;
    auto retained = cpu->run(c, 0, 256, Learning::frozen);
    check(wins(retained) > 0.85 * 2048, "consolidated word associations not retained");
    cpu->download(m);
    auto path = dir / "vocabulary.axn";
    save_model(m, path);
    auto again = load_model(path);
    check(identical(m, again) && again.course.vocabulary == 256, "consolidation checkpoint");
    resize_archive(again, 2048);
    cpu->restore(again);
    check(wins(cpu->run(c, 0, 256, Learning::frozen)) > 0.84 * 2048, "archive growth lost associations");
    resize_archive(again, 0);
    cpu->restore(again);
    check(wins(cpu->run(c, 0, 256, Learning::frozen)) < 0.66 * 2048, "archive ablation retained stale records");
}

void workshop(const std::filesystem::path &dir) {
    for (Word integer = 0; integer <= 3; ++integer) {
        Costs c{(integer & 1) ? ~0u : 0u, (integer & 2) ? ~0u : 0u};
        c.observe(0xaaaaaaaau);
        for (int bit = 0; bit < 32; ++bit) {
            Word value = ((c.low >> bit) & 1) + 2 * ((c.high >> bit) & 1);
            check(value == std::min(3u, integer + (bit & 1)), "bit-sliced synapse increment");
        }
    }
    Course c{Task::workshop, 91253, 0, 0};
    c.difficult = 2;
    c.chain = 5;
    auto m = hatch(8, 64, 73, c);
    auto cpu = make_cpu(m, 3);
    auto q = c;
    q.novel = 1;
    q.seed ^= 0x83a1253du;
    auto before = cpu->run(q, 0, 128, Learning::frozen);
    check(wins(before) < 5, "untrained circuit predicted unseen composites");
    cpu->run(c, 0, 112, Learning::full);
    check(wins(cpu->run(q, 0, 128, Learning::frozen)) == 1024, "forward composition");
    cpu->download(m);
    check(m.memory.empty() && m.maps.empty() && m.drawings.size() == 8, "unrelated sensory stores allocated");
    auto path = dir / "workshop.axn";
    save_model(m, path);
    auto fresh = load_model(path);
    check(identical(m, fresh) && fresh.course.chain == 5 && fresh.course.difficult == 2, "tool checkpoint");
    cpu->restore(fresh);
    q.task = Task::invention;
    check(wins(cpu->run(q, 0, 128, Learning::frozen)) == 1024, "inverse planning after forward-only experience");
    auto one = hatch(1, 128, 1, c);
    auto pupil = make_cpu(one, 1);
    // Teach a tool outside the generator through an external sensory stream.
    // No examples of the two-step program occur before the probe.
    std::ostringstream events;
    for (Word i = 0; i < 96; ++i) {
        Word x = mix(i + 15), y = shift_pixels(x, 7) ^ 0xbaca1119u;
        events << "demo 0x" << std::hex << x << " orbit 0x" << y << '\n';
    }
    events << "draw 0x12345678 orbit orbit\n";
    Word target = shift_pixels(shift_pixels(0x12345678, 7) ^ 0xbaca1119u, 7) ^ 0xbaca1119u;
    events << "make 0x" << std::hex << target << " orbit orbit\n";
    std::istringstream stream(events.str());
    std::ostringstream reply;
    tutor(*pupil, one, stream, reply);
    std::ostringstream end;
    end << "prediction 0x" << std::hex << std::setw(8) << std::setfill('0') << target << "\ninput 0x12345678\n";
    check(reply.str().ends_with(end.str()) && one.cursor == 96, "external generative tool learning");
}

void observation() {
    Course c{Task::detector, 913257, 0, 0};
    auto initial = hatch(32, 128, 993, c);
    auto engine = make_cpu(initial, 4);
    for (Word condition = 0; condition < 2; ++condition) {
        c.reversed = condition;
        Detector toy{0, bool(condition)};
        int active = 0;
        for (Word i = 0; i < 8; ++i) {
            auto pair = toy.demonstration(i);
            auto repeat = toy.demonstration(i ^ 1);
            check(pair.a == repeat.a && pair.b == repeat.b, "training display failed to repeat");
            active += toy.music(pair);
        }
        check(active == 4, "unbalanced observed effects");
        engine->restore(initial);
        engine->run(c, 0, 8, Learning::full);
        auto trained = initial;
        engine->download(trained);
        check(trained.memory[0].used == 8, "missing observational memory");
        auto q = c;
        q.seed ^= 0xe62f3a51u;
        q.novel = 1;
        check(wins(engine->run(q, 0, 1, Learning::frozen)) >= 28, "first relational tray choice");
        auto after = initial;
        engine->download(after);
        check(identical(after, trained), "unrewarded test choice changed the learner");
    }
}

void controlled_experiments(const std::filesystem::path &dir) {
    Course c{Task::workshop, 71231, 0, 0};
    c.lesson = 17401;
    c.chain = 5;
    c.difficult = 3;
    c.experiments = Course::Experiments::paired;
    for (Word i = 0; i < 256; i += 2) {
        Word a = Workshop::picture(c, 0, i), b = Workshop::picture(c, 0, i + 1);
        check(population(a ^ b) == 1, "experiment varied multiple inputs");
        check(!(mix(a ^ 0x341c0ffdu) & 1) && !(mix(b ^ 0x341c0ffdu) & 1), "support/query leak");
        check(Workshop::sequence(c, 1, i, 1) == Workshop::sequence(c, 2, i + 1, 1), "tool changed in a pair");
    }
    auto m = hatch(4, 64, 379, c);
    auto cpu = make_cpu(m, 4);
    c.experiments = Course::Experiments::curious;
    m.course = c;
    cpu->run(c, 0, 4097, Learning::full);
    cpu->download(m);
    m.cursor = 4097;
    auto path = dir / "discovery.axn";
    save_model(m, path);
    auto copy = load_model(path);
    check(identical(copy, m) && copy.course.experiments == c.experiments, "intervention checkpoint");
    auto restarted = make_cpu(copy, 3);
    same_counts(cpu->run(c, 4097, 4095, Learning::full), restarted->run(c, 4097, 4095, Learning::full));
    cpu->download(m);
    restarted->download(copy);
    check(identical(copy, m), "interrupted experiment changed the causal graph");
    for (Word animal = 0; animal < m.animals; ++animal) {
        Workshop world = Workshop::make(c, animal);
        int slot = m.drawings[animal].palette.find(world.cue(1));
        Word w = mix(world.key ^ (2u * 0x62f35de9u));
        for (Word j = 0; j < 32; ++j) {
            Word input = (j * ((w >> 9) | 1) + (w >> 22)) & 31;
            Word parents = 0;
            for (Word lag : {0, 5, 13, 7, 3, 10, 27}) parents |= 1u << ((input - lag) & 31);
            check(m.interventions[animal].circuit[slot].parents[j] == parents, "recovered causal afferents differ");
        }
    }
    c.novel = 1;
    c.seed ^= 0xe62f3a51u;
    check(wins(cpu->run(c, 0, 128, Learning::frozen)) >= 500, "unseen nonlinear tool sequence");
}

// Exercise the device through its ordinary command and checkpoint interface.
std::string neural_command(std::vector<std::string> args, bool individual = false) {
    args.insert(args.begin(), {"axion", individual ? "agent" : "life"});
    std::vector<char *> words;
    for (auto &s : args) words.push_back(s.data());
    std::ostringstream report;
    auto previous = std::cout.rdbuf(report.rdbuf());
    try {
        auto fn = individual ? life::actor_command : life::command;
        fn(int(words.size()), words.data());
    } catch (...) { std::cout.rdbuf(previous); throw; }
    std::cout.rdbuf(previous);
    return report.str();
}

std::string ecosystem_run(std::vector<std::string> args) {
    auto log = neural_command(std::move(args));
    return log.substr(log.rfind("tick="));
}

bool same_bytes(const std::filesystem::path &a, const std::filesystem::path &b) {
    std::ifstream x(a, std::ios::binary), y(b, std::ios::binary);
    return x && y && std::equal(std::istreambuf_iterator<char>(x), {}, std::istreambuf_iterator<char>(y), {});
}

std::string report_line(const std::string &report, const std::string &label) {
    auto at = report.find(label);
    check(at != std::string::npos, "missing experimental control");
    return report.substr(at, report.find('\n',at)-at);
}

std::uint64_t metric(const std::string &line, std::string name) {
    auto key = "  " + name + "=";
    auto offset = line.find(key);
    check(offset != std::string::npos, "missing ecological measurement");
    return std::stoull(line.substr(offset + key.size()));
}

void ecosystems(const std::filesystem::path &dir) {
    auto full = (dir/"full.eco").string(), part = (dir/"part.eco").string(), resumed = (dir/"resumed.eco").string();
    auto result = ecosystem_run({"--side","32","--seed","992","--steps","241","--dispatch","direct","--save",full});
    ecosystem_run({"--side","32","--seed","992","--steps","73","--save",part});
    auto continuation = ecosystem_run({"--load",part,"--steps","168","--report-every","31","--save",resumed});
    std::ifstream a(full, std::ios::binary), b(resumed, std::ios::binary);
    check(std::equal(std::istreambuf_iterator<char>(a), {}, std::istreambuf_iterator<char>(b), {}),
          "resumed ecosystem diverged from the continuous CUDA simulation");
    check(metric(result,"births") > 0 && metric(result,"deaths") > 0 && metric(result,"tape_edits") > 0
          && metric(result,"edits") > 0 && metric(result,"gate_flips") > 0 && metric(result,"moves") > 0
          && metric(continuation,"edits") == metric(result,"edits"), "shared ecosystem lacked neural or heritable variation");
    auto assay = ecosystem_run({"--load",full,"--steps","0","--assay","32","--save",resumed});
    check(metric(assay,"different_gates") > 0 && metric(assay,"different_choices") > 0, "neural history transplant failed");
    std::ifstream original(full, std::ios::binary), tested(resumed, std::ios::binary);
    check(std::equal(std::istreambuf_iterator<char>(original), {}, std::istreambuf_iterator<char>(tested), {}),
          "destructive synaptic assay");
    auto sparse = (dir/"sparse.eco").string();
    auto donors = ecosystem_run({"--side","32","--density","1","--seed","31","--steps","0","--save",sparse});
    check(metric(donors,"alive") > 0 && metric(donors,"alive") < 32, "invalid sparse donor fixture");
    check(metric(ecosystem_run({"--side","32","--founders",sparse,"--steps","16"}),"updates") > 0, "sparse transplant failed");
    for (std::string neurons : {"64","128","256","512","1024"}) {
        auto quiet = ecosystem_run({"--side","16","--neurons",neurons,"--steps","64","--plasticity","off","--mutation","off"});
        check(!metric(quiet,"edits") && !metric(quiet,"tape_edits") && !metric(quiet,"gate_flips")
              && metric(quiet,"spikes") && metric(quiet,"updates"), "device did not honor neural or genetic controls");
    }
    auto empty = ecosystem_run({"--side","16","--density","0","--steps","65"});
    check(!metric(empty,"alive") && !metric(empty,"births") && !metric(empty,"updates"), "empty habitat acquired an organism");
    for (std::string contest : {"ancestry","body","brain","plasticity","agency"}) {
        auto x = ecosystem_run({"--side","16","--seed","73","--steps","0","--founders",full,"--contest",contest});
        auto y = ecosystem_run({"--side","16","--seed","73","--steps","0","--founders",full,"--contest",contest,"--swap-cohorts"});
        check(metric(x,"treatment_alive") > 0 && metric(x,"control_alive") > 0
              && metric(x,"treatment_alive") == metric(y,"control_alive"), "reciprocal transplant allocation changed");
    }
    std::fstream corrupt(part, std::ios::in | std::ios::out | std::ios::binary);
    corrupt.seekp(-1, std::ios::end);
    char last = 0;
    corrupt.seekg(-1, std::ios::end); corrupt.read(&last, 1);
    corrupt.seekp(-1, std::ios::end); last ^= 1; corrupt.write(&last, 1); corrupt.close();
    bool rejected = false;
    try { ecosystem_run({"--load",part,"--steps","0"}); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected, "damaged ecosystem checkpoint accepted");
}

void grove_learning(const std::filesystem::path &dir) {
    auto full = (dir/"orchard.eco").string(), pause = (dir/"orchard-pause.eco").string(), again = (dir/"orchard-again.eco").string();
    for (auto parameters : std::vector<std::vector<std::string>>{
            {"--habitat","grove"}, {"--habitat","compound","--digestion","4"},
            {"--habitat","grove","--species","32"}, {"--habitat","compound","--species","16","--synapse-bits","2","--digestion","8","--season","8"},
            {"--habitat","coupled","--species","16","--synapse-bits","2","--digestion","4","--relations","on","--season","8"},
            {"--habitat","thicket","--species","16","--relations","on","--synapse-bits","2","--fruit","inherited"},
            {"--habitat","thicket","--species","8","--fruit","persistent","--relations","on"},
            {"--habitat","thicket","--species","8","--flora","clonal","--digestion","3"},
            {"--habitat","thicket","--species","16","--fruit","inherited","--relations","contexts","--climates","4","--season","8",
             "--climate-cue","on","--digestion","4","--synapse-bits","2","--programs","2","--contest","contexts"}}) {
        parameters.insert(parameters.end(), {"--seed","991","--side","32"});
        auto a = parameters, b = parameters;
        a.insert(a.end(), {"--steps","179","--dispatch","direct","--schedule","dense","--save",full});
        b.insert(b.end(), {"--steps","63","--schedule","occupied","--save",pause});
        auto x = ecosystem_run(a);
        ecosystem_run(b);
        auto y = ecosystem_run({"--load",pause,"--steps","116","--report-every","39","--assay","2","--schedule","occupied","--save",again});
        check(same_bytes(full,again), "grove reserves, neural traces or genomes diverged on resume");
        check(metric(x,"attempts") >= metric(x,"opened") && metric(x,"opened") && metric(y,"deaths"), "inert grove physics");
        check(metric(report_line(y,"assay=identical_history"),"different_choices") == 0, "identical history changed actuator");
        if (x.find("flora=") != std::string::npos) {
            check(metric(x,"seedings") > 0, "producer population failed to propagate");
            check((metric(x,"plant_edits") == 0) == (x.find("flora=clonal") != std::string::npos), "plant mutation treatment failed");
            auto fixed = ecosystem_run({"--load",full,"--steps","64","--flora","fixed"});
            check(metric(fixed,"seedings") == metric(x,"seedings") && metric(fixed,"plant_boundaries") == metric(x,"plant_boundaries") &&
                  metric(fixed,"response_disagreements") == metric(x,"response_disagreements"), "stationary plant genotypes or expressed responses changed");
        }
        if (x.find("climates=")!=std::string::npos) {
            std::uint64_t attempts=0, opened=0;
            for (std::string cohort:{"control_late","control_early","treatment_late","treatment_early"}) {
                attempts+=metric(x,cohort+"_tries"); opened+=metric(x,cohort+"_opened");
            }
            check(attempts==metric(x,"attempts") && opened==metric(x,"opened") && metric(x,"climate_differences")>0,
                  "seasonal chemistry or harvest ledgers diverged");
        }
    }

    auto panel = neural_command({"--load","examples/learner.eco","--steps","0","--conditioning","32","--problem","dimensions","--varieties","16","--probe-seed","107","--save",again});
    check(same_bytes("examples/learner.eco",again), "conditioning altered the habitat");
    auto real = report_line(panel,"conditioning=intact "), fake = report_line(panel,"conditioning=yoked ");
    auto frozen = report_line(panel,"conditioning=fixed_contacts ");
    auto n = metric(real,"items"), changed = metric(real,"changed_items");
    check(n == 8*32*16*8 && changed + metric(real,"stable_items") == n, "incorrect held-out pair denominators");
    check(metric(real,"naive") <= n/4 && metric(real,"naive") == metric(fake,"naive"), "first choice leaked information");
    check(metric(real,"learned") > n*9/10 && metric(real,"recovered") > changed*9/10, "fresh unseen pairs or dimension shifts failed");
    check(metric(fake,"delivered") == metric(real,"delivered") && metric(fake,"learned") < n*35/100,
          "uncoupled outcomes convey private solutions");
    check(metric(frozen,"learned") == metric(frozen,"erased") && metric(real,"erased") < n*35/100,
          "restored synapses retained associations");
    auto delayed = neural_command({"--load","examples/delayed.eco","--steps","0","--conditioning","32","--probe-delay","4","--dissect"});
    auto d4 = report_line(delayed,"conditioning=conduction_4 "), d1 = report_line(delayed,"conditioning=conduction_1 ");
    check(metric(d4,"learned") > metric(d1,"learned")+metric(d4,"items")/4, "assigning credit to another decision did not disrupt the delayed learner");

    auto founder = neural_command({"--habitat","compound","--synapse-bits","2","--side","16","--steps","0","--dissect"});
    check(founder.find("distinct_rules=1  rule_aaaa=") != std::string::npos, "founders expressed an acquired update rule");
    auto nursery = std::vector<std::string>{"--habitat","compound","--side","8","--nursery","8","--seed","412","--steps","0","--save",full};
    neural_command(nursery); nursery.back() = again; neural_command(nursery);
    check(same_bytes(full,again), "GPU nursery is not reproducible");
    nursery.insert(nursery.end(), {"--breeding","drift"});
    neural_command(nursery); check(!same_bytes(full,again), "neutral parent sampling ignored");

    auto links = neural_command({"--load","examples/relations.eco","--steps","0","--problem","mixed-laws","--varieties","16",
                                 "--exposures","32","--conditioning","12","--probe-seed","106261","--save",again});
    check(same_bytes("examples/relations.eco",again), "relational probes wrote back to the parent world");
    auto intact = report_line(links,"conditioning=intact "), yoked = report_line(links,"conditioning=yoked ");
    auto no_edges = report_line(links,"conditioning=fixed_pairs ");
    auto items = metric(intact,"items"), discordant = metric(intact,"discordant_items");
    check(items == 8*12*128 && 4*metric(intact,"naive") <= items && discordant > 0, "private-law counterbalancing failed");
    check(metric(intact,"learned") > items*3/4 && metric(intact,"discordant_correct") > discordant/2, "learner lost its private interaction law");
    check(metric(intact,"readout_cut") < items/3 && metric(no_edges,"learned") < items/3 &&
          metric(yoked,"delivered") == metric(intact,"delivered"), "equal-outcome or recurrent lesion control failed");
    auto noise = report_line(neural_command({"--load","examples/relations.eco","--steps","0","--problem","unstructured","--varieties","16",
                                             "--exposures","32","--conditioning","12","--probe-seed","106261"}),"conditioning=intact ");
    check(metric(noise,"learned") < metric(noise,"items")/3, "unseen independent responses are predictable");
    for (auto width : {"64","128","256","512","1024"}) {
        for (auto mode : {"on","waves","contexts"}) {
            bool waves = std::string_view(mode) != "on";
            auto params = std::vector<std::string>{"--habitat","thicket","--relations",mode,"--fruit","inherited","--species","16",
                "--synapse-bits","2","--neurons",width,"--founders",waves ? "examples/pathways.eco" : "examples/relations.eco","--founder-site","1",
                "--digestion","3","--side","16","--seed","741","--steps","96"};
            if (std::string_view(mode)=="contexts")
                params.insert(params.end(),{"--context-founders","examples/contexts.eco","--alternate-founders","examples/repertoire.eco",
                                            "--programs","2","--polarity","evolvable","--climates","2","--season","8"});
            auto other = params;
            params.insert(params.end(), {"--schedule","dense","--dispatch","direct","--save",full});
            other.insert(other.end(), {"--schedule","occupied","--save",again});
            ecosystem_run(params); ecosystem_run(other);
            check(same_bytes(full,again), "relational populations diverged across schedules");
        }
    }
    auto field = ecosystem_run({"--load","examples/thicket.eco","--steps","64"});
    check(metric(field,"alive") && metric(field,"births") && metric(field,"plant_edits"), "released thicket lost a population");
    auto sparse_test = [&](const char *name) {
        auto log = neural_command({"--load","examples/pathways.eco","--steps","0","--conditioning","16","--problem",name,
                                   "--varieties","16","--exposures","16","--probe-seed","740897","--save",again});
        check(same_bytes("examples/pathways.eco",again), "sparse tests changed a habitat");
        return report_line(log,"conditioning=intact ");
    };
    auto chains = sparse_test("chains"), islands = sparse_test("islands");
    auto remote = metric(chains,"remote_items");
    check(metric(chains,"items") == 4*16*16*14 && remote == 4*16*16*8 && metric(chains,"practiced_items") == 4*16*32,
          "incorrect sparse training or long-distance probe counts");
    check(metric(chains,"remote_correct") > remote*3/4 && metric(chains,"remote_local") < remote/3,
          "recurrent propagation lost long-distance generalization");
    check(metric(islands,"learned") < metric(islands,"items")/3, "inferred across an unobserved component boundary");
    auto sample = neural_command({"--habitat","thicket","--fruit","inherited","--relations","waves","--synapse-bits","2",
                                  "--founders","examples/pathways.eco","--founder-site","1","--side","16","--steps","0",
                                  "--survey","32","--save",full});
    auto inventory = report_line(sample,"survey=32 "), expressed = report_line(sample,"phenotype=");
    check(metric(inventory,"distinct") == 1 && metric(expressed,"count") == 32, "identical germlines gave different standardized behavior");
    auto repeated = neural_command({"--load",full,"--steps","0","--survey","32","--save",again});
    check(same_bytes(full,again) && report_line(repeated,"phenotype=") == expressed, "behavioral survey mutated its habitat or changed its sample");
    auto representative = metric(expressed,"site");
    auto hatched = neural_command({"--from",full,"--site",std::to_string(representative),"--inspect"},true);
    check(hatched.find("propagation_steps=") != std::string::npos, "survey exemplar could not be hatched");
}

void returning_memory(const std::filesystem::path &dir) {
    auto snapshot=(dir/"intact.eco").string();
    auto source=std::string("examples/repertoire.eco");
    auto rows=neural_command({"--load",source,"--save",snapshot,"--steps","0","--revisit","4","--regimes","4",
                              "--varieties","8","--exposures","12","--probe-delay","4","--memory-task","interleaved"});
    check(same_bytes(source,snapshot),"returning memory assay changed the original habitat");
    std::istringstream input(rows);
    std::string line;
    Word checked=0;
    while (std::getline(input,line)) {
        if (!line.starts_with("revisit=")) continue;
        auto arm=line.substr(8,line.find(' ')-8);
        auto stage=metric(line,"reminders"), items=metric(line,"items"), correct=metric(line,"correct");
        check(items==16*4*(6+32) && metric(line,"unary_items")==16*4*6 && metric(line,"paired_items")==16*4*32,
              "mixed-domain probes included a reminder or practiced pair");
        check(metric(line,"unary_correct")+metric(line,"paired_correct")==correct,"mixed-domain subtotals disagree");
        std::uint64_t by_regime=0,acquired=0;
        for (Word i=0;i<4;++i) {
            by_regime+=metric(line,"recall_"+std::to_string(i));
            acquired+=metric(line,"acquired_"+std::to_string(i));
        }
        check(by_regime==correct && acquired==metric(line,"acquired"),"per-environment scores disagree");
        auto ceiling=metric(line,"feedback_ceiling_twelfths");
        if (arm=="ambiguous" || stage<4) {
            check(ceiling==3*items && !metric(line,"identified_items"),"a hidden context leaked before an informative outcome arrived");
            if (arm!="assigned") check(4*correct<=items,"shared physical input revealed an unobserved world");
        }
        if (arm=="intact" || arm=="paired_hint") check(12*correct<=ceiling,"animal exceeded its counterfactual information bound");
        if (arm=="primary_only") check(!metric(line,"alternate_trials"),"primary expression clamp failed");
        if (arm=="alternate_only") check(metric(line,"alternate_trials")==metric(line,"training_trials"),"alternate expression clamp failed");
        ++checked;
    }
    check(checked==17*6,"missing hidden-world interventions");
    auto polarized=(dir/"polarized.eco").string();
    constexpr Word experienced_ticks=16;
    neural_command({"--habitat","thicket","--relations","contexts","--programs","2","--polarity","evolvable",
                    "--species","16","--fruit","inherited","--climates","4","--season","8","--synapse-bits","2","--digestion","4",
                    "--founders",source,"--side","16","--seed","701","--steps",std::to_string(experienced_ticks),"--save",polarized});
    auto expression=neural_command({"--from",polarized,"--inspect"},true);
    check(expression.find("synaptic_polarity=evolvable")!=std::string::npos,"hatching lost the physical contact frame");
    life::ContextLayer before{},after{};
    life::ExpressionLayer initial_switch{},live_switch{};
    life::AssociationLayer initial_links{},live_links{};
    std::ifstream habitat(polarized,std::ios::binary);
    constexpr size_t header=16+offsetof(life::Settings,balanced);
    constexpr size_t sites=16*16;
    life::Site body{};
    Word site=0,occupied=0;
    habitat.seekg(header);
    for (;site<sites;++site) {
        habitat.read(reinterpret_cast<char *>(&body),sizeof(body));
        if (body.body.energy && body.body.born<experienced_ticks) break;
    }
    check(site<sites,"no experienced resident in the habitat");
    auto pupil=life::hatch_from_habitat(polarized.c_str(),site,occupied,initial_links,before,initial_switch);
    auto resident=life::hatch_from_habitat(polarized.c_str(),site,occupied,live_links,after,live_switch,true);
    check(occupied==site && resident.ticks>0 && !pupil.ticks && !std::memcmp(&pupil.genes,&resident.genes,sizeof(pupil.genes)),
          "resident export changed its inherited circuit or reset its lifetime");
    check(body.body.energy && resident.ticks==experienced_ticks-body.body.born && ((resident.due>>(2*(resident.ticks&7)))&3)==body.body.recent,
          "resident age or delivered metabolic signal changed during extraction");
    life::Tissue actual{};
    habitat.seekg(header+sites*(sizeof(body)+sizeof(life::Genome))+body.body.slot*resident.tiles*sizeof(actual));
    habitat.read(reinterpret_cast<char *>(&actual),sizeof(actual));
    check(habitat.good() && !std::memcmp(&actual,resident.tissue,sizeof(actual)),"resident lost its somatic state");
    auto reset=(dir/"reset.eco").string(),twice=(dir/"twice.eco").string();
    neural_command({"--load",polarized,"--reset-contacts","--steps","0","--save",reset});
    neural_command({"--load",reset,"--reset-contacts","--steps","0","--save",twice});
    check(!same_bytes(polarized,reset) && same_bytes(reset,twice),"one-time contact intervention changed the wrong state");
    life::ContextLayer reset_context{}; life::AssociationLayer reset_links{}; life::ExpressionLayer reset_switch{};
    auto cleared=life::hatch_from_habitat(reset.c_str(),site,occupied,reset_links,reset_context,reset_switch,true);
    check(!std::memcmp(cleared.tissue,resident.tissue,resident.tiles*sizeof(life::Tissue)) &&
          !std::memcmp(&after.genes,&reset_context.genes,sizeof(after.genes)) && cleared.ticks==resident.ticks &&
          cleared.due==resident.due && after.activity==reset_context.activity && after.proposals==reset_context.proposals &&
          after.landmarks==reset_context.landmarks && live_switch.history==reset_switch.history,
          "ecological contact intervention changed neural firing, genes, context or pending outcomes");
    for (const auto &bad:std::vector<std::vector<std::string>>{
            {"--alternate-founders",source,"--steps","0"},{"--polarity","evolvable","--steps","0"},
            {"--relations","contexts","--habitat","thicket","--context-founders","examples/pathways.eco","--steps","0"}}) {
        bool rejected=false;
        try { neural_command(bad); } catch (const std::invalid_argument &) { rejected=true; }
        check(rejected,"incompatible inherited program transplant accepted");
    }
    auto crowded=(dir/"crowded.eco").string(),fixed=(dir/"fixed-schedule.eco").string();
    auto founding=neural_command({"--habitat","thicket","--fruit","inherited","--species","16","--relations","contexts",
        "--synapse-bits","2","--programs","2","--polarity","evolvable","--climates","4","--season","16","--side","256",
        "--density","100","--seed","1847","--founders",source,"--steps","0","--save",crowded});
    check(founding.find("schedule=dense  adaptive_schedule=1")!=std::string::npos,"crowded habitat used the sparse scheduler");
    auto varying=neural_command({"--load",crowded,"--steps","279","--save",reset});
    neural_command({"--load",crowded,"--steps","279","--schedule","dense","--save",fixed});
    check(varying.find("next_schedule=occupied")!=std::string::npos && same_bytes(reset,fixed),
          "population-aware graph switching changed the trajectory or failed to adapt");
}

void food_ecology(const std::filesystem::path &dir) {
    auto habitat=(dir/"food.eco").string(), split=(dir/"food-split.eco").string(), replay=(dir/"food-replay.eco").string();
    for (auto [width,level]:std::array<std::pair<int,int>,5>{{{64,0},{128,1},{256,3},{512,2},{1024,3}}}) {
        auto settings=std::vector<std::string>{"--habitat","thicket","--fruit","inherited","--climates","4","--season","8",
            "--stand-size","8","--metabolism","food-fuel","--seep","2","--leach","1","--side","16","--neurons",std::to_string(width),
            "--synapse-bits","2","--species","8","--digestion","4","--seed","1847","--founders","examples/prospector.eco"};
        if (level) settings.insert(settings.end(),{"--relations",level==1 ? "on" : level==2 ? "waves" : "contexts"});
        if (level==3) settings.insert(settings.end(),{"--programs","2","--polarity","evolvable"});
        auto a=settings,b=settings;
        a.insert(a.end(),{"--steps","83","--schedule","dense","--dispatch","direct","--save",habitat});
        b.insert(b.end(),{"--steps","13","--schedule","occupied","--save",split});
        auto totals=ecosystem_run(a); ecosystem_run(b);
        ecosystem_run({"--load",split,"--steps","70","--schedule","occupied","--save",replay});
        check(same_bytes(habitat,replay),"dietary state, pending digestion or plant ancestry diverged during replay");
        std::uint64_t food=0;
        for (int j=0;j<4;++j) food+=metric(totals,"food_"+std::to_string(j));
        check(food==metric(totals,"grazed") && food && metric(totals,"converted") && metric(totals,"minerals_lost"),
              "dietary flux did not balance across a changing material supply");
    }
    auto low=neural_command({"--load","examples/prospector.eco","--steps","0","--valuation","16","--outcome-task","inferred",
                             "--probe-seed","169387","--varieties","16","--exposures","16","--pairings","1","--save",replay});
    check(same_bytes("examples/prospector.eco",replay),"outcome learning assay modified its donor population");
    auto v=report_line(low,"valuation=intact "),blind=report_line(low,"valuation=no_taste ");
    auto quiet=report_line(low,"valuation=silent "); auto items=metric(v,"items");
    check(items==16*4*16*4 && 4*metric(v,"naive")<=items && 4*metric(v,"explored")<=items,
          "unobserved food labels leaked through exploration");
    check(4*metric(blind,"changed")<=items && metric(v,"changed")>3*items/4 && metric(quiet,"changed")<items/2,
          "passive sensory learning or readout isolation failed");
    auto realistic=neural_command({"--load",split,"--steps","0","--growth","3","--appetites","48","--exposures","8",
                                   "--varieties","8","--save",replay});
    check(same_bytes(split,replay),"common garden or resident probe changed its donor");
    auto residents=report_line(realistic,"appetite=resident ");
    check(metric(residents,"correct")<=metric(residents,"available") && metric(residents,"available")<=metric(residents,"goals") &&
          metric(residents,"goals")==metric(residents,"residents")*32,"resident food affordance accounting disagrees");
    auto grows=report_line(realistic,"growth=intact ");
    check(metric(grows,"survived")<=12 && metric(grows,"lived_ticks")<=12*8*8,"invalid individual lifetime budget");
    auto sampled=neural_command({"--load","examples/prospector.eco","--steps","0","--pupils","7","--varieties","8","--exposures","8",
                                 "--pairings","1","--probe-seed","47927","--save",replay});
    auto population=report_line(sampled,"pupils=screen ");
    auto best=report_line(sampled,"pupils=audit ");
    check(same_bytes("examples/prospector.eco",replay) && metric(population,"residents")==7 &&
          metric(population,"genotypes")<=7 && metric(population,"total")<=metric(population,"possible") &&
          metric(best,"worlds")==64 && metric(best,"changed")<=metric(best,"points"),
          "individual cognitive screening changed its residents or used incorrect counterbalancing");
    auto plain=std::vector<std::string>{"--habitat","thicket","--fruit","inherited","--climates","4","--season","8",
        "--stand-size","8","--metabolism","balanced","--side","16","--species","8","--relations","contexts","--synapse-bits","2",
        "--programs","2","--polarity","evolvable","--seed","601","--founders","examples/prospector.eco","--steps","0"};
    auto original=plain;
    original.insert(original.end(),{"--save",habitat}); ecosystem_run(original);
    for (auto m:{"body","soma","dendrites","pathways","memory","outcomes","fidelity"}) plain.insert(plain.end(),{"--graft",m,"examples/prospector.eco"});
    plain.insert(plain.end(),{"--plant-founders",habitat,"--food-genes",habitat,"--save",replay}); ecosystem_run(plain);
    check(same_bytes(habitat,replay),"same-source modular graft or spatial producer transplant changed the founders");
    ecosystem_run({"--load",habitat,"--steps","12","--save",replay});
    auto erase=(dir/"fresh-contacts.eco").string(),twice=(dir/"twice-food.eco").string();
    ecosystem_run({"--load",replay,"--reset-contacts","--steps","0","--save",erase});
    ecosystem_run({"--load",erase,"--reset-contacts","--steps","0","--save",twice});
    check(same_bytes(erase,twice) && !same_bytes(replay,erase),"reset missed or repeatedly changed a learned outcome");
    auto slow=std::vector<std::string>{"--habitat","thicket","--metabolism","food-fuel","--side","16","--species","4",
        "--fruit","inherited","--climates","4","--season","128","--seep","2","--leach","3","--mouth-origin","whole",
        "--seed","601","--founders","examples/prospector.eco","--digestion","4","--mutation","off"};
    auto soft=slow;
    soft.insert(soft.end(),{"--steps","32"});
    auto quick=ecosystem_run(soft);
    slow.insert(slow.end(),{"--husk","3"});
    auto first=slow; first.insert(first.end(),{"--steps","5","--save",split}); ecosystem_run(first);
    slow.insert(slow.end(),{"--steps","32","--schedule","dense","--dispatch","direct","--save",habitat});
    auto thick=ecosystem_run(slow);
    ecosystem_run({"--load",split,"--steps","27","--schedule","occupied","--save",replay});
    check(same_bytes(habitat,replay) && metric(thick,"tastes_without_food")>metric(quick,"tastes_without_food")
          && metric(thick,"opened")<metric(quick,"opened"),"processing a mouthful failed to separate tasting from ingestion");
    for (auto module:std::vector<std::string>{"outcomes","subsistence"}) {
        auto mini=std::vector<std::string>{"--habitat","thicket","--metabolism","balanced","--side","8","--seed","411",
              "--nursery","3","--nursery-varieties","4","--nursery-problem",module,"--breeding","organ","--steps","0"};
        if (module=="outcomes") mini.insert(mini.end(),{"--practice-schedule","short-long","--valuation","2","--exposures","1"});
        auto nursery=neural_command(mini);
        check(metric(report_line(nursery,"nursery=organ "),"tested_lifetimes")==64*3*16,"incorrect ecological nursery selection count");
    }
}

void inheritance_and_dispersal(const std::filesystem::path &dir) {
    auto whole=(dir/"inherited.eco").string(),part=(dir/"parents.eco").string(),split=(dir/"settled.eco").string();
    const std::vector<std::string> thicket{"--habitat","thicket","--metabolism","food-fuel","--seep","2","--leach","3",
        "--fruit","inherited","--species","8","--climates","4","--season","16","--relations","contexts","--programs","2",
        "--synapse-bits","2","--polarity","evolvable","--founders","examples/prospector.eco"};
    for (const auto &habitat:std::array<std::vector<std::string>,2>{{{},thicket}}) {
        auto base=habitat;
        base.insert(base.end(),{"--side","16","--seed","601","--modifier-origin","random"});
        std::vector<std::uint64_t> control;
        for (auto mode:{"modular","uniform","cost-only"}) {
            auto fixed=base;
            fixed.insert(fixed.end(),{"--inheritance",mode,"--mutation","off","--steps","192"});
            auto end=ecosystem_run(fixed);
            std::vector<std::uint64_t> values;
            for (auto key:{"births","alive","energy","updates","copy_exponent_0","copy_exponent_3"})
                values.push_back(metric(end,key));
            check(metric(end,"births")>0 && metric(end,"edits")==0 && metric(end,"copy_exponent_0")>0,
                  "copying cost control did not express inherited modifiers");
            if (control.empty()) control=values;
            else check(values==control,"matched copying costs changed a mutation-frozen population across treatments");
        }
        base.insert(base.end(),{"--inheritance","modular"});
        auto a=base,b=base;
        a.insert(a.end(),{"--steps","191","--save",whole});
        b.insert(b.end(),{"--steps","67","--save",part});
        auto end=ecosystem_run(a); ecosystem_run(b);
        ecosystem_run({"--load",part,"--schedule","occupied","--steps","124","--save",split});
        check(same_bytes(whole,split) && metric(end,"edits")>0,"mutation modifiers or copying costs changed on resume");
    }
    for (auto planting:{"novel","repeat"}) {
        auto a=std::vector<std::string>{"--load",whole,"--colonize","49703","--settlers","9","--vegetation",planting,"--steps","49"};
        auto first=a;
        a.insert(a.end(),{"--passages","3","--save",split});
        auto report=neural_command(a);
        first.insert(first.end(),{"--dispatch","direct","--save",part}); ecosystem_run(first);
        ecosystem_run({"--load",part,"--colonize","49704","--settlers","9","--vegetation",planting,"--passages","2",
                       "--schedule","occupied","--steps","49","--save",part});
        check(same_bytes(split,part) && metric(report_line(report,"passage=3 "),"parent_pool")==9,
              "resident germline sampling, regeneration or lineage history changed across passage boundaries");
        if (std::string_view(planting)=="novel") std::filesystem::copy_file(split,dir/"new-plants.eco");
        else check(!same_bytes(split,dir/"new-plants.eco"),"vegetation treatment failed to change the habitat");
    }
    auto extinct=neural_command({"--side","8","--density","0","--colonize","1","--passages","3","--steps","17"});
    check(metric(report_line(extinct,"passage=1 "),"parent_pool")==0 && extinct.find("passage=2 ")==std::string::npos,
          "extinct lineages reappeared after dispersal");
    auto sweep=std::vector<std::string>{"--side","128","--seed","2203","--colonize","3179","--passages","2","--settlers","37","--steps","17"};
    auto warp=sweep;
    warp.insert(warp.end(),{"--resident-index","warp","--save",whole});
    sweep.insert(sweep.end(),{"--resident-index","grid","--save",split});
    ecosystem_run(warp); ecosystem_run(sweep);
    check(same_bytes(whole,split),"parallel resident sampling changed the physical order of eligible founders");
}

void navigation(const std::filesystem::path &dir) {
    auto world=(dir/"river.eco").string(),part=(dir/"brook.eco").string(),again=(dir/"brook-again.eco").string();
    for (auto [width,level]:std::array<std::pair<int,int>,5>{{{64,0},{128,1},{256,3},{512,2},{1024,3}}}) {
        std::vector<std::string> settings{"--side","16","--seed","1823","--neurons",std::to_string(width),"--habitat","thicket",
            "--fruit","inherited","--species","8","--climates","4","--season","16","--metabolism","food-fuel","--seep","2",
            "--leach","1","--spring-flow","16","--digestion","3","--navigation","competitive","--synapse-bits","2","--founders","examples/wayfarers.eco"};
        if (level) settings.insert(settings.end(),{"--relations",level==1 ? "on" : level==2 ? "waves" : "contexts"});
        if (level==3) settings.insert(settings.end(),{"--programs","2","--polarity","evolvable"});
        auto a=settings,b=settings;
        a.insert(a.end(),{"--steps","47","--dispatch","direct","--schedule","dense","--save",world});
        b.insert(b.end(),{"--steps","13","--schedule","occupied","--save",part});
        auto full=ecosystem_run(a);ecosystem_run(b);ecosystem_run({"--load",part,"--schedule","occupied","--steps","34","--save",again});
        check(same_bytes(world,again) && metric(full,"seep_0")+metric(full,"seep_1")>0,
              "map development, delayed food or spring minerals changed across a scheduler or checkpoint boundary");
    }
    auto e=neural_command({"--load",part,"--steps","0","--appetites","23","--save",again});
    auto field=report_line(e,"appetite=resident ");
    check(same_bytes(part,again) && metric(field,"closer")<=metric(field,"moving") && metric(field,"passable")<=metric(field,"open")
          && metric(field,"open")<=metric(field,"away"),"counterfactual field navigation changed the world or exceeded accessible moves");
    auto audit=neural_command({"--load","examples/wayfarers.eco","--steps","0","--crossings","3","--distance","5","--exposures","8",
                               "--destination","food","--diversion","both","--mazes","2","--rooms","16","--migration","3","--save",again});
    auto first=report_line(audit,"crossings=intact "),gap=report_line(audit,"crossings=no-appetite ");
    check(metric(first,"naive")<=metric(first,"items")/4 && metric(first,"changed")>metric(first,"items")/2
          && metric(gap,"changed")<metric(first,"changed"),"hidden corridor or appetite information leaked or failed to reach the motor");
    auto maze=report_line(audit,"maze=intact ");
    check(metric(maze,"shortest")<=metric(maze,"arrived") && metric(maze,"arrived")<=metric(maze,"journeys")
          && metric(maze,"changed")<=metric(maze,"changed_items") && same_bytes("examples/wayfarers.eco",again),
          "physical route accounting or immutable maze trials failed");
    auto common=report_line(audit,"migration=intact ");
    auto isolated=neural_command({"--load","examples/wayfarers.eco","--steps","0","--migration","3","--rooms","16","--exposures","8","--value-controls","off"});
    check(common==report_line(isolated,"migration=intact "),"frozen in-lifetime questions changed later reproduction");
    for (auto course:{"migration","navigation"}) {
        auto nursery=neural_command({"--side","8","--habitat","thicket","--metabolism","food-fuel","--navigation","competitive","--history","mixed",
            "--nursery","2","--nursery-problem",course,"--breeding","map","--destination","mixed","--rooms","8","--exposures","2","--steps","0"});
        check(metric(report_line(nursery,"nursery=map "),"tested_lifetimes")==64*2*8,"navigation breeding lifetime count is incorrect");
    }
    auto lesson=(dir/"atlas.lesson").string(),pupil=(dir/"atlas.mind").string();
    { std::ofstream q(lesson);q<<"orient 2 0 2 8\norient 2 0 1 2\norient 2 0 3 1\norient 2 0 0 4\n"; }
    auto saved=neural_command({"--load","examples/wayfarer.mind","--lesson",lesson,"--save",pupil},true);
    auto many=neural_command({"--load",pupil,"--lesson",lesson,"--batch-bench","19","--decisions","8"},true);
    check(saved.find("probe_matches=4/4")!=std::string::npos && same_bytes("examples/wayfarer.mind",pupil)
          && metric(report_line(many,"repeat=1 "),"correct")==19*8,"batched navigation diverged from the frozen individual");
    { std::ofstream q(lesson);q<<"stride 1 0 1 0\nstride 4 0 1 2\nsavour 4 1\ntravel 1 4 0x03020100\n"; }
    neural_command({"--resident",part,"--lesson",lesson,"--save",pupil},true);
    auto resident=neural_command({"--load",pupil,"--inspect"},true);
    check(resident.find("map_plastic=")!=std::string::npos,"resident extraction omitted its navigation circuit");
}

void temporal_memory(const std::filesystem::path &dir) {
    auto full=(dir/"basin-full.eco").string(),part=(dir/"basin-part.eco").string(),again=(dir/"basin-again.eco").string();
    std::vector<std::string> world{"--side","16","--density","100","--seed","6271","--habitat","thicket","--metabolism","food-fuel",
        "--navigation","predictive","--synapse-bits","2","--fruit","inherited","--climates","4","--season","16","--seep","2","--history-origin","random",
        "--spring-flow","16","--basin-width","8","--founders","examples/nomads.eco","--contest","temporal-plasticity"};
    for (unsigned width:{64u,128u,256u,512u,1024u}) {
        auto opts=world;opts.insert(opts.end(),{"--neurons",std::to_string(width)});
        auto a=opts,b=opts;
        a.insert(a.end(),{"--steps","37","--dispatch","direct","--schedule","dense","--save",full});
        b.insert(b.end(),{"--steps","11","--schedule","occupied","--save",part});
        neural_command(a);neural_command(b);
        neural_command({"--load",part,"--steps","26","--schedule","occupied","--save",again});
        check(same_bytes(full,again),"temporal state or basin geography changed at a resumption or scheduling boundary");
    }
    auto assay=neural_command({"--load",full,"--steps","0","--junctions","2","--retention","3","--exposures","3","--save",again});
    auto intact=report_line(assay,"junctions=intact "),hidden=report_line(assay,"junctions=hidden ");
    check(same_bytes(full,again) && metric(intact,"items")==128 && metric(hidden,"revised")<=32 && metric(report_line(assay,"junctions=fixed-support "),"items")==128,
          "ambiguous-junction assay altered inherited residents or exposed the hidden entrance");
    auto nursery=neural_command({"--habitat","thicket","--metabolism","food-fuel","--navigation","episodic","--side","8",
        "--nursery","2","--nursery-problem","junctions","--breeding","history","--passage-test","continuous","--practice-timing","variable",
        "--exposures","2","--steps","0"});
    check(metric(report_line(nursery,"nursery=history "),"tested_lifetimes")==1024,"temporal breeding lifetime count is incorrect");
    auto a=(dir/"trace-a.lesson").string(),b=(dir/"trace-b.lesson").string(),both=(dir/"trace-all.lesson").string();
    auto one=(dir/"trace-one.mind").string(),two=(dir/"trace-two.mind").string(),resume=(dir/"trace-resume.mind").string();
    std::string first="stride 2 0\nstride 4 0\ntravel 8 0 0x03020100\n",last="stride 2 0\nstride 4 0\norient 8 1 1 0\n";
    { std::ofstream(a)<<first;std::ofstream(b)<<last;std::ofstream(both)<<first<<last; }
    neural_command({"--resident",full,"--lesson",both,"--save",one},true);
    neural_command({"--resident",full,"--lesson",a,"--save",two},true);
    neural_command({"--load",two,"--lesson",b,"--save",resume},true);
    check(same_bytes(one,resume) && neural_command({"--load",one,"--inspect"},true).find("trace_plastic=")!=std::string::npos,
          "resuming an independently acting creature lost its temporal circuit");
    auto parent=neural_command({"--from",full,"--inspect"},true);
    auto site=report_line(parent,"donor_site=").substr(std::string_view("donor_site=").size());
    neural_command({"--founders",full,"--founder-site",site,"--side","8","--habitat","thicket","--metabolism","food-fuel",
                    "--navigation","predictive","--steps","0","--save",again});
    auto child=neural_command({"--from",again,"--inspect"},true);
    check(report_line(parent,"trace_plastic=")==report_line(child,"trace_plastic="),"single-founder transplantation lost temporal genes");
    for (auto mode:{"history","history-local"}) {
        neural_command({"--founders",full,"--founder-site",site,"--side","8","--habitat","thicket","--metabolism","food-fuel",
                        "--navigation","associative","--synapse-bits","2","--nursery","24","--nursery-problem","junctions",
                        "--breeding",mode,"--exposures","3","--steps","0","--density","100","--save",again});
        for (auto pos:{"0","15","31","63"}) {
            auto descendant=neural_command({"--from",again,"--site",pos,"--inspect"},true);
            for (auto label:{"metabolism=","map_plastic=","primary_table=","effector_ports="})
                check(report_line(descendant,label)==report_line(parent,label),"temporal-only selection varied a separate inherited program");
        }
    }
    auto pupils=neural_command({"--load",again,"--steps","0","--junction-pupils","3","--junction-types","2","--exposures","2","--retention","1"});
    check(metric(report_line(pupils,"junction_pupils=screen "),"residents")==3,"individual junction screen lost sampled resident identities");
    auto mixed=neural_command({"--load",again,"--steps","0","--junctions","8","--passage-test","mixed","--passage-noise","mixed","--retention","2","--exposures","2","--save",part});
    check(same_bytes(again,part) && metric(report_line(mixed,"junctions=intact "),"items")==512,
          "varying the sensory schedule changed the residents or lost a paired rotation");
    // Five decisions from the original per-query CUDA path on this learned
    // animal. The unequal period also exercises wrap-around in a warp batch.
    { std::ofstream q(a);q<<"orient 1 1 2 0\norient 1 8 3 0\norient 1 64 1 0\norient 1 0 3 1\norient 8 0 1 1\n"; }
    auto single=neural_command({"--load","examples/tracker.mind","--lesson",a,"--save",one},true);
    auto batch=neural_command({"--load","examples/tracker.mind","--lesson",a,"--batch-bench","47","--decisions","37"},true);
    check(same_bytes("examples/tracker.mind",one) && single.find("probe_matches=5/5")!=std::string::npos
          && metric(report_line(batch,"repeat=1 "),"correct")==47*37,"projecting the temporal graph changed a learned decision or state");
}

void food_actor(const std::filesystem::path &dir) {
    auto full=(dir/"flavour-full.mind").string(),half=(dir/"flavour-part.mind").string(),again=(dir/"flavour-again.mind").string();
    auto all=(dir/"food-all.lesson").string(),start=(dir/"food-a.lesson").string(),end=(dir/"food-b.lesson").string();
    auto frozen=(dir/"food-probe.lesson").string();
    {
        std::ofstream a(all),b(start),c(end),q(frozen);
        for (Word t=0;t<375;++t) {
            Word cue=(1u<<(t&15))|(t%3==0 ? 1u<<(16+((t/3)&15)) : 0);
            std::string args=std::to_string(cue)+" 0 "+std::to_string(1u<<(t&3))+" 228";
            std::string event=t%5==0 ? "taste "+std::to_string(t&3)+" "+std::to_string((t+1)&3) :
                              t%7==0 ? "seek "+args+" 108" : "sample "+args+(t&1 ? " 108" : "");
            a<<event<<'\n'; (t<193 ? b : c)<<event<<'\n';
            if (!(t%7)) q<<"seek "<<args<<" 108\n";
        }
    }
    for (const auto &source:{"examples/flavours.eco","examples/prospector.eco"}) {
        neural_command({"--from",source,"--lesson",all,"--save",full},true);
        neural_command({"--from",source,"--lesson",start,"--save",half},true);
        neural_command({"--load",half,"--lesson",end,"--save",again},true);
        check(same_bytes(full,again),"taste credit or neural state changed across an external checkpoint");
        neural_command({"--load",full,"--lesson",frozen,"--save",again},true);
        check(same_bytes(full,again),"a counterfactual homeostatic probe modified the actor");
    }
    { std::ofstream q(frozen); q<<"seek 128 0 1 228 228\nseek 128 0 1 27 27\n"; }
    auto outputs=neural_command({"--load",full,"--lesson",frozen,"--save",again},true);
    check(metric(report_line(outputs,"seek=0 "),"motor")==metric(report_line(outputs,"seek=1 "),"motor") &&
          same_bytes(full,again),"hidden plant chemistry reached the animal before its action");
    // This published Scout has only tried three grips on this object.
    { std::ofstream q(frozen); for (unsigned need:{1,2,4,8}) q<<"seek 0x4000000 0 "<<need<<" 0x78 0x78\n"; }
    auto saved=neural_command({"--load","examples/scout.mind","--lesson",frozen,"--save",again},true);
    check(saved.find("probe_matches=4/4")!=std::string::npos && same_bytes("examples/scout.mind",again),
          "the saved Scout lost its choice for a previously untried grip");
    bool rejected=false;
    try { neural_command({"--from","examples/repertoire.eco","--lesson",start},true); }
    catch (const std::invalid_argument &) { rejected=true; }
    check(rejected,"a sensory interface accepted a missing organ");
}

void external_actor(const std::filesystem::path &dir) {
    std::string original = (dir/"original.mind").string(), full = (dir/"whole.mind").string();
    std::string part = (dir/"split.mind").string(), resumed = (dir/"resumed.mind").string();
    std::string empty = (dir/"nothing.lesson").string(), probes = (dir/"probes.lesson").string();
    std::string entire = (dir/"entire.lesson").string(), early = (dir/"early.lesson").string(), late = (dir/"late.lesson").string();
    std::ofstream{empty};
    { std::ofstream out(probes); out << "probe 0xffffffff 0x01070000\nprobe 0 0 0 0\n"; }
    {
        std::ofstream all(entire), left(early), right(late);
        for (Word i = 0; i < 347; ++i) {
            std::string op = i%7 == 0 ? "probe " : (i%3 ? "trial " : "step ");
            Word cue=mix(i);
            if ((i&3)==0) cue=1u<<(cue&31);
            if ((i&3)==1) cue=(1u<<(cue&15))|(1u<<(16+((cue>>9)&15)));
            if (!(i%31)) cue=0;
            std::string tokens = op + std::to_string(cue) + " " + std::to_string(mix(i+37));
            tokens += op == "step " ? (i&1 ? " +" : " -") : " 11 8";
            all << tokens << '\n'; (i < 149 ? left : right) << tokens << '\n';
        }
    }
    for (const auto &world : std::vector<std::string>{"examples/learner.eco","examples/delayed.eco","examples/relations.eco",
            "examples/pathways.eco","examples/contexts.eco","examples/repertoire.eco",(dir/"polarized.eco").string()}) {
        neural_command({"--from",world,"--lesson",empty,"--save",original},true);
        neural_command({"--load",original,"--lesson",probes,"--save",resumed},true);
        check(same_bytes(original,resumed), "an external frozen probe changed its brain or clock");
        neural_command({"--load",original,"--lesson",entire,"--save",full},true);
        neural_command({"--load",original,"--lesson",early,"--save",part},true);
        neural_command({"--load",part,"--lesson",late,"--save",resumed},true);
        check(same_bytes(full,resumed), "external sensory stream diverged across batching or save/resume");
        { std::ofstream invalid(late,std::ios::app); invalid << "trial 0 0 1 2\n"; }
        bool fail = false;
        try { neural_command({"--load",resumed,"--lesson",late,"--save",resumed},true); }
        catch (const std::invalid_argument &) { fail = true; }
        check(fail && same_bytes(full,resumed), "failed external lesson modified its saved state");
        auto size = std::filesystem::file_size(late);
        std::filesystem::resize_file(late,size-std::string("trial 0 0 1 2\n").size());
    }
    neural_command({"--resident",(dir/"polarized.eco").string(),"--lesson",empty,"--save",original},true);
    neural_command({"--load",original,"--lesson",entire,"--save",full},true);
    neural_command({"--load",original,"--lesson",early,"--save",part},true);
    neural_command({"--load",part,"--lesson",late,"--save",resumed},true);
    check(same_bytes(full,resumed),"an experienced resident diverged after an external checkpoint");
    auto demonstration = neural_command({"--from","examples/learner.eco","--lesson","examples/grasps.lesson","--save",full},true);
    check(demonstration.find("probe_matches=8/8") != std::string::npos && std::filesystem::file_size(full) < 512, "deployed learner failed the new texture combinations");
    neural_command({"--from","examples/relations.eco","--lesson","examples/relations.lesson","--save",full},true);
    check(same_bytes(full,"examples/relations.mind"), "external learned relational state is not reproducible");
    std::fstream corrupt(full,std::ios::binary|std::ios::in|std::ios::out);
    corrupt.seekg(-1,std::ios::end); char ch=0; corrupt.read(&ch,1); ch ^= 1;
    corrupt.seekp(-1,std::ios::end); corrupt.write(&ch,1); corrupt.close();
    bool fail = false;
    try { neural_command({"--load",full,"--lesson",empty},true); } catch (const std::runtime_error &) { fail = true; }
    check(fail, "damaged neural actor accepted");
}

void gate_constraints() {
    // Enumerate local supports independently of the planner, including tables
    // that span several machine words and a fully specified impossible output.
    for (int arity = 0; arity <= 8; ++arity) {
        LearnedGate gate{};
        gate.arity = arity;
        for (int i = 0; i < arity; ++i) gate.parents |= 1u << (3*i + 1);
        for (int k = 0; k < 8; ++k) gate.rows[k] = mix(701 + k + arity);
        for (Word seed = 0; seed < 512; ++seed) {
            Word known = mix(seed + 37) & gate.parents, on = known & mix(seed);
            Clamps in{known ^ on, on}, out{};
            if (seed % 3) out.add(1, seed & 1);
            auto r = gate.filter(in, out, 0, 0);
            Word can_on = 0, can_off = 0, possible = 0;
            for (int x = 0; x < (1 << arity); ++x) {
                Word bits = 0;
                for (int i = 0; i < arity; ++i) if ((x >> i) & 1) bits |= 1u << (3*i+1);
                Word y = (gate.rows[x >> 5] >> (x & 31)) & 1;
                if ((bits & known) != on || (out.on && !y) || (out.off && y)) continue;
                possible |= 1u << y; can_on |= bits; can_off |= ~bits;
            }
            check(!(can_on & r.input.off) && !(can_off & r.input.on), "table pruned a feasible input");
            check(!(possible & 1 && r.output.on) && !(possible & 2 && r.output.off), "table pruned a feasible output");
            if (!possible) check(((r.output.off | out.off) & (r.output.on | out.on)) != 0, "missing contradiction");
        }
    }
}

void nonlinear_goals_gpu() {
    Course c{Task::workshop, 83199, 0, 0};
    c.lesson = 4078; c.chain = 5; c.difficult = 3; c.experiments = Course::Experiments::paired;
    auto m = hatch(12, 64, 992, c);
    auto gpu = make_gpu(m, Layout::automatic);
    gpu->run(c, 0, 8192, Learning::full);
    gpu->download(m);
    c.novel = 1; c.seed ^= 0xe62f3a51u; c.task = Task::invention;
    c.plan_nodes = 0;
    auto local = wins(gpu->run(c, 0, 16, Learning::frozen));
    c.plan_nodes = 4096;
    auto solved = gpu->run(c, 0, 16, Learning::frozen);
    check(wins(solved) >= 175 && local < 20, "learned nonlinear constraints failed new goals");
    auto again = m; gpu->download(again);
    check(identical(again, m), "planning changed the stored mechanism");
    same_counts(solved, make_cpu(m, 3)->run(c, 0, 16, Learning::frozen));
}

void lesson_parity() {
    for (auto width : {64u, 128u, 256u, 2048u, 4096u, 8192u, 1048576u}) {
        auto m = hatch(width >= 8192 ? 1 : 5, width, 31, {Task::pattern, 127, 0, 0});
        for (auto layout : {Layout::automatic, Layout::block}) {
            if (layout == Layout::block && width > 128) continue;
            if (width <= 4096) resize_archive(m, 128);
            auto cpu = make_cpu(m, 2), gpu = make_gpu(m, layout);
            auto c = m.course;
            Word count = width >= 8192 ? 3 : 37;
            auto compare = [&](Learning mode, Word n) {
                same_counts(cpu->run(c, 4, n, mode), gpu->run(c, 4, n, mode));
                auto left = m, right = m;
                cpu->download(left);
                gpu->download(right);
                check(identical(left, right), "different learned memories");
            };
            compare(Learning::full, count);
            c.recall = Recall::content;
            compare(Learning::motor, 2);
            c.task = Task::detector;
            compare(Learning::full, 8);
            c.novel = 1;
            compare(Learning::frozen, 1);
            c.novel = 0;
            c.task = Task::errand;
            c.recall = Recall::associative;
            compare(Learning::no_scaling, count);
            c.survey_steps = 8;
            compare(Learning::full, 2);
            c.novel = 1;
            compare(Learning::frozen, 3);
            c.task = Task::workshop;
            c.novel = 0;
            c.difficult = 2;
            compare(Learning::full, width <= 128 ? 96 : 3);
            c.difficult = 3;
            c.experiments = Course::Experiments::curious;
            compare(Learning::full, width == 64 ? 2048 : 16);
            c.novel = 1;
            c.task = Task::invention;
            compare(Learning::frozen, 3);
            cpu->restore(m);
            gpu->restore(m);
            compare(Learning::frozen, 1);
        }
    }
}

void parity() {
    for (Word width = 64; width <= (1u << 20); width *= 2) {
        auto m = hatch(width > 8192 ? 1 : 5, width, 343, {Task::trail, 13068, 0, 0});
        if (width >= (1u << 18)) {
            // Force sums past INT32_MAX with both signs on the two value heads.
            for (auto &w : m.cells) {
                w.sensory = ~0u;
                w.motor[0] = 2048;
                w.motor[1] = -2048;
            }
        }
        for (Layout layout : {Layout::warp, Layout::block, Layout::grid}) {
            if (layout == Layout::warp && width > 128) continue;
            if (layout == Layout::block && width > 4096) continue;
            if (layout == Layout::grid && width > 64 && width <= 4096) continue;
            auto gpu = make_gpu(m, layout);
            auto cpu = make_cpu(m, 3);
            for (auto mode : {Learning::full, Learning::motor, Learning::no_scaling, Learning::frozen}) {
                Course c = m.course;
                c.reversed = mode == Learning::motor;
                if (mode == Learning::frozen) c.task = Task::mixed;
                Word count = width < 256 ? 37 : (width > 8192 ? 2 : 7);
                same_counts(cpu->run(c, 194, count, mode), gpu->run(c, 194, count, mode));
                auto left = m, right = m;
                cpu->download(left);
                gpu->download(right);
                check(identical(left, right), "GPU synapses differ from CPU");
            }
        }
    }
}

} // namespace
int main(int argc, char **argv) {
    const char *tmp = std::getenv("TMPDIR");
    std::string base = (tmp ? tmp : "/var/tmp") + std::string("/axion-test.XXXXXX");
    if (!::mkdtemp(base.data())) return 1;
    int result = 0;
    try {
        relational_fibres();
        causal_contacts();
        predictive_relations();
        neuron();
        gate_constraints();
        worlds();
        behavior();
        persistence(base);
        selection();
        lessons(base);
        workshop(base);
        controlled_experiments(base);
        observation();
        consolidation(base);
        std::cout << "PASS generative transfer, causal discovery, inverse construction and sensory teaching stream\n";
        std::cout << "PASS vocabulary splits, planning, ordered instructions, external tutoring, memory persistence\n";
        std::cout << "PASS neurons, worlds, conditioning, reversal, associative consolidation, checkpoints and selection\n";
        if (argc == 2 && std::string_view(argv[1]) == "--gpu") {
            ecosystems(base);
            grove_learning(base);
            returning_memory(base);
            food_ecology(base);
            inheritance_and_dispersal(base);
            food_actor(base);
            navigation(base);
            temporal_memory(base);
            external_actor(base);
            std::cout << "PASS evolved contingent learning, delayed credit, held-out objects, nursery and deployable neural actors\n";
            nonlinear_goals_gpu();
            std::cout << "PASS CUDA ecology, genotype interventions, conservation, exact resume and learned nonlinear goals\n";
            parity();
            lesson_parity();
            std::cout << "PASS CPU/GPU spike counts, outcomes and every synapse across all widths and kernels\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        result = 1;
    }
    std::filesystem::remove_all(base);
    return result;
}
