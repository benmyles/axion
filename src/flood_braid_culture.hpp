void braid_culture(Habitat &h, const Options &o, const std::vector<braid::Genes> &parents) {
    Word cases = o.braid_villages, generations = o.braid_lives, visits = o.braid_visits, seed = o.braid_seed,
         size = parents.size();
    Temporary genes(s.d, Count(size) * sizeof(parents[0]));
    s.d.check(s.d.upload(genes.ptr, parents.data(), Count(size) * sizeof(parents[0])),
              "examine living auditory lineages");
    if (!o.braid_chorister.empty()) {
        Word index = o.braid_village, twin = o.braid_history;
        Temporary trained(s.d, sizeof(braid::Mind)), facts(s.d, 11 * 4);
        Function graduate{};
        s.bind(graduate, "braid_graduate", Image::runnels);
        launch(graduate, 1, 1, genes.ptr, size, index, seed, generations, visits, twin, trained.ptr, facts.ptr);
        synchronize();
        Word read[11];
        braid::Mind juvenile{};
        s.d.check(s.d.download(&juvenile, trained.ptr, sizeof(juvenile)), "retain the graduate's actual contacts");
        s.d.check(s.d.download(read, facts.ptr, sizeof(read)), "identify the matched body");
        Word slot = 0, which = 0;
        while (slot < n && !(h.bodies[slot].energy && which++ == read[0]))
            ++slot;
        if (slot == n)
            throw std::logic_error("float graduate lacks its sampled body");
        transfer(h, false);
        auto image = extract_individual(h, slot, false);
        image.braided.mind = juvenile;
        image.braided.words = {};
        for (Word i = 0; i < braid::capacity(juvenile.genes); ++i)
            image.braided.words.wave[i] = Count(1) << i;
        storage::write_checked(o.braid_chorister, image);
        std::cout << "braid_graduate=" << index << "  history=" << twin << "  slot=" << slot << "  rig=" << read[1]
                  << "  stable=" << read[2] << "  native=";
        for (Word i = 0; i < 8; ++i)
            std::cout << (i ? "," : "") << read[i + 3];
        std::cout << '\n';
    }
    if (!cases)
        return;
    Temporary raw(s.d, Count(cases) * sizeof(braid::FilterVillage));
    std::vector<braid::FilterVillage> data(cases);
    Function run{};
    s.bind(run, "braid_cultures", Image::runnels);
    std::unique_ptr<EvidenceFile> paper;
    if (!o.braid_song.empty()) {
        paper = std::make_unique<EvidenceFile>(o.braid_song);
        paper->out
            << "arm\tcase\ttwin\tgerm\trig\tstable\tfinal_"
               "stable\tdistinct\tquery\torder\tresult\twords\tunderstood\toperated\tstrokes\tmotor\tfood\twanted";
        for (Word i = 0; i < 8; ++i)
            paper->out << "\tfounder" << i << "\telder" << i;
        paper->out << '\n';
    }
    const char *names[]{"shared", "muted", "unpaired", "opaque", "shifted", "unhandled", "one_body"};
    for (Word arm = 0; arm < braid::cohort_arms; ++arm) {
        launch(run, (cases + 31) / 32, 32, genes.ptr, size, cases, seed, generations, visits, arm, raw.ptr);
        synchronize();
        s.d.check(s.d.download(data.data(), raw.ptr, Count(cases) * sizeof(data[0])),
                  "follow transmitted physical conventions");
        Count coined = 0, words = 0, seen = 0, paid = 0, practice = 0, attested = 0, targets = 0, grounded = 0,
              food = 0, both = 0, opposed = 0, named = 0, rich = 0, uttered = 0, conveyed = 0, worked = 0, earned = 0,
              complete = 0, vary = 0, translated = 0;
        std::array<Count, 8> kept{};
        std::set<std::array<Word, 8>> codes;
        for (Word i = 0; i < cases; ++i) {
            auto &v = data[i];
            auto &q = v.workshop;
            for (Word j = 0; j < 2; ++j) {
                coined += q.coined[j];
                words += q.emitted[j];
                seen += q.shown[j];
                paid += q.paid[j];
                practice += q.practised[j];
                food += q.food[j];
                named += std::popcount(q.consistent[j]);
                rich += v.distinct[j] >= 3;
                std::array<Word, 8> signature{};
                for (Word k = 0; k < 8; ++k)
                    signature[k] = (q.consistent[j] & (1u << k)) ? q.first[j][k] + 1 : 0;
                if (q.consistent[j])
                    codes.insert(signature);
                for (Word k = 0; k < 8; ++k)
                    kept[k] += q.stage[j][k];
                for (Word k = 0; k < 56; ++k) {
                    auto &a = q.cases[j][k];
                    attested += a.available;
                    targets += a.received && a.end == a.wanted;
                    grounded += a.received;
                }
                for (Word k = 0; k < 24; ++k) {
                    auto &a = v.requests[j][k];
                    uttered += !!a.spoken;
                    conveyed += a.understood;
                    worked += a.pulls;
                    earned += a.food;
                    complete += a.exact && a.spoken;
                    if (k) {
                        auto &other = v.requests[j ^ 1][k];
                        translated += a.exact && other.exact && a.spoken && other.spoken &&
                                      (a.phrase != other.phrase || a.spoken != other.spoken);
                    }
                    if (paper) {
                        auto &f = paper->out;
                        f << names[arm] << '\t' << i << '\t' << j << '\t' << q.muscle[j] << '\t' << q.rig[j] << '\t'
                          << q.consistent[j] << '\t' << q.familiar[j] << '\t' << v.distinct[j] << '\t' << k << '\t'
                          << a.phrase << '\t' << a.after << '\t' << a.spoken << '\t' << a.understood << '\t' << a.exact
                          << '\t' << a.pulls << '\t' << a.sequence << '\t' << a.food << '\t' << braid::assembly(k);
                        for (Word t = 0; t < 8; ++t)
                            f << '\t' << q.first[j][t] << '\t' << q.final[j][t];
                        f << '\n';
                    }
                }
            }
            for (Word k = 0; k < 56; ++k) {
                auto &a = q.cases[0][k], &b = q.cases[1][k];
                bool diff = a.available && b.available && a.wanted != b.wanted;
                opposed += diff;
                bool success = diff && a.received && b.received && a.end == a.wanted && b.end == b.wanted;
                both += success;

                vary += success && a.strokes && b.strokes;
            }
        }
        std::cout << "braid_culture=" << names[arm] << "  communities=" << 2 * cases << "  generations=" << generations
                  << "  meetings=" << visits << "  coined=" << coined << "  spoken=" << words << "  seen=" << seen
                  << "  adult_work=" << paid << "  independent_pulls=" << practice << "  stable_calls=" << named
                  << "  distinct_codes=" << codes.size() << "  three_meanings=" << rich
                  << "  all_pairs=" << Count(cases) * 112 << "  defined=" << attested << "  understood=" << grounded
                  << "  productive=" << targets << "  carbon=" << food << "  different=" << opposed << "  both=" << both
                  << "  both_moved=" << vary << "  filter_targets=" << Count(cases) * 48 << "  encoded=" << uttered
                  << "  delivered=" << conveyed << "  physical=" << complete << "  filter_food=" << earned
                  << "  response_pulls=" << worked << "  different_phrases=" << translated / 2
                  << "  first_retained=" << kept[0] << "  final_recorded=" << kept[std::min(8u, generations) - 1]
                  << '\n';
    }
    if (paper)
        paper->publish();
}
