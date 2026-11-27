struct GateTrial {
    std::vector<GateSubject> subjects;
    Word start, seed, phase;
    bool sham;

    void report(const Habitat &h, EvidenceFile *trace = nullptr) const {
        struct Outcome {
            Count enrolled = 0, retained = 0, opening = 0, energy = 0, acquired = 0, changed = 0;
        };
        std::array<Outcome, 3> totals{};
        if (trace)
            trace->out << "slot\tborn\tarm\tacquired\tchanged\topening_energy\tretained_energy\tlast_action\n";
        const char *names[] = {"intact", "reverted", "rotated"};
        for (Word slot = 0; slot < subjects.size(); ++slot) {
            const auto &record = subjects[slot];
            if (!record.arm)
                continue;
            const auto &animal = h.bodies[slot];
            Word energy = animal.born == record.born ? animal.energy : 0;
            auto &v = totals[record.arm - 1];
            ++v.enrolled;
            v.retained += energy > 0;
            v.opening += record.opening_energy;
            v.energy += energy;
            v.acquired += record.acquired;
            v.changed += record.changed;
            if (trace)
                trace->out << slot << '\t' << record.born << '\t' << names[record.arm - 1] << '\t' << record.acquired
                           << '\t' << record.changed << '\t' << record.opening_energy << '\t' << energy << '\t'
                           << (energy ? int(animal.action) : -1) << '\n';
        }
        for (Word arm = 0; arm < totals.size(); ++arm) {
            const auto &v = totals[arm];
            std::cout << "gate_trial=" << names[arm] << "  tick=" << h.header.tick << "  start=" << start
                      << "  seed=" << seed << "  phase=" << phase << "  sham=" << sham << "  enrolled=" << v.enrolled
                      << "  surviving=" << v.retained << "  opening_energy=" << v.opening
                      << "  retained_energy=" << v.energy << "  acquired_bits=" << v.acquired
                      << "  changed_bits=" << v.changed << '\n';
        }
    }
};
