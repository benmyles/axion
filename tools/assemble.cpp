// Ahead-of-time assembly using the installed driver. The build machine's GPU
// determines the native target. Device code is still authored in kernels.cu.
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

extern "C" {
int cuInit(unsigned);
int cuDeviceGet(int *, int);
int cuDeviceGetAttribute(int *, int, int);
int cuDevicePrimaryCtxRetain(void **, int);
int cuDevicePrimaryCtxRelease_v2(int);
int cuCtxSetCurrent(void *);
int cuLinkCreate_v2(unsigned, int *, void **, void **);
int cuLinkAddFile_v2(void *, int, const char *, unsigned, int *, void **);
int cuLinkComplete(void *, void **, std::size_t *);
int cuLinkDestroy(void *);
int cuGetErrorName(int, const char **);
}

namespace {
void check(int status) {
    if (!status) return;
    const char *name = nullptr;
    cuGetErrorName(status, &name);
    throw std::runtime_error(name ? name : "CUDA driver error");
}
struct Assembler {
    int device{};
    void *context{}, *link{};
    ~Assembler() {
        if (link) cuLinkDestroy(link);
        if (context) cuDevicePrimaryCtxRelease_v2(device);
    }
    void write(const char *arch, const char *source, const char *dest) {
        check(cuInit(0));
        check(cuDeviceGet(&device, 0));
        int major, minor;
        check(cuDeviceGetAttribute(&major, 75, device));
        check(cuDeviceGetAttribute(&minor, 76, device));
        std::string target = "sm_" + std::to_string(major) + std::to_string(minor);
        if (arch != target) throw std::runtime_error("native build needs CUDA_ARCH=" + target);
        check(cuDevicePrimaryCtxRetain(&context, device));
        check(cuCtxSetCurrent(context));
        check(cuLinkCreate_v2(0, nullptr, nullptr, &link));
        check(cuLinkAddFile_v2(link, 1, source, 0, nullptr, nullptr)); // CU_JIT_INPUT_PTX
        void *binary;
        std::size_t bytes;
        check(cuLinkComplete(link, &binary, &bytes));
        std::ofstream file(dest, std::ios::binary);
        file.exceptions(std::ios::badbit | std::ios::failbit);
        file.write(static_cast<const char *>(binary), std::streamsize(bytes));
        file.close();
    }
};
}
int main(int argc, char **argv) {
    if (argc != 4) return 2;
    try { Assembler{}.write(argv[1], argv[2], argv[3]); }
    catch (const std::exception &e) {
        std::cerr << "CUDA assembly failed (" << e.what() << ")\n";
        return 1;
    }
}
