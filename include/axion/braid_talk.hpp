#pragma once
#include "braid_trials.hpp"
#include "braid_view.hpp"
namespace axion::life::braid {
struct Speech {
    Count wave[max_phrase]{};
    Word n = 0;
};
struct Table {
    Word seed = 0, rig[3]{}, paint[3]{}, strands[3]{15, 15, 15}, index = 0, tutor = 8192, source = 8192, tuition = 0,
         food = 0, fees = 0, ready = 0, start = 0, end = 0, ground = 0, proved = 0, draws = 0, intent = all,
         heard_words = 0, sockets[3]{}, armed = 0;
    Speech message{};
    Optics eyes[3]{};
};
struct Exchange {
    Word accepted = 0, before = 0, goal = 0, known = 0, after = 0, paid[3]{}, food[3]{}, tutor = 0, edits = 0,
         samples = 0, issued = 0, revealed = 0, consulted = 0, heard = 0, separate = 0, borrowed = 0, choice = 0,
         attempts = 0, feasible = 0, articulated = 0, spoken_mask = 0;
    Plan work{}, description{};
    Speech output{};
    Word probes[4]{}, effects[4]{};
};
enum Command : Word {
    open_table,
    set_station,
    feed,
    calibrate,
    exhibit,
    teach,
    utter,
    revoice,
    build_law,
    inspect,
    filter,
    continuous,
    guided_pull
};
enum Wire : Word { intact, mute, no_repeat, unchanged, foreign, repeat_probe, unseeing, misarticulate };

} // namespace axion::life::braid

namespace axion::life::braid {
enum Conversation : Word {
    exchange,
    first_view,
    unchanging,
    quiet,
    misheard,
    native_only,
    loan,
    unlent,
    misleading,
    conversation_arms
};
struct Passage {
    Word key, n, utterance, length, focus, public_a[8], public_b[8], wrist[3], rig[3], laws;
    struct Flight {
        Word screen = 0, observed = 0, target = 0, before = 0, after = 0, source = 0, known = 0, human = 0, said = 0,
             n = 0, decoded = 0, plan = 0, steps = 0, queries = 0, tests[4]{}, seen[4]{}, claim = 0, fee = 0, food = 0,
             ceiling = 0, borrowed = 0, other = 0, later_start = 0, later_goal = 0, later_end = 0, later_work = 0,
             later_steps = 0, later_received = 0, native_order = 0;
    } twin[2];
};
} // namespace axion::life::braid
