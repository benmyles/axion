#include "axion/engine.hpp"
#include "axion/planning.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

namespace axion {
namespace {

struct SerialMemory {
    MemoryBank &bank;
    RecallState state;
    Archive archive;

    SerialMemory(MemoryBank &b, Archive a) : bank(b), archive(a) { state.load(b); }
    Votes read(const Engram &q, Recall mode) const {
        Votes sum;
        for (Word i = 0; i < state.used; ++i) sum.add(recall_row(q, bank.rows[i], mode));
        if (archive.entries) archive.vote(sum, q);
        return sum;
    }
    SpikeCode translate(SpikeCode cue) const {
        int best = -1;
        for (Word i = 0; i < state.used; ++i)
            best = std::max(best, binding_score(cue, bank.rows[i], i));
        if (Word durable = archive.recall(cue.lo, best >> 5)) return {durable, 0};
        return best >= 32 * 56 ? bank.rows[31 - (best & 31)].item[3] : SpikeCode{};
    }
    void write(const Engram &row) {
        if (archive.entries && state.used == memory_rows) archive.consolidate(bank.rows[state.next]);
        bank.rows[state.next] = row;
    }
};

struct SerialPlaces {
    PlaceMemory &map;
    int identify(SpikeCode code, bool learn) {
        if (!(code.lo | code.hi)) return -1;
        int best = -1;
        for (Word i = 0; i < map.used; ++i) best = std::max(best, best_match(code, map.places[i], i));
        int slot = match_slot(best, learn);
        if (slot < 0 && learn && map.used < 32) {
            slot = int(map.used++);
            map.places[slot] = {code, {}};
        }
        return slot;
    }
    Doors doors(int slot) const { return map.places[slot].exits; }
    void link(int from, int action, int to) { map.places[from].exits.arm[action] = 1u << to; }
    Word frontier() const {
        Word mask = 0;
        for (Word i = 0; i < map.used; ++i)
            if (!doors(int(i)).arm[0] || !doors(int(i)).arm[1]) mask |= 1u << i;
        return mask;
    }
    Word predecessors(Word active) const {
        Word mask = 0;
        for (Word i = 0; i < map.used; ++i) {
            Doors exits = doors(int(i));
            if (active & (exits.arm[0] | exits.arm[1])) mask |= 1u << i;
        }
        return mask;
    }
};

struct SerialSearch {
    const InterventionMemory *experience;
    Word slots[8];
    Word literals[8][32];
    Word length;

    int score(Word input, Word target, bool perturb, int salt) const {
        if (experience) {
            int best = 32 * 33;
            for (int candidate = 0; candidate < 32; ++candidate) {
                Word image = input ^ (perturb ? 1u << candidate : 0u);
                for (Word k = 0; k < length; ++k) {
                    Word next = 0;
                    Inputs x(image);
                    for (int j = 0; j < 32; ++j) {
                        int bit = experience->circuit[slots[k]].predict(image, j);
                        Word f = literals[k][j];
                        if (bit < 0) bit = ((x.branch[f >> 6] >> (f & 31)) ^ (f >> 5)) & 1;
                        next |= Word(bit) << j;
                    }
                    image = next;
                }
                best = std::min(best, 32 * population(image ^ target) + (candidate ^ salt));
            }
            return best;
        }
        Word planes[32], next[32];
        for (int i = 0; i < 32; ++i)
            planes[i] = (0u - ((input >> i) & 1)) ^ (perturb ? 1u << i : 0u);
        for (Word k = 0; k < length; ++k) {
            for (int j = 0; j < 32; ++j) {
                Word f = literals[k][j];
                next[j] = apply_literal(f, planes[f & 31], planes[neighbor_of(f)]);
            }
            std::copy_n(next, 32, planes);
        }
        // Exchange one pair of row/column address bits at a time.
        for (int d = 1; d < 32; d *= 2) {
            Word keep = ~0u / ((1u << d) + 1);
            for (int j = 0; j < 32; ++j) {
                int other = j ^ d;
                if (other < j) continue;
                Word left = planes[j], right = planes[other];
                planes[j] = (left & keep) | ((right & keep) << d);
                planes[other] = (right & ~keep) | ((left & ~keep) >> d);
            }
        }
        int best = 32 * 33;
        for (int j = 0; j < 32; ++j)
            best = std::min(best, 32 * population(planes[j] ^ target) + (j ^ salt));
        return best;
    }
};

struct SerialConstraints {
    LearnedGate gates[4][32];
    Word slots[8], choices[256];
    unsigned short levels[9][32]{};

    Implications filter(int stage, Clamps in, Clamps out) const {
        Implications result{};
        for (int cell = 0; cell < 32; ++cell) {
            auto r = gates[slots[stage]][cell].filter(in, out, cell, stage);
            result.input.on |= r.input.on;
            result.input.off |= r.input.off;
            result.output.on |= r.output.on;
            result.output.off |= r.output.off;
            result.choice = std::min(result.choice, r.choice);
        }
        return result;
    }
    void stamp(int stage, Word bits, int depth) {
        for (; bits; bits &= bits - 1) levels[stage][__builtin_ctz(bits)] = static_cast<unsigned short>(depth);
    }
    Word retract(int stage, int depth) {
        Word erased = 0;
        for (int cell = 0; cell < 32; ++cell) {
            if (levels[stage][cell] >= depth) {
                levels[stage][cell] = 0;
                erased |= 1u << cell;
            }
        }
        return erased;
    }
    void save(int depth, Word choice) { choices[depth] = choice; }
    Word saved(int depth) const { return choices[depth]; }
};

struct SerialPredictor {
    struct Program {
        Word cue[8];
        void put(Word index, Word token) { cue[index] = token; }
        Word get(Word index) const { return cue[index]; }
    };
    DrawingMemory *memory;
    InterventionMemory *experience;
    Word previous(Word cue, Word fallback) const {
        int slot = experience && memory ? memory->palette.find(cue) : -1;
        if (slot < 0 || !experience->circuit[slot].observed) return fallback;
        return experience->circuit[slot].previous_input;
    }
    int uncertainty(Word cue, Word a, Word b) const {
        int slot = experience && memory ? memory->palette.find(cue) : -1;
        if (slot < 0) return 0;
        const auto &table = experience->circuit[slot];
        int total = 0;
        for (int j = 0; j < 32; ++j) total += table.unknown(a, j) + table.unknown(b, j);
        return total;
    }
    Transition residual(const Span &span, Word image, Word result) const {
        Word active = image ^ span.anchor;
        Transition delta{active, result ^ span.response};
        while (active) {
            auto row = span.row[__builtin_ctz(active)];
            delta.input ^= row.input;
            delta.output ^= row.output;
            active &= active - 1;
        }
        return delta;
    }
    Word apply(Word cue, Word picture, Recall mode) const {
        int slot = memory ? memory->palette.find(cue) : -1;
        if (mode == Recall::cortical || slot < 0) return picture;
        const Span &span = memory->spans[slot];
        if (mode == Recall::associative && span.confirmed >= 2) {
            auto delta = residual(span, picture, 0);
            if (!delta.input) return delta.output;
        }
        Inputs input(picture);
        Word out = 0;
        for (int j = 0; j < 32; ++j) {
            int value = experience && mode == Recall::associative ? experience->circuit[slot].predict(picture, j) : -1;
            if (value < 0) value = memory->circuit[slot].fire(input, j, mode == Recall::content ? 1 : branches);
            out |= Word(value) << j;
        }
        return out;
    }
    bool plan(Word target, const Program &program, Word length, Word nodes, Word &answer) const {
        SerialConstraints search;
        Word loaded = 0;
        for (Word k = 0; k < length; ++k) {
            int slot = memory->palette.find(program.get(k));
            if (slot < 0) return false;
            search.slots[k] = Word(slot);
            if ((loaded >> slot) & 1) continue;
            for (int j = 0; j < 32; ++j)
                if (!search.gates[slot][j].read(*memory, experience, slot, j)) return false;
            loaded |= 1u << slot;
        }
        return solve_program(search, target, length, nodes, answer);
    }
    Word imagine(Word target, const Program &program, Word length, Word nodes) const {
        Word answer;
        if (nodes && plan(target, program, length, nodes, answer)) return answer;
        SerialSearch search;
        search.experience = experience;
        search.length = length;
        for (Word k = 0; k < length; ++k) {
            int slot = memory->palette.find(program.get(k));
            if (slot < 0) return target;
            search.slots[k] = Word(slot);
            for (int j = 0; j < 32; ++j)
                search.literals[k][j] = memory->circuit[slot].hypothesis(j);
        }
        return improve_input(search, target);
    }
    Word construct(Word target, const Program &program, Word length, Recall mode, Word nodes) const {
        if (!memory || mode != Recall::associative) return target;
        Word column[32], source[32], bias = 0;
        for (int j = 0; j < 32; ++j) source[j] = column[j] = 1u << j;
        for (Word k = 0; k < length; ++k) {
            int slot = memory->palette.find(program.get(k));
            if (slot < 0) return target;
            const auto &span = memory->spans[slot];
            if (span.confirmed < 2) return imagine(target, program, length, nodes);
            for (int j = 0; j < 32; ++j)
                if (span.row[j].input != (1u << j)) return imagine(target, program, length, nodes);
            bias = residual(span, bias, 0).output;
            for (Word &col : column) {
                Word next = 0;
                for (Word active = col; active; active &= active - 1)
                    next ^= span.row[__builtin_ctz(active)].output;
                col = next;
            }
        }
        Word remaining = target ^ bias, answer = 0, free = ~0u;
        for (int bit = 0; bit < 32 && remaining; ++bit) {
            Word mask = 1u << bit;
            int pivot = -1;
            for (int j = 0; j < 32; ++j)
                if ((free & (1u << j)) && (column[j] & mask)) { pivot = j; break; }
            if (pivot < 0) continue;
            free &= ~(1u << pivot);
            if (remaining & mask) { remaining ^= column[pivot]; answer ^= source[pivot]; }
            for (int j = 0; j < 32; ++j) {
                if ((free & (1u << j)) && (column[j] & mask)) {
                    column[j] ^= column[pivot];
                    source[j] ^= source[pivot];
                }
            }
        }
        return remaining ? target : answer;
    }
    void observe(Word cue, Word picture, Word target, Recall mode) {
        int slot = memory->palette.find(cue);
        bool fresh = slot < 0;
        if (fresh) slot = memory->palette.recruit(cue);
        if (experience) {
            auto &table = experience->circuit[slot];
            Word intervention = table.intervention(picture, fresh);
            Word changed = target ^ table.previous_output;
            for (int j = 0; j < 32; ++j)
                table.witness(j, picture, (target >> j) & 1, (changed & (1u << j)) ? intervention : 0, fresh);
            table.previous_input = picture;
            table.previous_output = target;
            table.observed = 1;
        }
        auto &span = memory->spans[slot];
        if (fresh) span = {{}, picture, target, 0};
        else if (mode == Recall::associative) {
            auto delta = residual(span, picture, target);
            if (delta.input) {
                int pivot = __builtin_ctz(delta.input);
                for (int j = 0; j < 32; ++j) update_pivot(span.row[j], j, pivot, delta);
            } else if (delta.output) span = {{}, picture, target, 0};
            else span.confirmed = std::min(2u, span.confirmed + 1);
        }
        Inputs input(picture);
        for (int j = 0; j < 32; ++j)
            memory->circuit[slot].observe(input, j, (target >> j) & 1, mode == Recall::content ? 1 : branches, fresh);
    }
};

template<Learning L>
class SerialNetwork {
    std::vector<Cell<L>> cells;
    std::vector<Word> previous, next;
public:
    explicit SerialNetwork(int n) : cells(n), previous(n / 32), next(n / 32) {}

    void reset(Word key) {
        std::fill(previous.begin(), previous.end(), 0u);
        for (int i = 0; i < int(cells.size()); ++i) cells[i].reset(key, i);
    }

    void tick(Word input) {
        int sensors = population(input);
        int tiles = int(previous.size());
        for (int tile = 0; tile < tiles; ++tile) {
            Word fired = 0;
            Word local = previous[tile];
            for (int lane = 0; lane < 32; ++lane) {
                int i = tile * 32 + lane;
                Word remote = previous[remote_tile(tile, i, tiles)];
                if (cells[i].tick(i, input, local, remote, active_cost(sensors, local, remote)))
                    fired |= 1u << lane;
            }
            next[tile] = fired;
        }
        previous.swap(next);
    }

    SpikeCode signature(Word receptors) const {
        int active = population(receptors);
        SpikeCode code{receptors, 0};
        for (unsigned lane = 0; lane < 32; ++lane)
            code.hi |= Word(onset_spike(cells[cells.size()/2 + lane].weights.local, receptors, active)) << lane;
        return code;
    }

    WideSums read() const {
        WideSums sum{};
        for (const auto &cell : cells) {
            Sums x = cell.read();
            sum.left += x.left;
            sum.right += x.right;
            sum.mass += x.mass;
        }
        return sum;
    }

    void commit(int action, int q) { for (auto &cell : cells) cell.commit(action, q); }
    void reinforce(int error) { for (auto &cell : cells) cell.reinforce(error); }

    Word spike_count() const {
        Word count = 0;
        for (const auto &cell : cells) count += cell.spikes;
        return count;
    }

    Counts batch(Weights *weights, MemoryBank *bank, PlaceMemory *map, DrawingMemory *drawing, InterventionMemory *intervention, Archive archive, Course course, Word animal, Word first, Word count) {
        for (size_t i = 0; i < cells.size(); ++i) {
            cells[i].weights = weights[i];
            cells[i].spikes = 0;
        }
        Counts totals;
        if (is_drawing(course.task)) {
            SerialPredictor predictor{drawing, intervention};
            totals = draw_trials<L>(predictor, course, animal, first, count);
        } else if (course.task == Task::routes || course.task == Task::errand) {
            PlaceMemory empty_map{};
            MemoryBank empty_words{};
            SerialPlaces places{map ? *map : empty_map};
            SerialMemory words(bank ? *bank : empty_words, archive);
            totals = navigate<L>(*this, places, words, course, animal, first, count);
            if constexpr (L != Learning::frozen) if (bank) words.state.save(*bank);
        } else if (bank || course.task == Task::detector) {
            MemoryBank empty{};
            SerialMemory memory(bank ? *bank : empty, archive);
            if (course.task == Task::detector) totals = detector_trials<L>(*this, memory, course, animal, first, count);
            else totals = study<L>(*this, memory, course, animal, first, count);
            if constexpr (L != Learning::frozen) if (bank) memory.state.save(*bank);
        } else if (is_lesson(course.task)) totals = study_cortex<L>(*this, course, animal, first, count);
        else totals = simulate<L>(*this, course, animal, first, count);
        if constexpr (L != Learning::frozen)
            for (size_t i = 0; i < cells.size(); ++i) weights[i] = cells[i].weights;
        return totals;
    }
};

class Cpu final : public Engine {
    Model model;
    unsigned threads;

    template<Learning L>
    void execute(Run &result, Course course, Word first, Word episodes) {
        bool routes = (course.task == Task::routes || course.task == Task::errand) && course.recall != Recall::cortical;
        if (routes && model.maps.empty() && L != Learning::frozen) model.maps.resize(model.animals);
        bool drawing = is_drawing(course.task);
        if (drawing && course.recall != Recall::cortical && model.drawings.empty() && L != Learning::frozen)
            model.drawings.resize(model.animals);
        if (drawing && course.experiments != Course::Experiments::off && course.recall != Recall::cortical
            && model.interventions.empty() && L != Learning::frozen) model.interventions.resize(model.animals);
        bool recall = !drawing && course.task != Task::routes && is_lesson(course.task) && course.recall != Recall::cortical;
        if (recall && model.memory.empty() && L != Learning::frozen) model.memory.resize(model.animals);
        Word slots = model.course.archive_slots;
        if (recall && slots && model.archive.empty() && L != Learning::frozen)
            model.archive.resize(size_t(slots) * model.animals);
        std::atomic<Word> index{0};
        auto worker = [&] {
            SerialNetwork<L> net(drawing ? 0 : model.neurons);
            for (Word animal; (animal = index.fetch_add(1, std::memory_order_relaxed)) < model.animals;) {
                auto *weights = model.cells.data() + size_t(animal) * model.neurons;
                DrawingMemory *paint = drawing && !model.drawings.empty() ? &model.drawings[animal] : nullptr;
                auto *intervention = model.interventions.empty() ? nullptr : &model.interventions[animal];
                PlaceMemory *map = routes && !model.maps.empty() ? &model.maps[animal] : nullptr;
                MemoryBank empty{};
                MemoryBank *bank = recall ? (model.memory.empty() ? &empty : &model.memory[animal]) : nullptr;
                Archive archive{recall && !model.archive.empty() ? model.archive.data() + size_t(animal) * slots : nullptr, slots};
                auto &t = result.animals[animal];
                for (Word done = 0; done < episodes;) {
                    Word chunk = course.survey_steps && model.neurons > 4096 ? 4 : chunk_episodes;
                    Word count = std::min(chunk, episodes - done);
                    auto c = net.batch(weights, bank, map, paint, intervention, archive, course, animal, first + done, count);
                    t.wins += c.wins;
                    t.ticks += c.ticks;
                    t.spikes += c.spikes;
                    t.matches += c.matches;
                    t.image = c.image;
                    done += count;
                }
            }
        };
        std::vector<std::jthread> workers;
        for (unsigned i = 1; i < threads; ++i) workers.emplace_back(worker);
        worker();
    }

public:
    Cpu(const Model &m, unsigned requested) : model(m) {
        threads = std::min(m.animals, requested ? requested : std::max(1u, std::thread::hardware_concurrency()));
    }

    Run run(Course course, Word first, Word episodes, Learning learning) override {
        validate_course(course);
        Run result{std::vector<Totals>(model.animals)};
        auto start = std::chrono::steady_clock::now();
        switch (learning) {
            case Learning::frozen: execute<Learning::frozen>(result, course, first, episodes); break;
            case Learning::motor: execute<Learning::motor>(result, course, first, episodes); break;
            case Learning::no_scaling: execute<Learning::no_scaling>(result, course, first, episodes); break;
            case Learning::full: execute<Learning::full>(result, course, first, episodes); break;
        }
        result.seconds = result.wall_seconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
        return result;
    }
    void restore(const Model &source) override {
        check_storage(source, model.animals, model.neurons);
        model.cells = source.cells;
        model.memory = source.memory;
        model.maps = source.maps;
        model.drawings = source.drawings;
        model.archive = source.archive;
        model.interventions = source.interventions;
        model.course.archive_slots = source.course.archive_slots;
    }
    void download(Model &out) override {
        check_storage(out, model.animals, model.neurons);
        out.cells = model.cells;
        out.memory = model.memory;
        out.maps = model.maps;
        out.drawings = model.drawings;
        out.archive = model.archive;
        out.interventions = model.interventions;
        out.course.archive_slots = model.course.archive_slots;
    }
    std::string name() const override {
        return "CPU (" + std::to_string(threads) + (threads == 1 ? " thread)" : " threads)");
    }
};

} // namespace

std::unique_ptr<Engine> make_cpu(const Model &model, unsigned threads) {
    check_storage(model, model.animals, model.neurons);
    return std::make_unique<Cpu>(model, threads);
}
} // namespace axion
