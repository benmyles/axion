#pragma once
#include "braid_discourse.hpp"

namespace axion::life::braid {
enum ReportArm : Word {
    with_witness,
    silent_witness,
    echo_question,
    incorrect_witness,
    missing_context,
    forget_answer,
    private_scene,
    sighted,
    untaught,
    report_arms
};
AX_FN Word held_subject(Word k) { return 3 - (k >> 1) + 4 * (k & 1); }
struct Probe {
    unsigned char before = none, asked = 0, reframed = 0, witness = none, answer = none, reverse = none,
                  reverse_from_report = 0, repeat = none;
};
struct Interrogation {
    Word key = 0, rig = 0, wrist = 0, stream = 0, strokes = 0, board = 0, prefix = 0, aim = 0, ends[2]{}, nouns[2]{},
         forms[2]{}, training = 0;
    Word frame_answers[2][4]{}; // up to four held-out referents, one byte each
    Probe trial[report_arms][4][2]{};
};
} // namespace axion::life::braid
