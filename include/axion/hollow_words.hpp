#pragma once
#include "hollow.hpp"
#include "siphon_words.hpp"
namespace axion::life::hollow {
struct Lexicon {
    siphon::Vocabulary words{};
    Word tag = 0x484f4c57, extent = sizeof(Lexicon);
};
struct Pieces {
    int x[4]{}, y[4]{};
    Word loose = 15;
};
struct Marsh {
    Pieces pieces{};
    Ground ground{};
    Path body{};
    Word gate = 0, stock = 512, held = 4, seen = 0, phase = 0, loose = 0, consumed = 0;
};
struct Topic {
    Word hue = 4, bath = 4;
};
struct Contact {
    Word work = 0, food = 0, returned = 0, held = 4, hue = 4, view = 4, walked = 0, corrected = 0, stock = 0;
};
enum Activity : Word {
    arrive,
    survey,
    hear,
    sip,
    wheel,
    raft,
    recognize,
    departure,
    face,
    lead_step,
    exhibit,
    take_home,
    grasp,
    put_down
};
struct Question {
    Word seed = 0, conditions = 0, rotation = 0, hue = 0, ray = 0, distance = 4, facing = 0;
    siphon::Sentence first{}, follow{}, effect{};
};
struct Answer {
    siphon::Report spoken[3]{};
    Word cost = 0, memories = 0, corrected = 0;
};
} // namespace axion::life::hollow
