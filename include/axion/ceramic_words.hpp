#pragma once
#include "clay.hpp"
#include "siphon_words.hpp"
namespace axion::life::clay {
struct Instrument {
    Tile objects[2]{};
    Word glaze[2]{};
};
struct Hearing {
    Word strokes = 0, glints = 0, mask = 0, acoustic = 0, visible = 0, context = 4, previous = 4, alternatives = 0,
         non_target = 0;
};
struct Kit {
    Instrument instruments[2]{};
    siphon::Animal actors[2]{};
    Word energy[2]{}, organs[2]{};
};
struct Prompt {
    siphon::Test problem{};
    Word actor = 0, object = 0, taps = 1, visibility = 0;
};
struct Answer {
    siphon::Report response{};
    Hearing ear{};
};
} // namespace axion::life::clay

namespace axion::life::press {
enum class Listening : Word { both, prior, no_sight, no_taste, no_topic, fixed };
struct OralTest {
    Word seed=0, rotate=0, offered=15;
    Listening exposure=Listening::both;
    siphon::Sentence first{}, follow{};
};
struct OralResponse {
    siphon::Report first{}, follow{};
    Word demonstrations=0,tastings=0,paid=0,food=0;
};
}
