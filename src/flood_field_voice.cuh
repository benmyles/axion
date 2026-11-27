#include "axion/field_voice.hpp"
namespace axion::life::material::field_voice {
AX_FN Samples scene(Word seed) {
    Samples room{};
    room.seed = seed;
    Word k = mix(seed + 4729);
    Word a = draw(k) & 3, b = a ^ (1 + draw(k) % 3);
    room.soil[0] = a;
    room.soil[1] = b;
    for (Word i = 0; i < 2; ++i) {
        Word band = room.soil[i ^ (mix(seed + 617) & 1)];
        if ((draw(k) & 7) == 0)
            band ^= 1 + draw(k) % 3;
        room.chip[i] = {static_cast<Byte>(1 + draw(k) % 255), static_cast<Byte>(0x30 | (band << 2) | (draw(k) & 3)), 0,
                        0};
    }
    Word used = 0;
    for (Word i = 0; i < 4; ++i) {
        room.pigment[i] = draw(k) & 255;
        room.site[i] = scree::pick(~used, draw(k));
        used |= 1u << room.site[i];
    }
    return room;
}
AX_FN bool names(const grounding::Mind &words, Count pulse, Word pigment) {
    Word id = grounding::recognize(words, pulse);
    return id && grounding::admits(words.word[id].certain, pigment);
}
AX_FN Count identify(const grounding::Mind &words, Word pigment, Word alternative) {
    for (Word i = 1; i <= words.count; ++i) {
        auto &c = words.word[i];
        if (c.viable && grounding::admits(c.certain, pigment) && !grounding::admits(c.possible, alternative))
            return c.wave;
    }
    return 0;
}
AX_FN Word reference(const grounding::Mind &words, Count wave, Word a, Word b) {
    Word id = grounding::recognize(words, wave);
    if (!id)
        return 2;
    auto &cell = words.word[id];
    if (grounding::admits(cell.certain, a) && !grounding::admits(cell.possible, b))
        return 0;
    if (grounding::admits(cell.certain, b) && !grounding::admits(cell.possible, a))
        return 1;
    return 2;
}
AX_FN Word distance(Word i, Word j) {
    return __builtin_abs(int(i & 7) - int(j & 7)) + __builtin_abs(int(i >> 3) - int(j >> 3));
}
AX_FN Word less(Word a, Word b) { return a < b ? 1 : a > b ? 2 : 3; }
AX_FN Word property(const Samples &r, Word chip, Word mask, Word kind, Word decision = 2) {
    if (kind >= 8)
        return decision < 2 ? 1u << (decision ^ (kind == 9)) : 3u;
    if (kind < 2)
        return kind ? 3 ^ mask : mask;
    if (kind == 2 || kind == 3)
        return kind == 2 ? 1 : 2;
    Word a = r.site[2], b = r.site[3], c = r.site[chip];
    if (kind == 4)
        return less(a & 7, b & 7);
    if (kind == 5)
        return less(b & 7, a & 7);
    if (kind == 6)
        return less(distance(a, c), distance(b, c));
    return less(distance(b, c), distance(a, c));
}
AX_FN Word forecast(const Samples &r, Word chip, const Inquiry &p, const Genes &g) {
    Word seen = 0;
    Word material = worksite::surface_color(g, catalytic_band(r.chip[chip]));
    for (Word j = 0; j < 2; ++j)
        seen |= Word(scree::fires(p.chemistry, 4 * material + (r.soil[j] ^ scree::wild_odor(g)), g.release)) << j;
    return seen;
}
AX_FN Word match(const FieldLanguage &known, const FieldUtterance &utterance) {
    for (Word j = 0; j < known.count; ++j) {
        auto &form = known.question[j];
        if (form.length != utterance.length)
            continue;
        bool same = true;
        for (Word k = 0; k < form.length; ++k)
            if (k != form.slot && form.wave[k] != utterance.wave[k])
                same = false;
        if (same)
            return j;
    }
    return 4;
}
AX_FN Word test(Grain grain, Word soil, Word &paid, Word solvent = 0) {
    // A fresh duplicate of the physical coupon, one detritus unit and a
    // measured body-funded charge. No observation updates the reporting mind.
    grain.charge = static_cast<Byte>(1 + ((grain.substance >> 5) & 3));
    Word opening = grain.charge;
    Half stock = 1, food = 0;
    excite(grain, 4, brine::surface(soil, solvent), stock, food);
    paid += 1 + opening - grain.charge;
    return food;
}
} // namespace axion::life::material::field_voice
AX_FN void field_operation(const material::Inquiry *memory, const material::Genes *genome,
                           const grounding::Mind *language, material::FieldLanguage *acoustic, material::Samples *tray,
                           material::FieldUtterance utterance, Word command, Word value, Word pointed, Word reserve,
                           material::FieldAnswer *output, Count *channel, material::ListenerInput *listener_board,
                           brine::Animal *cortex, Word liquid, Word vision) {
    using namespace material;
    using namespace material::field_voice;
    auto &room = *tray;
    auto &known = *acoustic;
    FieldAnswer reply{};
    *channel = 0;
    if (command == 0) {
        room = scene(value);
        room.solution = liquid;
    }
    if (language)
        for (Word j = 0; j < 4; ++j)
            reply.names[j] = identify(*language, room.pigment[j], room.pigment[j ^ 1]);
    if (command == 1 && value < 2 && reserve > 4) {
        Word mask = 0;
        for (Word j = 0; j < 2; ++j) {
            Word made = test(room.chip[value], room.soil[j], reply.paid, room.solution);
            if (cortex && vision)
                brine::observe(cortex->genes, cortex->memory,
                               brine::cue(catalytic_band(room.chip[value]), room.soil[j]), (made != 0) ^ (vision == 2));
            mask |= Word(made != 0) << j;
            reply.calories += made;
        }
        room.observed[value] = mask;
        if (cortex && vision && room.witnesses <= ~0u - 2)
            room.witnesses += 2;
        room.measured |= 1u << value;
        reply.evidence = mask;
    }
    if (command == 4 && cortex && value < 2 && reserve > 3) {
        Word chip = catalytic_band(room.chip[value]);
        Word side = brine::choose(cortex->genes, cortex->memory, brine::cue(chip, room.soil[0]),
                                  brine::cue(chip, room.soil[1]), mix(room.seed + value * 7187));
        room.taken[value] = side;
        if (vision && room.witnesses != ~0u)
            ++room.witnesses;
        room.tried |= 1u << value;
        reply.destination = side;
        reply.calories = test(room.chip[value], room.soil[side], reply.paid, room.solution);
        if (vision)
            brine::observe(cortex->genes, cortex->memory, brine::cue(chip, room.soil[side]),
                           (reply.calories != 0) ^ (vision == 2));
    }
    if (command == 5 && value < 2 && pointed >= 2 && (room.tried & (1u << value)))
        pointed = room.taken[value] ^ (pointed == 3);
    if ((command == 2 || (command == 5 && cortex && value < 2 && (room.tried & (1u << value)))) && language &&
        value < 2 && pointed < 2 && (room.measured & (1u << value)) && utterance.length) {
        Word slot = 8;
        for (Word k = 0; k < utterance.length; ++k)
            if (reference(*language, utterance.wave[k], room.pigment[0], room.pigment[1]) == value) {
                if (slot != 8) {
                    slot = 8;
                    break;
                }
                slot = k;
            }
        if (slot != 8) {
            Word id = match(known, utterance);
            if (id == 4 && known.count < 4) {
                id = known.count++;
                auto &frame = known.question[id];
                frame = {};
                frame.length = utterance.length;
                if (command == 5)
                    frame.contradictions = 0x80000000u;
                frame.slot = slot;
                for (Word k = 0; k < utterance.length; ++k)
                    if (k != slot)
                        frame.wave[k] = utterance.wave[k];
            }
            if (id < 4 && known.question[id].slot == slot) {
                auto &form = known.question[id];
                for (Word h = 0; h < (form.contradictions >> 31 ? 10u : 8u); ++h) {
                    Word predictions = property(room, value, room.observed[value], h,
                                                room.tried & (1u << value) ? room.taken[value] : 2);
                    if (!(predictions & (1u << pointed)))
                        form.contradictions = scree::observe(form.contradictions, h, true, genome->practice);
                }
                ++form.examples;
                ++known.examples;
                reply.recognized = 1;
                reply.referent = value;
            }
        }
    }
    if (command == 3 && language) {
        Word id = match(known, utterance);
        if (id < 4) {
            const auto &frame = known.question[id];
            reply.recognized = 1;
            Word chip = reference(*language, utterance.wave[frame.slot], room.pigment[0], room.pigment[1]);
            reply.referent = chip;
            if (chip < 2) {
                Word predicted = forecast(room, chip, *memory, *genome), choices = 0, decision = 2;
                if (cortex) {
                    Word band = catalytic_band(room.chip[chip]);
                    auto w = brine::efficacy(cortex->genes, cortex->memory);
                    Word total = brine::mass(w);
                    Word a = brine::estimate(cortex->genes, w, brine::cue(band, room.soil[0])),
                         b = brine::estimate(cortex->genes, w, brine::cue(band, room.soil[1]));
                    decision = a == b ? (mix(room.seed + chip * 7187) & 1) : Word(b > a);
                    predicted = Word(brine::impulse(cortex->genes, a, total)) |
                                (Word(brine::impulse(cortex->genes, b, total)) << 1);
                    Word difference = a > b ? a - b : b - a;
                    reply.prior_uncertain = !total || 8 * difference <= total * ((cortex->genes.readout >> 23) & 7) ||
                                            predicted == 0 || predicted == 3;
                    reply.uncertain = reply.prior_uncertain || room.witnesses < brine::arrival(cortex->genes);
                }
                bool unique = true;
                reply.expected = predicted;
                for (Word h = 0; h < (frame.contradictions >> 31 ? 10u : 8u); ++h)
                    if (!scree::fires(frame.contradictions, h, genome->release)) {
                        reply.eligible |= 1u << h;
                        Word row = property(room, chip, predicted, h, decision);
                        if (row != 1 && row != 2)
                            unique = false;
                        choices |= row;
                    }
                if (unique && (choices == 1 || choices == 2)) {
                    Word station = choices == 2;
                    reply.destination = station;
                    reply.wave = reply.names[2 + station];
                    reply.foil = reply.names[2 + (station ^ 1)];
                    *channel = reply.wave;
                }
                // The receiver gets the exhibited chip and the labeled physical
                // deposits in a separately permuted orientation, plus one sound.
                auto &board = *listener_board;
                board.piece = room.chip[chip];
                board.solution = room.solution;
                for (Word j = 0; j < 2; ++j) {
                    Word ix = j ^ (mix(room.seed + 421) & 1);
                    board.pigments[ix] = room.pigment[2 + j];
                    board.soil[ix] = room.soil[j];
                    board.sites[ix] = room.site[2 + j];
                }
            }
        }
    }
    reply.questions = known.count;
    *output = reply;
}
KERNEL void flood_field_utterance(const material::Inquiry *memory, const material::Genes *genome,
                                  const grounding::Mind *language, material::FieldLanguage *acoustic,
                                  material::Samples *tray, material::FieldUtterance utterance, Word command, Word value,
                                  Word pointed, Word reserve, material::FieldAnswer *output, Count *channel,
                                  material::ListenerInput *listener_board, brine::Animal *cortex, Word liquid,
                                  Word vision) {
    if (!tid())
        field_operation(memory, genome, language, acoustic, tray, utterance, command, value, pointed, reserve, output,
                        channel, listener_board, cortex, liquid, vision);
}
AX_FN void field_recipient(const grounding::Mind *learner, const Count *channel, const material::ListenerInput *room,
                           Word hear, Word reserve, material::ListenerResult *out) {
    material::ListenerResult result{};
    if (learner && *channel && hear && reserve > 2) {
        Word chosen = material::field_voice::reference(*learner, *channel, room->pigments[0], room->pigments[1]);
        if (chosen < 2) {
            result.chosen = room->sites[chosen];
            result.calories = material::field_voice::test(room->piece, room->soil[chosen], result.paid, room->solution);
        }
    }
    *out = result;
}
KERNEL void flood_field_recipient(const grounding::Mind *learner, const Count *channel,
                                  const material::ListenerInput *room, Word hear, Word reserve,
                                  material::ListenerResult *out) {
    if (!tid())
        field_recipient(learner, channel, room, hear, reserve, out);
}
