#pragma once
#include "rattle.hpp"
namespace axion::life::rattle {
enum Treatment : Word {
    open,
    deaf,
    locked,
    mistimed,
    anonymous,
    first_only,
    cut_upstream,
    cut_sidepool,
    wild_drums,
    invited,
    away,
    lottery,
    old_attention,
    quiet_choice,
    chorus,
    chorus_deaf,
    chorus_locked,
    chorus_plain,
    chorus_invited,
    treatments
};
AX_FN bool one_ear(Word arm) { return (arm >= invited && arm <= quiet_choice) || arm == chorus_invited; }
constexpr Word seasons = 3, probes = 8;
struct Drum {
    Genes source[2];
    Word polarity, tones;
};
struct Trial {
    Word genes[4], upstream, orientation, tags, drums[2];
    struct Season {
        Word contact[2], state[2], goals[2], choices[2], packets[2][2], food[2], first[2], last[2], edits[2], turns[2],
            heard[2], requests[2], consulted[2];
    } season[seasons];
};
struct Family {
    Genes genes;
    Word food;
};
} // namespace axion::life::rattle
