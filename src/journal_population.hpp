#pragma once
#include "axion/census.hpp"
#include "axion/journal.hpp"
#include <algorithm>
#include <vector>

namespace axion::journal {
inline void ranked_forms(Report &report, std::vector<CensusMember> members, bool flight = false) {
    std::erase_if(members, [](const auto &r) { return !r.energy; });
    std::sort(members.begin(), members.end(), [](const auto &a, const auto &b) {
        if (a.form != b.form)
            return a.form < b.form;
        return a.energy != b.energy ? a.energy > b.energy : a.slot < b.slot;
    });
    struct Group {
        size_t count, first;
    };
    std::vector<Group> top;
    size_t types = 0;
    for (size_t i = 0; i < members.size();) {
        size_t end = i + 1;
        while (end < members.size() && members[end].form == members[i].form)
            ++end;
        Group g{end - i, i};
        ++types;
        auto at = std::find_if(top.begin(), top.end(), [&](Group old) { return old.count < g.count; });
        if (at != top.end() || top.size() < 8)
            top.insert(at, g);
        if (top.size() > 8)
            top.pop_back();
        i = end;
    }
    report.metrics["ranked_residents"] = members.size();
    report.metrics["ranked_morphs"] = types;
    for (size_t j = 0; j < top.size(); ++j) {
        auto group = top[j];
        auto &a = members[group.first];
        Specimen p;
        p.role = flight ? "ranked_flight" : "ranked_body";
        p.path = (flight ? "Flight form " : "Body form ") + std::to_string(a.form);
        p.measures = {{"rank", j + 1},        {"members", group.count}, {"slot", a.slot}, {"energy", a.energy},
                      {"founder", a.founder}, {"neurons", a.cells},     {"tape", a.tape}};
        if (flight) {
            p.measures["roaming"] = a.form;
            p.measures["productions"] = a.low;
            p.measures["receptors"] = a.high;
        } else {
            p.measures["body"] = a.form;
            p.measures["effectors_low"] = a.low;
            p.measures["effectors_high"] = a.high;
        }
        report.specimens.push_back(std::move(p));
    }
}
} // namespace axion::journal
