#pragma once
#include "effort_bundle.hpp"
#include "siphon_words.hpp"
namespace axion::life::effort {
enum Gesture : Word { alternation, compare, explain, remove, enact, material, fold, unrecognized };

struct Speech {
    Count sounds[32]{};
    Word n = 0;
};
struct Topic {
    Bundle offer[2]{};
    Word ready = 0, chosen = 2;
};
struct Board {
    Word seed = 0, fruit = 0, drag = 0, head = 0, ray = 0, capital[4]{}, consumed = 0, loose = 0, work = 0;
    Bundle lanes[2]{}, contrast[2]{};
    Word focus = 0, opened = 0, examined = 0, trigger = 0, shown = 0, round = 0, pigments = 0, pose = 0, handed = 0,
         handle = 0;
};
struct SurfaceQuestion {
    Word seed = 0, mode = 0, turns = 0;
    Speech first{}, second{};
};
struct Reply {
    Count word = 0;
    int value[2]{};
    Word op = unrecognized, choice = 2, feature = 8, decisive = 0, ready = 0, accepted = 0;
};
struct SurfaceAnswer {
    Reply first{}, second{};
    Word before = 0, after = 0, paid = 0, demonstrations = 0;
};
struct Bite {
    Word paid = 0, food = 0, loose = 0, steps = 0, reached = 0;
};
struct Dialogue {
    Word seed = 0, mode = 0, turn = 0;
    Bundle real[2]{};
    Speech first{}, why{}, edit{}, repeat{};
};
struct Transcript {
    Reply replies[4]{};
    Bite action{};
    Word sampled = 0, practice = 0, face = 0;
};
} // namespace axion::life::effort
