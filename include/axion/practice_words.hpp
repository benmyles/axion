#pragma once
#include "practice_tree.hpp"
#include "linkage_words.hpp"
namespace axion::life::practice {
struct Card {
    unsigned char bytes[32]{};
    Word size = 0;
};
struct Bench {
    linkage::Rack physical{};
    Word climate = 0, used = 0, allowance = 8, stage = 0, watch = 4, known = 0;
    Request public_view{};
    Word posterior = 0, remembered = 0, position[4]{}, expected[4]{};
    Card explanation{};
    effort::Speech pending{}, report{};
};
struct Exchange {
    effort::Speech said{}, heard{}, earlier{};
    Word recognised = 0, asked = 0, watch = 4, stroke = 8, seen = 4, spent = 0, food = 0, arrived = 0, steps = 0,
         fitted = 0, decoded = 0, nbytes = 0, local = 8, start = 0, after = 0, meaningful = 0;
};
enum Action : Word {
    enter,
    layout,
    water,
    tuition,
    current_order,
    utter,
    pass,
    cut,
    bad_report,
    alter_request,
    mute_query,
    last_report,
    lock_escrow
};
struct Invitation {
    Word world = 0, fixture = 0, width = 4, weather = 0, practice = 2, wire = 0, wallet = 8, spool = 1, lock = 0,
         care = 255;
    effort::Speech request{}, revision{};
};
struct Record {
    Exchange source{}, inspection{}, amendment{}, second{}, worker{};
    Word glyphs = 0, edges = 0, handles = 0, labels = 0, buttons = 0, reader = 0, before = 0, after = 0, goal = 0,
         care = 0, reserve = 0, tutor = 0, models = 0, spool = 0;
};
struct Panel {
    Mind agent{};
    Context memory[4]{};
    Request target{};
};
} // namespace axion::life::practice
