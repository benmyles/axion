#pragma once
#include "engine.hpp"

namespace axion {

// Two independent lifetimes start from identical inherited weights. Each gets
// a different food rule. Both are scored with frozen plasticity on unseen trials.
std::vector<std::uint64_t> lifetime_scores(Engine &engine, const Model &genome, Course course,
                                      Word budget, Word probes, Learning mode);
void breed(Model &population, const std::vector<std::uint64_t> &scores);
} // namespace axion
