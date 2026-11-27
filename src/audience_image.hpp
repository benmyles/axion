#pragma once
#include "axion/audience_batch.hpp"
#include "individual_io.hpp"

namespace axion::life::audience {
inline Animal embodied(const storage::AgentImage &image, const Tissue &tissue) {
    if (!image.riverine || !image.binds_floats || !image.resident.animal.body.eyes)
        throw std::invalid_argument("a waterside encounter needs a resident with a functioning eye and hand circuit");
    const auto &a = image.resident.animal;
    const auto &g = image.resident.germ;
    const auto &hand = image.braided.mind;
    retina::Observer eye{{g.pigments[0], g.pigments[1]},
                         a.heading,
                         (hand.genes.muscle >> 24) & 7,
                         (hand.genes.arbor >> 25) & 7,
                         drift::wiring(hand.genes, drift::recruitment),
                         braid::eye_wiring(braid::individual_eyes, hand.genes, a.heading),
                         hand.volley >> 16,
                         tissue.spike,
                         image.resident.slot,
                         tissue.gate_a,
                         tissue.gate_b,
                         {g.pigments[2], g.pigments[3]}};
    // The watershed carries its public acoustic nerve in the resident and its
    // braid organ. The optional dry-land auditory extension is a different ear.
    Animal body{eye,      {g.hearing.low, g.hearing.high}, Word(a.body.pores != 0), Word(a.energy != 0),
                a.energy, Word(a.body.limbs != 0)};
    body.lexical_law = hand.genes.plastic >> 8;
    body.receptive = bool(hand.genes.arbor & (1u << 14));
    return body;
}
inline Count public_voice(const storage::AgentImage &a) {
    return braid::timbre(a.braided.mind) ^ 0x53686f7265556c00ull;
}
constexpr Word report_contact = 0xa17eu;
inline bool recall(const storage::AgentImage &pupil, Count voice, Alignment &out) {
    Word id = braid::acquaintance(pupil.braid_report.memory, voice, false);
    if (id == braid::none || braid::contact(pupil.braid_report.memory, id).panel != report_contact)
        return false;
    out = {{pupil.resident.ear.low[4 + id], pupil.resident.ear.high[4 + id]}};
    return true;
}
inline void retain(storage::AgentImage &pupil, Count voice, Alignment code) {
    auto &m = pupil.braid_report.memory;
    Word id = braid::acquaintance(m, voice, true);
    braid::remember_contact(m, id, {voice, 0, report_contact});
    pupil.resident.ear.low[4 + id] = code.bank[0];
    pupil.resident.ear.high[4 + id] = code.bank[1];
    pupil.narrates_floats = true;
}
inline Animal embodied(const storage::AgentImage &a) {
    Word width = a.grows_cells ? a.cortex.active : living_tiles(a.brain.tiles, a.brain.genes.development);
    if (!width)
        throw std::invalid_argument("the enrolled resident has no conducting cells");
    return embodied(a, a.brain.tissue[regulation::innervated_band(a.resident.germ, width)]);
}
inline regulation::Program read_germ(const std::string &path) {
    std::ifstream f(path);
    std::string name, extra;
    regulation::Program p{};
    if (!(f >> name >> p.first >> p.second) || name != "AXION-AUDIENCE-1" || f >> extra)
        throw std::invalid_argument("invalid audience circuit");
    return p;
}
inline Word treatment(std::string_view arg) {
    constexpr std::string_view arms[]{"intact",   "exchanged", "screened", "muted",
                                      "reversed", "disguised", "hidden",   "private"};
    for (Word i = 0; i < std::size(arms); ++i)
        if (arg == arms[i])
            return i;
    throw std::invalid_argument("unknown audience treatment");
}
inline Word access_mode(std::string_view arg) {
    constexpr std::string_view modes[]{"split", "common", "neither", "mixed", "unequal"};
    for (Word i = 0; i < std::size(modes); ++i)
        if (arg == modes[i])
            return i;
    throw std::invalid_argument("choose split, common, neither, mixed or unequal viewing histories");
}
} // namespace axion::life::audience
