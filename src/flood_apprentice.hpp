Word contact_mode(const Habitat &h, const Options &o) const {
    bool structural =
        o.contact_remodel != ~0u ? bool(o.contact_remodel) : bool((h.loom.mode & regulation::structural_port) || (h.drift_water.law.ports & drift::regrowth));
    return structural ? apprentice::remodel : 0u;
}
void apprenticeship(const Habitat &h, const Options &o) {
    if (!o.apprentices)
        return;
    if (o.slot >= n || !h.bodies[o.slot].energy || !h.recording_floats || !h.float_events[o.slot].stamp)
        throw std::invalid_argument("apprenticeship needs a living resident with a recorded episode");
    Word slot = o.slot, count = o.apprentices, seed = o.class_seed, practice = o.class_practice,
         reversal = o.feedback_reversal;
    const auto &g = h.genomes[slot];
    Word band = regulation::innervated_band(g, h.conducting(slot));
    Tissue tissue{};
    move_neural(nullptr, &tissue, Count(slot) * tiles + band, 1);
    auto source = regulation::source(g, band, tissue);
    if (o.regulatory_override) {
        source.program = {o.feedback_first, o.feedback_second};
    }
    apprentice::Teachers framework{};
    framework.motor = source;
    Word vocal = regulation::auditory_band(g, h.conducting(slot));
    if ((h.drift_water.law.ports & drift::auditory) && vocal != band) {
        move_neural(nullptr, &tissue, Count(slot) * tiles + vocal, 1);
        framework.voice = regulation::source(g, vocal, tissue);
    }
    Word frames = o.feedback_frames, hand = o.feedback_hand | contact_mode(h, o), varied = o.feedback_rigs;
    Temporary schedules(s.d, Count(count) * frames * 64 * (1 + reversal) * sizeof(unsigned short));
    Temporary output(s.d, Count(count) * sizeof(apprentice::LessonScore));
    Function teach{};
    s.bind(teach, "apprentice_language", Image::runnels);
    FieldTimer timer(s.d);
    std::vector<apprentice::LessonScore> results(count);
    const char *names[] = {"unrestricted", "inherited", "quiet", "displaced", "yoked"};
    std::unique_ptr<EvidenceFile> trace;
    if (!o.feedback_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.feedback_trace);
        trace->out << "arm\tcase\tlanguage\timages\tknown\tfirst\tsecond\tbefore\tdiagnostic\tprobe_mask\tprobe_"
                      "replies\toffered\tanswered\tcorrect\tanswer_"
                      "mask\treplies\tchanged\tboth\terased\tattempts\tmatches\treleased\tedits";
        if (frames == 4)
            for (Word f = 0; f < 4; ++f)
                trace->out << "\tboard" << f << "\tmask" << f << "\treply" << f;
        trace->out << '\n';
    }
    for (Word arm = 0; arm < 5; ++arm) {
        Count mechanical[4]{}, seen[4]{}, different[4]{}, right[4]{};
        timer.start(stream);
        launch(teach, (count + 31) / 32, 32, riggers.ptr, float_events.ptr, slot, framework, count, seed, practice, arm,
               reversal, frames, hand, varied, schedules.ptr, output.ptr);
        timer.stop(stream);
        synchronize();
        float ms = timer.milliseconds();
        s.d.check(s.d.download(results.data(), output.ptr, Count(count) * sizeof(results[0])),
                  "read regulatory classroom outcomes");
        Count healthy = 0, before = 0, recovered = 0, correct = 0, answered = 0, possible = 0, changes = 0, both = 0,
              erased = 0, release = 0, edits = 0, attempts = 0, matches = 0;
        const auto &e = h.float_events[slot];
        for (Word i = 0; i < count; ++i) {
            const auto &r = results[i];
            if (frames == 4)
                for (Word f = 0; f < 4; ++f) {
                    mechanical[f] += r.mechanical[f];
                    seen[f] += r.predicted[f];
                    different[f] += r.divergent[f];
                    right[f] += r.divergent_right[f];
                }
            healthy += r.names == 8 && r.qualified == 8;
            before += r.intermediate == 8;
            recovered += r.intermediate == 8 && r.qualified == 8;
            correct += r.correct;
            answered += r.answered;
            possible += r.offered;
            changes += r.changed;
            both += r.both;
            erased += r.unseen_answers;
            release += r.released;
            edits += r.updates;
            attempts += r.attempts;
            matches += r.matches;
            if (trace) {
                trace->out << names[arm] << '\t' << i << '\t' << mix(seed + i * 7919) << '\t' << e.images << '\t'
                           << e.known << '\t' << source.program.first << '\t' << source.program.second << '\t'
                           << r.intermediate << '\t' << r.qualified << '\t' << r.probe_mask << '\t' << r.probe_replies
                           << '\t' << r.offered << '\t' << r.answered << '\t' << r.correct << '\t' << r.reply_mask
                           << '\t' << r.replies << '\t' << r.changed << '\t' << r.both << '\t' << r.unseen_answers
                           << '\t' << r.attempts << '\t' << r.matches << '\t' << r.released << '\t' << r.updates;
                if (frames == 4)
                    for (Word f = 0; f < 4; ++f)
                        trace->out << '\t' << r.physical[f] << '\t' << r.mechanical_mask[f] << '\t' << r.responses[f];
                trace->out << '\n';
            }
        }
        std::cout << "apprentice_language=" << names[arm] << "  cases=" << count << "  slot=" << slot
                  << "  tick=" << h.header.tick << "  seed=" << seed << "  switch=" << reversal
                  << "  practice=" << practice << "  varied_rigs=" << varied
                  << "  remodel=" << bool(hand & apprentice::remodel) << "  hand=" << (hand & 1)
                  << "  qualified=" << healthy << "  early=" << before << "  twice=" << recovered
                  << "  offered=" << possible << "  answered=" << answered << "  correct=" << correct
                  << "  changed=" << changes << "  both=" << both << "  erased=" << erased << "  attempts=" << attempts
                  << "  agreements=" << matches << "  released_bits=" << release << "  contact_edits=" << edits
                  << "  first=" << source.program.first << "  second=" << source.program.second << "  band=" << band
                  << "  graft=" << o.regulatory_override << "  cuda_ms=" << ms << '\n';
        if (frames == 4)
            for (Word f = 0; f < 4; ++f)
                std::cout << "apprentice_scene=" << names[arm] << "  frame=" << f << "  offered=" << 4 * count
                          << "  perceived=" << seen[f] << "  correct=" << mechanical[f]
                          << "  different=" << different[f] << "  different_correct=" << right[f] << "  seed=" << seed
                          << "  switch=" << reversal << '\n';
    }
    if (trace)
        trace->publish();
}

void rear_regulation(const Habitat &h, const Options &o) {
    if (!o.feedback_generations)
        return;
    if (o.slot >= n || !h.bodies[o.slot].energy || !h.braiding)
        throw std::invalid_argument("select the living background for the regulatory nursery");
    Word count = o.feedback_cohort, seed = o.class_seed, free = o.feedback_free, generations = o.feedback_generations;
    Word slot = o.slot, home_band = regulation::innervated_band(h.genomes[slot], h.conducting(slot));
    Tissue tissue{};
    move_neural(nullptr, &tissue, Count(slot) * tiles + home_band, 1);
    auto body = regulation::source(h.genomes[slot], home_band, tissue);
    Word background = (h.riggers[slot].genes.plastic >> 12) & 15;
    std::cout << "regulatory_natal=" << slot << "  band=" << home_band << "  spikes=" << body.cells.spike
              << "  background=" << body.cells.background << '\n';
    std::vector<apprentice::Animal> founders;
    founders.reserve(count);
    Word first = mix(seed ^ 0x7af319u), stride = mix(seed + 2751) | 1;
    for (Word j = 0; j < n && founders.size() < count; ++j) {
        Word id = (first + j * stride) & (n - 1);
        if (!h.bodies[id].energy)
            continue;
        auto &g = h.genomes[id];
        auto source = regulation::source(g, regulation::innervated_band(g, h.conducting(id)), {});
        Word law = free ? ((h.riggers[id].genes.plastic >> 12) & 15) : background;
        founders.push_back({source.program, law | (g.brain.reproduction & 0x1f00u), id});
    }
    if (founders.empty())
        throw std::invalid_argument("empty regulatory nursery");
    Word available = founders.size();
    while (founders.size() < count)
        founders.push_back(founders[founders.size() % available]);
    Temporary a(s.d, Count(count) * sizeof(founders[0])), b(s.d, Count(count) * sizeof(founders[0])),
        food(s.d, Count(count) * sizeof(apprentice::Harvest));
    Temporary weights(s.d, Count(count / 32) * sizeof(apprentice::Rations));
    Function live{}, breed{};
    s.bind(live, "apprentice_feeding", Image::runnels);
    s.bind(breed, "apprentice_reproduce", Image::runnels);
    std::unique_ptr<EvidenceFile> trace, donor;
    if (!o.feedback_donor.empty())
        donor = std::make_unique<EvidenceFile>(o.feedback_donor);
    if (!o.feeding_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.feeding_trace);
        trace->out << "arm\tgeneration\tslot\tancestor\tfirst\tsecond\tlaw\tswallowed\tescaped\tshift\treturn\tcommitte"
                      "d\tmatches\n";
    }
    std::vector<apprentice::Animal> pop(count);
    std::vector<apprentice::Harvest> captured(count);
    Word one = 1, pulses = 96, mode = apprentice::inherited | contact_mode(h, o);
    for (Word selected = 0; selected < 2; ++selected) {
        s.d.check(s.d.upload(a.ptr, founders.data(), Count(count) * sizeof(founders[0])),
                  "introduce living regulatory modules");
        s.d.check(s.d.zero(weights.ptr, 0, Count(count / 32) * sizeof(apprentice::Rations)),
                  "empty breeding material accounts");
        FieldTimer timer(s.d);
        double kernel_ms = 0;
        bool measuring = false;
        auto current = a.ptr, next = b.ptr;
        for (Word g = 0; g <= generations; ++g) {
            if (g == 0 || g == generations || !(g & (g - 1))) {
                if (measuring) {
                    timer.stop(stream);
                    synchronize();
                    kernel_ms += timer.milliseconds();
                    measuring = false;
                }
                Word test = mix(seed + 0x71b59245u), repeats = 32;
                launch(live, (count + 31) / 32, 32, current, count, test, repeats, pulses, mode, body.cells, food.ptr);
                synchronize();
                s.d.check(s.d.download(pop.data(), current, Count(count) * sizeof(pop[0])),
                          "read inherited regulatory programs");
                s.d.check(s.d.download(captured.data(), food.ptr, Count(count) * sizeof(captured[0])),
                          "weigh the validation harvest");
                Word best = 0, alive = 0;
                Count eaten = 0, escaped = 0, shift = 0, back = 0;
                std::set<std::pair<Word, Word>> circuits;
                for (Word j = 0; j < count; ++j) {
                    auto &p = pop[j];
                    auto &v = captured[j];
                    if (p.ancestor == ~0u)
                        continue;
                    ++alive;
                    if (alive == 1 || v.swallowed > captured[best].swallowed)
                        best = j;
                    eaten += v.swallowed;
                    escaped += v.escaped;
                    shift += v.after_shift;
                    back += v.after_return;
                    circuits.emplace(p.program.first, p.program.second);
                    if (trace && (g == 0 || g == generations))
                        trace->out << selected << '\t' << g << '\t' << j << '\t' << p.ancestor << '\t'
                                   << p.program.first << '\t' << p.program.second << '\t' << (p.traits & 15) << '\t'
                                   << v.swallowed << '\t' << v.escaped << '\t' << v.after_shift << '\t'
                                   << v.after_return << '\t' << v.confident << '\t' << v.matches << '\n';
                }
                if (eaten + escaped != Count(alive) * repeats * pulses)
                    throw std::logic_error("a bead left the audited nursery");
                auto p = pop[best];
                if (selected && g == generations && donor) {
                    if (!alive)
                        throw std::runtime_error("no surviving donor at the requested generation");
                    donor->out << "AXION-REGULATOR-1 " << p.program.first << ' ' << p.program.second << ' '
                               << (p.traits & 15) << ' ' << seed << ' ' << generations << ' ' << p.ancestor << ' '
                               << captured[best].swallowed << ' ' << repeats * pulses << '\n';
                }
                std::cout << "regulatory_brood=" << (selected ? "food" : "uniform") << "  generation=" << g
                          << "  living=" << alive << "  circuits=" << circuits.size() << "  offered=" << eaten + escaped
                          << "  swallowed=" << eaten << "  washed=" << escaped << "  diverted=" << shift
                          << "  returning=" << back << "  best=" << (alive ? best : ~0u) << "  ancestor=" << p.ancestor
                          << "  first=" << p.program.first << "  second=" << p.program.second
                          << "  law=" << (p.traits & 15) << "  best_intake=" << captured[best].swallowed
                          << "  individual_budget=" << repeats * pulses << "  seed=" << seed << "  loci=" << free
                          << '\n';
            }
            if (g == generations)
                break;
            Word climate = mix(seed + g * 4817 + 71), birth = mix(seed + g * 6619 + 307);
            if (!measuring) {
                timer.start(stream);
                measuring = true;
            }
            launch(live, (count + 31) / 32, 32, current, count, climate, one, pulses, mode, body.cells, food.ptr);
            launch(breed, count / 32, 32, current, food.ptr, count, birth, selected, free, next, weights.ptr);
            std::swap(current, next);
        }
        std::vector<apprentice::Rations> mass(count / 32);
        s.d.check(s.d.download(mass.data(), weights.ptr, mass.size() * sizeof(mass[0])),
                  "audit material that funded real births");
        apprentice::Rations total{};
        for (auto &r : mass) {
            total.carbon += r.carbon;
            total.washed += r.washed;
            total.adults += r.adults;
            total.offspring += r.offspring;
            total.left += r.left;
        }
        if (total.carbon + total.washed != total.adults * pulses || total.carbon != 8 * total.offspring + total.left)
            throw std::logic_error("brood carbon is missing");
        std::cout << "brood_budget=" << (selected ? "food" : "uniform") << "  adults=" << total.adults
                  << "  delivered=" << total.adults * pulses << "  swallowed=" << total.carbon
                  << "  washed=" << total.washed << "  births=" << total.offspring << "  unused=" << total.left
                  << "  cuda_ms=" << kernel_ms << "  generations=" << generations << '\n';
    }
    if (trace)
        trace->publish();
    if (donor)
        donor->publish();
}

void report_regulation(const Habitat &h) {
    if (!regulatory_books.ptr)
        return;
    std::vector<regulation::Ledger> shards((n / 4 + 127) / 128);
    s.d.check(s.d.download(shards.data(), regulatory_books.ptr, shards.size() * sizeof(shards[0])),
              "read field motor contacts");
    regulation::Ledger total{};
    for (auto &r : shards) {
        total.contacts += r.contacts;
        total.attempted += r.attempted;
        total.matched += r.matched;
        total.proposed += r.proposed;
        total.admitted += r.admitted;
        total.releases += r.releases;
        total.soma_changes += r.soma_changes;
    }
    if (total.matched > total.attempted || total.attempted > total.contacts || total.admitted > total.proposed ||
        total.admitted > total.releases || total.releases > 16 * total.contacts ||
        total.soma_changes > 32 * total.contacts)
        throw std::logic_error("field regulation ledger exceeds its physical contact budget");
    std::cout << "regulatory_field=" << h.header.tick << "  feedback=" << !(h.loom.mode & regulation::silent_feedback)
              << "  contacts=" << total.contacts << "  attempted=" << total.attempted << "  matched=" << total.matched
              << "  proposed=" << total.proposed << "  admitted=" << total.admitted << "  releases=" << total.releases
              << "  soma_changes=" << total.soma_changes << "  counter_bytes=" << shards.size() * sizeof(shards[0])
              << '\n';
}
void survey_regulation(const Habitat &h, const Options &o, const std::optional<apprentice::Donor> &tracer) {
    if (!o.feedback_survey)
        return;
    if (o.survey_carriers && !tracer)
        throw std::invalid_argument("carrier enrollment needs --trace-donor");
    if (!h.recording_floats)
        throw std::invalid_argument("wild pupil selection needs a resident visual record");
    Function read{}, survey{};
    s.bind(read, "braid_regulatory_sources", Image::floodplain);
    s.bind(survey, "apprentice_survey", Image::runnels);
    Word seed = o.class_seed, practice = o.class_practice, dialects = o.feedback_dialects,
         hand = o.feedback_hand | contact_mode(h, o), varied = o.feedback_rigs;
    const auto possible = calibrate_names(s, seed);
    Word stride = mix(seed + 37) | 1, first = mix(seed ^ 0x6483u), living = 0, sighted = 0, eligible = 0;
    std::vector<Word> ids;
    ids.reserve(std::min(n, o.feedback_survey));
    for (Word j = 0; j < n; ++j) {
        Word id = (first + j * stride) & (n - 1);
        if (!h.bodies[id].energy)
            continue;
        ++living;
        if (!h.bodies[id].body.eyes || !h.float_events[id].stamp)
            continue;
        ++sighted;
        if (o.after_birth != ~0u && h.bodies[id].born <= o.after_birth)
            continue;
        if (o.survey_carriers) {
            auto &g = h.genomes[id];
            auto source = regulation::source(g, regulation::innervated_band(g, h.conducting(id)), {});
            if (!drift::matches(signature(*tracer), source.program, h.riggers[id].genes))
                continue;
        }
        const auto &g = h.riggers[id].genes;
        if ((g.arbor & (3u << 14)) != (3u << 14) || !possible[(g.plastic >> 8) & 15])
            continue;
        ++eligible;
        if (ids.size() < o.feedback_survey)
            ids.push_back(id);
    }
    Word count = ids.size(), size = count * dialects, best = ~0u, best_twice = 0, best_points = 0, best_post = 0,
         passed = 0, always = 0;
    Count both_courses = 0;
    std::unique_ptr<EvidenceFile> trace;
    if (!o.survey_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.survey_trace);
        trace->out << "slot\tborn\tgeneration\tcells\tmodules\tdialects\tpost\ttwice\tpoints\n";
    }
    float elapsed = 0;
    if (count) {
        Temporary slots(s.d, Count(count) * sizeof(Word)), organs(s.d, Count(count) * sizeof(apprentice::Teachers)),
            scores(s.d, Count(size) * sizeof(Word));
        s.d.check(s.d.upload(slots.ptr, ids.data(), Count(count) * sizeof(Word)),
                  "enroll randomly ordered resident slots");
        launch(read, (count + 127) / 128, 128, world, c, slots.ptr, count, organs.ptr);
        FieldTimer timer(s.d);
        timer.start(stream);
        launch(survey, (size + 31) / 32, 32, riggers.ptr, slots.ptr, organs.ptr, count, dialects, seed, practice, hand,
               varied, scores.ptr);
        timer.stop(stream);
        synchronize();
        elapsed = timer.milliseconds();
        std::vector<Word> values(size);
        s.d.check(s.d.download(values.data(), scores.ptr, Count(size) * sizeof(Word)),
                  "read blinded qualification questions");
        for (Word k = 0; k < count; ++k) {
            Word twice = 0, points = 0, post = 0;
            for (Word a = 0; a < dialects; ++a) {
                Word v = values[k * dialects + a], early = v & 15, late = (v >> 4) & 15;
                if ((v >> 8) != 8)
                    continue;
                twice += early == 8 && late == 8;
                post += late == 8;
                points += early + late;
            }
            passed += !!twice;
            always += twice == dialects;
            both_courses += twice;
            if (trace) {
                auto id = ids[k];
                const auto &a = h.bodies[id];
                trace->out << id << '\t' << a.born << '\t' << a.generation << '\t' << 32 * h.conducting(id) << '\t'
                           << h.genomes[id].module_count << '\t' << dialects << '\t' << post << '\t' << twice << '\t'
                           << points << '\n';
            }
            if (best == ~0u || std::tie(twice, post, points) > std::tie(best_twice, best_post, best_points)) {
                best = k;
                best_twice = twice;
                best_points = points;
                best_post = post;
            }
        }
        field_portrait(h, ids[best]);
    }
    std::cout << "regulatory_survey=" << h.header.tick << "  living=" << living << "  observing=" << sighted
              << "  born_after=" << o.after_birth << "  carrier_only=" << o.survey_carriers << "  eligible=" << eligible
              << "  enrolled=" << count << "  dialects=" << dialects << "  qualified_residents=" << passed
              << "  every_course=" << always << "  paired_passes=" << both_courses
              << "  best_slot=" << (count ? ids[best] : ~0u) << "  best_twice=" << best_twice
              << "  best_post=" << best_post << "  best_points=" << best_points << "  seed=" << seed
              << "  cuda_ms=" << elapsed << '\n';
    if (trace)
        trace->publish();
}
void count_carriers(apprentice::Donor donor, Word start) {
    Word blocks = (n + 127) / 128;
    std::vector<apprentice::Carriers> groups(blocks);
    Temporary totals(s.d, Count(blocks) * sizeof(groups[0]));
    Function check{};
    s.bind(check, "shore_carriers", Image::floodplain);
    s.d.check(s.d.zero(totals.ptr, 0, groups.size() * sizeof(groups[0])), "clear carrier census");
    launch(check, blocks, 128, world, c, donor, start, totals.ptr);
    synchronize();
    s.d.check(s.d.download(groups.data(), totals.ptr, groups.size() * sizeof(groups[0])),
              "sample expressed heritable modules");
    apprentice::Carriers sum{};
    for (const auto &r : groups) {
        sum.alive += r.alive;
        sum.exact += r.exact;
        sum.newborn += r.newborn;
        sum.both_ports += r.both_ports;
        sum.neighbours += r.neighbours;
    }
    std::cout << "regulatory_carriers=" << tick << "  start=" << start << "  living=" << sum.alive
              << "  exact=" << sum.exact << "  strict_birth=1  born_since=" << sum.newborn
              << "  both_ports=" << sum.both_ports << "  one_bit_away=" << sum.neighbours
              << "  first=" << donor.program.first << "  second=" << donor.program.second << '\n';
}
