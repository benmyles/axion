#pragma once
#include "model.hpp"
#if !defined(__CUDA__)
#include <string_view>
#endif
namespace axion::life {
// Sixteen receptive fields and four-bit efficacies. A signed inherited
// program controls acquisition and loss after actual sensory-motor outcomes.
struct AuditoryLayer {
    Word rule=0x11e011e0u;
    Word forms[16]{},efficacy[2]{},projections[8]{};
};
static_assert(sizeof(AuditoryLayer)==108);
#if !defined(__CUDA__)
inline Word word_digest(std::string_view token) {
    Word hash=2166136261u;
    for (unsigned char c:token) {
        if (c>='A' && c<='Z') c+=('a'-'A');
        hash=(hash^c)*16777619u;
    }
    return hash;
}
#endif
}
