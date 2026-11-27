void context_cohort_study(const Habitat &h, const Options &o, const std::vector<Word> &slots, capillary::Course course,
                          DevicePtr bodies, DevicePtr priors, DevicePtr histories, DevicePtr natural) {
    using namespace capillary;
    Word count = slots.size();
    auto series = o.context_series;
    Function fn{};
    s.bind(fn, "context_cohort", Image::runnels);
    Temporary results(s.d, Count(count) * sizeof(LifeGrade));
    FieldTimer due(s.d);
    due.start(stream);
    launch(fn, count, 32, bodies, priors, histories, count, course, series, natural, results.ptr);
    due.stop(stream);
    synchronize();
    std::vector<LifeGrade> all(count);
    s.d.check(s.d.download(all.data(), results.ptr, all.size() * sizeof(all[0])), "read paired lifetime phenotypes");
    std::unique_ptr<EvidenceFile> out;
    if (!o.cap_screen.empty()) {
        out = std::make_unique<EvidenceFile>(o.cap_screen);
        out->out << "slot\tborn\tgeneration\tcells\tgene\tmotor\tword\tbasal_lut\tmodule_mask\tlocal_lut\tpaired_"
                    "lut\tstreams\tfirst_"
                    "episodes\tsecond_episodes\tswitches\tenergy\tprobes";
        for (const char *prefix : {"native_", "paired_"})
            for (const char *name :
                 {"first", "second", "first_all", "second_all", "whole", "immediate", "switches", "food"})
                out->out << '\t' << prefix << name;
        out->out << '\n';
    }
    struct Group {
        Word n = 0, both = 0, alternate = 0, passed = 0, against = 0;
        Count regular[life_metrics]{}, control[life_metrics]{}, work = 0;
    };
    std::array<Group, 16> types{};
    struct Pair {
        Word size = 0, complete = 0, altered = 0;
    };
    Pair pairings[16][16]{};
    Count energy = 0, meals = 0;
    Word best = 0, full = 0, changed_full = 0;
    for (Word i = 0; i < count; ++i) {
        Word slot = slots[i];
        auto &body = h.bodies[slot];
        Word word = h.riggers[slot].genes.plastic >> 8, basal = word & 15;
        Word mask = pressure_regulator(h.genomes[slot], h.conducting(slot)), lut = basal ^ (course.regional ? mask : 0);
        auto &g = all[i];
        auto &t = types[lut];
        ++t.n;
        bool native = g.native[complete_life] == series.streams, paired = g.paired[complete_life] == series.streams;
        auto &pair = pairings[basal][lut];
        ++pair.size;
        pair.complete += native;
        pair.altered += paired;
        t.both += native;
        t.alternate += paired;
        t.passed += paired && !native;
        t.against += native && !paired;
        for (Word k = 0; k < life_metrics; ++k) {
            t.regular[k] += g.native[k];
            t.control[k] += g.paired[k];
        }
        t.work += g.heat;
        energy += g.heat;
        meals += g.native[earned_food];
        full += native;
        changed_full += paired;
        auto rank = [](const LifeGrade &e) {
            return std::tuple{e.native[complete_life], e.native[later_laws], e.native[warm_laws],
                              e.native[earned_food]};
        };
        if (rank(g) > rank(all[best]))
            best = i;
        if (out) {
            out->out << slot;
            for (Count q : std::initializer_list<Count>{body.born, body.generation, 32 * h.conducting(slot),
                                                        h.genomes[slot].pigments[2], h.genomes[slot].grip, word, basal,
                                                        mask, lut, lut ^ 4, series.streams, series.at,
                                                        series.rounds - series.at, g.changes, g.heat, g.probes})
                out->out << '\t' << q;
            for (Word v : g.native)
                out->out << '\t' << v;
            for (Word v : g.paired)
                out->out << '\t' << v;
            out->out << '\n';
        }
    }
    if (out)
        out->publish();
    for (Word lut = 0; lut < 16; ++lut)
        if (types[lut].n) {
            auto &t = types[lut];
            std::cout << "context_rule=" << lut << "  paired_rule=" << (lut ^ 4) << "  bodies=" << t.n
                      << "  warm_trials=" << Count(t.n) * series.streams * series.at
                      << "  later_trials=" << Count(t.n) * series.streams * (series.rounds - series.at)
                      << "  native_warm=" << t.regular[warm_laws] << "  paired_warm=" << t.control[warm_laws]
                      << "  native_later=" << t.regular[later_laws] << "  paired_later=" << t.control[later_laws]
                      << "  native_complete=" << t.both << "  paired_complete=" << t.alternate
                      << "  gained=" << t.passed << "  lost=" << t.against << "  native_food=" << t.regular[earned_food]
                      << "  paired_food=" << t.control[earned_food] << "  equal_heat=" << t.work << '\n';
        }
    if (course.regional)
        for (Word b = 0; b < 16; ++b)
            for (Word m = 0; m < 16; ++m) {
                auto v = pairings[b][m];
                if (v.size)
                    std::cout << "context_pairing=" << b << "  material=" << m << "  bodies=" << v.size
                              << "  native_complete=" << v.complete << "  paired_complete=" << v.altered << '\n';
            }
    Word slot = slots[best];
    Word basal = (h.riggers[slot].genes.plastic >> 8) & 15,
         mask = pressure_regulator(h.genomes[slot], h.conducting(slot));
    auto &actor = h.bodies[slot];
    std::cout << "context_population=" << tick << "  studied=" << count << "  streams=" << series.streams
              << "  episodes=" << series.rounds << "  retained_contacts=" << !course.empty_contacts
              << "  initial_rock=" << course.rock << "  next_rock=" << series.second << "  transition=" << series.at
              << "  full=" << full << "  paired_full=" << changed_full
              << "  cases=" << Count(count) * series.streams * series.rounds << "  meals=" << meals
              << "  heat=" << energy << "  slot=" << slot << "  born=" << actor.born
              << "  generation=" << actor.generation << "  cells=" << 32 * h.conducting(slot)
              << "  basal_rule=" << basal << "  module_mask=" << mask
              << "  native_rule=" << (basal ^ (course.regional ? mask : 0)) << "  regional=" << course.regional
              << "  best_lives=" << all[best].native[complete_life] << "  living_works=" << o.cap_natural
              << "  cuda_ms=" << due.milliseconds() << '\n';
}
