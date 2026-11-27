#pragma once
#include <cstddef>
#include <dlfcn.h>
#include <stdexcept>
#include <string>

namespace axion::cuda {
// Stable Linux CUDA Driver ABI. Only these entry points are needed. Loading
// them on demand also lets the same executable run on a CPU-only machine.
using DevicePtr = unsigned long long;
using Context = struct CUctx_st *;
using Module = struct CUmod_st *;
using Function = struct CUfunc_st *;
using Event = struct CUevent_st *;
using Stream = struct CUstream_st *;
using Result = int;

struct Driver {
    void *lib = nullptr;
    Result (*init)(unsigned);
    Result (*device)(int *, int);
    Result (*device_name)(char *, int, int);
    Result (*attribute)(int *, int, int);
    Result (*retain)(Context *, int);
    Result (*release)(int);
    Result (*push)(Context);
    Result (*pop)(Context *);
    Result (*module_load)(Module *, const void *, unsigned, int *, void **);
    Result (*module_free)(Module);
    Result (*function)(Function *, Module, const char *);
    Result (*func_attr)(int *, int, Function);
    Result (*alloc)(DevicePtr *, size_t);
    Result (*free_mem)(DevicePtr);
    Result (*upload)(DevicePtr, const void *, size_t);
    Result (*download)(void *, DevicePtr, size_t);
    Result (*copy_device)(DevicePtr, DevicePtr, size_t);
    Result (*zero)(DevicePtr, unsigned char, size_t);
    Result (*launch)(Function, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
                     unsigned, Stream, void **, void **);
    Result (*cooperative)(Function, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned,
                          unsigned, Stream, void **);
    Result (*occupancy)(int *, Function, int, size_t);
    Result (*event_new)(Event *, unsigned);
    Result (*event_free)(Event);
    Result (*record)(Event, Stream);
    Result (*wait)(Event);
    Result (*elapsed)(float *, Event, Event);
    Result (*error)(Result, const char **);

    ~Driver() { if (lib) dlclose(lib); }

    template<class T>
    void load(T &slot, const char *name) {
        slot = reinterpret_cast<T>(dlsym(lib, name));
        if (!slot) throw std::runtime_error(std::string("missing CUDA driver entry ") + name);
    }
    void open() {
        lib = dlopen("libcuda.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!lib) throw std::runtime_error(dlerror());
        load(init, "cuInit");
        load(device, "cuDeviceGet");
        load(device_name, "cuDeviceGetName");
        load(attribute, "cuDeviceGetAttribute");
        load(retain, "cuDevicePrimaryCtxRetain");
        load(release, "cuDevicePrimaryCtxRelease_v2");
        load(push, "cuCtxPushCurrent_v2");
        load(pop, "cuCtxPopCurrent_v2");
        load(module_load, "cuModuleLoadDataEx");
        load(module_free, "cuModuleUnload");
        load(function, "cuModuleGetFunction");
        load(func_attr, "cuFuncGetAttribute");
        load(alloc, "cuMemAlloc_v2");
        load(free_mem, "cuMemFree_v2");
        load(upload, "cuMemcpyHtoD_v2");
        load(download, "cuMemcpyDtoH_v2");
        load(copy_device, "cuMemcpyDtoD_v2");
        load(zero, "cuMemsetD8_v2");
        load(launch, "cuLaunchKernel");
        load(cooperative, "cuLaunchCooperativeKernel");
        load(occupancy, "cuOccupancyMaxActiveBlocksPerMultiprocessor");
        load(event_new, "cuEventCreate");
        load(event_free, "cuEventDestroy_v2");
        load(record, "cuEventRecord");
        load(wait, "cuEventSynchronize");
        load(elapsed, "cuEventElapsedTime");
        load(error, "cuGetErrorString");
    }
    void check(Result status, const char *operation) const {
        if (!status) return;
        const char *message = nullptr;
        error(status, &message);
        throw std::runtime_error(std::string(operation) + " failed (" + std::to_string(status)
                + ") " + (message ? message : "unknown CUDA error"));
    }
};

} // namespace axion::cuda
