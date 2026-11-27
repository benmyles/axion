#pragma once
#include "axion/narrative_image.hpp"
#include "axion/narrative_school.hpp"
#include "individual_io.hpp"
#include "pulse_words.hpp"

namespace axion::life::utterance {
inline Word band_of(const storage::AgentImage &a) {
    if (!a.riverine || !a.binds_floats)
        throw std::invalid_argument("spoken transitions need a waterside resident");
    auto width = a.grows_cells ? a.cortex.active : living_tiles(a.brain.tiles, a.brain.genes.development);
    if (!width)
        throw std::invalid_argument("the resident has no active spiking cells");
    return regulation::auditory_band(a.resident.germ, width);
}
inline Senses senses(const storage::AgentImage &a, const Tissue &t, Word gating = 1) {
    auto &g = a.resident.germ;
    auto &body = a.resident.animal;
    auto &h = a.braided.mind;
    auto cortex = regulation::source(g, band_of(a), t);
    return {{g.pigments[0], g.pigments[1]},
            braid::eye_wiring(a.accounts.tag == fixed_shared ? 0 : braid::individual_eyes, h.genes,
                              a.accounts.tag == free_frame ? body.heading : 0),
            {h.genes.plastic >> 8, h.genes.arbor, cortex.program, gating, g.hearing.expression},
            {g.voice, g.hearing.expression},
            cortex.cells,
            Word(body.body.eyes != 0),
            Word(body.body.pores != 0),
            Word(body.body.limbs != 0),
            g.hearing.low};
}
inline Senses senses(const storage::AgentImage &a, Word gating = 1) {
    return senses(a, a.brain.tissue[band_of(a)], gating);
}
inline Culture culture(Word language, Word salt, Genes genes) {
    constexpr const char *words[]{
        "north",  "east", "south", "west", "amber", "jade",  "plum",   "teal",  "went",   "from", "to",       "as",
        "stayed", "at",   "and",   "then", "after", "in",    "came",   "began", "rested", "what", "happened", "where",
        "did",    "they", "go",    "how",  "it",    "begin", "finish", "ba",    "di",     "ku",   "mo",       "pa",
        "ri",     "sa",   "tu",    "va",   "wi",    "xi",    "yo",     "za",    "fe",     "gu",   "ha",       "before"};
    static_assert(std::size(words) == names);
    Culture c{};
    c.language = language;
    c.salt = salt;
    c.throat = genes;
    for (Word i = 0; i < names; ++i)
        c.waves[i] = acoustic::spoken_word(words[i]);
    return c;
}
inline Genes read_voice(const std::string &path) {
    std::ifstream f(path);
    std::string format, extra;
    Genes g{};
    if (!(f >> format >> g.dendrites >> g.motor) || format != "AXION-RESONATORS-1" || f >> extra)
        throw std::invalid_argument("damaged vocal germline");
    return g;
}
inline Waves read_line(const std::string &s) {
    Waves w{};
    std::istringstream in(s);
    std::string word;
    while (in >> word) {
        if (w.n == voice_span || !(w.wave[w.n] = acoustic::spoken_word(word)))
            throw std::invalid_argument("utterance exceeds the available vocal span");
        ++w.n;
    }
    return w;
}
inline std::string render(const Waves &w) {
    std::string out;
    for (Word j = 0; j < w.n; ++j) {
        if (j)
            out += ' ';
        out += acoustic::spell(w.wave[j]);
    }
    return out;
}
} // namespace axion::life::utterance
