#include "axion/evolution.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace axion {

std::vector<std::uint64_t> lifetime_scores(Engine &engine, const Model &genome, Course course,
                                      Word budget, Word probes, Learning mode) {
    std::vector<std::uint64_t> fitness(genome.animals);
    // Common random numbers make comparisons between lineages paired. A new
    // generation receives a fresh set of layouts, noise and exploratory draws.
    course.common_trials = 1;
    for (Word rule = 0; rule < 2; ++rule) {
        engine.restore(genome);
        Course life = course;
        life.reversed = rule;
        if (is_lesson(life.task)) life.lesson = mix(course.lesson ^ course.seed);
        engine.run(life, 0, budget, mode);
        life.seed ^= 0x3d95bc97u;
        life.novel = 1;
        auto test = engine.run(life, 0, probes, Learning::frozen);
        for (Word i = 0; i < genome.animals; ++i) fitness[i] += test.animals[i].wins;
    }
    return fitness;
}

void breed(Model &m, const std::vector<std::uint64_t> &scores) {
    if (m.animals < 4 || scores.size() != m.animals) throw std::invalid_argument("breeding needs at least four scored animals");
    std::vector<Word> rank(m.animals);
    std::iota(rank.begin(), rank.end(), 0u);
    Word seed = trial_key(m.seed, 0xe1a7b4c9u, m.generation);
    std::sort(rank.begin(), rank.end(), [&](Word a, Word b) {
        if (scores[a] != scores[b]) return scores[a] > scores[b];
        return mix(a ^ seed) < mix(b ^ seed);
    });
    Word survivors = std::max(2u, m.animals / 8);
    std::vector<Weights> next(m.cells.size());
    for (Word animal = 0; animal < m.animals; ++animal) {
        Word key = mix(seed + animal);
        Word a = animal < survivors ? rank[animal] : rank[mix(key) % survivors];
        Word b = animal < survivors ? a : rank[mix(key + 1) % survivors];
        for (Word n = 0; n < m.neurons; ++n) {
            Word mutation = mix(key + n * 0x9e3779b9u);
            Word parent = (mutation >> 31) ? b : a;
            Weights w = m.cells[size_t(parent) * m.neurons + n];
            if (animal >= survivors) {
                if ((mutation & 3) == 0) w.sensory ^= 1u << ((mutation >> 2) & 31);
                if (((mutation >> 7) & 3) == 0) w.local ^= 1u << ((mutation >> 9) & 31);
                if (((mutation >> 14) & 3) == 0) w.remote ^= 1u << ((mutation >> 16) & 31);
            }
            next[size_t(animal) * m.neurons + n] = w;
        }
    }
    m.cells.swap(next);
    m.memory.clear();
    m.maps.clear();
    m.drawings.clear();
    m.archive.clear();
    m.interventions.clear();
    ++m.generation;
    m.cursor = 0;
}
} // namespace axion
