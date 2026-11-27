#pragma once
#include "braid_episode.hpp"
namespace axion::life::braid {
struct FieldStudent {
    Word names = 0, forms = 0, offered = 0, answered = 0, correct = 0;
    Word changed = 0, both = 0, empty_answers = 0, class_correct = 0;
    Word replies = 0, answer_mask = 0, diagnostic = 0, diagnostic_mask = 0, diagnostic_codes = 0;
};
} // namespace axion::life::braid
