#pragma once
#include "siphon.hpp"
namespace axion::life::siphon {
constexpr Word external_forms = 32, sample_count = 48, phrase_length = 8;
struct Form {
    Count wave = 0;
    Word faint = 0, strong = 0;
};
struct Caption {
    Byte word[phrase_length]{};
    Byte n = 0, address = 0, appearance = 0, context = 0;
};
struct Vocabulary {
    Form forms[external_forms]{};
    Caption examples[sample_count]{};
    Word count = 0, kept = 0, written = 0, signature = 0x43574f52u, extent = sizeof(Vocabulary), padding = 0;
};
struct Sentence {
    Count wave[phrase_length]{};
    Word n = 0;
};
struct Room {
    Scene site{};
    Word seed = 0, step = 0, wire = 0, body = 31, fruit = 0, exposed = 0, context = 4, struck = 0;
    Intent last{};
    Outcome effect{};
};
struct Test {
    Sentence words{};
    Word seed = 0, inlet = 0, filter = 0, angle = 0, wire = 0;
};
struct Report {
    Sentence said{};
    Word recognized = 0, accepted = 0, role = 2, referent = 4, spoken = 0, candidates = 0, queries = 0, answers = 0,
         cost_a = 0, cost_b = 0, food_a = 0, food_b = 0, frames = 0, forms = 0, property = 4, appearance = 4,
         source = 4;
};
#if !defined(__CUDA__)
inline bool valid(const Vocabulary &v) {
    if (!v.count || v.count > external_forms || v.kept > sample_count || v.kept > v.written ||
        v.signature != 0x43574f52u || v.extent != sizeof(v))
        return false;
    for (Word j = 0; j < v.count; ++j) {
        auto &f = v.forms[j];
        Word mask=(f.faint&0x40000000u)?0x7ffffu:4095u;
        if (!f.wave || f.strong>mask || (f.faint&0x3fffffffu)>mask ||
            (!(f.faint&0x80000000u) && ((f.faint|f.strong)&4095)>255))
            return false;
        for (Word k = 0; k < j; ++k)
            if (v.forms[k].wave == f.wave)
                return false;
    }
    for (Word j = 0; j < v.kept; ++j) {
        auto &e = v.examples[j];
        if (!e.n || e.n > phrase_length || e.address >= 4 || e.appearance >= 4 || (e.context & 127) > 4 ||
            (e.context >= 128 && e.n < 2))
            return false;
        for (Word k = 0; k < e.n; ++k)
            if (!e.word[k] || e.word[k] > v.count)
                return false;
    }
    return true;
}
#endif
static_assert(sizeof(Form) == 16 && sizeof(Caption) == 12 && sizeof(Vocabulary) == 1112);
} // namespace axion::life::siphon
