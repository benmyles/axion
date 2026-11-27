#pragma once
#include "linkage_words.hpp"
#include "rattle.hpp"
namespace axion::life::rattle {
enum Action : Word { assemble, divert, listen_word, relay, pull, practise, view };
struct Table {
    Word seed = 0, round = 0, tick = 0, diverted = 0, stage = 0, stock[2]{}, source = 8192, eaten = 0, tags[2]{},
         waves[2]{}, drive = 0, asked = 0, answer = silence, decision = 0, spoken = 0, opening = 0, crossed = 0;
};
struct Exchange {
    effort::Speech request{}, report{}, reply{};
    Word understood = 0, paid = 0, food = 0, inquiries = 0, reports = 0, pulls = 0, upstream = 0, changed = 0,
         grounded = 0, writes = 0, switches = 0, ended = 0;
};
} // namespace axion::life::rattle

namespace axion::life::rattle {
struct Spoken {
    Word rounds = 0, voiced = 0, reports = 0, asked = 0, correct = 0, earned = 0, choices[2]{}, targets[2]{},
         requests[2]{}, cited = 0, causal = 0, other_pan = 0, decoded = 0, understood = 0, verified = 0, stock = 0,
         paid = 0, experienced = 0;
};
} // namespace axion::life::rattle
