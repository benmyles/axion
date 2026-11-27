#pragma once
#include "narrative_world.hpp"
#include "retina_study.hpp"

namespace axion::life::utterance {
// The external tutor and the optical/motor learner have separate memories.
// A culture can be replaced by another saved speaker or by human examples.
struct Culture {
    Count waves[names]{};
    Word language = 0, salt = 0;
    Genes throat{};
    Senses origin{};
};
enum Language : Word { river, reverse, varied, native };
enum Exposure : Word { coupled, exchanged_scenes, shuffled_words, fixed, separated };
struct Course {
    Word seed = 0, split = 0, singles = 192, couples = 64, length = 5, arm = coupled, natural_light = 0, previously = 0;
    Word decoding = 1, fast = 1, hearing = 0, hand = 0;
    Word purse = 32768;
    const Word *beds{};
    Word sites = 0;
};
AX_FN Phrase question(Word task) {
    Phrase q{};
    if (!task) {
        q.push(22);
        q.push(23);
    } else if (task == 1) {
        q.push(24);
        q.push(25);
        q.push(26);
        q.push(27);
    } else if (task == 2) {
        q.push(28);
        q.push(25);
        q.push(29);
        q.push(30);
    } else {
        q.push(24);
        q.push(25);
        q.push(29);
        q.push(31);
    }
    return q;
}
AX_FN Word title(const Culture &c, Word label) {
    if (c.language != varied || label > 8)
        return label;
    Word base = label > 4 ? 5u : 1u, code = braid::assembly(mix(c.salt + base * 1177) % 24);
    return base + braid::port(code, label - base);
}
AX_FN Packet sentence(const Culture &culture, Word task, Word image) {
    Word a = 1 + (image & 3), b = 1 + ((image >> 2) & 3), u = 5 + ((image >> 4) & 3), v = 5 + (image >> 6);
    Word words[phases]{}, n = 0;
    if (culture.language == native && !task) {
        auto song = call(culture.throat, image);
        for (Word j = 0; j < song.n; ++j)
            words[n++] = 32 + ((song.calls >> (4 * j)) & 15);
    } else if (task) {
        if (task == 1)
            words[n++] = b;
        else {
            words[n++] = task == 2 ? u : v;
            words[n++] = 14;
            words[n++] = task == 2 ? a : b;
        }
    } else if (culture.language == reverse) {
        words[n++] = v;
        words[n++] = 14;
        words[n++] = b;
        words[n++] = a == b ? 13 : 19;
        words[n++] = 10;
        words[n++] = a;
        words[n++] = 18;
        words[n++] = u;
    } else if (culture.language == varied) {
        words[n++] = a;
        words[n++] = u;
        words[n++] = a == b ? 13 : 9;
        words[n++] = v;
        words[n++] = b;
    } else if (a == b) {
        words[n++] = u;
        words[n++] = 13;
        words[n++] = 14;
        words[n++] = a;
        words[n++] = 12;
        words[n++] = v;
    } else {
        words[n++] = u;
        words[n++] = 9;
        words[n++] = 10;
        words[n++] = a;
        words[n++] = 11;
        words[n++] = b;
        words[n++] = 12;
        words[n++] = v;
    }
    Count out = Count(n) << 56;
    for (Word i = 0; i < n; ++i)
        out |= Count(title(culture, words[i])) << (6 * i);
    return {out};
}
AX_FN Phrase tell(const Culture &c, Word task, Trail scene) {
    Phrase out{};
    if (!scene.size())
        return out;
    bool backward = !task && (c.language == reverse || (c.language == native && (c.throat.motor & (1u << 28))));
    bool elide = !task && (c.language == varied || (c.language == native && (c.throat.motor & (1u << 29))));
    Word n = task >= 2 ? 1 : scene.size();
    Packet before{};
    for (Word j = 0; j < n; ++j) {
        Word at = task == 3 ? scene.size() - 1 : backward ? scene.size() - 1 - j : j;
        auto piece = sentence(c, task, scene.at(at));
        if (!piece.size())
            return {};
        if (j) {
            if (c.language == native && !task)
                out.push(32 + ((c.throat.dendrites >> 16) & 15));
            else if (backward)
                out.push(17);
            else {
                out.push(15);
                if (c.language != varied && !task)
                    out.push(16);
            }
        }
        for (Word i = prefix(before, piece, j && elide); i < piece.size(); ++i)
            out.push(piece.at(i));
        before = piece;
    }
    return out;
}
AX_FN bool held(Word image, Word split) {
    return (((image & 3) + ((image >> 4) & 3) + split) & 3) == 0;
}
struct Scene {
    Trail truth{}, look{}, peer{};
    Word attempts = 0, stains = 0, incomplete = 0;
};
AX_FN Word local_image(Word e, Word ground, const Senses &body) {
    auto begin = drift::overlay(ground, e & 3, (e >> 4) & 3), end = drift::overlay(ground, (e >> 2) & 3, e >> 6);
    return body.eyes ? impression(retina::project({ground, begin, end}, body.glass), body.retina) : unseen;
}
AX_FN Scene stage(const Senses &eye, Course c, Word key, Word length, bool test, const Senses *other = nullptr) {
    Scene scene{};
    for (Word j = 0; j < length; ++j) {
        bool obtained = false;
        for (Word attempt = 0; attempt < 96; ++attempt) {
            Word roll = mix(key ^ mix(j * 65213u + attempt * 3671u + 9493u));
            Basin tray{};
            tray.palette = mix(roll + 9311) & 255;
            tray.fibres = mix(roll + 539) & 15;
            tray.ground = mix(roll + 915) & 255;
            tray.bright = !c.natural_light;
            Word from = (roll >> 8) & 3;
            if (c.sites) {
                auto p = drift::Path::recover(c.beds[mix(roll + 1987) % c.sites]);
                Word links = (roll & 1) ? p.upper : p.lower;
                for (Word k = 0; k < 4; ++k)
                    tray.ducts |= drift::outlet(k, links) << (2 * k);
            } else
                tray.ducts = braid::assembly(mix(roll + 7937) % 24);
            auto e = release(tray, from, eye);
            ++scene.attempts;
            if (held(e.physical, c.split) != test)
                continue;
            scene.truth.append(e.physical);
            scene.look.append(e.view);
            if (other)
                scene.peer.append(local_image(e.physical, e.ground, *other));
            scene.stains += ((e.physical >> 4) & 3) != (e.physical >> 6);
            scene.incomplete += e.view == unseen;
            obtained = true;
            break;
        }
        if (!obtained) {
            ++scene.incomplete;
            return scene;
        }
    }
    return scene;
}
AX_FN Phrase hear_tutor(Vocabulary &v, const Culture &c, const Phrase &line, bool acquire) {
    Phrase heard{};
    for (Word j = 0; j < line.n; ++j)
        if (!heard.push(v.find(c.waves[line.word[j] - 1], acquire)))
            return {};
    return heard;
}
struct ReferenceCourse {
    Word examples = 0, trials = 0, swapped = 0, other = 0, mispaired = 0;
};
AX_FN Phrase reference_question(const Culture &c, Word family, Word colour) {
    Phrase q{};
    q.push(22);
    q.push(19);
    q.push(family ? 48 : 17);
    q.push(title(c, 5 + colour));
    return q;
}
AX_FN Word reference_target(Trail physical, Word family, Word colour, Word swapped) {
    Word last = family ^ swapped;
    for (Word i = 0; i < physical.size(); ++i) {
        Word k = last ? physical.size() - 1 - i : i, pigment = (physical.at(k) >> (family ? 6 : 4)) & 3;
        if (pigment == colour) {
            Word dest = last ? k - 1 : k + 1;
            return dest < physical.size() ? dest : depth;
        }
    }
    return depth;
}
AX_FN Scene reference_scene(const Senses &pupil, Course c, Word family, Word j, Word swapped,
                            const Senses *other = nullptr) {
    for (Word k = 0; k < 64; ++k) {
        Word seed = mix(c.seed ^ mix(j * 13763 + family * 94109 + k * 11071 + 70387));
        auto scene = stage(pupil, c, seed, 3 + ((seed >> 8) % 5), false, other);
        if (reference_target(scene.truth, family, j & 1, swapped) != depth)
            return scene;
    }
    return {};
}
struct ReferenceResult {
    Word taught = 0, spoken = 0, edits = 0, spikes = 0, paid = 0;
};
struct ReferenceExam {
    Count physical = 0, retinal = 0;
    Packet said{}, wanted{};
    Word target = depth, picked = depth, routes = 0, heard = 0;
};
struct Teaching {
    Word singles = 0, couples = 0, edits = 0, revisions = 0, paired = 0, unseen = 0, spoken = 0, spikes = 0,
         terminal = 0, heat = 0;
};
struct Exam {
    Count physical = 0, retinal = 0, heard = 0, voice = 0, expected = 0;
    Word task = 0, words = 0, required = 0, correct = 0, recovered = 0, units = 0, visible = 0, stained = 0,
         competent = 0;
};
struct WordSample {
    Phrase expected{}, spoken{};
};
struct Accent {
    Word images = 0, original_difference = 0, different = 0, edited_notes = 0, taught_a = 0, taught_b = 0, within = 0,
         across = 0;
    Word heat = 0, plastic_edits = 0, wave_count = 0;
    Word paired_food = 0, swapped_food = 0, paired_portions = 0, changed_with_scene = 0;
};
struct AccentCourse {
    Word links = 1, practice = 48, arm = 0, neighbor = 0;
    Genes alternate{};
};
struct Meal {
    Word upper = 0, lower = 0, settled = 0;
};
struct Returns {
    Word upper = 0, lower = 0, adult_heat = 0, school_heat = 0, seen = 0, unique = 0, edits = 0;
};
constexpr Word feeds = 8;
AX_FN Returns joint_life(Genes g, const Senses &pupil, const Senses &caller, Course c, Word world, Word practice,
                         bool crossed) {
    Receptors ear{};
    Returns r{};
    for (Word j = 0; j < practice + feeds; ++j) {
        bool mature = j >= practice;
        auto scene = stage(pupil, c, mix(world ^ mix(j * 4517 + 0x412379u)), 1, mature, &caller);
        if (!scene.truth.size())
            continue;
        auto voice = call(g, scene.peer.size() ? scene.peer.at(0) : unseen);
        if (!mature) {
            r.school_heat += 2 + 2 * voice.n;
            if (scene.look.size() && voice.n) {
                r.edits += associate(ear, voice, scene.look.at(0), pupil.learning.law);
                ++r.seen;
            }
            continue;
        }
        r.adult_heat += 2 * voice.n;
        Word vote = evoke(ear, voice, crossed);
        Word label = scene.truth.at(0), src = braid::pigment(pupil.glass.places, label & 3),
             dest = braid::pigment(pupil.glass.places, (label >> 2) & 3);
        Word start = braid::pigment(pupil.glass.hues, (label >> 4) & 3),
             end = braid::pigment(pupil.glass.hues, label >> 6);
        r.upper += located(vote, 0) == src && located(vote, 2) == start;
        r.lower += located(vote, 1) == dest && located(vote, 3) == end;
        r.unique += located(vote, 0) < 4 && located(vote, 1) < 4 && located(vote, 2) < 4 && located(vote, 3) < 4;
    }
    return r;
}
AX_FN Trail reversed(Trail a) {
    Trail out{};
    for (Word i = a.size(); i; --i)
        out.append(a.at(i - 1));
    return out;
}
AX_FN Count acoustic_digest(const Phrase &s) {
    Count sum = 1469598103934665603ull;
    for (Word i = 0; i < s.n; ++i)
        sum = (sum ^ s.word[i]) * 1099511628211ull;
    return sum;
}
} // namespace axion::life::utterance
