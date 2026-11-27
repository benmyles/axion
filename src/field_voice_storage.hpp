#pragma once
#include "axion/field_voice.hpp"
#include <stdexcept>
namespace axion::life::material {
inline void validate(const FieldLanguage &speech) {
    if (speech.count > 4 || speech.signature != 0x51464c44u || speech.extent != sizeof(speech))
        throw std::runtime_error("invalid question cortex");
    Word evidence = 0;
    for (Word i = 0; i < speech.count; ++i) {
        const auto &q = speech.question[i];
        if (!q.length || q.length > 8 || q.slot >= q.length || q.wave[q.slot] || !q.examples ||
            (q.contradictions & 0x80000000u ? bool(q.contradictions & 0x7ff00000u) : q.contradictions > 65535))
            throw std::runtime_error("invalid acquired question form");
        for (Word k = 0; k < q.length; ++k)
            if (k != q.slot && !q.wave[k])
                throw std::runtime_error("unvoiced question form");
        evidence += q.examples;
    }
    if (evidence != speech.examples)
        throw std::runtime_error("invalid number of question experiences");
}
} // namespace axion::life::material
