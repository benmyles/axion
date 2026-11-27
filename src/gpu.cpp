#include "axion/engine.hpp"
#include <algorithm>
#include <chrono>
#include "cuda_driver.hpp"
#include <stdexcept>

extern "C" {
extern const char axion_foraging[] __attribute__((weak));
extern const char axion_lessons[] __attribute__((weak));
extern const char axion_routes[] __attribute__((weak));
extern const char axion_drawing[] __attribute__((weak));
}

namespace axion {
namespace {

using namespace cuda;

class Gpu final : public Engine {
    Driver d;
    Context context{};
    int device{};
    bool current{};
    Module modules[4]{};
    bool loaded[7]{};
    Event start{}, stop{};
    DevicePtr weights{}, totals{}, scratch{}, eligibility{}, banks{}, maps{}, drawings{}, archive{}, interventions{};
    bool has_memory{}, has_map{}, has_drawings{};
    unsigned family{};
    Word archive_slots{};
    bool has_archive{}, has_interventions{};
    bool grid{};
    Word grid_blocks[12]{}, grid_threads[12]{};
    Function functions[28]{};
    Word animals{}, neurons{}, blocks{}, threads{};
    std::string description, kernel;
    Word sms{}, scratch_blocks{};
    template<class T>
    void activate(DevicePtr &buffer, bool &active) {
        size_t bytes = size_t(animals) * sizeof(T);
        if (!buffer) d.check(d.alloc(&buffer, bytes), "allocate experience");
        if (!active) d.check(d.zero(buffer, 0, bytes), "clear experience");
        active = true;
    }
    template<class T>
    void upload_store(const std::vector<T> &store, DevicePtr &buffer, bool &active) {
        active = !store.empty();
        if (active) {
            activate<T>(buffer, active);
            d.check(d.upload(buffer, store.data(), animals * sizeof(T)), "restore experience");
        }
    }
    template<class T>
    void download_store(std::vector<T> &store, DevicePtr buffer, bool active) {
        store.resize(active ? animals : 0);
        if (active) d.check(d.download(store.data(), buffer, animals * sizeof(T)), "read experience");
    }
    void prepare_archive() {
        size_t bytes = size_t(animals) * archive_slots * sizeof(Association);
        if (!archive) d.check(d.alloc(&archive, bytes), "allocate consolidated associations");
        if (!has_archive) d.check(d.zero(archive, 0, bytes), "clear consolidated associations");
        has_archive = true;
    }
    static unsigned family_for(const Course &course, bool experimental) {
        Task task = course.task;
        if (is_drawing(task)) {
            bool inverse = (task == Task::invention && course.novel) || (task == Task::canvas && course.inverse);
            return 3 + 2 * unsigned(inverse) + unsigned(experimental || course.experiments != Course::Experiments::off);
        }
        if (task == Task::routes || task == Task::errand) return 2;
        return unsigned(is_lesson(task));
    }
    void reserve_grid(Word max_blocks) {
        if (max_blocks <= scratch_blocks) return;
        size_t bytes = sizeof(Word) * (neurons + 2 * (neurons / 32));
        DevicePtr next{};
        d.check(d.alloc(&next, bytes + 2 * sizeof(Word) + max_blocks * sizeof(WideSums)), "allocate wide state");
        try {
            d.check(d.zero(next + bytes, 0, 2 * sizeof(Word)), "initialize grid barrier");
            if (scratch) d.check(d.free_mem(scratch), "resize wide state");
        } catch (...) {
            d.free_mem(next);
            throw;
        }
        scratch = next;
        scratch_blocks = max_blocks;
    }
    void load_family(unsigned index) {
        if (loaded[index]) return;
        unsigned module_index = std::min(index, 3u);
        const char *images[] = {axion_foraging, axion_lessons, axion_routes, axion_drawing};
        char log[4096]{};
        int options[] = {5, 6}; // CU_JIT_ERROR_LOG_BUFFER, CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES
        void *values[] = {log, reinterpret_cast<void *>(sizeof(log))};
        Module program = modules[module_index];
        if (!program) {
            auto rc = d.module_load(&program, images[module_index], 2, options, values);
            if (rc && log[0]) throw std::runtime_error(std::string("CUDA module error: ") + log);
            d.check(rc, "load GPU program");
        }
        const char *suffix[] = {"_frozen", "_motor", "_full", "_no_scaling"};
        const char *prefix[] = {"", "lesson_", "route_", "draw", "investigate", "inverse", "causal_inverse"};
        Word max_blocks = scratch_blocks;
        try {
            for (unsigned i = 0; i < 4; ++i) {
                auto slot = index * 4 + i;
                auto name = index >= 3 ? std::string(prefix[index]) + suffix[i] : prefix[index] + kernel + suffix[i];
                d.check(d.function(&functions[slot], program, name.c_str()), "resolve kernel");
                if (grid && index < 3) {
                    Word width = !index && i != unsigned(Learning::frozen) ? 256 : 128;
                    int resident;
                    d.check(d.occupancy(&resident, functions[slot], width, 0), "cooperative occupancy");
                    grid_threads[slot] = width;
                    grid_blocks[slot] = std::min(Word(resident) * sms, (neurons + width - 1) / width);
                    if (!grid_blocks[slot]) throw std::runtime_error("cooperative grid cannot reside on device");
                    max_blocks = std::max(max_blocks, grid_blocks[slot]);
                }
            }
            reserve_grid(max_blocks);
        } catch (...) {
            if (!modules[module_index]) d.module_free(program);
            throw;
        }
        modules[module_index] = program;
        loaded[index] = true;
    }
public:
    ~Gpu() override {
        if (current) {
            if (stop) d.event_free(stop);
            if (start) d.event_free(start);
            if (interventions) d.free_mem(interventions);
            if (archive) d.free_mem(archive);
            if (drawings) d.free_mem(drawings);
            if (maps) d.free_mem(maps);
            if (banks) d.free_mem(banks);
            if (eligibility) d.free_mem(eligibility);
            if (scratch) d.free_mem(scratch);
            if (totals) d.free_mem(totals);
            if (weights) d.free_mem(weights);
            for (auto module : modules) if (module) d.module_free(module);
            Context removed;
            d.pop(&removed);
        }
        if (context) d.release(device);
    }

    void setup(const Model &model, Layout layout) {
        if (!axion_foraging) throw std::runtime_error("CPU-only build (build with CUDA=1 for PTX)");
        if (layout == Layout::warp && model.neurons > 128)
            throw std::runtime_error("warp layout supports 64 or 128 neurons");
        if (layout == Layout::block && model.neurons > 4096)
            throw std::runtime_error("block layout supports at most 4096 cells per animal");
        d.open();
        d.check(d.init(0), "cuInit");
        d.check(d.device(&device, 0), "cuDeviceGet");
        int major, minor;
        d.check(d.attribute(&major, 75, device), "compute capability major");
        d.check(d.attribute(&minor, 76, device), "compute capability minor");
        if (major < 8) throw std::runtime_error("Axion requires SM 8.0 or newer");
        d.check(d.retain(&context, device), "retain primary context");
        d.check(d.push(context), "push primary context");
        current = true;
        animals = model.animals;
        neurons = model.neurons;
        bool warp = layout == Layout::warp || (layout == Layout::automatic && neurons <= 128);
        kernel = warp ? "warp" + std::to_string(neurons) : "block";
        if (neurons > 1024) kernel = "striped" + std::to_string(neurons);
        grid = layout == Layout::grid || neurons > 4096;
        if (grid) kernel = "grid";
        threads = warp ? 128 : (neurons > 1024 ? 512 : neurons);
        blocks = warp ? (animals + threads / 32 - 1) / (threads / 32) : animals;
        if (grid) {
            int coop, count;
            d.check(d.attribute(&coop, 95, device), "cooperative launch support");
            d.check(d.attribute(&count, 16, device), "SM count");
            if (!coop) throw std::runtime_error("wide networks require cooperative CUDA launches");
            threads = 128;
            sms = Word(count);
        }
        load_family(family_for(model.course, !model.interventions.empty()));
        d.check(d.alloc(&weights, model.cells.size() * sizeof(Weights)), "allocate weights");
        d.check(d.alloc(&totals, animals * sizeof(Totals)), "allocate counters");
        static_assert(sizeof(Totals) == 40);
        restore(model);
        d.check(d.event_new(&start, 0), "create start event");
        d.check(d.event_new(&stop, 0), "create stop event");
        char name[256]{};
        d.check(d.device_name(name, sizeof(name), device), "GPU name");
        description = std::string(name) + " (SM " + std::to_string(major) + "." + std::to_string(minor)
                    + ", " + kernel + ")";
    }

    Run run(Course course, Word first, Word episodes, Learning learning) override {
        validate_course(course);
        Run result{std::vector<Totals>(animals)};
        auto wall = std::chrono::steady_clock::now();
        d.check(d.zero(totals, 0, animals * sizeof(Totals)), "clear counters");
        family = family_for(course, has_interventions);
        load_family(family);
        bool cooperative = grid && family < 3;
        if (cooperative && learning != Learning::frozen && !eligibility)
            d.check(d.alloc(&eligibility, neurons * sizeof(Word)), "allocate wide eligibility");
        bool writing = course.recall != Recall::cortical && learning != Learning::frozen;
        bool use_words = family == 1 || course.task == Task::errand;
        if (use_words && writing && !has_memory) activate<MemoryBank>(banks, has_memory);
        if (family == 2 && writing && !has_map) activate<PlaceMemory>(maps, has_map);
        if (family >= 3 && writing && !has_drawings) activate<DrawingMemory>(drawings, has_drawings);
        if (family >= 3 && writing && course.experiments != Course::Experiments::off && !has_interventions)
            activate<InterventionMemory>(interventions, has_interventions);
        DevicePtr causal = family >= 3 && has_interventions ? interventions : 0;
        course.archive_slots = archive_slots;
        if (use_words && writing && archive_slots && !has_archive) prepare_archive();
        DevicePtr active_banks = 0, active_maps = 0, paint = 0, long_memory = 0;
        if (course.recall != Recall::cortical) {
            if (family >= 3 && has_drawings) paint = drawings;
            if (use_words && has_archive) long_memory = archive;
            if (use_words && has_memory) active_banks = banks;
            if (family == 2 && has_map) active_maps = maps;
        }
        d.check(d.record(start, nullptr), "record start");
        for (Word done = 0; done < episodes;) {
            Word begin = first + done;
            Word length = std::min(cooperative ? 4u : chunk_episodes, episodes - done);
            auto mode = unsigned(learning) + (4 * family);
            if (cooperative) {
                for (Word animal = 0; animal < animals; ++animal) {
                    void *args[] = {&weights, &totals, &course, &animals, &begin, &length, &active_banks, &active_maps, &paint, &long_memory, &causal,
                                    &neurons, &animal, &scratch, &eligibility};
                    d.check(d.cooperative(functions[mode], grid_blocks[mode], 1, 1, grid_threads[mode], 1, 1,
                                          0, nullptr, args), "launch cooperative simulator");
                }
            } else {
                void *args[] = {&weights, &totals, &course, &animals, &begin, &length, &active_banks, &active_maps, &paint, &long_memory, &causal};
                Word launch_blocks = family >= 3 ? (animals + 3) / 4 : blocks;
                Word launch_threads = family >= 3 ? 128 : threads;
                d.check(d.launch(functions[mode], launch_blocks, 1, 1, launch_threads, 1, 1,
                                 0, nullptr, args, nullptr), "launch simulator");
            }
            done += length;
        }
        d.check(d.record(stop, nullptr), "record stop");
        d.check(d.wait(stop), "finish simulator");
        float ms;
        d.check(d.elapsed(&ms, start, stop), "measure GPU time");
        result.seconds = ms * 0.001;
        d.check(d.download(result.animals.data(), totals, animals * sizeof(Totals)), "read counters");
        result.wall_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall).count();
        return result;
    }

    std::string resources(Learning mode) const override {
        int regs, local, shared;
        auto index = unsigned(mode) + (4 * family);
        auto f = functions[index];
        d.check(d.func_attr(&regs, 4, f), "register count");
        d.check(d.func_attr(&local, 3, f), "local memory");
        d.check(d.func_attr(&shared, 1, f), "shared memory");
        return "  regs/thread=" + std::to_string(regs) + "  local_B/thread=" + std::to_string(local)
             + "  shared_B/block=" + std::to_string(shared)
             + (family >= 3 ? "  mapping=warp_per_canvas" : "")
             + (grid && family < 3 ? "  cooperative_blocks=" + std::to_string(grid_blocks[index]) : "");
    }
    void restore(const Model &model) override {
        check_storage(model, animals, neurons);
        d.check(d.upload(weights, model.cells.data(), model.cells.size() * sizeof(Weights)), "restore weights");
        upload_store(model.memory, banks, has_memory);
        upload_store(model.maps, maps, has_map);
        upload_store(model.drawings, drawings, has_drawings);
        upload_store(model.interventions, interventions, has_interventions);
        if (archive_slots != model.course.archive_slots && archive) {
            d.check(d.free_mem(archive), "resize archive");
            archive = 0;
        }
        archive_slots = model.course.archive_slots;
        has_archive = !model.archive.empty();
        if (has_archive) {
            prepare_archive();
            d.check(d.upload(archive, model.archive.data(), model.archive.size() * sizeof(Association)), "restore archive");
        }
    }
    void download(Model &model) override {
        check_storage(model, animals, neurons);
        d.check(d.download(model.cells.data(), weights, model.cells.size() * sizeof(Weights)), "read weights");
        download_store(model.memory, banks, has_memory);
        download_store(model.maps, maps, has_map);
        download_store(model.drawings, drawings, has_drawings);
        download_store(model.interventions, interventions, has_interventions);
        model.course.archive_slots = archive_slots;
        model.archive.resize(has_archive ? size_t(animals) * archive_slots : 0);
        if (has_archive)
            d.check(d.download(model.archive.data(), archive, model.archive.size() * sizeof(Association)), "read archive");
    }
    std::string name() const override { return description; }
};

} // namespace
std::unique_ptr<Engine> make_gpu(const Model &model, Layout layout) {
    check_storage(model, model.animals, model.neurons);
    auto gpu = std::make_unique<Gpu>();
    gpu->setup(model, layout);
    return gpu;
}
} // namespace axion
