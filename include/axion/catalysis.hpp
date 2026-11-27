#pragma once
#include "capillary_sheet.hpp"

namespace axion::life::capillary {
// Wetting depends on polarity mismatch. A high-valence coating facing a
// low-valence carrier reverses the local sill threshold. Equal compounds are inert.
AX_FN Word conditioned_mineral(Word geology, Word bound, Word carrier) {
    return geology ^ (((bound ^ carrier) & 1) << 4) ^ (bound & ~carrier & 2);
}
struct Reaction {
    Word before, after, carrier, coating, flow;
};
AX_FN Reaction cycle(Word geology, Word bound, Word carrier, Word row) {
    Word law = conditioned_mineral(geology, bound, carrier);
    Word flow = surge(row, law), mobile = flow ? bound : carrier, coating = flow ? carrier : bound;
    return {law, conditioned_mineral(geology, coating, mobile), mobile, coating, flow};
}
} // namespace axion::life::capillary
