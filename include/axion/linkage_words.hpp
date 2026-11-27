#pragma once
#include "linkage_design.hpp"
#include "effort_words.hpp"
namespace axion::life::linkage {
struct Bench {
    Rig rig{};
    Word sockets = 0, spool = 1, pose = 0;
    Pattern lock{};
};
struct Rack {
    Bench bench{};
    Material layers[4]{};
    Pattern request{};
    Word world = 0, fixture = 0, width = 0, labels = 0, buttons = 0, reader = 0, block = 0, has_goal = 0,
         redeemed = 0, round = 0;
    Word stock = 256, tutor = 1024, swallowed = 0, demonstrated = 0;
    effort::Speech proposal{};
};
struct Tongue {
    siphon::Vocabulary names{}, operators{};
    siphon::Animal body{};
    RecollectionGenes law{};
};
struct Uttered {
    effort::Speech words{};
    Word understood = 0, planned = 0, executed = 0, spent = 0, food = 0, aligned = 0, associations = 0, shown = 0,
         steps = 0, inserted = 0, start = 0, after = 0, goal = 0, care = 0, local_motors = 0, world_motors = 0,
         local_tail = 0, world_tail = 0, refused = 0;
};
enum Task : Word {
    enter_rack,
    exchange_rig,
    show_practice,
    own_practice,
    request_words,
    pass_words,
    drink,
    read_order
};
} // namespace axion::life::linkage
namespace axion::life::linkage {
struct Request {
    Word world = 0, fixture = 0, width = 4, tuition = 2, wire = 0;
    effort::Speech first{}, revision{}, why{};
};
struct Conversation {
    Uttered first{}, amended{}, reason{}, peer{};
    Word labels = 0, buttons = 0, order = 0, joints = 0, glyphs = 0, grip = 0, barred = 0, allowed = 0, learnt = 0;
};
} // namespace axion::life::linkage
