#include "axion/genesis.hpp"
#include "axion/journal.hpp"
#include <algorithm>
#include <iostream>
#include <string_view>

namespace {
using namespace std::literals;

void usage() {
    std::cout << R"(Axion Genesis  |  GPU-native embodied artificial life & spiking neocortex

  ./axion genesis bench
  ./axion genesis evolve   [--pop N] [--world 256|512|1024] [--ticks T] [--challenge 0|1|2] [--controls] [--save FILE]
  ./axion genesis probe    --load FILE [--ticks T] [--seed S]
  ./axion genesis converse --load FILE [--group 1..4] [--script "..."]
  ./axion genesis inspect  --load FILE
  ./axion journal rebuild  [--journal-dir DIR]

Commands:
  genesis     Blackwell-native embodied ALife, CA-DNA morphogenesis, 3-factor STDP & spiking cognition.
              Run `./axion genesis --help` for full options.
  journal     Rebuild WebP observatory plates and journal/JOURNAL.md.
)";
}
} // namespace

int main(int argc, char **argv) {
    try {
        return axion::journal::execute(argc, argv, [](int argc, char **argv) {
            if (argc > 1 && std::string_view(argv[1]) == "genesis")
                return axion::genesis::command(argc, argv);
            if (argc == 1 || std::find(argv + 1, argv + argc, "--help"sv) != argv + argc ||
                std::string_view(argv[1]) == "help") {
                usage();
                return 0;
            }
            throw std::invalid_argument("unknown command '" + std::string(argv[1]) + "' (use `axion genesis ...` or `axion journal rebuild`)");
        });
    } catch (const std::exception &error) {
        std::cerr << "axion: " << error.what() << '\n';
        return 1;
    }
}
