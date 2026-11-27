#pragma once
#include "axion/hollow.hpp"
namespace axion::life::hollow {
constexpr Word competition = 512, reverse_access = 1024, equal_access = 2048, crossed_motor = 4096,
               treatment_bits = competition | reverse_access | equal_access | crossed_motor;
constexpr Word lineage_tag = 1u << 31;
AX_FN bool may_steer(Word state, Word discharge) {
    return !(state & competition) || (state & equal_access) ||
           (bool(discharge & lineage_tag) != bool(state & reverse_access));
}
} // namespace axion::life::hollow
