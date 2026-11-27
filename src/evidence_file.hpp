#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace axion {
// Publish a complete, independently replayable experiment without replacing a prior result.
class EvidenceFile {
    std::filesystem::path final_, partial_;

  public:
    std::ofstream out;
    explicit EvidenceFile(const std::string &name) : final_(name) {
        if (std::filesystem::exists(final_))
            throw std::runtime_error("evidence file already exists: " + name);
        std::string pattern = name + ".part-XXXXXX";
        std::vector<char> temp(pattern.begin(), pattern.end());
        temp.push_back(0);
        int fd = mkstemp(temp.data());
        if (fd < 0)
            throw std::runtime_error("cannot create evidence file: " + name);
        close(fd);
        partial_ = temp.data();
        out.open(partial_);
        if (!out) {
            std::filesystem::remove(partial_);
            throw std::runtime_error("cannot write evidence file: " + name);
        }
    }
    EvidenceFile(const EvidenceFile &) = delete;
    ~EvidenceFile() {
        if (!partial_.empty()) {
            std::error_code error;
            std::filesystem::remove(partial_, error);
        }
    }
    void publish() {
        out.close();
        if (!out)
            throw std::runtime_error("incomplete experiment trace");
        std::filesystem::create_hard_link(partial_, final_);
        std::filesystem::remove(partial_);
        partial_.clear();
    }
};
} // namespace axion
