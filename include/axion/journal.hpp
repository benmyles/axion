#pragma once
#include "model.hpp"
#if !defined(__CUDA__)
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#endif

namespace axion::journal {
using Count = std::uint64_t;
#if !defined(__CUDA__)
struct Checkpoint { std::string path; Count checksum; bool saved; };
struct Report {
    std::string command, source, device, archive;
    std::map<std::string, Count> metrics;
    std::vector<std::string> args, lines;
    std::vector<Checkpoint> checkpoints;
    void line(std::string entry);
};
Report *current();
void checkpoint(const std::string &path, Count checksum, bool saved);
int execute(int argc, char **argv, const std::function<int(int, char **)> &command);
#endif
} // namespace axion::journal
