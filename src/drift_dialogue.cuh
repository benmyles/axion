#include "axion/drift.hpp"
namespace axion::life::braid {
// One lane follows all four turns. Each control begins with the same acquired
// sound contacts. No diagnostic result is sent back into the food web.
KERNEL void drift_dialogue(const drift::Encounter *observations, Word size, const Narration *learned,
                           const drift::Conversation *course, drift::Answers *out) {
    Word i = tid();
    if (i >= size * 12)
        return;
    Word record = i / 12, mode = (i / 4) % 3, site = i & 3;
    const auto &q = observations[record].view;
    if (!q.stamp)
        return;
    auto m = *learned;
    experience(m, mode == 2 ? Episode{} : q.scene);
    m.topic = m.reply = m.pending = 0;
    const auto &c = *course;
    auto line = fill_question(c.anchor, c.slot, c.names.wave[site]);
    Word encoded = 0;
    for (Word j = 0; j < 4; ++j) {
        if (mode == 1 && j)
            m.topic = 0;
        const auto &sounds = j == 0 ? line : j == 1 ? c.actual : j == 2 ? c.before : c.inverse;
        auto answer = question(m, sounds, false);
        Word code = 8;
        if (answer.answered)
            for (Word k = 0; k < 8; ++k)
                if (answer.words.wave[0] == c.names.wave[k]) {
                    code = k;
                    break;
                }
        encoded |= code << (4 * j);
    }
    out[record].turns[mode][site] = encoded;
}
} // namespace axion::life::braid
