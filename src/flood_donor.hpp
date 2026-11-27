apprentice::Donor read_donor(const std::string &path) {
    std::ifstream input(path);
    std::string tag, extra;
    apprentice::Donor d{};
    if (!(input >> tag >> d.program.first >> d.program.second >> d.law >> d.seed >> d.generations >> d.ancestor) ||
        d.law > 15)
        throw std::invalid_argument("invalid reared regulatory donor");
    if (tag == "AXION-REGULATOR-1") {
        if (!(input >> d.harvest >> d.offered) || !d.generations || !d.offered || d.harvest > d.offered)
            throw std::invalid_argument("invalid reared regulatory donor");
    } else if (tag == "AXION-DRIFT-GERM-1" || tag == "AXION-INLET-GERM-1" || tag == "AXION-INLET-GERM-2" ||
               tag == "AXION-INLET-BROOD-1" || tag == "AXION-INLET-CROSS-1") {
        d.field = tag == "AXION-INLET-BROOD-1" ? 2u : tag == "AXION-INLET-CROSS-1" ? 3u : 1u;
        if (!(input >> d.tick >> d.copies >> d.population) || !d.copies || d.copies > d.population)
            throw std::invalid_argument("invalid watershed regulatory sample");
        if (tag != "AXION-DRIFT-GERM-1") {
            d.coupled = tag == "AXION-INLET-GERM-1" ? 1u : 2u;
            if (!(input >> d.wiring >> d.hand_active) || d.wiring >= (d.coupled == 2 ? 131072u : 4096u) ||
                d.hand_active > 1)
                throw std::invalid_argument("invalid lateral dendritic germline");
        }
    } else
        throw std::invalid_argument("invalid reared regulatory donor");
    if (input >> extra)
        throw std::invalid_argument("invalid reared regulatory donor");
    return d;
}
drift::Signature signature(const apprentice::Donor &d) {
    return {d.program,
            d.coupled ? drift::lateral_hand(d.law, d.wiring, d.hand_active) | (Word(d.coupled == 2) << 31) : ~0u};
}
std::array<Word, 16> calibrate_names(Session &s, Word seed) {
    Temporary scores(s.d, 16 * sizeof(Word));
    Function calibration{};
    s.bind(calibration, "braid_class_laws", Image::runnels);
    void *args[]{&seed, &scores.ptr};
    s.d.check(s.d.launch(calibration, 1, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
              "calibrate the local name contacts");
    std::array<Word, 16> results{};
    s.d.check(s.d.download(results.data(), scores.ptr, sizeof(results)), "read naming qualification");
    return results;
}
// A developmental duplication keeps the ancestral program in an existing
// auditory band. Its receptor address and allocations remain ordinary genes.
bool branch_cortex(Germ &g, Word width, Word motor) {
    Word old = regulation::module_index(g, motor), available = g.module_count, used = 0, normalized = 0;
    for (Word j = 0; j < 16; ++j) {
        Word module = regulation::module_index(g, j);
        used |= 1u << module;
        normalized |= module << (2 * j);
    }
    if (available == 4) {
        Word free = (~used) & 15u;
        if (!free)
            return false;
        available = std::countr_zero(free);
    }
    if (width < 2)
        return false;
    Word extension = regulation::auditory_band(g, width);
    while ((extension & 15) == (motor & 15))
        extension = (extension + 1) % width;
    g.allocation = normalized;
    g.hearing.high = (g.hearing.high & 0x7ffffffu) | (motor << 27);
    g.brain.receptors = (g.brain.receptors & 0x7ffffffu) | (extension << 27);
    auto assign = [&](Word address, Word module) {
        Word shift = 2 * (address & 15);
        g.allocation = (g.allocation & ~(3u << shift)) | (module << shift);
    };
    g.module[available] = g.module[old];
    if (available == g.module_count)
        ++g.module_count;
    assign(motor, old);
    assign(extension, available);
    return true;
}
void place_donor(Session &s, Habitat &h, const Options &o, const apprentice::Donor &d) {
    if (o.receive_loci == 4 && !d.coupled)
        throw std::invalid_argument("the donor has no expressed dendritic wiring");
    if (o.slot != ~0u && (o.receive_count != 1 || o.slot >= h.sites() || !h.bodies[o.slot].energy))
        throw std::invalid_argument("a named donor recipient needs a single living body");
    Word n = h.sites(), stride = mix(o.class_seed + 251) | 1, first = mix(o.class_seed ^ 0xca758e1u), received = 0;
    std::array<Word, 16> laws{};
    if (o.receive_named)
        laws = calibrate_names(s, o.class_seed);
    for (Word j = 0; j < n && received < o.receive_count; ++j) {
        Word id = o.slot == ~0u ? (first + j * stride) & (n - 1) : o.slot;
        if (!h.bodies[id].energy)
            continue;
        const auto &ear = h.riggers[id].genes;
        if (o.receive_named &&
            (!h.bodies[id].body.eyes || (ear.arbor & (3u << 14)) != (3u << 14) || !laws[(ear.plastic >> 8) & 15])) {
            if (o.slot != ~0u)
                throw std::invalid_argument("the named pupil did not meet the requested enrollment");
            continue;
        }
        auto &g = h.genomes[id];
        Word band = regulation::innervated_band(g, h.conducting(id));
        auto old = g.module[regulation::module_index(g, band)];
        if (o.receive_branch && !branch_cortex(g, h.conducting(id), band)) {
            if (o.slot != ~0u)
                throw std::invalid_argument("the named body has no free cortical module for duplication");
            continue;
        }
        band = regulation::innervated_band(g, h.conducting(id));
        auto &m = g.module[regulation::module_index(g, band)];
        if (o.receive_loci & 1)
            m = {d.program.first, d.program.second};
        auto &law = h.riggers[id].genes.plastic;
        Word old_law = law;
        if ((o.receive_loci & 2) && (o.donor_ports & 1))
            law = (law & ~0xf000u) | (d.law << 12);
        if ((o.receive_loci & 2) && (o.donor_ports & 2)) {
            law = (law & ~15u) | d.law;
            if (d.coupled) {
                drift::wire(h.riggers[id].genes, d.wiring, d.coupled == 2);
                auto &arbor = h.riggers[id].genes.arbor;
                arbor = (arbor & ~(1u << 12)) | (d.hand_active << 12);
            }
        }
        if (o.receive_loci == 4)
            drift::wire(h.riggers[id].genes, d.wiring, d.coupled == 2);
        if (!received)
            std::cout << "regulatory_transplant=" << id << "  tick=" << h.header.tick << "  band=" << band
                      << "  old_first=" << old.routing << "  old_second=" << old.gate
                      << "  old_law=" << ((old_law >> 12) & 15) << "  old_hand=" << (old_law & 15)
                      << "  first=" << m.routing << "  second=" << m.gate << "  law=" << d.law
                      << "  donor_first=" << d.program.first << "  donor_second=" << d.program.second
                      << "  ports=" << o.donor_ports << "  loci=" << o.receive_loci << "  nursery_seed=" << d.seed
                      << "  generations=" << d.generations << "  branched=" << o.receive_branch
                      << "  vocal_band=" << regulation::auditory_band(g, h.conducting(id))
                      << "  ancestor=" << d.ancestor << "  watershed=" << d.field << "  donor_tick=" << d.tick
                      << "  copies=" << d.copies << "  source_population=" << d.population
                      << "  swallowed=" << d.harvest << "  offered=" << d.offered << '\n';
        ++received;
    }
    std::cout << "regulatory_release=" << h.header.tick << "  placed=" << received << "  requested=" << o.receive_count
              << "  calibrated=" << o.receive_named << "  seed=" << o.class_seed << '\n';
}
