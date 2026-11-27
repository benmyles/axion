#pragma once
#include "runnel.hpp"
#include "effort_words.hpp"
namespace axion::life::runnel {
struct Chamber {
    unsigned short water = 0, carbon = 0;
};
struct Flume {
    Word stone = 0, world = 0, layout = 0, span = 0, cursor = 0, stage = 0, cue = 0, direction = 0, tutor = 0,
         tuition = 0, clock = 0, swallowed = 0;
    Word question = 0, unavailable = 0, indirect = 1, retry = 0, referrals = 0, testimonies = 0, installed = 0,
         abstained = 0;
    Word source_food = 0, outlet_in = 0, outlet_back = 0;
    Byte fold[max_span]{};
    Chamber cells[max_span + 1]{};
    effort::Speech pending{};
};
enum Command : Word {
    open_flume,
    layout_flume,
    school_flume,
    order_flume,
    speak_flume,
    pass_flume,
    build_flume,
    start_pump,
    clear_flume,
    view_flume,
    sample_flume
};
struct Dispatch {
    effort::Speech input{}, reply{};
    Word understood = 0, asked = 0, reported = 0, learned = 0, paid = 0, cached = 0, meal = 0, attempts = 0,
         installed = 0, water = 0, food = 0, stock_water = 0, stock_carbon = 0, report_pose = 4, report_effect = 4,
         example = 16, transform = 8;
};
struct Colloquy {
    effort::Speech order{}, first{}, enquiry{}, testimony{}, procedure{}, reason{}, later{};
    Word started = 0, heard = 0, said = 0, learned = 0, installed = 0, succeeded = 0, paid = 0, food = 0, repaired = 0,
         later_success = 0, later_food = 0, later_paid = 0, later_ask = 0, different = 0, opening = 0, other = 0,
         corner = 0, reference = 0, direction = 0, geology = 0, frame = 0, other_frame = 0, public_pose = 4,
         later_pose = 4, reason_via = 16, rotation = 8, later_via = 16, later_rotation = 8, consultations = 0,
         tuition = 0;
};
struct Lexicon {
    effort::Speech names[2]{}, verbs[2]{};
};
AX_FN Word landmark(Word layout, Word j) { return mix(layout + j * 28657); }
AX_FN Word local_cue(Word pin) { return (pin >> 19) & 15; }
AX_FN Word upstream(Word pin) { return (pin >> 8) & 1; }
} // namespace axion::life::runnel
