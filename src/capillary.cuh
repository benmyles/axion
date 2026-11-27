#include "axion/capillary_course.hpp"

namespace axion::life::capillary {
KERNEL void capillary_learn(Body body, Course c, Word n, const Work *natural, Tissue natal, History history,
                            Output *outputs, Step *trace) {
    Word i = tid();
    if (i >= n)
        return;
    Word pair = i >> 1, rock = c.rock ^ ((i & 1) << 4);
    auto trials = develop(body, c, mix(c.seed ^ mix(pair * 1187 + 3631)), rock, natal,
                          trace && i < 2 ? trace + Count(i) * (c.views + c.probes) : nullptr, history);
    auto w = natural ? natural[pair] : channel(mix(c.seed ^ mix(pair * 3517 + 77893)), c.depth, c.scarce);
    outputs[i] = assess(body, c, trials, w);
}
KERNEL void capillary_release(Course c, Word n, const Work *natural, const Output *proposals, Result *scores) {
    Word i = tid();
    if (i >= n)
        return;
    Word pair = i >> 1;
    auto w = natural ? natural[pair] : channel(mix(c.seed ^ mix(pair * 3517 + 77893)), c.depth, c.scarce);
    scores[i] = open_nets(w, c.rock ^ ((i & 1) << 4), proposals[i], c.arm);
}
} // namespace axion::life::capillary

namespace axion::life::capillary {
KERNEL void capillary_population(const Body *bodies, const Mind *priors, const History *histories, Word animals,
                                 Course c, Word lives, const Work *natural, CohortGrade *scores) {
    Word id = tid() / 32, lane = tid() & 31;
    if (id >= animals)
        return;
    Body body = bodies[id];
    body.pressure_xor = c.regional ? body.module_mask : 0;
    CohortGrade score{};
    for (Word trial = lane; trial < lives; trial += 32) {
        auto w = natural ? natural[trial] : channel(mix(c.seed ^ mix(trial * 3517 + 77893)), c.depth, c.scarce);
        Word wins = 0;
        for (Word m = 0; m < 2; ++m) {
            Word material = c.rock ^ (m << 4);
            auto trained = develop(body, c, mix(c.seed ^ mix(trial * 1187 + 3631)), material,
                                   c.empty_contacts ? Tissue{} : priors[id].tissue, nullptr,
                                   histories && !c.empty_contacts ? histories[id] : History{});
            auto f = read_material(c.arm == erased ? Tissue{} : trained.tissue,
                                   c.arm == erased ? History{} : trained.history, body, c.adaptive, c.arm != lookup);
            auto p = design(w, f.model, channels(body.dendrites), 63, c.moves);
            Count output = release(w, p.motions, material);
            wins += p.ready && target(w, {output, output}) == 1;
            score.opens += p.ready;
            bool sound = true;
            for (Word r = 0; r < states && sound; ++r)
                sound = predict(f.model, r & channels(body.dendrites)) == Word(surge(r, material));
            score.strong += sound;
            score.disagreed += trained.informative;
        }
        score.caught += wins;
        score.first += wins == 2;
    }
    auto reduce = [](Word v) {
        Word out;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(out) : "r"(v));
        return out;
    };
    score = {reduce(score.first), reduce(score.caught), reduce(score.opens), reduce(score.strong),
             reduce(score.disagreed)};
    if (!lane)
        scores[id] = score;
}
KERNEL void capillary_graduate(Body body, Course c, Tissue natal, History history, Lessons *out) {
    *out = develop(body, c, mix(c.seed ^ 0xe39a127du), c.rock, natal, nullptr, history);
}
} // namespace axion::life::capillary

#include "axion/capillary_tutor.hpp"
namespace axion::life::capillary {
KERNEL void capillary_speech(Saved *pupil, Body body, Course c, const Gloss *teacher, Word examples, Word purse,
                             SpeechRecord *paid) {
    SpeechRecord bill{};
    auto &mouth = pupil->speech;
    // A tutor can point to the six fixtures, four pigments and two positions
    // of the pressure indicator. The nominal afferents are shared at test.
    for (Word pass = 0; pass < 4; ++pass)
        for (Word atom = 0; atom < atoms; ++atom) {
            if (purse - bill.heat < 2)
                continue;
            Word tint = (atom - 4) & 3;
            auto fixture = cell(atom == 9 ? 9 : 16);
            Word contact = indicated(fixture, atom, tint, (tint + 1) & 3, actual(fixture, 0, c.rock));
            bill.pointed += ground(mouth, body, contact, teacher->sound[atom], c.gate, bill.spikes);
            bill.heat += 2;
        }
    auto model = read_material(pupil->mind.tissue, pupil->history, body, c.adaptive).model;
    for (Word t = 0; t < examples; ++t) {
        Word key = mix(c.seed ^ mix(t * 7369 + 710761));
        auto scene = display(key, c.rock, false);
        for (Word j = 0; j < kinds; ++j) {
            auto task = challenge(*teacher, scene, c.rock, j, mix(key + j * 9721));

            auto heard = acoustic(*teacher, task.response), q = acoustic(*teacher, task.question);
            Word cost = 2 * (heard.n + q.n) + 2;
            if (cost > purse - bill.heat)
                continue;
            bill.heat += cost;
            bill.examples += associate(mouth, body, scene, model, q, heard, c.gate, bill.spikes, bill.edits);
        }
    }
    bill.terminal = body.cells.spike;
    *paid = bill;
}
KERNEL void capillary_converse(Saved *speaker, Body body, Course c, const Gloss *standard, Word n, Word mask,
                               SpokenCase *out) {
    Word i = tid();
    if (i >= n)
        return;
    Word q = i % kinds, layout = i / kinds, twin = layout & 1, rock = c.rock ^ (twin << 4);
    Word key = mix(c.seed ^ mix((layout >> 1) * 13477 + 91577));
    auto scene = display(key, rock, true);
    auto looked = develop(body, c, mix(c.seed ^ mix((layout >> 1) * 11987 + 38321)), rock);
    if (mask == 1)
        looked = develop(body, c, mix(c.seed ^ mix((layout >> 1) * 11987 + 38321)), rock ^ 16);
    auto model = read_material(looked.tissue, looked.history, body, c.adaptive, c.arm != lookup).model;
    if (c.arm == erased)
        model = {};
    auto task = challenge(*standard, scene, rock, q, mix(key + q * 15373), Word(mask == 2 && q == 0));
    auto prompt = acoustic(*standard, task.question);
    if (mask == 4)
        for (Word j = 0; j < prompt.n / 2; ++j) {
            auto old = prompt.wave[j];
            prompt.wave[j] = prompt.wave[prompt.n - 1 - j];
            prompt.wave[prompt.n - 1 - j] = old;
        }
    auto words = say(speaker->speech, body, scene, model, prompt);
    if (!body.eyes || mask == 3 || mask == 5)
        words.voice = {};
    auto expected = acoustic(*standard, task.response);
    Word pegs = 0;
    for (Word j = 0; j < grips; ++j)
        pegs |= Word(scene.work.handle[j].material) << (3 * j);
    out[i] = {message(*standard, expected),
              message(*standard, words.voice),
              looked.tissue.on,
              looked.tissue.off,
              words.recognized,
              words.voice.n,
              words.findings.certain,
              task.cause,
              scene.pigment,
              scene.pad,
              scene.earlier,
              scene.later,
              scene.changed,
              task.named,
              task.desired,
              q,
              twin,
              rock,
              address(scene.work.feed, scene.work.bed, 18),
              pegs};
}

KERNEL void capillary_meeting(Saved *speaker, Saved *listener, Body adult, Body youth, Course c, const Gloss *standard,
                              Word n, Word hearing, AdviceCase *receipt) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = i >> 1, rock = c.rock ^ ((i & 1) << 4);
    Word seed = mix(c.seed ^ mix(k * 2647 + 8713));
    auto education = develop(adult, c, seed, rock ^ (hearing == 1 ? 16 : 0));
    auto passive = c;
    passive.arm = capillary::passive;
    auto watches = develop(youth, passive, seed, rock);
    Word key = mix(c.seed ^ mix(k * 7829 + 59393));
    auto hint = display(key, rock, true);
    // The public apparatus has one opaque fork with two simultaneous material
    // changes. These physical endpoints are identical in the two substrates.
    hint.work = cell(1 | 8 | 16);
    hint.changed = 9;
    hint.earlier = actual(hint.work, 0, rock);
    hint.later = actual(hint.work, 9, rock);
    auto request = challenge(*standard, hint, rock, 0, key);
    auto q = acoustic(*standard, request.question);
    auto spoken = say(speaker->speech, adult, hint,
                      read_material(education.tissue, education.history, adult, c.adaptive).model, q);
    if (hearing == 4)
        for (Word j = 0; j < spoken.voice.n / 2; ++j) {
            auto old = spoken.voice.wave[j];
            spoken.voice.wave[j] = spoken.voice.wave[spoken.voice.n - 1 - j];
            spoken.voice.wave[spoken.voice.n - 1 - j] = old;
        }
    if (hearing == 2)
        for (Word j = 0; j < spoken.voice.n; ++j) {
            auto &wave = spoken.voice.wave[j];
            if (wave == standard->sound[0])
                wave = standard->sound[3];
            else if (wave == standard->sound[3])
                wave = standard->sound[0];
        }
    if (hearing == 3 || hearing == 5)
        spoken.voice = {};
    auto advice = hear(listener->speech, youth, watches.tissue, hint, q, spoken.voice);
    auto goal = channel(mix(key + 811), c.depth, c.scarce);
    auto pick = [&](Model m) { return design(goal, m, channels(youth.dendrites), 63, c.moves); };
    auto base = pick(fit(watches.tissue, youth.dendrites).model), told = pick(advice.belief);
    auto physical = [&](Plan p, Word r) { return Word(p.ready && actual(goal, p.motions, r)); };
    receipt[i] = {advice.belief.on,
                  advice.belief.off,
                  message(*standard, spoken.voice),
                  physical(base, rock),
                  physical(told, rock),
                  physical(told, rock ^ 16),
                  advice.retained,
                  advice.considered,
                  base.motions,
                  told.motions,
                  Word(told.ready && !physical(told, rock)),
                  spoken.voice.n};
}
} // namespace axion::life::capillary

#include "axion/capillary_play.hpp"
#include "compound_conversation.cuh"
namespace axion::life::capillary {
KERNEL void capillary_table(Workshop *shared, Saved *person, Body *bodies, Request q, Reply *output) {
    auto &t = *shared;
    Reply r{};
    auto &m = person[q.who];
    auto &body = bodies[q.who];
    Word who = q.who, peer = 1 - who;
    auto budget = q.amount[who];
    auto scene = experience(t, who, m.mind);
    auto private_law = [&]() {
        return bool(m.adaptive & 1) ? revise(m.mind.tissue, m.history, body).fit.model
                                    : fit(m.mind.tissue, body.dendrites).model;
    };
    Model physical_model = private_law();
    Model belief = t.advised[who] ? t.received[who] : physical_model;
    if(audible_workshop(t,bodies,q,r)){}
    else if (q.action == set_cell || q.action == set_rig) {
        Work w = q.action == set_cell ? cell(q.a) : channel(mix(q.a), q.b, q.seed);
        setup(t, w, q.e, q.c, q.d);
    } else if(q.action==attach_second){
        if(!t.ready||!t.seal||t.second_seal||!t.drops||!body.limbs||budget<2)r.status=1;
        else{t.second_seal=1;t.second_row=q.a;t.second_rock=q.b;--t.drops;++r.heat[who];}
    } else if(q.action==prime_pair){
        if(!t.ready||!t.seal||!t.second_seal||!body.limbs||budget<5)r.status=1;
        else{
            auto work=current_work(t);Word rows[2]{address(work.feed,work.bed,18),t.second_row};
            for(Word port=0;port<2;++port){
                press_port(t,port,rows[port]);press_port(t,port,9);
            }
            r.heat[who]=4;
        }
    } else if(q.action==signal_quadrant){
        if(!t.ready||!t.seal||!t.second_seal||t.feeder_stock<(q.c?4u:2u)||!body.eyes||!bodies[peer].eyes||!body.limbs||!bodies[peer].limbs||!body.mouth||!bodies[peer].mouth||budget<4||q.amount[peer]<4||budget>=(q.c?65534u:65535u)||q.amount[peer]>=65535)r.status=1;
        else{
            auto work=current_work(t);Word rows[2]{address(work.feed,work.bed,18),t.second_row};
            Word optics=projector(sealed_port(t,0,rows[0]),false)|(projector(sealed_port(t,1,rows[1]),false)<<2);
            auto writer=compound_body(body,0,q.compound[who]),reader=compound_body(bodies[peer],0,q.compound[peer]);
            if(!(t.shutters&(1u<<who)))writer.body.eyes=0;
            if(!(t.shutters&(1u<<peer)))reader.body.eyes=0;
            auto v=compound_turn(writer,reader,optics,q.a,q.seed,true);
            Word visible=0;
            for(Word port=0;port<2;++port){
                if(v.mark&(1u<<port))press_port(t,port,rows[port]);
                visible|=press_port(t,port,rows[port])<<port;
            }
            if(visible!=v.sensed)r.status=3;
            Word offered=q.c?nutrient_bed(q.a,bool(t.material&1)):(1u<<(2*q.a));
            r.crop=take_crop(offered,v.choice);
            if(q.b){r.edits+=nourish_ports(writer,q.a,v.mark,taste_ports(writer.body,r.crop,q.c));r.edits+=nourish_ports(reader,v.sensed,v.choice,taste_ports(reader.body,r.crop,q.c));}
            r.compound[who]=writer.tissue;r.compound[peer]=reader.tissue;r.contact=3;
            r.new_spike[who]=writer.body.cells.spike;r.new_spike[peer]=reader.body.cells.spike;
            r.marked=v.mark;r.lamp=visible;r.feeder=v.choice;r.erased_choice=v.blank;r.missing_first=v.without_first;r.missing_second=v.without_second;
            r.share=__builtin_popcount(r.crop);r.heat[who]=__builtin_popcount(v.mark);r.heat[peer]=3;
            t.feeder_stock-=2*r.share;t.shared_food+=2*r.share;
        }
    } else if (q.action == signal_peer) {
        if (!t.ready || !t.seal || t.feeder_stock < 2 || !body.eyes || !bodies[peer].eyes || !body.limbs ||
            !bodies[peer].limbs || !body.mouth || !bodies[peer].mouth || budget < 3 || q.amount[peer] < 3 ||
            budget >= 65535 || q.amount[peer] >= 65535)
            r.status = 1;
        else {
            auto w = current_work(t);
            Word row = address(w.feed, w.bed, 18), mobile = t.seal == 2 ? t.pad : t.source,
                 bound = t.seal == 2 ? t.source : t.pad;
            Junction junction{row, t.material, bound, mobile};
            auto writer = connect_signal(q.appetitive[who], body),
                 reader = connect_signal(q.appetitive[peer], bodies[peer]);
            if (!(t.shutters & (1u << who)))
                writer.body.eyes = 0;
            if (!(t.shutters & (1u << peer)))
                reader.body.eyes = 0;
            auto turn = encounter_signal(writer, reader, junction, q.a, q.seed, false, true);
            if (turn.write) {
                auto change = cycle(t.material, bound, mobile, row);
                if (change.flow)
                    t.seal = 3 - t.seal;
                bound = change.coating;
                mobile = change.carrier;
            }
            auto last = cycle(t.material, bound, mobile, row);
            if (last.flow)
                t.seal = 3 - t.seal;
            if (q.b) {
                r.edits += digest_signal(writer, q.a, turn.write, turn.food);
                r.edits += digest_signal(reader, turn.light, turn.choice, turn.food);
            }
            r.appetitive[who] = signal_tissue(writer);
            r.appetitive[peer] = signal_tissue(reader);
            r.new_spike[who] = writer.body.cells.spike;
            r.new_spike[peer] = reader.body.cells.spike;
            r.contact = 3;
            r.heat[who] = turn.write;
            r.heat[peer] = 2;
            r.share = turn.food;
            t.feeder_stock -= 2 * turn.food;
            t.shared_food += 2 * turn.food;
            r.marked = turn.write;
            r.lamp = turn.light;
            r.feeder = turn.choice;
            r.erased_choice = turn.erased;
            r.opposite_choice = turn.inverse;
        }
    } else if (q.action == seal_cell) {
        if (!t.ready || t.seal || t.net || !t.drops || !body.limbs || budget < 2 || t.layout.time != 1 ||
            t.layout.catcher != bit(18))
            r.status = 1;
        else {
            t.seal = 1;
            --t.drops;
            ++r.heat[who];
        }
    } else if (q.action == change_mineral) {
        if (!t.ready)
            r.status = 1;
        else
            t.material = q.a;
    } else if (q.action == recover_drops) {
        if (!t.ready || !body.limbs || q.a > t.settled || q.a >= budget)
            r.status = 1;
        else {
            Word remaining = q.a;
            for (Word flowed = 0; flowed < 2; ++flowed) {
                Word amount = remaining < t.rested[flowed] ? remaining : t.rested[flowed];
                t.rested[flowed] -= amount;
                t.outlet[flowed ? t.pad : t.source] -= amount;
                t.coating[flowed ? t.source : t.pad] -= amount;
                remaining -= amount;
            }
            t.settled -= q.a;
            t.drops += q.a;
            r.heat[who] = q.a;
        }
    } else if (q.action == recall_field) {
        t.ready = 0;
    } else if (q.action == set_context) {
        m.adaptive = q.a ? q.a : 2u;
        body.pressure_xor = m.adaptive == 3 ? body.module_mask : 0;
        r.saved = 1;
        t.advised[who] = 0;
    } else if (q.action == new_sample) {
        m.mind.tissue = {};
        m.history = {};
        t.advised[who] = 0;
        r.saved = 1;
    } else if (q.action == experiment) {
        if (!t.ready || !body.eyes || t.seal) {
            r.status = 1;
        } else {
            Course c{};
            c.seed = q.seed;
            c.views = q.a;
            c.probes = q.b;
            c.budget = budget ? budget - 1 : 0;
            Lessons acquisition{};
            Word salt = mix(q.seed ^ 0xe39a127du);
            if (bool(m.adaptive & 1)) {
                auto e = explore_context(body, c, salt, t.material, m.mind.tissue, m.history);
                acquisition = e.record;
                m.history = e.memory;
                r.rechecks = e.deliberate;
                r.written = e.anchors;
            } else
                acquisition = develop(body, c, salt, t.material, m.mind.tissue);
            m.mind.tissue = acquisition.tissue;
            r.heat[who] = acquisition.heat;
            r.edits = acquisition.edits;
            r.spikes = acquisition.spikes;
            r.observed = acquisition.looks;
            r.terminal = acquisition.terminal;
            r.saved = 1;
            t.advised[who] = 0;
        }
    } else if (q.action == open_eye) {
        t.shutters = (t.shutters & ~(1u << who)) | (q.a << who);
    } else if (q.action == set_net) {
        if (t.ready && !t.seal && body.limbs && budget > 1) {
            t.net = 1 + who;
            r.heat[who]++;
        } else
            r.status = 1;
    } else if (q.action == move_handles || q.action == design_work) {
        if (!t.ready || !body.limbs)
            r.status = 1;
        else {
            Word desired = t.current ^ q.a;
            bool act = true;
            if (q.action == design_work) {
                auto base = current_work(t);
                auto result = design(base, belief, channels(body.dendrites), 63, q.a);
                desired = t.current ^ result.motions;
                r.intended = desired;
                r.ready = result.ready;
                r.possible = result.alternatives;
                act = r.ready;
            }
            Word cost = __builtin_popcount(desired ^ t.current);
            if (act) {
                auto base = current_work(t);
                Word delta = desired ^ t.current;
                auto moves = order(base, delta);
                if (cost >= budget || !place(t, desired))
                    r.status = 1;
                else {
                    r.heat[who] += cost;
                    t.handled[who] = feel(base, moves);
                    t.voluntary[who] ^= delta;
                }
            }
        }
    } else if (q.action == pulse || q.action == probe_cell) {
        if (q.action == probe_cell && t.ready && (t.drops || t.seal) && budget > 2) {
            if (t.layout.time != 1 || t.layout.catcher != bit(18) || !body.limbs || !body.eyes ||
                !(t.shutters & (1u << who)))
                r.status = 1;
            else {
                auto w = current_work(t);
                Word previous = address(w.feed, w.bed, 18);
                Revision candidate{};
                if (bool(m.adaptive & 1))
                    candidate = revise(m.mind.tissue, m.history, body);
                else
                    candidate.fit = fit(m.mind.tissue, body.dendrites);
                auto choice =
                    recheck(m.mind.tissue, candidate, previous, mix(q.seed), body, budget - 1, bool(m.adaptive & 1));
                Word delta = 0;
                for (Word j = 0; j < w.n; ++j)
                    if ((previous ^ choice.choice) & (1u << ((w.handle[j].material + 2) % 6)))
                        delta |= 1u << j;
                if (!choice.spent || !place(t, t.current ^ delta))
                    r.status = 1;
                else {
                    r.heat[who] = __builtin_popcount(delta);
                    t.handled[who] = feel(w, order(w, delta));
                    t.voluntary[who] ^= delta;
                    r.rechecks =
                        candidate.review && bool(candidate.diagnostic & bit(choice.choice & channels(body.dendrites)));
                    r.intended = choice.choice;
                    r.ready = 1;
                }
            }
        }
        if (!t.ready || (!t.drops && !t.seal) || budget < (q.action == probe_cell ? 3u : 2u) || r.status)
            r.status = 1;
        else {
            ++r.heat[who];
            Word mobile = t.seal == 2 ? t.pad : t.source, bound = t.seal == 2 ? t.source : t.pad;
            if (t.seal) {
                auto w = current_work(t);
                r.pump_row = address(w.feed, w.bed, 18);
                auto next = cycle(t.material, bound, mobile, r.pump_row);
                r.source_before = mobile;
                r.bound_before = bound;
                r.physical = next.flow;
                r.outgoing = next.carrier;
                if (next.flow)
                    t.seal = 3 - t.seal;
            } else {
                --t.drops;
                r.physical = actual(t.layout, t.current, t.material);
                r.outgoing = r.physical ? t.pad : t.source;
                ++t.outlet[r.outgoing];
                ++t.coating[r.physical ? t.source : t.pad];
                if (t.net && r.physical && q.amount[t.net - 1] < 65535) {
                    r.gained = t.net;
                    ++t.eaten;
                } else {
                    ++t.settled;
                    ++t.rested[r.physical];
                }
            }
            t.net = 0;
            for (Word j = 0; j < 2; ++j)
                if (bodies[j].eyes && (t.shutters & (1u << j)) && q.amount[j] > r.heat[j] + 1) {
                    auto &v = t.seen[j];
                    if ((person[j].adaptive || (q.action == probe_cell && j == who)) && t.layout.time == 1 &&
                        t.layout.catcher == bit(18)) {
                        auto &self = person[j];
                        auto &sense = bodies[j];
                        auto w = current_work(t);
                        Word sensed = address(w.feed, w.bed, 18);
                        bool acted =
                            bool(t.voluntary[j] & (v.current ^ t.current)) || (q.action == probe_cell && j == who);
                        auto candidates = acted && bool(self.adaptive & 1)
                                              ? revise(self.mind.tissue, self.history, sense)
                                              : Revision{};
                        Word permission = 0;
                        r.edits +=
                            material_receive(self.mind.tissue, sense, sensed, r.physical, r.spikes, 1, &permission);
                        Word addr = sensed & channels(sense.dendrites);
                        if (bool(self.adaptive & 1))
                            remember(self.history, addr, r.physical, acted, candidates.diagnostic, permission);
                        r.contact |= 1u << j;
                        r.new_spike[j] = sense.cells.spike;
                        t.advised[j] = 0;
                        if (j == who) {
                            r.stimulus = sensed;
                            r.written = acted && bool(candidates.diagnostic & bit(addr)) &&
                                        bool(permission & (r.physical ? 1 : 2));
                        }
                    }
                    Word frame = mobile | (bound << 2);
                    v.first = v.optical_frame == frame ? v.last : absent;
                    v.optical_frame = frame;
                    v.last = r.physical;
                    v.previous = v.current;
                    v.current = t.current;
                    ++r.heat[j];
                    ++r.observed;
                }
            t.voluntary[0] = t.voluntary[1] = 0;
        }
    } else if (q.action == follow_words) {
        auto moves = t.pending[who];
        if (!t.ready || !moves.size() || !body.limbs || moves.size() >= budget)
            r.status = 1;
        else {
            auto base = current_work(t);
            auto receipt = follow(base, moves);
            t.pending[who] = {};
            t.current ^= receipt.position;
            t.voluntary[who] ^= receipt.position;
            for (Word k = 0; k < 5; ++k)
                t.movable[k] = portions(receipt.supply, k);
            r.ready = !receipt.denied;
            r.intended = receipt.position;
            r.heat[who] = moves.size();
            r.accepted = receipt.moved;
            r.considered = moves.size();
        }
    } else if (q.action == caption_grips) {
        utterance::Growth change{};
        change.cells = body.cells;
        Word cost = 1 + q.question.n + q.statement.n;
        if (!t.ready || cost >= budget ||
            !practise(m.speech.manual, body, t.handled[who], q.question, q.statement, 1, change))
            r.status = 1;
        else {
            r.heat[who] = cost;
            r.saved = 1;
            r.spikes = change.firings;
            r.terminal = change.cells.spike;
            r.edits = change.contacts;
        }
    } else if (q.action == acquire_name) {
        if (!t.ready || !q.statement.n || budget < 3 || !(t.shutters & (1u << who)))
            r.status = 1;
        else {
            bool visible = indicated(t.layout, q.a, t.source, t.pad, t.seen[who].last) == q.a;
            if (!visible)
                r.status = 1;
            else {
                ground(m.speech, body, q.a, q.statement.wave[0], 1, r.spikes);
                r.heat[who] = 2;
                r.terminal = body.cells.spike;
                r.saved = 1;
            }
        }
    } else if (q.action == acquire_form) {
        Word cost = 2 + q.question.n + q.statement.n;
        if (scene.later == absent || cost >= budget ||
            !associate(m.speech, body, scene, belief, q.question, q.statement, 1, r.spikes, r.edits))
            r.status = 1;
        else {
            r.heat[who] = cost;
            r.terminal = body.cells.spike;
            r.saved = 1;
        }
    } else if (q.action == answer_question || q.action == send_answer) {
        if (cue(m.speech.manual, q.question)) {
            if (t.ready) {
                auto w = current_work(t);
                auto p = design(w, belief, channels(body.dendrites), 63, grips);
                auto sentence = instruct(m.speech.manual, body, q.question, feel(w, order(w, p.motions)));
                if (sentence.n < budget) {
                    r.words = sentence;
                    r.heat[who] = sentence.n;
                    r.ready = p.ready;
                    r.intended = p.motions;
                }
                if (q.action == send_answer) {
                    t.pending[peer] = {};
                    if (r.words.n && r.words.n < q.amount[peer]) {
                        auto read = understand(person[peer].speech.manual, q.question, r.words);
                        t.pending[peer] = read;
                        r.accepted = read.size();
                        r.heat[peer] = r.words.n;
                    }
                }
            }
        } else {
            auto message = say(m.speech, body, scene, belief, q.question);
            r.cause = message.findings.certain;
            r.possible = message.findings.possible_sets;
            if (!message.recognized)
                r.status = 2;
            else if (message.voice.n < budget) {
                r.words = message.voice;
                r.heat[who] = r.words.n;
                if (q.action == send_answer && r.words.n && q.amount[peer] > r.words.n) {
                    auto other_scene = experience(t, peer, person[peer].mind);
                    auto judgement = hear(person[peer].speech, bodies[peer], person[peer].mind.tissue, other_scene,
                                          q.question, r.words);
                    t.advised[peer] = judgement.interpreted;
                    t.received[peer] = judgement.belief;
                    r.heat[peer] = r.words.n;
                    r.considered = judgement.considered;
                    r.accepted = judgement.retained;
                    r.narrowed = r.accepted && r.accepted < r.considered;
                }
            }
        }
    }
    auto recent = experience(t, who, m.mind);
    r.tissue = m.mind.tissue;
    r.belief = t.advised[who] ? t.received[who] : private_law();
    r.adaptive = m.adaptive;
    r.history = m.history;
    r.threshold = 4 + ((body.motor >> 23) & 31);
    r.prior = recent.earlier;
    r.result = recent.later;
    r.change = recent.changed;
    r.current = t.current;
    r.stock = t.drops;
    r.eaten = t.eaten;
    r.incoming = t.seal == 2 ? t.pad : t.source;
    r.pad = t.seal == 2 ? t.source : t.pad;
    r.second_seal=t.second_seal;
    r.sealed = t.seal;
    r.geology = t.material;
    r.feeder_stock = t.feeder_stock;
    r.settled = t.settled;
    if (!conserved(t))
        r.status = 3;
    *output = r;
}
} // namespace axion::life::capillary

namespace axion::life::capillary {
KERNEL void capillary_plate(Work w, Word moves, Word rock, Plate *out) {
    auto changed = assemble(w, moves);
    auto left = w.feed, right = source(w, moves);
    Plate p{left, right, w.catcher, w.waste};
    for (Word k = 0; k < w.time; ++k) {
        left = physical(w.bed, left, rock);
        right = physical(changed, right, rock);
        p.before |= left;
        p.after |= right;
    }
    *out = p;
}
AX_FN Bounds direct(Model m, Word inputs, const Sheet &bed, Bounds past) {
    Bounds next{};
    auto la = from_west(past.on), ua = from_west(past.maybe), lb = from_north(past.on), ub = from_north(past.maybe);
    for (Word j = 0; j < 64; ++j) {
        Count pixel = bit(j);
        Word channel = texture(bed, j) << 2;
        bool all = true, possible = false;
        for (Word k = 0; k < 4; ++k) {
            if (!(((k & 1) ? ua : ~la) & ((k & 2) ? ub : ~lb) & pixel))
                continue;
            auto outcome = predict(m, (channel | k) & inputs);
            all &= outcome == 1;
            possible |= outcome != 0;
        }
        if (all)
            next.on |= pixel;
        if (possible)
            next.maybe |= pixel;
    }
    return next;
}
template <Word mode>
AX_FN void stream(Model learned, Word sensors, Sheet surface, Projection map, Die die, Word seed, Word n, Word steps,
                  Bounds *out) {
    Word i = tid();
    if (i >= n)
        return;
    Count start = scatter(mix(seed ^ i));
    Bounds state{start, start};
    auto inlet = [&](Word j) { return scatter(mix(seed ^ (i * 15331u + j * 4621u))) & (first_column | 0xffull); };
    if constexpr (mode == 2) {
        if ((learned.on | learned.off) == full) {
            for (Word j = 0; j < steps; ++j)
                start = propagate(die, start | inlet(j));
            out[i] = {start, start};
            return;
        }
    }
    for (Word j = 0; j < steps; ++j) {
        Count load = inlet(j);
        state.on |= load;
        state.maybe |= load;
        if constexpr (mode == 0)
            state = direct(learned, sensors, surface, state);
        else
            state = imagine(map, state);
    }
    out[i] = state;
}
#define AX_CAP_FLOW(name, kind)                                                                                        \
    KERNEL void name(Model learned, Word sensors, Sheet surface, Projection map, Die die, Word seed, Word n,           \
                     Word steps, Bounds *out) {                                                                        \
        stream<kind>(learned, sensors, surface, map, die, seed, n, steps, out);                                        \
    }
AX_CAP_FLOW(capillary_scalar, 0)
AX_CAP_FLOW(capillary_bounds, 1)
AX_CAP_FLOW(capillary_image, 2)
#undef AX_CAP_FLOW
} // namespace axion::life::capillary

namespace axion::life::capillary {
KERNEL void capillary_apprentice(Saved *pupil, Body body, Course c, const LanguageRack *rack, Word demos, Word purse,
                                 SpeechRecord *receipt) {
    SpeechRecord bill{};
    utterance::Growth change{};
    change.cells = body.cells;
    auto &manual = pupil->speech.manual;
    for (Word i = 0; i < demos; ++i) {
        bool pair = i % 3 == 2;
        Work w{};
        w.n = grips;
        Protocol seq{};
        Word slot = (i - i / 3) % 48, code = (slot * 29 + (c.seed % 48)) % 48;
        Word a = code % 6, b = (a + 1 + mix(i + c.seed) % 5) % 6;
        w.handle[a] = {Byte(8 + a), Byte((code / 6) & 3)};
        if (code / 24)
            surface(w.bed, w.handle[a].material) |= bit(8 + a);
        seq.append(a);
        if (pair) {
            Word v = mix(c.seed ^ mix(i * 9151 + 1451));
            w.handle[b] = {Byte(8 + b), Byte(v & 3)};
            if (v & 4)
                surface(w.bed, v & 3) |= bit(8 + b);
            seq.append(b);
        }
        auto result = perform(w, seq);
        if (result.denied)
            continue;
        auto sensation = feel(w, seq);
        auto sentence = label(*rack, sensation);
        Word heat = seq.n + rack->prompt.n + sentence.n + 1;
        if (heat > purse - bill.heat)
            break;
        bill.heat += heat;
        bill.examples += practise(manual, body, sensation, rack->prompt, sentence, c.gate, change);
    }
    bill.spikes = change.firings;
    bill.edits = change.contacts;
    bill.terminal = change.cells.spike;
    *receipt = bill;
}
KERNEL void capillary_instructions(Saved *adult, Saved *child, Body guide, Body pupil, Course c,
                                   const LanguageRack *rack, Word n, Word hearing, Instruction *out) {
    Word i = tid();
    if (i >= n)
        return;
    Word pair = i / 2, rock = c.rock ^ ((i & 1) << 4), salt = mix(c.seed ^ mix(pair * 9151 + 61931));
    auto school = develop(guide, c, salt, rock ^ (hearing == 1 ? 16 : 0));
    auto w = channel(mix(c.seed ^ mix(pair * 3517 + 77893)), c.depth, c.scarce);
    auto theory = read_material(school.tissue, school.history, guide, c.adaptive).model;
    auto plan = design(w, theory, channels(guide.dendrites), 63, c.moves);
    auto seq = order(w, plan.motions);
    auto desired = feel(w, seq);
    auto speech = instruct(adult->speech.manual, guide, rack->prompt, hearing == 4 ? feel(w, seq, true) : desired);
    if (hearing == 2)
        for (Word j = 0; j < speech.n; ++j) {
            auto &wave = speech.wave[j];
            if (wave == rack->substance[0])
                wave = rack->substance[3];
            else if (wave == rack->substance[3])
                wave = rack->substance[0];
        }
    if (hearing == 3 || hearing == 5 || !pupil.limbs || !pupil.eyes)
        speech = {};
    auto heard = understand(child->speech.manual, rack->prompt, speech);
    auto worked = follow(w, heard);
    // A silent rig can pass a drop with no labour. Grade this on the same release as any instructed build.
    auto earlier = actual(w, 0, rock), later = actual(w, worked.position, rock);
    Word possible = 0;
    for (Word m = 0; m < (1u << w.n) && !possible; ++m)
        if (Word(__builtin_popcount(m)) <= c.moves && stocked(w, m))
            possible = actual(w, m, rock);
    out[i] = {desired.bits,
              heard.bits,
              fingerprint(speech),
              plan.motions,
              plan.ready,
              seq.n,
              speech.n,
              worked.position,
              worked.denied,
              later,
              earlier,
              possible,
              Word(perform(w, seq, true).denied != 0)};
}
} // namespace axion::life::capillary
