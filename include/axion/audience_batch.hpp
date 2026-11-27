#pragma once
#include "audience_course.hpp"

namespace axion::life::audience {
struct Commitment {
    Word movement, speech;
};
static_assert(sizeof(Commitment) == 8);
AX_FN Commitment seal(const Contact &r) {
    Word edits = __builtin_popcount(r.before ^ r.after);
    Word movement = r.prior_aim | (r.final_aim << 8) | (r.viewed << 16) | (r.picked << 20) | (r.taught << 22) |
                    (r.opened << 23) | (edits << 24) | (r.work << 29) | (Word(r.release_from < 4) << 31);
    Word speech =
        r.prior_voice | (r.final_voice << 12) | (r.said_to << 24) | (Word(r.first_wave && r.second_wave) << 27);
    return {movement, speech};
}
AX_FN Contact unseal(Commitment c) {
    Contact r{};
    r.prior_aim = c.movement & 255;
    r.final_aim = (c.movement >> 8) & 255;
    r.viewed = (c.movement >> 16) & 15;
    r.picked = (c.movement >> 20) & 3;
    r.taught = (c.movement >> 22) & 1;
    r.opened = (c.movement >> 23) & 1;
    r.work = (c.movement >> 29) & 3;
    r.release_from = c.movement >> 31 ? r.picked : 4;
    r.prior_voice = c.speech & 4095;
    r.final_voice = (c.speech >> 12) & 4095;
    r.said_to = (c.speech >> 24) & 7;
    r.first_wave = r.second_wave = (c.speech >> 27) & 1;
    return r;
}
enum Metric : Word {
    prior,
    final,
    statements,
    fresh,
    truthful,
    prior_words,
    final_words,
    helped,
    harmed,
    heard,
    changed,
    both,
    edits,
    effort,
    neither_seen,
    one_seen,
    two_seen,
    offer_neither,
    offer_one,
    offer_two,
    fresh_neither,
    fresh_one,
    fresh_two,
    metrics
};
struct Sweep {
    Word values[metrics]{}, example = ~0u;
};
AX_FN void weigh(Sweep &s, const Contact &r, const Catch &f, Word altered) {
    auto &v = s.values;
    v[prior] += f.prior;
    v[final] += f.final;
    v[statements] += bool(r.first_wave && r.second_wave);
    v[fresh] += f.fresh;
    v[truthful] += f.truthful;
    v[prior_words] += f.voiced_before;
    v[final_words] += f.voiced_after;
    v[helped] += f.final > f.prior;
    v[harmed] += f.final < f.prior;
    v[heard] += r.taught;
    v[edits] += altered;
    v[effort] += r.work;
    Word visible = __builtin_popcount(r.viewed);
    ++v[neither_seen + visible];
    v[offer_neither + visible] += r.work != 0;
    v[fresh_neither + visible] += f.fresh;
}
AX_FN bool illustrate(const Layout &bed, const Contact &a, const Catch &x, const Contact &b, const Catch &y) {
    return x.fresh && y.fresh && a.picked != b.picked &&
           braid::port(bed.map, bed.source[0]) != braid::port(bed.map, bed.source[1]);
}
} // namespace axion::life::audience
