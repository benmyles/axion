#pragma once
#include "narrative.hpp"

namespace axion::life::utterance {
struct Senses {
    retina::Genes retina{};
    braid::Optics glass{};
    Biology learning{};
    Genes voice{};
    regulation::Cells cells{};
    Word eyes = 0, mouth = 0, limbs = 0, echo = 0;
};
struct Basin {
    Word palette = 0, ducts = 0, wet = 15, fibres = 0, ground = 0, bright = 1;
    Word grains[4]{16, 16, 16, 16};
};
struct Transit {
    Word from = 0, to = 0, before = 0, after = 0, ground = 0, physical = unseen, view = unseen, light = 0,
         available = 0;
};
AX_FN Transit release(Basin &b, Word from, const Senses &body) {
    Transit e{};
    Word to = braid::port(b.ducts, from);
    e.from = from;
    e.to = to;
    e.before = e.after = b.palette;
    if (!(b.wet & (1u << from)) || !b.grains[from] || (to != from && b.grains[to] == 65535))
        return e;
    auto p = passage(b.palette, from, to, b.fibres, b.wet);
    --b.grains[from];
    ++b.grains[to];
    b.palette = p.after;
    Word ground = b.ground;
    if (b.bright) {
        ground = drift::overlay(ground, from, p.entering ^ 3u);
        ground = drift::overlay(ground, to, p.leaving ^ 3u);
    }
    auto viewed = retina::project(flash(p, ground, from, to), body.glass);
    e.after = p.after;
    e.ground = ground;
    e.available = 1;
    e.physical = from | (to << 2) | (p.entering << 4) | (p.leaving << 6);
    e.view = body.eyes ? impression(viewed, body.retina) : unseen;
    e.light = (viewed.rest << 16) | (viewed.onset << 8) | viewed.end;
    return e;
}
struct Waves {
    Count wave[voice_span]{};
    Word n = 0;
};
AX_FN Phrase register_waves(Vocabulary &v, const Waves &heard, bool acquire) {
    Phrase out{};
    for (Word j = 0; j < heard.n; ++j)
        if (!out.push(v.find(heard.wave[j], acquire)))
            return {};
    return out;
}
AX_FN Waves vocalize(const Vocabulary &v, const Phrase &r) {
    Waves out{};
    for (Word j = 0; j < r.n; ++j)
        out.wave[out.n++] = v.sounds[r.word[j] - 1];
    return out;
}
enum Action : Word { inspect, open, pour, clear, pair_speech, respond, receive, recount, curtain, cast_net, serve };
struct Workbench {
    Basin bed{};
    Trail physical{}, net{};
    Word ready = 0, shutters = 3, cast = 0, catcher = 0, stock = 64, held = 0, settled = 0, eaten = 0;
};
struct Request {
    Word action = inspect, who = 0, source = 0, energy = 0, other_energy = 0;
    Basin bed{};
    Waves prompt{}, statement{};
};
struct Reply {
    Word status = 0, teacher_heat = 0, pupil_heat = 0, edits = 0, sprouts = 0, witnessed = 0;
    Transit event{};
    Trail seen{}, other{}, decoded{};
    Waves reply{};
    Word palette = 0, grains = 0, initial = 0, prompts_known = 0, stock = 0, held = 0, settled = 0, eaten = 0, bite = 0,
         shutters = 0;
};
} // namespace axion::life::utterance
