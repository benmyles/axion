#pragma once
#include "braid_inquiry.hpp"
namespace axion::life::braid {
enum ViewTest : Word {
    aligned_views,
    common_names,
    unchanging_panel,
    unpaired_views,
    wrong_witness,
    send_only,
    receive_only,
    covered_witness,
    random_sockets,
    same_socket,
    turned_lens,
    readapted,
    view_tests
};
constexpr Word view_period = 12;
struct EyeSample {
    unsigned char own = 0, other = 0, asked = 0, heard = 0, replies = 0, first = none, order = 0;
};
struct EyeReply {
    unsigned char opening = none, request = none, sound = none, answer = none, remembered = none;
};
struct ViewCase {
    Word key = 0, board = 0, ends[2]{}, motor = 0, length = 0, rig = 0, wrist = 0, mode = 0, visits = 0, learning = 0,
         arbor = 0, throat = 0;
    Optics glass[2]{};
    Word survivors[2]{}, before_turn[2]{}, hypotheses[2]{}, anchors[2]{};
    EyeSample panels[2][2 * view_period]{};
    EyeReply turns[2][2][8]{}, public_turn[2][8]{};
    unsigned char selected[2][8]{}, coverage[2][8][2]{}, pending[2][8]{}, eligible[2][8]{};
};
} // namespace axion::life::braid
