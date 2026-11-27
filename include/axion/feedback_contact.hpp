#pragma once
#include "apprentice.hpp"
#include "braid_discourse.hpp"

namespace axion::life::braid {
struct ContactPractice {
    regulation::Source source;
    Word mode, index, cohort;
    unsigned short *schedule = nullptr;
    Word lesson = 0, updates = 0, released = 0, attempts = 0, matches = 0;
    AX_FN Word hear(Narration &memory, Genes genes, const Speech &line, Count heard) {
        // The pupil tries before the teacher replies. Keep the original public
        // topic and reply available when the teacher's example is encoded.
        Word access = mode & 255u;
        auto topic = memory.topic, reply = memory.reply, pending = memory.pending;
        auto prediction = question(memory, line, false);
        memory.topic = topic;
        memory.reply = reply;
        memory.pending = pending;
        bool tried = prediction.answered, match = tried && prediction.words.wave[0] == heard;
        attempts += tried;
        matches += match;
        Word address = lesson++ * cohort + index;
        Word pulse = 0;
        Word edits = receive_form(memory, genes, line, heard, [&](Word old, Word conflict, Word proposed) {
            bool sensory = access != apprentice::quiet;
            pulse = regulation::discharge(source.program, source.cells, old, conflict, proposed, sensory && tried,
                                          sensory && match);
            if (access == apprentice::unrestricted)
                pulse = all;
            else if (access == apprentice::displaced)
                pulse = regulation::rolled(pulse, 7);
            else if (access == apprentice::yoked)
                pulse = schedule[(lesson - 1) * cohort + ((index + cohort / 2) % cohort)];
            released += __builtin_popcount(pulse);
            auto next = regulation::candidate(source.program, source.cells.spike, old, proposed,
                                              (mode & apprentice::remodel) && access != apprentice::unrestricted);
            return regulation::apply(old, next, pulse);
        });
        if (schedule && access == apprentice::inherited)
            schedule[address] = static_cast<unsigned short>(pulse);
        updates += edits;
        return edits;
    }
};
} // namespace axion::life::braid
