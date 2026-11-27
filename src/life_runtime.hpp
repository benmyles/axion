#pragma once
#include "cuda_driver.hpp"

extern "C" const char axion_genesis[] __attribute__((weak));

namespace axion::life::runtime {
using namespace cuda;

enum class Image {
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
            for (auto module : modules)
                if (module) d.module_free(module);
            Context removed;
            d.pop(&removed);
        }
        if (ctx) d.release(device);
    }

    void open() {
        if (!axion_genesis) throw std::runtime_error("this command requires a CUDA build");
        d.open();
        d.check(d.init(0), "initialize CUDA");
        d.check(d.device(&device, 0), "select CUDA device");
        d.check(d.retain(&ctx, device), "retain GPU");
        d.check(d.push(ctx), "activate GPU");
        current = true;
        char label[256];
        d.check(d.device_name(label, sizeof(label), device), "identify GPU");
        name = label;
    }

    void bind(Function &f, const std::string &symbol, Image image = Image::genesis) {
        const char *sources[]{axion_genesis};
        auto &module = modules[int(image)];
        if (!module)
            d.check(d.module_load(&module, sources[int(image)], 0, nullptr, nullptr), "load CUDA kernel image");
        d.check(d.function(&f, module, symbol.c_str()), symbol.c_str());
    }
};

struct Temporary {
    Driver &d;
    DevicePtr ptr{};
    Temporary(Driver &driver, size_t bytes) : d(driver) {
        if (bytes) d.check(d.alloc(&ptr, bytes), "allocate GPU buffer");
    }
    ~Temporary() {
        if (ptr) d.free_mem(ptr);
    }
    Temporary(const Temporary &) = delete;
    Temporary &operator=(const Temporary &) = delete;
};

} // namespace axion::life::runtime
