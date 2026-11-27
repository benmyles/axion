#pragma once
#include "cuda_driver.hpp"

extern "C" const char axion_scenes[] __attribute__((weak));
extern "C" const char axion_mazes[] __attribute__((weak));
extern "C" const char axion_springbeds[] __attribute__((weak));
extern "C" const char axion_mapactors[] __attribute__((weak));
extern "C" const char axion_trails[] __attribute__((weak));
extern "C" const char axion_travel[] __attribute__((weak));
extern "C" const char axion_life[] __attribute__((weak));
extern "C" const char axion_assays[] __attribute__((weak));
extern "C" const char axion_actors[] __attribute__((weak));
extern "C" const char axion_memories[] __attribute__((weak));
extern "C" const char axion_outcomes[] __attribute__((weak));
extern "C" const char axion_appraisals[] __attribute__((weak));
extern "C" const char axion_foodactors[] __attribute__((weak));
extern "C" const char axion_sequence_field[] __attribute__((weak));
extern "C" const char axion_sequence_actors[] __attribute__((weak));
extern "C" const char axion_sequence_routes[] __attribute__((weak));
extern "C" const char axion_sequence_mazes[] __attribute__((weak));
extern "C" const char axion_sequence_springs[] __attribute__((weak));
extern "C" const char axion_sequence_appraisals[] __attribute__((weak));
extern "C" const char axion_junctions[] __attribute__((weak));
extern "C" const char axion_recollection[] __attribute__((weak));
extern "C" const char axion_runnels[] __attribute__((weak));
extern "C" const char axion_mind[] __attribute__((weak));
extern "C" const char axion_genesis[] __attribute__((weak));
extern "C" const char axion_siphons[] __attribute__((weak));
extern "C" const char axion_floodplain[] __attribute__((weak));
extern "C" const char axion_grounding[] __attribute__((weak));
extern "C" const char axion_procedure[] __attribute__((weak));
extern "C" const char axion_composition[] __attribute__((weak));
extern "C" const char axion_development[] __attribute__((weak));
extern "C" const char axion_causes[] __attribute__((weak));
extern "C" const char axion_vocal_brood[] __attribute__((weak));
extern "C" const char axion_dialogue[] __attribute__((weak));
namespace axion::life::runtime {
using namespace cuda;
enum class Image {
    habitat,
    assays,
    actors,
    memories,
    outcomes,
    appraisals,
    foodactors,
    trails,
    travel,
    mazes,
    springbeds,
    mapactors,
    scenes,
    sequence_field,
    sequence_actors,
    sequence_routes,
    sequence_mazes,
    sequence_springs,
    sequence_appraisals,
    junctions,
    dialogue,
    vocal_brood,
    recollection,
    causes,
    development,
    composition,
    procedure,
    grounding,
    floodplain,
    siphons,
    runnels,
    mind,
    genesis,
    count
};
struct Session {
    Driver d;
    Context ctx{};
    Module modules[int(Image::count)]{};
    int device{};
    bool current{};
    std::string name;
    Session() = default;
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    ~Session() {
        if (current) {
            for (auto module : modules) if (module) d.module_free(module);
            Context removed; d.pop(&removed);
        }
        if (ctx) d.release(device);
    }
    void open() {
        if (!axion_life) throw std::runtime_error("this command requires a CUDA build");
        d.open(); d.check(d.init(0), "initialize CUDA"); d.check(d.device(&device, 0), "select CUDA device");
        d.check(d.retain(&ctx, device), "retain GPU"); d.check(d.push(ctx), "activate GPU"); current = true;
        char label[256]; d.check(d.device_name(label, sizeof(label), device), "identify GPU"); name = label;
    }
    void bind(Function &f, const std::string &symbol, Image image = Image::habitat) {
        const char *sources[]{axion_life,
                              axion_assays,
                              axion_actors,
                              axion_memories,
                              axion_outcomes,
                              axion_appraisals,
                              axion_foodactors,
                              axion_trails,
                              axion_travel,
                              axion_mazes,
                              axion_springbeds,
                              axion_mapactors,
                              axion_scenes,
                              axion_sequence_field,
                              axion_sequence_actors,
                              axion_sequence_routes,
                              axion_sequence_mazes,
                              axion_sequence_springs,
                              axion_sequence_appraisals,
                              axion_junctions,
                              axion_dialogue,
                              axion_vocal_brood,
                              axion_recollection,
                              axion_causes,
                              axion_development,
                              axion_composition,
                              axion_procedure,
                              axion_grounding,
                              axion_floodplain,
                              axion_siphons,axion_runnels,axion_mind,axion_genesis};
        auto &module = modules[int(image)];
        if (!module) d.check(d.module_load(&module,sources[int(image)],0,nullptr,nullptr),"load CUDA kernel image");
        d.check(d.function(&f,module,symbol.c_str()),symbol.c_str());
    }
};
// Short-lived device buffers for an experiment or report. Only the resulting
// genomes or observations cross back into the main simulation.
struct Temporary {
    Driver &d;
    DevicePtr ptr{};
    Temporary(Driver &driver, size_t bytes) : d(driver) {
        if (bytes) d.check(d.alloc(&ptr, bytes), "allocate experimental buffer");
    }
    ~Temporary() { if (ptr) d.free_mem(ptr); }
    Temporary(const Temporary &) = delete;
    Temporary &operator=(const Temporary &) = delete;
};
} // namespace axion::life::runtime
