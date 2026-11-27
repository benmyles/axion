#pragma once
#include "arena.hpp"
#include "workshop.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace axion {

struct Model {
    Word animals, neurons, seed, cursor, generation;
    Course course;
    std::vector<Weights> cells;
    Learning learning = Learning::full;
    std::vector<MemoryBank> memory{};
    std::vector<PlaceMemory> maps{};
    std::vector<DrawingMemory> drawings{};
    std::vector<Association> archive{};
    std::vector<InterventionMemory> interventions{};
};
void resize_archive(Model &model, Word slots);
void validate_course(const Course &course);
bool valid_shape(Word animals, Word neurons);
void check_storage(const Model &model, Word animals, Word neurons);
Model hatch(Word animals, Word neurons, Word seed, Course course);
void save_model(const Model &model, const std::filesystem::path &path);
Model load_model(const std::filesystem::path &path);

struct Totals {
    std::uint64_t wins{}, ticks{}, spikes{}, matches{};
    Word image{}, padding{};
};

struct Run {
    std::vector<Totals> animals;
    double seconds{}, wall_seconds{};
};

enum class Layout { automatic, warp, block, grid };

class Engine {
public:
    virtual ~Engine() = default;
    virtual Run run(Course course, Word first, Word episodes, Learning learning) = 0;
    virtual void restore(const Model &model) = 0;
    virtual std::string resources(Learning) const { return {}; }
    virtual void download(Model &model) = 0;
    virtual std::string name() const = 0;
};

std::unique_ptr<Engine> make_cpu(const Model &model, unsigned threads);
std::unique_ptr<Engine> make_gpu(const Model &model, Layout layout);

inline constexpr Word chunk_episodes = 32;

} // namespace axion
