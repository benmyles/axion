#include "axion/retina_study.hpp"
#include "axion/audience.hpp"
#include "axion/narrative_world.hpp"

#include "flood_sounding.cuh"
#include "flood_ink.cuh"

template <bool Stirred, bool Gaze, bool Public = false, bool Inked = false, bool Sampled = true, bool Echo = false>
AX_FN void daylight(World w, Configuration c, const Word *clock, Word offset, drift::Law law, retina::Ledger *books,
                    retina::GazeLedger *fixations, retina::StirPatch *patches, retina::StirSample *survey,
                    Word sample_count, Word rotating, audience::Ledger *visits = nullptr,
                    audience::Witness *witnesses = nullptr,utterance::Ledger *ink_books=nullptr,
                    utterance::Sample *ink_notes=nullptr,Word ink_count=0,utterance::Watch watch={}) {
    Word tick = *clock + offset, side = c.side, width = side / 2;
    Word shift = (law.ports & retina::cadence_mask) >> retina::cadence_shift;
    Word tile = (tid() << shift) | (tick & ((1u << shift) - 1));
    if (tile >= width * width)
        return;
    Word x = (2 * (tile % width) + ((tick >> 3) & 1)) & (side - 1);
    Word y = (2 * (tile / width) + ((tick >> 4) & 1)) & (side - 1);
    Word xx = (x + 1) & (side - 1), yy = (y + 1) & (side - 1);
    DriftSquare sites{y * side + x, y * side + xx, yy * side + xx, yy * side + x};
    Word rock = mix(c.seed ^ mix((x >> 3) + 19391 * (y >> 3)));
    Word phase = ((rock >> 13) + ((law.ports & drift::tide) ? tick >> law.tide_shift : 0)) & 3;
    auto view = drift_bed(w, sites, 0, phase, rock);
    Word key = mix(c.seed ^ mix(73129u * tick + tile) ^ 0xab1f03d9u);
    Word src = key & 3, owner = ~0u;
    bool demonstration = false;
    // Light scatters from moving water even when a sill retains its organic stock.
    // The glint carries no food. This launch follows every feeding commitment.
    if (!w.land[sites[src]].water)
        return;
    Word channel = (law.ports & retina::channel_mask) >> retina::shift;
    Word links = drift::connections(view, law.ports & drift::fibres, law.ports & drift::seepage);
    Word upper = 0;
    if (law.ports & drift::winding) {
        auto path = drift::woven_path(view, links, law.ports & drift::fibres, law.ports & drift::seepage);
        if (path.turns && (key & (1u << 20))) {
            upper = 2;
            links = path.upper;
        }
    }
    Word rest = 0;
    for (Word j = 0; j < 4; ++j)
        rest |= (w.braids.floats[sites[j]] & 3u) << (2 * j);
    Word dye = (key >> 10) & 3;
    Word outlet = drift::outlet(src, links);
    if constexpr (Stirred) {
        if (key & (1u << 8)) {
            if (!stir_water<Public>(w, c, sites, tick, tile, key, (retina::tide_lamp(phase, rock) + upper) & 3,
                                    law.ports, links, rest, dye, patches, survey, sample_count, rotating, src, outlet,
                                    owner, visits, witnesses))
                return;
            demonstration = true;
        }
    }
    Word alternate =
        (law.ports & drift::independent_glints) ? ((key >> 15) & 3u) : ((src + 1 + ((key >> 15) % 3)) & 3u);
    Word other = drift::outlet(alternate, links);
    auto photons = retina::reflected(rest, dye, src, outlet);
    utterance::Ledger ink{};
    utterance::Passage bath{};
    utterance::Song song{},natal{};
    Word palette=rest,wet=0,fibres=0,record=0,best=0,rank=0,visible_ink=0,ink_images=0;
    if constexpr (Inked) {
        if (demonstration) {
            ink_exchange(w,sites,palette,src,outlet,bath,wet,fibres);
            ink.releases=1;ink.moves=src!=outlet;ink.exchanges=bath.after!=palette;
            ink.stained=bath.entering!=bath.leaving;
            // The water's albedo is independent of the mobile pigment. A
            // matching colour can disappear against the background.
            rest=mix(rock^mix(key+8339))&255;
            photons=utterance::flash(bath,rest,src,outlet);
            dye=bath.entering;
            if constexpr(Sampled) {
                rank=(tile*16127u+mix(c.seed^mix(tick+71431)))&(width*width-1);
                record=rank<ink_count;
            }
        }
    }
    retina::Ledger total{};
    retina::GazeLedger attention{};
    Word looked = 0, public_onsets = 0, public_ends = 0;
    total.wet = 1;
    total.unobstructed = outlet != src;
    for (Word j = 0; j < 4; ++j) {
        Word slot = w.slots[sites[j]];
        auto &body = w.animals[slot];
        if (!body.energy || !body.body.eyes)
            continue;
        ++total.eyes;
        auto &dna = w.dna[slot];
        retina::Genes genes{dna.pigments[0], dna.pigments[1]};
        if (body.energy < 2 || !((genes.receptors >> (16 + (w.motor[slot] & 7))) & 1))
            continue;
        Word band = motor_band(w, c, slot);
        auto soma = read_band(w, c.tiles, slot, band);
        auto &hand = w.braids.minds[slot];
        auto tag = braid::drift_tag(hand.lived);
        bool displaced = channel != retina::dark &&
                         (channel == retina::displaced || ((law.ports & drift::visual_contest) && tag &&
                                                           tag != ((law.ports & drift::swapped) ? 2u : 1u)));
        auto scene = photons;
        if (displaced)
            scene.end = drift::overlay(rest, other, Inked && demonstration ? bath.leaving : dye);
        if (channel == retina::dark)
            scene.end = rest;
        retina::Banks banks{hand.volley >> 16, soma.gate_a, soma.gate_b};
        auto eyes = braid::eyes_of(w, slot);
        Word lamp = braid::pigment(eyes.hues, (retina::tide_lamp(phase, rock) + upper) & 3);
        Word synapse = (law.ports & drift::tidal_banks) ? retina::bank(genes, lamp) : 0;
        Word old = banks.get(synapse);
        auto light = retina::project(scene, eyes);
        if constexpr (Gaze)
            if (fixations) {
                retina::Observer eye{};
                eye.genes = genes;
                eye.heading = body.heading;
                eye.iris = {dna.pigments[2], dna.pigments[3]};
                eye.wiring = drift::wiring(hand.genes, law.ports);
                auto action =
                    retina::orient(eye, light.rest, light.onset, lamp, old, soma.spike, drift::branch(law.ports, tag));
                bool unique = action.prediction && !(action.prediction & (action.prediction - 1));
                ++attention.offered;
                attention.unique += unique;
                attention.bought_unique += unique && (action.opened || slot == owner);
                attention.silent += !action.onset;
                attention.bought_silent += !action.onset && (action.opened || slot == owner);
                if (!action.opened && slot != owner)
                    continue;
            }
        --body.energy;
        ++body.worked;
        ++total.attended;
        if constexpr (Public)
            looked |= 1u << j;
        if (displaced) {
            ++total.unpaired;
            total.same_source += src == alternate;
            total.same_outlet += outlet == other;
        }
        if constexpr (Stirred) {
            if (demonstration && w.braids.episodes && (law.ports & drift::sight) &&
                (slot == owner || (law.ports & drift::audience))) {
                Word pre = retina::initial(genes, light.rest, light.onset, body.heading);
                Word possible =
                    retina::advance(pre, old, drift::wiring(hand.genes, law.ports), drift::branch(law.ports, tag));
                w.braids.episodes[slot] = retina::seen_probe(rest, dye, src, possible, (hand.genes.muscle >> 24) & 7,
                                                             eyes, tick + 1, channel, light);
            }
        }
        auto event = retina::expose(genes, light, body.heading, old, soma.spike);
        if constexpr (Inked) if (demonstration) {
            Word image=utterance::impression(light,genes);
            utterance::Trail trail{w.braids.trails[slot]};auto prior=trail;
            if (image!=utterance::unseen) {
                trail.append(watch.anchored==2?utterance::stabilize(image,body.heading):image);w.braids.trails[slot]=trail.bits;
                visible_ink|=1u<<j;ink_images|=image<<(8*j);
                ++ink.seen;ink.full+=trail.size()==utterance::depth;
            } else ++ink.omitted;
            if (slot==owner && (w.braids.mode&utterance::voice_port)){
                natal=utterance::call({dna.voice,dna.hearing.expression},image);song=natal;
                if constexpr(Echo)song=utterance::imitate(w.ears[slot],dna.hearing.low,song,image);
            }
            Word quality=1+Word(image!=utterance::unseen)*(1+trail.size())+Word(slot==owner)*10;
            bool following=slot==watch.slot && tick-watch.start<watch.ticks;
            if constexpr(Sampled) if ((record && quality>best) || following) {
                utterance::Sample sample={tick+1,tile,slot,body.born,owner,src,outlet,rest,palette,bath.after,wet,fibres,
                    eyes.places,eyes.hues,genes.receptors,channel,(light.rest<<16)|(light.onset<<8)|light.end,image,
                    prior.size(),trail.size(),Word(displaced),prior.bits,trail.bits,dna.voice,dna.hearing.expression,
                    slot==owner?song.calls:0,slot==owner?song.n:0,0,body.heading,watch.anchored,
                    Word(Echo),dna.hearing.low,slot==owner?natal.calls:0,{}};
                if constexpr(Echo) if(slot==owner)for(Word k=0;k<8;++k) {
                    sample.ear.low[k]=w.ears[slot].low[k];sample.ear.high[k]=w.ears[slot].high[k];
                }
                if (record && quality>best) {best=quality;ink_notes[rank]=sample;}
                if (following) ink_notes[ink_count+(tick-watch.start)]=sample;
            }
        }
        if constexpr (Public) {
            public_onsets |= event.pre << (4 * j);
            public_ends |= event.post << (4 * j);
        }
        total.opening += event.pre != 0;
        total.ending += event.post != 0;
        total.both += bool(event.pre && event.post);
        total.eligible += event.admitted;
        bool changed = event.spikes != soma.spike;
        total.spikes += nbits(event.spikes ^ soma.spike);
        soma.spike = event.spikes;
        if (c.plastic && (law.ports & drift::plastic) && event.contacts != old) {
            total.edits += nbits(event.contacts ^ old);
            banks.put(synapse, event.contacts);
            if (synapse) {
                soma.gate_a = banks.a;
                soma.gate_b = banks.b;
                changed = true;
            } else
                hand.volley = (hand.volley & 65535u) | (banks.hand << 16);
        }
        if constexpr (Stirred) {
            if (demonstration && owner != slot) {
                auto &book = patches[(tile >> 5) & 127].counts;
                count(book.witnesses, 1);
                if (c.plastic && (law.ports & drift::plastic) && event.contacts != old)
                    count(book.changed, nbits(event.contacts ^ old));
            }
        }
        if (changed)
            assign_band(w, c.tiles, slot, band, soma);
    }
    if constexpr (Public) {
        // First finish every aperture movement. A neighbour can see an open
        // eye and the same tracer, but cannot see the owner's neural response.
        if (__builtin_popcount(looked) >= 2) {
            auto &v = visits[(tile >> 5) & 127];
            count(v.meetings, 1);
            for (Word j = 0; j < 4; ++j) {
                if (!(looked & (1u << j)))
                    continue;
                Word slot = w.slots[sites[j]];
                const auto &a = w.animals[slot];
                Word front = (j + 1 + 2 * (a.heading & 1)) & 3;
                if (!(looked & (1u << front)) || !c.plastic || !(law.ports & drift::plastic))
                    continue;
                const auto &g = w.dna[slot];
                Word band = motor_band(w, c, slot);
                auto tissue = read_band(w, c.tiles, slot, band);
                auto glass = braid::eyes_of(w, slot);
                Word face = braid::pigment(glass.hues, audience::silhouette(w.animals[w.slots[sites[front]]].body));
                Word pre = (public_onsets >> (4 * j)) & 15, post = (public_ends >> (4 * j)) & 15;
                Word old = tissue.gate_b >> 16;
                Word next = audience::remember({g.hearing.low, g.hearing.high}, old, face, pre, post);
                count(v.writes, 1);
                if (next != old) {
                    count(v.edits, nbits(old ^ next));
                    tissue.gate_b = (tissue.gate_b & 65535u) | (next << 16);
                    assign_band(w, c.tiles, slot, band, tissue);
                }
            }
        }
    }
    auto &book = books[(tile >> (shift + 5)) & 127u];
    auto group = drift_warp(true, tile / 128, shift > 2);
    drift_count(w.account[side >= 128 ? tile / 128 : 0].heat, total.attended, group);
    auto pooled = drift_warp(true, 0, false);
    if constexpr (Inked) {
        if (demonstration && song.n) ink_voices(w,c,sites,owner,song,visible_ink,ink_images,outlet,bath.leaving,ink.moves,key>>15,ink);
        if constexpr(Echo) if (ink.calls){
            ink.borrowed=song.calls!=natal.calls;ink.retuned=utterance::changed_pitches(song,natal);
        }
        if constexpr(Sampled) if (record && best) ink_notes[rank].paid=ink.vocal_heat;
        if constexpr(Sampled) if (watch.slot==owner && ink.vocal_heat && tick-watch.start<watch.ticks) ink_notes[ink_count+(tick-watch.start)].paid=ink.vocal_heat;
        drift_count(w.account[side>=128?tile/128:0].heat,Word(ink.vocal_heat+ink.ear_heat+ink.reaching_heat),group);
        auto &v=ink_books[(tile>>(shift+5))&127];
#define AX_INK_SUM(name) drift_count(v.name,Word(ink.name),pooled)
        AX_INK_SUM(releases);AX_INK_SUM(moves);AX_INK_SUM(stained);AX_INK_SUM(exchanges);AX_INK_SUM(seen);
        AX_INK_SUM(omitted);AX_INK_SUM(full);AX_INK_SUM(calls);AX_INK_SUM(syllables);AX_INK_SUM(listeners);
        AX_INK_SUM(spike_changes);AX_INK_SUM(vocal_heat);AX_INK_SUM(ear_heat);
        if constexpr(Echo){AX_INK_SUM(borrowed);AX_INK_SUM(retuned);}AX_INK_SUM(acoustic_edits);AX_INK_SUM(strikes);AX_INK_SUM(blind_strikes);AX_INK_SUM(intercepted);
        AX_INK_SUM(blind_food);AX_INK_SUM(altered);AX_INK_SUM(reaching_heat);
#undef AX_INK_SUM
    }
#define RETINA_COUNT(member) drift_count(book.member, Word(total.member), pooled)
    RETINA_COUNT(wet);
    RETINA_COUNT(eyes);
    RETINA_COUNT(attended);
    RETINA_COUNT(opening);
    RETINA_COUNT(ending);
    RETINA_COUNT(both);
    RETINA_COUNT(eligible);
    RETINA_COUNT(edits);
    RETINA_COUNT(spikes);
    RETINA_COUNT(unobstructed);
    if ((law.ports & drift::visual_contest) || channel == retina::displaced) {
        RETINA_COUNT(unpaired);
        RETINA_COUNT(same_source);
        RETINA_COUNT(same_outlet);
    }
    if constexpr (Gaze)
        if (fixations) {
            auto &census = fixations[(tile >> (shift + 5)) & 127u];
            drift_count(census.offered, Word(attention.offered), pooled);
            drift_count(census.unique, Word(attention.unique), pooled);
            drift_count(census.bought_unique, Word(attention.bought_unique), pooled);
            drift_count(census.silent, Word(attention.silent), pooled);
            drift_count(census.bought_silent, Word(attention.bought_silent), pooled);
        }
#undef RETINA_COUNT
}

KERNEL void flood_daylight(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                           retina::Ledger *books) {
    daylight<false, false>(w, c, clock, offset, law, books, nullptr, nullptr, nullptr, 0, 0);
}
KERNEL void flood_gaze(World w, Configuration c, const Word *clock, Word offset, drift::Law law, retina::Ledger *books,
                       retina::GazeLedger *fixations) {
    daylight<false, true>(w, c, clock, offset, law, books, fixations, nullptr, nullptr, 0, 0);
}
KERNEL void flood_soundings(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                            retina::Ledger *books, retina::GazeLedger *fixations, retina::StirPatch *patches,
                            retina::StirSample *survey, Word sample_count, Word rotating) {
    daylight<true, true>(w, c, clock, offset, law, books, fixations, patches, survey, sample_count, rotating);
}
KERNEL void flood_audiences(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                            retina::Ledger *books, retina::GazeLedger *fixations, retina::StirPatch *patches,
                            retina::StirSample *survey, Word sample_count, Word rotating, audience::Ledger *visits,
                            audience::Witness *witnesses) {
    daylight<true, true, true>(w, c, clock, offset, law, books, fixations, patches, survey, sample_count, rotating,
                               visits, witnesses);
}
#define AX_INK_KERNEL(name,watch,sampled,echo) \
KERNEL void name(World w, Configuration c,const Word *clock,Word offset,drift::Law law,retina::Ledger *books, \
    retina::GazeLedger *fixations,retina::StirPatch *patches,retina::StirSample *survey,Word n,Word moving, \
    audience::Ledger *visits,audience::Witness *faces,utterance::Ledger *ink,utterance::Sample *notes,Word slots,utterance::Watch trace) { \
    daylight<true,true,watch,true,sampled,echo>(w,c,clock,offset,law,books,fixations,patches,survey,n,moving,visits,faces,ink,notes,slots,trace); \
}
AX_INK_KERNEL(flood_ink,false,true,false)
AX_INK_KERNEL(flood_public_ink,true,true,false)
AX_INK_KERNEL(flood_fast_ink,false,false,false)
AX_INK_KERNEL(flood_fast_public_ink,true,false,false)
AX_INK_KERNEL(flood_echo,false,true,true)
AX_INK_KERNEL(flood_public_echo,true,true,true)
AX_INK_KERNEL(flood_fast_echo,false,false,true)
AX_INK_KERNEL(flood_fast_public_echo,true,false,true)
#undef AX_INK_KERNEL
KERNEL void retina_residents(World w, Configuration c, const Word *slots, Word count, retina::Observer *out) {
    Word i = tid();
    if (i >= count)
        return;
    Word slot = slots[i], band = motor_band(w, c, slot);
    const auto &g = w.dna[slot];
    const auto &m = w.braids.minds[slot];
    auto tissue = read_band(w, c.tiles, slot, band);
    out[i] = {{g.pigments[0], g.pigments[1]},
              w.animals[slot].heading,
              (m.genes.muscle >> 24) & 7,
              (m.genes.arbor >> 25) & 7,
              drift::wiring(m.genes, drift::recruitment),
              braid::eyes_of(w, slot),
              m.volley >> 16,
              tissue.spike,
              slot,
              tissue.gate_a,
              tissue.gate_b,
              {g.pigments[2], g.pigments[3]}};
}
KERNEL void narrative_residents(World w,Configuration c,const Word *slots,Word count,Word gating,Word optics,utterance::Senses *out) {
    Word i=tid();if(i>=count)return;
    Word slot=slots[i];const auto &g=w.dna[slot];const auto &a=w.animals[slot];const auto &h=w.braids.minds[slot];
    Word band=regulation::auditory_band(g,expressed_cells(w,slot,g.brain,c.tiles));
    auto cortex=regulation::source(g,band,read_band(w,c.tiles,slot,band));
    out[i]={{g.pigments[0],g.pigments[1]},braid::eye_wiring(optics==1?0:braid::individual_eyes,h.genes,optics?0:a.heading),
        {h.genes.plastic>>8,h.genes.arbor,cortex.program,gating,g.hearing.expression},{g.voice,g.hearing.expression},
        cortex.cells,Word(a.body.eyes!=0),Word(a.body.pores!=0),Word(a.body.limbs!=0),g.hearing.low};
}
template <bool active>
AX_FN void assess_retina(const retina::Observer *stock, Word n, Word seed, Word lives, retina::Course course,
                         retina::Score *scores, retina::Forecast *trace, Word *notes) {
    Word id = tid() / 32, lane = tid() & 31;
    if (id >= n)
        return;
    auto body = stock[id];
    retina::Score s{};
    for (Word life = lane; life < lives; life += 32) {
        retina::Forecast a[2];
        auto test = retina::exposure_order(seed, life, course);
        const Word frames = test.rounds + test.former;
        Word *first = notes ? notes + Count(2 * life) * frames : nullptr;
        auto experience = retina::acquire<active>(body, seed, life, false, test, first);
        for (Word twin = 0; twin < 2; ++twin) {
            if (twin && test.beds == 1)
                experience = retina::acquire<active>(body, seed, life, true, test, first ? first + frames : nullptr);
            else if (twin && first)
                for (Word j = 0; j < frames; ++j)
                    first[frames + j] = first[j];
            auto f = retina::forecast(body, seed, life, twin, test, experience);
            retina::release(f, body, seed, life, twin, test);
            f.harvested = Word(f.command == f.target);
            if constexpr (active)
                if (test.harvests > 1)
                    f.harvested += retina::harvest_again(body, experience, seed, life, twin, test, f);
            retina::accumulate(s, f);
            a[twin] = f;
            if (trace)
                trace[2 * life + twin] = f;
        }
        if (a[0].target != a[1].target) {
            ++s.distinct;
            s.paired += a[0].command == a[0].target && a[1].command == a[1].target;
        }
    }
    auto sum = [](Word v) {
#if __CUDA_ARCH__ >= 800
        Word r;
        asm("redux.sync.add.u32 %0,%1,0xffffffff;" : "=r"(r) : "r"(v));
        return r;
#else
        for (Word j = 16; j; j >>= 1) {
            Word k;
            asm("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;" : "=r"(k) : "r"(v), "r"(j));
            v += k;
        }
        return v;
#endif
    };
    retina::Score r{sum(s.right),     sum(s.blank),         sum(s.once),     sum(s.different), sum(s.different_right),
                    sum(s.certain),   sum(s.certain_right), sum(s.paired),   sum(s.distinct),  sum(s.edits),
                    sum(s.perceived), sum(s.spent),         sum(s.harvested)};
    if (!lane)
        scores[id] = r;
}
KERNEL void retina_panel(const retina::Observer *stock, Word n, Word seed, Word lives, retina::Course course,
                         retina::Score *scores, retina::Forecast *trace, Word *notes) {
    assess_retina<false>(stock, n, seed, lives, course, scores, trace, notes);
}
KERNEL void retina_active_panel(const retina::Observer *stock, Word n, Word seed, Word lives, retina::Course course,
                                retina::Score *scores, retina::Forecast *trace, Word *notes) {
    assess_retina<true>(stock, n, seed, lives, course, scores, trace, notes);
}
template <bool active>
AX_FN void feed_retina(const retina::Observer *habitus, const apprentice::Animal *stock, Word n, Word seed,
                       retina::Course course, Word bolus, apprentice::Harvest *food, Count *costs) {
    Word i = tid();
    if (i >= n)
        return;
    auto egg = stock[i];
    apprentice::Harvest meal{};
    if (egg.ancestor != ~0u) {
        auto b = habitus[egg.ancestor];
        if constexpr (active)
            b.iris = egg.program;
        else
            b.genes = {egg.program.first, egg.program.second};
        Word life = mix(seed + i * 3191);
        bool twin = bool(life & 64);
        if (course.rear_laps)
            course.laps = 1 + mix(life + 1177) % course.rear_laps;
        course = retina::exposure_order(seed, life, course);
        auto observation = retina::acquire<active>(b, seed, life, twin, course);
        auto f = retina::forecast(b, seed, life, twin, course, observation);
        retina::release(f, b, seed, life, twin, course);
        meal.matches = f.command == f.target;
        if constexpr (active)
            if (course.harvests > 1)
                meal.matches += retina::harvest_again(b, observation, seed, life, twin, course, f);
        meal.swallowed = meal.matches * bolus;
        meal.escaped = course.harvests * bolus - meal.swallowed;
        if constexpr (active) {
            meal.after_shift = f.spent;
            meal.swallowed += course.purse - f.spent;
        }
    }
    if constexpr (active) {
        auto group = drift_warp(true, 0, false);
        drift_count(costs[(i >> 5) & 127u], meal.after_shift, group);
    }
    food[i] = meal;
}

KERNEL void retina_feeding(const retina::Observer *habitus, const apprentice::Animal *stock, Word n, Word seed,
                           retina::Course course, Word bolus, apprentice::Harvest *food, Count *costs) {
    feed_retina<false>(habitus, stock, n, seed, course, bolus, food, costs);
}
KERNEL void retina_active_feeding(const retina::Observer *habitus, const apprentice::Animal *stock, Word n, Word seed,
                                  retina::Course course, Word bolus, apprentice::Harvest *food, Count *costs) {
    feed_retina<true>(habitus, stock, n, seed, course, bolus, food, costs);
}
KERNEL void retina_surfaces(World w, Configuration c, Word tick, drift::Law law, Word seed, Word n,
                            retina::Surface *samples) {
    Word k = tid();
    if (k >= n)
        return;
    Word total = c.side * c.side / 4, tile = (mix(seed + 99) + k * (mix(seed + 913) | 1u)) & (total - 1);
    Word x = (2 * (tile % (c.side / 2)) + ((tick >> 3) & 1)) & (c.side - 1),
         y = (2 * (tile / (c.side / 2)) + ((tick >> 4) & 1)) & (c.side - 1);
    Word xx = (x + 1) & (c.side - 1), yy = (y + 1) & (c.side - 1);
    DriftSquare sites{y * c.side + x, y * c.side + xx, yy * c.side + xx, yy * c.side + x};
    Word rock = mix(c.seed ^ mix((x >> 3) + 19391 * (y >> 3)));
    Word phase = ((rock >> 13) + ((law.ports & drift::tide) ? tick >> law.tide_shift : 0)) & 3;
    samples[k] = {drift_bed(w, sites, 0, phase, rock), c.side, tile, tick, law.ports, c.seed};
}

KERNEL void retina_compile_surfaces(const retina::Surface *raw, Word count, Word *result) {
    Word id = tid();
    if (id < count)
        result[id] = retina::as_built(raw[id]).shape();
}
