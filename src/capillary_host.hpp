#pragma once
#include "axion/capillary_manual.hpp"
#include "axion/context_study.hpp"
#include "axion/material_signal.hpp"
#include "axion/compound_sign.hpp"
#include "axion/signal_voice.hpp"
#include "narrative_host.hpp"

namespace axion::life::capillary {
inline Body phenotype(const storage::AgentImage &a) {
    auto o = utterance::senses(a);
    Word bands = a.grows_cells ? a.cortex.active : living_tiles(a.brain.tiles, a.brain.genes.development);
    Word mask = pressure_regulator(a.resident.germ, bands);
    return {o.learning, o.cells, a.resident.germ.pigments[2],        a.resident.germ.grip, o.eyes, o.limbs,
            o.mouth,    mask,    a.channels.adaptive == 3 ? mask : 0,a.resident.germ.stomach};
}
inline Count signal_ear(const storage::AgentImage &a){auto &g=a.resident.germ.hearing;return g.low|(Count(g.high)<<32);}
inline VoiceSchool voice_environment(Word dialect){
    VoiceSchool s{};const char *tones[4]={"tap","hum","tick","ring"};
    const char *directions[4]={"west","east","north","south"};
    for(Word i=0;i<4;++i){s.sound[0][i]=acoustic::spoken_word(tones[dialect?(i^1):i]);s.sound[1][i]=acoustic::spoken_word(directions[i]);}
    s.prompt[0]=utterance::read_line("What did you see");s.prompt[1]=utterance::read_line("Where is the food");return s;
}
inline Gloss language(Word dialect) {
    const char *terms[]{"wick",  "water", "sill",    "film",  "amber",  "jade",   "plum",   "teal",
                        "no",    "yes",   "west",    "north", "why",    "did",    "the",    "turn",
                        "could", "have",  "made",    "it",    "what",   "would",  "happen", "without",
                        "and",   "let",   "through", "via",   "passed", "before", "change", "alone",
                        "yield", "which", "from",    "into",  "two",    "parts",  "acted",  "together"};
    static_assert(std::size(terms) == words);
    Gloss book{};
    book.dialect = dialect;
    if (dialect) {
        terms[0] = "vein";
        terms[1] = "rain";
        terms[2] = "lip";
        terms[3] = "glaze";
    }
    for (Word i = 0; i < words; ++i)
        book.sound[i] = acoustic::spoken_word(terms[i]);
    return book;
}
inline LanguageRack rack_language(Word dialect) {
    LanguageRack r{};
    const char *verbs[]{"place", "lift"}, *parts[]{"wick", "water", "sill", "film"}, *rows[]{"upper", "lower"},
        *cols[]{"left", "middle", "right"};
    if (dialect) {
        verbs[0] = "seat";
        verbs[1] = "raise";
        parts[0] = "vein";
        parts[1] = "rain";
        parts[2] = "lip";
        parts[3] = "glaze";
    }
    for (Word i = 0; i < 2; ++i) {
        r.verb[i] = acoustic::spoken_word(verbs[i]);
        r.row[i] = acoustic::spoken_word(rows[i]);
    }
    for (Word i = 0; i < 4; ++i)
        r.substance[i] = acoustic::spoken_word(parts[i]);
    for (Word i = 0; i < 3; ++i)
        r.column[i] = acoustic::spoken_word(cols[i]);
    r.then = acoustic::spoken_word("then");
    r.prompt = utterance::read_line(dialect ? "How shall we build this channel" : "Tell me how to build a new channel");
    return r;
}
} // namespace axion::life::capillary
