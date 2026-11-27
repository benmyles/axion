void rattle_rear(Word mode, const Options &o) {
    if (!o.rattle_cycles && o.rattle_stock.empty())
        return;
    std::vector<rattle::Genes> stock;
    if (!o.rattle_stock.empty()) {
        Habitat donor;
        donor.read(o.rattle_stock);
        if (donor.rattling)
            for (Word i = 0; i < donor.sites(); ++i)
                if (donor.bodies[i].energy)
                    stock.push_back(donor.listeners[i].genes);
        if (stock.empty())
            throw std::invalid_argument("bank germ donor has no living listeners");
    }
    Word pop = o.rattle_pop, size = stock.size(), salt = o.rattle_seed, paid = o.rattle_paid, scarce = o.rattle_nursery,
         across = !!(mode & rattle::crossed);
    Temporary src(s.d, Count(std::max(size, 1u)) * sizeof(rattle::Genes)), a(s.d, Count(pop) * sizeof(rattle::Family)),
        b(s.d, Count(pop) * sizeof(rattle::Family));
    if (size)
        s.d.check(s.d.upload(src.ptr, stock.data(), Count(size) * sizeof(stock[0])), "bring donor bank germs");
    Function init{}, forage{}, spawn{}, settle{};
    s.bind(init, "rattle_families", Image::runnels);
    s.bind(forage, "rattle_foragers", Image::runnels);
    s.bind(spawn, "rattle_spawn", Image::runnels);
    s.bind(settle, "rattle_immigrate", Image::runnels);
    launch(init, (pop + 127) / 128, 128, a.ptr, pop, salt, src.ptr, size);
    for (Word g = 0; g < o.rattle_cycles; ++g) {
        launch(forage, (pop + 127) / 128, 128, a.ptr, pop, salt, g, scarce, across);
        if (!g || g + 1 == o.rattle_cycles || !((g + 1) & 63)) {
            synchronize();
            std::vector<rattle::Family> brood(pop);
            s.d.check(s.d.download(brood.data(), a.ptr, brood.size() * sizeof(brood[0])),
                      "count withdrawn nursery carbon");
            Count food = 0;
            for (auto &family : brood)
                food += family.food;
            std::cout << "bank_brood=" << g + 1 << "  residents=" << pop << "  food=" << food
                      << "  pulls=" << Count(pop) * (scarce < 2 ? 144 : 72) << "  grips=" << ((scarce & 1) ? 1 : 2)
                      << "  crossed=" << across << "  native_drums=" << Word(scarce >= 2)
                      << "  selection=" << (paid ? "food" : "uniform") << '\n';
        }
        launch(spawn, (pop + 127) / 128, 128, a.ptr, b.ptr, pop, salt, g, paid);
        std::swap(a.ptr, b.ptr);
    }
    launch(settle, (n + 127) / 128, 128, rattlers.ptr, n, a.ptr, pop, salt);
    synchronize();
}
void rattle_bench(const std::vector<rattle::Mind> &pool, Word calls) {
    Word count = std::min(Word(pool.size()), 8192u);
    Temporary m(s.d, Count(count) * sizeof(pool[0])), packed(s.d, Count(count) * 4), out(s.d, Count(calls) * 4);
    s.d.check(s.d.upload(m.ptr, pool.data(), Count(count) * sizeof(pool[0])), "install living listeners");
    Function compile{}, read{};
    s.bind(compile, "rattle_compile", Image::runnels);
    s.bind(read, "rattle_read", Image::runnels);
    Event begin{}, end{};
    s.d.check(s.d.event_new(&begin, 0), "start bank timer");
    s.d.check(s.d.event_new(&end, 0), "create read timer");
    auto time = [&](auto action) {
        action();
        synchronize();
        std::array<float, 7> times{};
        for (auto &ms : times) {
            s.d.check(s.d.record(begin, stream), "mark bank read start");
            action();
            s.d.check(s.d.record(end, stream), "mark bank read end");
            synchronize();
            s.d.check(s.d.elapsed(&ms, begin, end), "time bank read");
        }
        std::sort(times.begin(), times.end());
        return times[3];
    };
    auto build = time([&] { launch(compile, (count + 127) / 128, 128, m.ptr, count, packed.ptr); });
    std::vector<Word> baseline(calls), test(calls);
    float ms[2]{};
    for (Word arm = 0; arm < 2; ++arm) {
        ms[arm] = time([&] { launch(read, (calls + 127) / 128, 128, m.ptr, packed.ptr, count, calls, arm, out.ptr); });
        s.d.check(s.d.download(arm ? test.data() : baseline.data(), out.ptr, Count(calls) * 4),
                  "audit per-choice readout");
    }
    s.d.event_free(begin);
    s.d.event_free(end);
    Count mismatches = 0;
    for (Word i = 0; i < calls; ++i)
        mismatches += test[i] != baseline[i];
    std::cout << "bank_image=" << count << "  pulls=" << calls << "  bytes=" << 4 * count << "  compile_ms=" << build
              << "  live_ms=" << ms[0] << "  packed_ms=" << ms[1] << "  mismatches=" << mismatches << '\n';
    if (mismatches)
        throw std::logic_error("bank readout lost a decision");
}
void rattle_study(Habitat &h, const Options &o) {
    if (!h.rattling)
        return;
    std::vector<rattle::Mind> alive;
    Count contacts = 0, experienced = 0, pending = 0;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy) {
            const auto &m = h.listeners[i];
            alive.push_back(m);
            contacts += rattle::contacts(m);
            experienced += m.contacts != 0;
            pending += rattle::waiting(m);
        }
    Count pitch_up = 0, pitch_down = 0, gated = 0, flat = 0, silent = 0;
    for (auto &m : alive)
        for (Word setting = 0; setting < 4; ++setting) {
            Word low = rattle::knock(m.genes, false, setting & 1, setting >> 1);
            Word high = rattle::knock(m.genes, true, setting & 1, setting >> 1);
            pitch_up += low < 8 && high < 8 && high > low;
            pitch_down += low < 8 && high < 8 && high < low;
            gated += (low == 8) != (high == 8);
            silent += low == 8 && high == 8;
            flat += low == high && low < 8;
        }
    std::cout << "bank_drums=" << alive.size() << "  settings=" << 4 * alive.size() << "  rising=" << pitch_up
              << "  falling=" << pitch_down << "  gated=" << gated << "  flat=" << flat << "  silent=" << silent
              << '\n';
    auto &b = h.backwater.total;
    std::cout << "bank_field=" << h.header.tick << "  living=" << alive.size() << "  experienced=" << experienced
              << "  contacts=" << contacts << "  pending=" << pending << "  phase=" << h.backwater.phase
              << "  mode=" << h.backwater.mode << "  calls=" << b.calls << "  resonances=" << b.resonances
              << "  pulls=" << b.pulls << "  heard=" << b.voiced << "  two_sources=" << b.mixed
              << "  strung=" << b.conducted << "  food=" << b.food << "  returned=" << b.return_food
              << "  work=" << b.cost << "  digested=" << b.settled << "  writes=" << b.edits
              << "  switches=" << b.switches << "  tissue_bytes=" << sizeof(rattle::Mind) << '\n';
    std::cout << "bank_choice=" << h.header.tick << "  sound_changes=" << b.sound_changes
              << "  sound_gain=" << b.sound_gain << "  sound_loss=" << b.sound_loss
              << "  learned_changes=" << b.learned_changes << "  learned_gain=" << b.learned_gain
              << "  learned_loss=" << b.learned_loss << "  resident_changes=" << b.resident_changes
              << "  resident_gain=" << b.resident_gain << "  resident_loss=" << b.resident_loss
              << "  strand_changes=" << b.built_changes << "  strand_gain=" << b.built_gain
              << "  strand_loss=" << b.built_loss << '\n';
    Word bins = std::min(16u, c.side);
    for (Word y = 0; y < bins; ++y) {
        std::string row;
        Word width = c.side / bins;
        for (Word x = 0; x < bins; ++x) {
            Word ears = 0, live = 0, woven = 0;
            for (Word j = 0; j < width; ++j)
                for (Word i = 0; i < width; ++i) {
                    Word site = (y * width + j) * c.side + x * width + i, who = h.slots[site];
                    bool occupied = h.bodies[who].energy != 0;
                    ears += occupied && h.listeners[who].contacts != 0;
                    live += occupied;
                    woven += (h.land[site].fibre_east || h.land[site].fibre_south);
                }
            Word shade = std::min(3u, (4 * woven) / (width * width)), tone = live ? std::min(3u, 8 * ears / live) : 0;
            row += "0123456789abcdef"[shade + 4 * tone];
        }
        std::cout << "bank_map=" << y << "  bins=" << bins << "  samples=" << row << '\n';
    }
    if (h.backwater.format >= 3) {
        auto &t = h.mesh;
        std::cout << "bank_mesh=" << h.header.tick << "  trapped=" << t.trapped << "  simultaneous=" << t.opposed
                  << "  stuck_alone=" << t.pinched << "  extra=" << t.added << "  puller_food=" << t.to_worker
                  << "  gripper_food=" << t.to_gripper << '\n';
    }
    if (h.backwater.format >= 2)
        std::cout << "bank_requests=" << h.header.tick << "  made=" << h.queries.solicited
                  << "  replies=" << h.queries.replies << "  changed=" << h.queries.changed
                  << "  richer=" << h.queries.gain << "  poorer=" << h.queries.loss << '\n';
    if (!o.rattle_speaker.empty()) {
        Word slot = o.slot;
        if (slot == ~0u) {
            slot = 0;
            while (slot < n && !h.bodies[slot].energy)
                ++slot;
        }
        if (slot >= n || !h.bodies[slot].energy)
            throw std::invalid_argument("no living listener to export");
        storage::AgentImage child;
        if (o.rattle_carrier.empty()) {
            // Population extraction needs the somatic and personal nerve arrays.
            transfer(h, false);
            child = extract_individual(h, slot, false);
        } else
            child.read(o.rattle_carrier);
        if (!child.riverine)
            throw std::invalid_argument("a bank speaker requires a watershed body");
        child.hears_banks = true;
        child.rattle_organ.mind = rattle::rebase(h.listeners[slot], h.header.tick, 0);
        storage::write_checked(o.rattle_speaker, child);
        std::cout << "bank_speaker=" << slot << "  source_tick=" << h.header.tick
                  << "  contacts=" << rattle::contacts(child.rattle_organ.mind)
                  << "  carrier=" << (o.rattle_carrier.empty() ? "native" : "introduced") << '\n';
    }
    if (!o.rattle_trials && !o.rattle_calls)
        return;
    if (alive.empty())
        throw std::invalid_argument("bank trials require a living listener");
    if (o.rattle_calls)
        rattle_bench(alive, o.rattle_calls);
    if (!o.rattle_trials)
        return;
    Word size = alive.size(), cases = o.rattle_trials, salt = o.rattle_seed, experience = o.rattle_exposure,
         across = !!(h.backwater.mode & rattle::crossed);
    if (o.rattle_lesion) {
        for (auto &listener : alive) {
            if (o.rattle_lesion == 1)
                listener.genes.ear &= ~(1u << 23);
            else
                listener.genes.latch &= ~(1u << 23);
        }
        std::cout << "bank_lesion=" << (o.rattle_lesion == 1 ? "eligibility" : "precision") << "  copies=" << size
                  << '\n';
    }
    Temporary m(s.d, Count(size) * sizeof(alive[0])), raw(s.d, Count(cases) * sizeof(rattle::Trial));
    s.d.check(s.d.upload(m.ptr, alive.data(), Count(size) * sizeof(alive[0])), "copy resident hearing programs");
    Function test{};
    s.bind(test, "rattle_examine", Image::runnels);
    std::vector<rattle::Genes> imported;
    if (!o.rattle_visitors.empty()) {
        Habitat source;
        source.read(o.rattle_visitors);
        if (source.rattling)
            for (Word id = 0; id < source.sites(); ++id)
                if (source.bodies[id].energy)
                    imported.push_back(source.listeners[id].genes);
        if (imported.empty())
            throw std::invalid_argument("the visiting watershed has no living resonators");
        std::cout << "bank_visitors=" << imported.size() << "  source_tick=" << source.header.tick << "  home=" << size
                  << '\n';
    }
    Word guests = imported.size();
    Temporary drums(s.d, Count(guests) * sizeof(rattle::Genes));
    if (guests)
        s.d.check(s.d.upload(drums.ptr, imported.data(), Count(guests) * sizeof(rattle::Genes)),
                  "present the visiting acoustic bodies");
    std::unique_ptr<EvidenceFile> file;
    if (!o.rattle_trace.empty()) {
        file = std::make_unique<EvidenceFile>(o.rattle_trace);
        file->out << "arm\tcase\tseason\ttwin\tkey\tupstream\tphase\ttags\tdrum\tear\tlaw\tlatch\tcontacts\tstate\tgoal"
                     "\tchoice\tpacket0\tpacket1\tfood\tearly\tlate\tedits\tswitches\theard\trequests\tconsulted\tcross"
                     "ed\tdrum0\tdrum1\n";
    }
    const char *names[]{"open",        "deaf",          "locked",           "mistimed",      "anonymous",
                        "first_only",  "cut_upstream",  "cut_sidepool",     "wild_drums",    "invited",
                        "opposite",    "lottery",       "fixed_attention",  "silent_reply",  "chorus",
                        "chorus_deaf", "chorus_locked", "chorus_anonymous", "chorus_invited"};
    std::vector<rattle::Trial> trials(cases);
    Event start{}, end{};
    float study_ms = 0;
    s.d.check(s.d.event_new(&start, 0), "create bank trial clock");
    s.d.check(s.d.event_new(&end, 0), "create bank finish clock");
    for (Word arm = 0; arm < rattle::treatments; ++arm) {
        s.d.check(s.d.record(start, stream), "begin twin exposure");
        launch(test, (cases + 63) / 64, 64, m.ptr, size, cases, salt, experience, arm, across, drums.ptr, guests,
               raw.ptr);
        s.d.check(s.d.record(end, stream), "end twin exposure");
        synchronize();
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, start, end), "time twin banks");
        study_ms += ms;
        s.d.check(s.d.download(trials.data(), raw.ptr, Count(cases) * sizeof(trials[0])), "read bank probe decisions");
        for (Word phase = 0; phase < rattle::seasons; ++phase) {
            Count pay = 0, early = 0, late = 0, edits = 0, turns = 0, heard = 0, right = 0, both = 0, split = 0,
                  equal = 0, up = 0, asked = 0;
            for (Word i = 0; i < cases; ++i) {
                const auto &t = trials[i];
                const auto &p = t.season[phase];
                Word conflict = p.goals[0] ^ p.goals[1], different = p.choices[0] ^ p.choices[1];
                both += std::popcount(conflict & ~(p.goals[0] ^ p.choices[0]) & ~(p.goals[1] ^ p.choices[1]));
                split += std::popcount(conflict & different);
                equal += (p.packets[0][0] == p.packets[1][0] && p.packets[0][1] == p.packets[1][1]);
                for (Word w = 0; w < 2; ++w) {
                    pay += p.food[w];
                    early += p.first[w];
                    late += p.last[w];
                    edits += p.edits[w];
                    turns += p.turns[w];
                    heard += p.heard[w];
                    Word pos = (t.upstream ^ w ^ (phase & 1)) ? 15u : 240u;
                    if (rattle::one_ear(arm)) {
                        up += 8 - std::popcount(pos ^ p.requests[w]);
                        asked += p.consulted[w];
                    }
                    right += 8 - std::popcount(p.goals[w] ^ p.choices[w]);
                    if (file) {
                        auto &f = file->out;
                        f << names[arm] << '\t' << i << '\t' << phase << '\t' << w << '\t' << mix(salt + i * 7949u)
                          << '\t' << (t.upstream ^ w ^ (phase & 1)) << '\t' << t.orientation << '\t' << t.tags;
                        for (auto gene : t.genes)
                            f << '\t' << gene;
                        f << '\t' << p.contact[w] << '\t' << p.state[w] << '\t' << p.goals[w] << '\t' << p.choices[w]
                          << '\t' << p.packets[w][0] << '\t' << p.packets[w][1] << '\t' << p.food[w] << '\t'
                          << p.first[w] << '\t' << p.last[w] << '\t' << p.edits[w] << '\t' << p.turns[w] << '\t'
                          << p.heard[w] << '\t' << p.requests[w] << '\t' << p.consulted[w] << '\t' << across << '\t'
                          << t.drums[0] << '\t' << t.drums[1] << '\n';
                    }
                }
            }
            std::cout << "bank_test=" << names[arm] << "  season=" << phase + 1 << "  cases=" << cases
                      << "  exposure=" << experience << "  crossed=" << across << "  probes=" << 16 * cases
                      << "  correct=" << right << "  conflicts=" << 4 * cases << "  both=" << both
                      << "  revised=" << split << "  identical=" << equal << "  hits_early=" << early
                      << "  hits_late=" << late << "  window=" << 2 * cases * std::min(8u, experience)
                      << "  food=" << pay << "  heard=" << heard << "  source_requests=" << up
                      << "  upstream_during=" << asked << "  writes=" << edits << "  switches=" << turns << '\n';
        }
    }
    s.d.event_free(start);
    s.d.event_free(end);
    std::cout << "bank_timing=" << study_ms << "  cases=" << Count(cases) * rattle::treatments
              << "  probes=" << Count(cases) * rattle::treatments * 48
              << "  paid_pulls=" << Count(cases) * rattle::treatments * 6 * experience << '\n';
    if (file)
        file->publish();
}
