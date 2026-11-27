void story_screen(const Habitat &h, const Options &o) {
    if (!h.braiding)
        throw std::invalid_argument("screening requires living waterside brains");
    Word seed = o.class_seed, start = mix(seed + 60289), stride = mix(seed + 20161) | 1;
    std::vector<Word> slots;
    for (Word j = 0; j < n && slots.size() < o.story_residents; ++j) {
        Word id = (start + j * stride) & (n - 1);
        auto &a = h.bodies[id];
        if (a.energy && a.body.eyes && a.body.pores && (o.retina_born == ~0u || a.born > o.retina_born) &&
            (!o.story_history || (!h.trails.empty() && h.trails[id].size() >= o.story_history)))
            slots.push_back(id);
    }
    if (slots.empty()) {
        std::cout << "narrative_cohort=0  enrolled=0  selected=0\n";
        return;
    }
    Word count = slots.size(), gate = o.story_gate, trials = o.story_trials;
    Temporary chosen(s.d, count * sizeof(Word)), organs(s.d, Count(count) * sizeof(utterance::Senses)),
        tissue(s.d, Count(count) * sizeof(utterance::Memory)), score(s.d, count * sizeof(Word)),
        lex(s.d, sizeof(utterance::Culture));
    s.d.check(s.d.upload(chosen.ptr, slots.data(), slots.size() * sizeof(Word)), "enroll the living cortical bands");
    s.d.check(s.d.zero(tissue.ptr, 0, Count(count) * sizeof(utterance::Memory)),
              "give each candidate a fresh vocal organ");
    auto teacher = utterance::culture(o.story_language, 23771, {});
    s.d.check(s.d.upload(lex.ptr, &teacher, sizeof(teacher)), "present the same public dialect to all candidates");
    utterance::Course c{};
    c.seed = seed;
    c.singles = o.story_singles;
    c.couples = o.story_pairs;
    c.length = o.story_span;
    c.purse = o.story_ration;
    c.split = o.story_partition;
    c.arm = o.story_arm;
    c.natural_light = o.story_light;
    retina::Course water{};
    sensory_surfaces(water, o);
    c.beds = water.surfaces;
    c.sites = water.sites;
    Function gather{}, study{};
    s.bind(gather, "narrative_residents", Image::floodplain);
    s.bind(study, "narrative_cohort", Image::runnels);
    Word optics = ink_watch.anchored;
    launch(gather, (count + 63) / 64, 64, world, this->c, chosen.ptr, count, gate, optics, organs.ptr);
    FieldTimer time(s.d);
    time.start(stream);
    launch(study, (count + 31) / 32, 32, tissue.ptr, organs.ptr, count, lex.ptr, c, trials, score.ptr);
    time.stop(stream);
    synchronize();
    std::vector<Word> scores(count);
    s.d.check(s.d.download(scores.data(), score.ptr, scores.size() * sizeof(Word)), "hear the candidate narratives");
    Word best = std::max_element(scores.begin(), scores.end()) - scores.begin(),
         perfect = std::ranges::count(scores, trials);
    auto id = slots[best];
    auto &a = h.bodies[id];
    Count sum = 0;
    std::unique_ptr<EvidenceFile> table;
    if (!o.story_screen.empty()) {
        table = std::make_unique<EvidenceFile>(o.story_screen);
        table->out << "slot\tborn\tgeneration\tcells\tenergy\thistory\tcorrect\ttested\n";
    }
    for (Word i = 0; i < count; ++i) {
        sum += scores[i];
        auto slot = slots[i];
        auto &b = h.bodies[slot];
        if (table)
            table->out << slot << '\t' << b.born << '\t' << b.generation << '\t' << 32 * h.conducting(slot) << '\t'
                       << b.energy << '\t' << (h.trails.empty() ? 0 : h.trails[slot].size()) << '\t' << scores[i]
                       << '\t' << trials << '\n';
    }
    if (table)
        table->publish();
    std::cout << "narrative_cohort=" << tick << "  enrolled=" << count << "  selected=1  exposure=" << c.arm
              << "  light=" << c.natural_light << "  surfaces=" << c.sites << "  seed=" << seed << "  tests=" << trials
              << "  slot=" << id << "  born=" << a.born << "  generation=" << a.generation
              << "  cells=" << 32 * h.conducting(id) << "  events=" << (h.trails.empty() ? 0 : h.trails[id].size())
              << "  trail=" << (h.trails.empty() ? 0 : h.trails[id].bits) << "  best=" << scores[best]
              << "  perfect=" << perfect << "  total_correct=" << sum << "  cuda_ms=" << time.milliseconds() << '\n';
}
std::optional<utterance::Genes> rear_calls(const Habitat &h, const Options &o, utterance::Senses pupil,
                                           utterance::Senses caller, utterance::Course c) {
    Word population = o.inlet_cohort, groups = population / 32, seed = o.class_seed, bolus = o.inlet_bolus,
         selected = o.inlet_selected, mutation = 1;
    std::vector<Word> living;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy && h.bodies[i].body.pores)
            living.push_back(i);
    if (living.empty())
        throw std::invalid_argument("the resonator nursery needs living mouths");
    std::vector<apprentice::Animal> adults(population);
    for (Word i = 0; i < population; ++i) {
        Word slot = living[mix(seed + i * 10687) % living.size()];
        auto &g = h.genomes[slot];
        adults[i] = {{g.voice, g.hearing.expression}, g.brain.reproduction & 0x1f00u, slot};
    }
    Count bytes = Count(population) * sizeof(adults[0]);
    c.singles = o.call_practice;
    Temporary a(s.d, bytes), b(s.d, bytes), food(s.d, Count(population) * sizeof(apprentice::Harvest)),
        ledger(s.d, Count(groups) * sizeof(apprentice::Rations)), heat(s.d, 256 * sizeof(Count)),
        score(s.d, population * sizeof(Word));
    s.d.check(s.d.upload(a.ptr, adults.data(), bytes), "enroll the watershed resonators");
    s.d.check(s.d.zero(ledger.ptr, 0, groups * sizeof(apprentice::Rations)), "clear native family accounts");
    s.d.check(s.d.zero(heat.ptr, 0, 256 * sizeof(Count)), "clear native heat accounts");
    Function graze{}, breed{}, panel{};
    s.bind(graze, "narrative_families", Image::runnels);
    s.bind(breed, "apprentice_reproduce", Image::runnels);
    s.bind(panel, "narrative_choose", Image::runnels);
    DevicePtr parents = a.ptr, young = b.ptr;
    FieldTimer duration(s.d);
    duration.start(stream);
    for (Word t = 0; t < o.story_broods; ++t) {
        c.seed = mix(seed + t * 9341 + 61559);
        Word birth = mix(seed + t * 34213 + 4729);
        launch(graze, groups, 32, parents, population, pupil, caller, c, bolus, food.ptr, heat.ptr);
        launch(breed, groups, 32, parents, food.ptr, population, birth, selected, mutation, young, ledger.ptr);
        std::swap(parents, young);
    }
    c.seed = mix(seed ^ 0x1d3f5e91u);
    launch(panel, groups, 32, parents, population, pupil, caller, c, bolus, score.ptr);
    duration.stop(stream);
    synchronize();
    std::vector<Word> returns(population);
    std::vector<apprentice::Rations> patches(groups);
    std::array<Count, 256> costs{};
    s.d.check(s.d.download(adults.data(), parents, bytes), "recover unpaid and surviving families");
    s.d.check(s.d.download(returns.data(), score.ptr, population * sizeof(Word)), "weigh held-out catches");
    s.d.check(s.d.download(patches.data(), ledger.ptr, groups * sizeof(patches[0])), "weigh brood stock");
    s.d.check(s.d.download(costs.data(), heat.ptr, sizeof(costs)), "weigh heat from all calls");
    Count lives = 0, births = 0, resources = 0, settled = 0, surplus = 0, adult_heat = 0, development = 0;
    for (auto &x : patches) {
        lives += x.adults;
        births += x.offspring;
        resources += x.carbon;
        settled += x.washed;
        surplus += x.left;
    }
    for (Word i = 0; i < 128; ++i) {
        adult_heat += costs[i];
        development += costs[128 + i];
    }
    if (resources + settled + adult_heat != lives * utterance::feeds * (16 + 2 * bolus) ||
        resources != 8 * births + surplus || development > lives * c.singles * 18)
        throw std::logic_error("unfunded resonator brood");
    Word winner = ~0u, alive = 0;
    for (Word i = 0; i < population; ++i)
        if (adults[i].ancestor != ~0u) {
            ++alive;
            if (winner == ~0u || returns[i] > returns[winner])
                winner = i;
        }
    std::cout << "resonator_brood=" << o.story_broods << "  seed=" << seed << "  founders=" << population
              << "  parents=" << lives << "  births=" << births << "  alive=" << alive << "  stock=" << resources
              << "  settled=" << settled << "  left=" << surplus << "  adult_heat=" << adult_heat
              << "  juvenile_heat=" << development << "  juvenile_endowment=" << lives * c.singles * 18
              << "  selected=" << selected << "  bolus=" << bolus << "  cuda_ms=" << duration.milliseconds() << '\n';
    if (winner == ~0u)
        return {};
    auto p = adults[winner].program;
    std::cout << "resonator_germ=" << winner << "  original_slot=" << adults[winner].ancestor
              << "  developmental_returns=" << returns[winner] << "  dendrites=" << p.first << "  motor=" << p.second
              << '\n';
    return utterance::Genes{p.first, p.second};
}
void call_test(utterance::Genes genes, utterance::Senses pupil, utterance::Senses caller, utterance::Course c, Word n,
               Word rounds, Word reversed) {
    c.singles = rounds;
    Temporary result(s.d, Count(n) * sizeof(utterance::Returns));
    Function panel{};
    s.bind(panel, "narrative_listeners", Image::runnels);
    launch(panel, (n + 63) / 64, 64, genes, pupil, caller, c, n, reversed, result.ptr);
    synchronize();
    std::vector<utterance::Returns> scores(n);
    s.d.check(s.d.download(scores.data(), result.ptr, n * sizeof(scores[0])),
              "weigh the blind listeners' first catches");
    Count a = 0, b = 0, heat = 0, school = 0, established = 0, edits = 0;
    for (auto &v : scores) {
        a += v.upper;
        b += v.lower;
        heat += v.adult_heat;
        school += v.school_heat;
        established += v.unique;
        edits += v.edits;
    }
    std::cout << "resonator_exam=" << n << "  seed=" << c.seed << "  practice=" << rounds << "  reverse=" << reversed
              << "  opportunities=" << Count(n) * utterance::feeds << "  upstream=" << a << "  downstream=" << b
              << "  fully_addressed=" << established << "  heat=" << heat << "  developmental_heat=" << school
              << "  edits=" << edits << "  dendrites=" << genes.dendrites << "  motor=" << genes.motor << '\n';
}

void test_accents(const Options &o, utterance::Genes founder, utterance::Senses pupil, utterance::Senses origin,
                  utterance::Course c) {
    utterance::AccentCourse series{
        o.accent_generations, o.call_practice, o.accent_arm, Word(!o.accent_voice.empty()), {}};
    if (series.neighbor)
        series.alternate = utterance::read_voice(o.accent_voice);
    Word n = o.story_trials;
    Temporary cases(s.d, Count(n) * series.links * sizeof(utterance::Accent));
    Function hear{};
    s.bind(hear, "narrative_accents", Image::runnels);
    FieldTimer clock(s.d);
    clock.start(stream);
    launch(hear, (n + 31) / 32, 32, founder, pupil, origin, c, series, n, cases.ptr);
    clock.stop(stream);
    synchronize();
    std::vector<utterance::Accent> scores(Count(n) * series.links);
    s.d.check(s.d.download(scores.data(), cases.ptr, scores.size() * sizeof(scores[0])),
              "read paired private ears and actual emitted pitches");
    std::unique_ptr<EvidenceFile> trace;
    if (!o.accent_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.accent_trace);
        trace->out << "link\tfamily\timages\tfounders_differ\tpupils_differ\tchanged_notes\tfirst_exact\tsecond_"
                      "exact\town_food\tother_food\theat\tedits\twaves\tmatched_scene_food\tswapped_scene_"
                      "food\tmatched_portions\tchanged_with_scene\n";
    }
    using Row = std::array<Word, sizeof(utterance::Accent) / sizeof(Word)>;
    for (Word j = 0; j < series.links; ++j) {
        std::array<Count, std::tuple_size_v<Row>> sum{};
        for (Word i = 0; i < n; ++i) {
            auto row = std::bit_cast<Row>(scores[j * n + i]);
            if (trace)
                trace->out << j + 1 << '\t' << i;
            for (Word k = 0; k < row.size(); ++k) {
                sum[k] += row[k];
                if (trace)
                    trace->out << '\t' << row[k];
            }
            if (trace)
                trace->out << '\n';
        }
        std::cout << "acquired_voice=" << j + 1 << "  chains=" << n << "  images=" << sum[0]
                  << "  founder_difference=" << sum[1] << "  pupils_differ=" << sum[2]
                  << "  altered_syllables=" << sum[3] << "  first_exact=" << sum[4] << "  second_exact=" << sum[5]
                  << "  own_catch=" << sum[6] << "  other_catch=" << sum[7] << "  portions=" << 4 * sum[0]
                  << "  school_heat=" << sum[8] << "  edits=" << sum[9] << "  sounds=" << sum[10]
                  << "  arm=" << series.arm << "  natural_neighbor=" << series.neighbor << "  seed=" << c.seed
                  << "  matched_scene_food=" << sum[11] << "  swapped_scene_food=" << sum[12]
                  << "  matched_portions=" << sum[13] << "  changed_with_scene=" << sum[14]
                  << "  inherited_collaterals=" << pupil.echo << "  total_cuda_ms=" << clock.milliseconds() << '\n';
    }
    if (trace)
        trace->publish();
}
