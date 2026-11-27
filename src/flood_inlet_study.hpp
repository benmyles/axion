struct InletBackground {
    Temporary cells;
    Word count = 0;

    InletBackground(Basin &basin, const Habitat &h, bool needed, Word seed)
        : cells(basin.s.d, needed ? Count(std::min(h.sites(), 2048u)) * sizeof(inlet::Initial) : 0) {
        if (!needed)
            return;
        std::vector<Word> ids;
        Word n = h.sites(), start = mix(seed + 5459), stride = mix(seed + 45911) | 1;
        for (Word j = 0; j < n && ids.size() < 2048; ++j) {
            Word slot = (start + j * stride) & (n - 1);
            if (h.bodies[slot].energy)
                ids.push_back(slot);
        }
        count = ids.size();
        if (!count)
            throw std::invalid_argument("no living cortical background to sample");
        Temporary slots(basin.s.d, Count(count) * sizeof(Word));
        basin.s.d.check(basin.s.d.upload(slots.ptr, ids.data(), Count(count) * sizeof(Word)),
                        "select living motor populations");
        Function gather{};
        basin.s.bind(gather, "inlet_ambient", Image::floodplain);
        basin.launch(gather, (count + 63) / 64, 64, basin.world, basin.c, slots.ptr, count, cells.ptr);
        basin.synchronize();
    }
    void attach(inlet::Course &course, const Options &o) const {
        course.ambient = reinterpret_cast<const inlet::Initial *>(cells.ptr);
        course.backgrounds = count;
        course.carry = Word(o.inlet_ambient) | (Word(o.inlet_history) << 1);
        course.former_all = o.inlet_old_all;
    }
};
void inlet_interview(const Habitat &h, const Options &o, Word id) {
    if (id >= n || !h.bodies[id].energy || !h.bodies[id].body.eyes || !h.braiding)
        throw std::invalid_argument("choose a living sighted inlet witness with --slot or --follow-witness");
    std::ifstream file(o.inlet_questions);
    if (!file)
        throw std::invalid_argument("cannot read the prospective interview");
    inlet::Questions script{};
    script.names = interview_names(file);
    script.predict = interview_line(file, &script.variable);
    script.actual = interview_line(file);
    script.source = interview_line(file, &script.source_variable);
    std::string extra;
    if (file >> extra)
        throw std::invalid_argument("unexpected additional prospective question");
    braid::Narration speaker{};
    auto saved = std::ranges::find(h.narrators, id, &Habitat::ResidentNarration::slot);
    if (saved != h.narrators.end() && saved->born == h.bodies[id].born)
        speaker = saved->mind;
    Word ports = h.drift_water.law.ports;
    auto &hand = h.riggers[id];
    auto &g = h.genomes[id];
    Word band = regulation::innervated_band(g, h.conducting(id));
    Tissue current{};
    move_neural(nullptr, &current, Count(id) * tiles + band, 1);
    auto organ = regulation::source(g, band, current);
    inlet::Pupil pupil{
        {organ.program, hand.genes.plastic & 15, drift::wiring(hand.genes, ports), (hand.genes.arbor >> 12) & 1},
        braid::eye_wiring(braid::individual_eyes, hand.genes, h.bodies[id].heading),
        (hand.genes.arbor >> 25) & 7,
        (hand.genes.muscle >> 24) & 7};
    if (o.inlet_carried) {
        pupil.contacts = hand.volley >> 16;
        pupil.cells = organ.cells;
    }
    Word seed = mix(o.class_seed ^ 0x1835f781u), cases = o.inlet_trials * 2;
    inlet::Course course{o.inlet_practice, o.inlet_prior, o.inlet_landscape, ports};
    InletBackground ambient(*this, h, (o.inlet_ambient || o.inlet_history) && !o.inlet_carried, o.class_seed);
    ambient.attach(course, o);
    std::vector<inlet::Before> committed(cases);
    std::vector<inlet::After> heard(cases * 2);
    Temporary vocabulary(s.d, sizeof(speaker)), q(s.d, sizeof(script)), early(s.d, cases * sizeof(committed[0])),
        later(s.d, heard.size() * sizeof(heard[0]));
    s.d.check(s.d.upload(vocabulary.ptr, &speaker, sizeof(speaker)),
              "present the living speaker's acquired vocabulary");
    s.d.check(s.d.upload(q.ptr, &script, sizeof(script)), "voice the prospective questions");
    Function before{}, after{};
    s.bind(before, "inlet_before", Image::runnels);
    s.bind(after, "inlet_after", Image::runnels);
    FieldTimer timer(s.d);
    timer.start(stream);
    launch(before, (cases + 63) / 64, 64, vocabulary.ptr, pupil, q.ptr, seed, cases, course, early.ptr);
    synchronize();
    s.d.check(s.d.download(committed.data(), early.ptr, cases * sizeof(committed[0])),
              "seal acoustic answers before opening the fourth inlet");
    launch(after, (cases * 2 + 63) / 64, 64, vocabulary.ptr, pupil, q.ptr, seed, cases, course, early.ptr, later.ptr);
    timer.stop(stream);
    synchronize();
    s.d.check(s.d.download(heard.data(), later.ptr, heard.size() * sizeof(heard[0])),
              "hear the same question after two distinct releases");
    std::unique_ptr<EvidenceFile> trace;
    if (!o.inlet_voice_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.inlet_voice_trace);
        trace->out << "seed\tlife\ttwin\tredirected\tsteps\tprechange\tlandscape\tports\tsenses\tmuscles\tplaces\thues"
                      "\tdye\tstart\tsampled\tcontacts\toptions\tplanned\tlocal\tsaid_before\tpremature\tno_"
                      "scene\ttopic\tseen\tsaid_after\tsaid_start\tno_topic\tinitial_contacts\tinitial_"
                      "spikes\tbackground\tspikes\t"
                      "incoming\tearlier_rows\tfirst\tsecond\tlaw\twiring\tactive\tformer_all\n";
    }
    Count forecasts = 0, right = 0, early_answers = 0, blind = 0, sources = 0, actual = 0, cleared = 0, both = 0,
          corrected = 0, complete = 0, motor_right = 0, local_right = 0;
    for (Word i = 0; i < cases; ++i) {
        auto &r = committed[i];
        Word withheld = drift::face(pupil.senses, braid::port(braid::inverse_view(pupil.eyes.places), r.start));
        if (braid::visible(r.observed, braid::result) || r.sampled & (1u << withheld))
            throw std::logic_error("the withheld outlet entered the prospective interview");
        auto &natural = heard[2 * i], &moved = heard[2 * i + 1];
        if (natural.position == moved.position)
            throw std::logic_error("moving the couplings failed to change the outcome");
        forecasts += r.projected < 8;
        right += r.projected == natural.position;
        motor_right += r.preferred == natural.position;
        local_right += r.proximal == natural.position;
        early_answers += r.early < 8;
        blind += r.silent < 8;
        both += natural.actual == natural.position && moved.actual == moved.position;
        corrected += r.projected == natural.position && moved.actual == moved.position;
        for (Word turn = 0; turn < 2; ++turn) {
            auto &x = heard[2 * i + turn];
            actual += x.actual == x.position;
            sources += x.source == r.start;
            cleared += x.cleared < 8;
            complete += r.projected == natural.position && x.actual == x.position && x.source == r.start;
            if (trace)
                trace->out << seed << '\t' << i / 2 << '\t' << i % 2 << '\t' << turn << '\t' << course.steps << '\t'
                           << course.prior_steps << '\t' << course.landscape << '\t' << ports << '\t' << pupil.senses
                           << '\t' << pupil.muscles << '\t' << Word(pupil.eyes.places) << '\t' << Word(pupil.eyes.hues)
                           << '\t' << r.dye << '\t' << r.start << '\t' << r.sampled << '\t' << r.contacts << '\t'
                           << r.apertures << '\t' << r.preferred << '\t' << r.proximal << '\t' << r.projected << '\t'
                           << r.early << '\t' << r.silent << '\t' << r.topic << '\t' << x.position << '\t' << x.actual
                           << '\t' << x.source << '\t' << x.cleared << '\t' << r.initial.contacts << '\t'
                           << r.initial.cells.spike << '\t' << r.initial.cells.background << '\t' << r.spikes << '\t'
                           << r.incoming << '\t' << r.earlier_rows << '\t' << pupil.genes.program.first << '\t'
                           << pupil.genes.program.second << '\t' << pupil.genes.law << '\t' << pupil.genes.wiring
                           << '\t' << pupil.genes.active << '\t' << course.former_all << '\n';
        }
    }
    if (trace)
        trace->publish();
    if (!committed.empty()) {
        const auto &r = committed.front();
        auto &a = heard[0];
        auto &b = heard[1];
        std::cout << "inlet_speech_scene=0  marker=" << braid::pigment(pupil.eyes.hues, r.dye)
                  << "  planned=" << r.projected << "  premature=" << r.early << "  actual=" << a.position
                  << "  reported=" << a.actual << "  turned=" << b.position << "  second_report=" << b.actual
                  << "  origin=" << r.start << "  named_origin=" << a.source << '\n';
    }
    Word voice = (ports & drift::auditory) ? regulation::auditory_band(g, h.conducting(id)) : band;
    auto vocal_program = regulation::source(g, voice, {}).program;
    Word resolved = 0;
    for (const auto &form : speaker.forms)
        resolved += !braid::sequence_form(form) && form.examples && std::popcount(65535u ^ form.pruned) == 1;
    std::cout << "inlet_anatomy=" << id << "  motor_band=" << band << "  vocal_band=" << voice
              << "  motor_first=" << organ.program.first << "  motor_second=" << organ.program.second
              << "  vocal_first=" << vocal_program.first << "  vocal_second=" << vocal_program.second
              << "  resolved_forms=" << resolved << "  tissue_cells=" << 32 * h.conducting(id)
              << "  modules=" << g.module_count << '\n';
    std::cout << "inlet_conversation=" << id << "  tick=" << tick << "  born=" << h.bodies[id].born << "  seed=" << seed
              << "  carried=" << o.inlet_carried << "  practice=" << course.steps
              << "  prior_practice=" << course.prior_steps << "  former_all=" << course.former_all
              << "  background_samples=" << course.backgrounds << "  courses=" << cases
              << "  motor_correct=" << motor_right << "  proximal_correct=" << local_right
              << "  answered_before=" << forecasts << "  foresaw=" << right << "  premature=" << early_answers
              << "  no_scene=" << blind << "  releases=" << cases * 2 << "  actual=" << actual
              << "  origins=" << sources << "  both_releases=" << both << "  revised=" << corrected
              << "  complete=" << complete << "  cleared_topic=" << cleared << "  cuda_ms=" << timer.milliseconds()
              << '\n';
}

std::vector<apprentice::Animal> rear_inlets(const Options &o, const std::vector<apprentice::Animal> &founders,
                                            inlet::Course course) {
    Word count = o.inlet_cohort, generations = o.inlet_generations, selected = o.inlet_selected;
    Word all_loci = 2, bolus = o.inlet_bolus, groups = count / 32;
    std::vector<apprentice::Animal> population(count);
    for (Word i = 0; i < count; ++i)
        population[i] = founders[mix(o.class_seed + i * 11713u) % founders.size()];
    Count bytes = Count(count) * sizeof(population[0]);
    Temporary adults(s.d, bytes), offspring(s.d, bytes), intake(s.d, Count(count) * sizeof(apprentice::Harvest)),
        accounts(s.d, Count(groups) * sizeof(apprentice::Rations));
    s.d.check(s.d.upload(adults.ptr, population.data(), bytes), "hatch untrained copies of sampled field germlines");
    s.d.check(s.d.zero(accounts.ptr, 0, Count(groups) * sizeof(apprentice::Rations)), "empty the tidal nurseries");
    Function feed{}, breed{};
    s.bind(feed, "inlet_graze", Image::floodplain);
    s.bind(breed, "apprentice_reproduce", Image::runnels);
    DevicePtr current = adults.ptr, next = offspring.ptr;
    std::vector<apprentice::Rations> regions(groups);
    std::vector<apprentice::Harvest> meals(count);
    FieldTimer timer(s.d);
    timer.start(stream);
    for (Word generation = 0;; ++generation) {
        if (!generation || generation == generations || generation % 64 == 0) {
            // This common climate never funds a birth. Every evaluation uses
            // fresh tissues and keeps the natal population untouched.
            Word climate = mix(o.class_seed ^ 0x839ece01u);
            launch(feed, groups, 32, current, count, climate, course, bolus, intake.ptr);
            synchronize();
            s.d.check(s.d.download(meals.data(), intake.ptr, Count(count) * sizeof(meals[0])),
                      "weigh a common test tide");
            s.d.check(s.d.download(population.data(), current, bytes), "count live cortical germlines");
            Count eaten = 0, escaped = 0, correct = 0;
            Word live = 0;
            for (Word i = 0; i < count; ++i) {
                live += population[i].ancestor != ~0u;
                eaten += meals[i].swallowed;
                escaped += meals[i].escaped;
                correct += meals[i].matches;
            }
            if (eaten + escaped != Count(live) * (course.steps + course.prior_steps + bolus))
                throw std::logic_error("a comparison tide lost suspended food");
            std::cout << "inlet_brood=" << (selected ? "food" : "uniform") << "  generation=" << generation
                      << "  living=" << live << "  material=" << eaten + escaped << "  eaten=" << eaten
                      << "  escaped=" << escaped << "  first_meals=" << correct << "  bolus=" << bolus << '\n';
        }
        if (generation == generations)
            break;
        Word tide = mix(o.class_seed + generation * 9371u + 213), birth = mix(o.class_seed + generation * 7139u + 137);
        launch(feed, groups, 32, current, count, tide, course, bolus, intake.ptr);
        launch(breed, groups, 32, current, intake.ptr, count, birth, selected, all_loci, next, accounts.ptr);
        std::swap(current, next);
    }
    timer.stop(stream);
    synchronize();
    s.d.check(s.d.download(regions.data(), accounts.ptr, Count(groups) * sizeof(regions[0])),
              "audit the material paid for newborns");
    apprentice::Rations sum{};
    for (auto &g : regions) {
        sum.adults += g.adults;
        sum.offspring += g.offspring;
        sum.carbon += g.carbon;
        sum.washed += g.washed;
        sum.left += g.left;
    }
    if (sum.carbon != sum.offspring * 8 + sum.left ||
        sum.carbon + sum.washed != sum.adults * (course.steps + course.prior_steps + bolus))
        throw std::logic_error("unfunded reproduction at the unseen inlet");
    std::cout << "inlet_nursery=" << generations << "  population=" << count << "  adult_lives=" << sum.adults
              << "  births=" << sum.offspring << "  eaten=" << sum.carbon << "  settled=" << sum.washed
              << "  reserve=" << sum.left << "  bytes_per_genome=" << sizeof(population[0])
              << "  cuda_ms=" << timer.milliseconds() << '\n';
    return population;
}

void inlet_study(const Habitat &h, const Options &o) {
    if (!h.braiding)
        throw std::invalid_argument("an unseen-inlet study needs living float-hand tissue");
    struct Group {
        Word copies = 0, slot = ~0u;
    };
    using Genes = std::array<Word, 5>;
    std::map<Genes, Group> sampled;
    std::vector<apprentice::Animal> founders;
    Word living = 0, seed = o.class_seed;
    Word start = mix(seed ^ 0x19127c9u), stride = mix(seed ^ 0x52f03a3du) | 1;
    for (Word j = 0; j < (o.slot == ~0u ? n : 1u) && living < o.inlet_residents; ++j) {
        Word id = o.slot == ~0u ? (start + j * stride) & (n - 1) : o.slot;
        if (id >= n || !h.bodies[id].energy)
            continue;
        const auto &g = h.genomes[id];
        auto p = regulation::source(g, regulation::innervated_band(g, h.conducting(id)), {}).program;
        const auto &hand = h.riggers[id].genes;
        auto &group = sampled[{p.first, p.second, hand.plastic & 15, drift::wiring(hand, h.drift_water.law.ports),
                               (hand.arbor >> 12) & 1}];
        ++group.copies;
        group.slot = std::min(group.slot, id);
        if (o.inlet_generations) {
            Word traits = drift::lateral_hand(hand.plastic & 15, drift::wiring(hand, h.drift_water.law.ports),
                                              (hand.arbor >> 12) & 1);
            founders.push_back({p, traits | (((g.brain.reproduction >> 8) & 31) << 22), id});
        }
        ++living;
    }
    if (!living)
        throw std::invalid_argument("the requested inlet study has no living participant");
    Word ports = h.drift_water.law.ports;
    inlet::Course course{o.inlet_practice, o.inlet_prior, o.inlet_landscape, ports};
    InletBackground ambient(*this, h, o.inlet_ambient || o.inlet_history, o.class_seed);
    ambient.attach(course, o);
    if (o.inlet_generations) {
        sampled.clear();
        living = 0;
        for (auto &animal : rear_inlets(o, founders, course)) {
            if (animal.ancestor == ~0u)
                continue;
            auto &group = sampled[{animal.program.first, animal.program.second, animal.traits & 15,
                                   (animal.traits >> 5) & 131071u, (animal.traits >> 4) & 1}];
            group.slot = std::min(group.slot, animal.ancestor);
            ++group.copies;
            ++living;
        }
        if (!living)
            throw std::runtime_error("the material nursery has no living descendant");
    }
    std::unique_ptr<storage::AgentImage> host;
    if (!o.inlet_host.empty()) {
        host = std::make_unique<storage::AgentImage>();
        host->read(o.inlet_host);
        if (!host->binds_floats || !host->riverine)
            throw std::invalid_argument("the common inlet host needs embodied float tissue");
    }
    if (o.inlet_cross) {
        const auto &h = *host;
        Word width = h.grows_cells ? h.cortex.active : living_tiles(h.brain.tiles, h.brain.genes.development);
        auto program =
            regulation::source(h.resident.germ, regulation::innervated_band(h.resident.germ, width), {}).program;
        std::map<Genes, Group> crosses;
        for (const auto &[a, g] : sampled) {
            Genes key = a;
            key[0] = program.first;
            key[1] = program.second;
            if (o.inlet_cross == 1) {
                key[2] = h.braided.mind.genes.plastic & 15u;
                key[4] = (h.braided.mind.genes.arbor >> 12) & 1;
            }
            auto &entry = crosses[key];
            entry.copies += g.copies;
            entry.slot = std::min(entry.slot, g.slot);
        }
        sampled.swap(crosses);
    }
    std::vector<inlet::Animal> genes;
    std::vector<Group> subjects;
    for (const auto &[v, group] : sampled) {
        genes.push_back({{v[0], v[1]}, v[2], v[3], v[4]});
        subjects.push_back(group);
    }
    Word count = genes.size(), lives = o.inlet_trials, steps = o.inlet_practice;
    std::vector<inlet::Totals> scores(count);
    Temporary population(s.d, Count(count) * sizeof(genes[0])), totals(s.d, Count(count) * sizeof(scores[0]));
    s.d.check(s.d.upload(population.ptr, genes.data(), Count(count) * sizeof(genes[0])),
              "enrol the measured inlet tissues");
    Function probe{};
    s.bind(probe, "inlet_panel", Image::floodplain);
    inlet::Class school{};
    Function hear{};
    bool audition = !o.inlet_host.empty();
    if (audition) {
        auto &pupil = *host;
        auto &g = pupil.resident.germ;
        Word width =
            pupil.grows_cells ? pupil.cortex.active : living_tiles(pupil.brain.tiles, pupil.brain.genes.development);
        Word band = regulation::innervated_band(g, width);
        auto organ = regulation::source(g, band, pupil.brain.tissue[band]);
        school = {pupil.braided.mind,  organ.cells,
                  o.inlet_dialects,    o.class_practice,
                  o.feedback_reversal, o.inlet_shared,
                  o.feedback_frames,   o.feedback_hand | (o.contact_remodel == 1 ? Word(apprentice::remodel) : 0u)};
        Word vocal = regulation::auditory_band(g, width);
        bool auditory = o.voice_route != ~0u ? bool(o.voice_route) : bool(pupil.braid_report.memory.auditory_route);
        if (auditory && vocal != band)
            school.voice = regulation::source(g, vocal, pupil.brain.tissue[vocal]);
        s.bind(hear, "inlet_class", Image::runnels);
    }
    Temporary exams(s.d, audition ? Count(count) * school.lives * sizeof(Word) : 0);
    std::vector<Word> reports(audition ? Count(count) * school.lives : 0);
    std::vector<Word> first_words(count), both_words(count);
    Word winner = 0;
    std::unique_ptr<EvidenceFile> table;
    if (!o.inlet_table.empty()) {
        table = std::make_unique<EvidenceFile>(o.inlet_table);
        table->out << "panel\tseed\tindex\tslot\tcopies\tfirst\tsecond\tlaw\twiring\tactive\tpredicted\tlocal\tdisplace"
                      "d\tcertain\tcertain_right\tcrossed\tpaired\ttraining_food\tchannels_right\tbasins_right\tfirst_"
                      "dialects\tboth_dialects\n";
    }
    for (Word panel = 0; panel < 2; ++panel) {
        Word key = panel ? mix(seed ^ 0xb35cad27u) : seed;
        DevicePtr no_trace = 0;
        FieldTimer timer(s.d);
        timer.start(stream);
        launch(probe, count, 32, population.ptr, count, key, lives, course, totals.ptr, no_trace);
        timer.stop(stream);
        synchronize();
        float ms = timer.milliseconds();
        s.d.check(s.d.download(scores.data(), totals.ptr, Count(count) * sizeof(scores[0])),
                  "read unseen-inlet choices");
        if (audition) {
            Word language = mix(key ^ 0x488b7167u);
            launch(hear, (count * school.lives + 31) / 32, 32, population.ptr, count, language, school, exams.ptr);
            synchronize();
            s.d.check(s.d.download(reports.data(), exams.ptr, reports.size() * sizeof(Word)),
                      "hear independent dialect examinations");
            std::fill(first_words.begin(), first_words.end(), 0);
            std::fill(both_words.begin(), both_words.end(), 0);
            for (size_t j = 0; j < reports.size(); ++j) {
                auto v = reports[j];
                first_words[j / school.lives] += (v & 15) == 8;
                both_words[j / school.lives] += (v & 15) == 8 && ((v >> 4) & 15) == 8;
            }
        }
        auto rank = [&](Word i) {
            const auto &q = scores[i];
            Word weaker = course.landscape == inlet::mixed ? std::min(q.channel_right, q.basin_right) : 0u;
            Word joint = audition
                             ? std::min((course.landscape == inlet::mixed ? 2u * weaker : q.predicted) * school.lives,
                                        2 * lives * both_words[i])
                             : 0;
            return std::tuple{joint, weaker, q.paired, q.predicted, q.trained_food, subjects[i].copies};
        };
        Count first_choices = 0, paired = 0;
        for (Word i = 0; i < count; ++i) {
            const auto &a = genes[i];
            const auto &m = subjects[i];
            const auto &x = scores[i];
            if (x.proximal != lives / 2 || x.absent_right != 2 * lives || (steps >= 3 && x.full_evidence != 2 * lives))
                throw std::logic_error("withheld inlet or independent outlet lost its matched control");
            if (!panel && rank(i) > rank(winner))
                winner = i;
            first_choices += Count(m.copies) * x.predicted;
            paired += Count(m.copies) * x.paired;
            if (table)
                table->out << panel << '\t' << key << '\t' << i << '\t' << m.slot << '\t' << m.copies << '\t'
                           << a.program.first << '\t' << a.program.second << '\t' << a.law << '\t' << a.wiring << '\t'
                           << a.active << '\t' << x.predicted << '\t' << x.proximal << '\t' << x.displaced << '\t'
                           << x.committed << '\t' << x.assured << '\t' << x.crossed << '\t' << x.paired << '\t'
                           << x.trained_food << '\t' << x.channel_right << '\t' << x.basin_right << '\t'
                           << first_words[i] << '\t' << both_words[i] << '\n';
        }
        const auto &a = genes[winner];
        const auto &g = subjects[winner];
        const auto &v = scores[winner];
        std::cout << "inlet_panel=" << (panel ? "confirmation" : "selection") << "  tick=" << tick << "  seed=" << key
                  << "  residents=" << living << "  programs=" << count << "  courses=" << 2 * lives
                  << "  practice=" << steps << "  prechange=" << course.prior_steps
                  << "  former_all=" << course.former_all << "  history=" << o.inlet_history
                  << "  landscape=" << course.landscape << "  ports=" << ports << "  champion=" << winner
                  << "  slot=" << g.slot << "  copies=" << g.copies << "  generations=" << o.inlet_generations
                  << "  selected=" << o.inlet_selected << "  cross=" << o.inlet_cross
                  << "  background_samples=" << ambient.count << "  first=" << a.program.first
                  << "  second=" << a.program.second << "  law=" << a.law << "  wiring=" << a.wiring
                  << "  active=" << a.active << "  correct=" << v.predicted << "  local=" << v.proximal
                  << "  rotated=" << v.displaced << "  certain=" << v.committed << "  certain_right=" << v.assured
                  << "  different_targets=" << v.crossed << "  pairs_right=" << v.paired
                  << "  with_other_history=" << v.alternate_right << "  eligible=" << v.full_evidence
                  << "  detached_right=" << v.absent_right << "  detached_offered=" << 8 * lives
                  << "  channels=" << v.channel_cases << "  channels_right=" << v.channel_right
                  << "  basins=" << v.basin_cases << "  basins_right=" << v.basin_right
                  << "  contradicted=" << v.contradicted << "  recovered=" << v.recovered
                  << "  training_food=" << v.trained_food << "  population_correct=" << first_choices
                  << "  population_paired=" << paired << "  cuda_ms=" << ms << '\n';
        if (audition)
            std::cout << "inlet_audition=" << (panel ? "confirmation" : "selection") << "  slot=" << g.slot
                      << "  dialects=" << school.lives << "  early=" << first_words[winner]
                      << "  both=" << both_words[winner] << "  reversed=" << school.reversal
                      << "  shared_vocal_law=" << school.shared << "  hand=" << school.mode
                      << "  correct_at_inlet=" << v.predicted << "  first_reaches=" << 2 * lives << '\n';
        if (!panel)
            continue;
        Word one = 1, witnessed = o.inlet_trace.empty() ? 1u : lives;
        DevicePtr single = population.ptr + Count(winner) * sizeof(inlet::Animal);
        std::vector<inlet::Report> records(2 * witnessed);
        Temporary witness(s.d, records.size() * sizeof(records[0]));
        launch(probe, one, 32, single, one, key, witnessed, course, totals.ptr, witness.ptr);
        synchronize();
        s.d.check(s.d.download(records.data(), witness.ptr, records.size() * sizeof(records[0])),
                  "inspect both physical histories");
        for (Word twin = 0; twin < 2; ++twin) {
            const auto &r = records[twin];
            std::cout << "inlet_example=" << twin << "  map=" << r.physical_map << "  unseen=" << r.hold
                      << "  witnessed=" << r.sampled << "  chosen=" << r.answer << "  target=" << r.target
                      << "  local=" << r.proximal << "  contacts=" << r.contacts << "  options=" << r.choices
                      << "  practiced=" << steps << '\n';
        }
        if (o.inlet_trace.empty())
            continue;
        EvidenceFile trace(o.inlet_trace);
        trace.out
            << "seed\tlife\ttwin\tsteps\tports\tfirst\tsecond\tlaw\twiring\tactive\tchannels\tpipes\toutputs\thold\tswa"
               "p\ttarget\tsampled\thits\tcontacts\tspikes\toptions\tdraw\tanswer\tlocal\trotated\tprechange\tlandscape"
               "\tshape\tinitial_spikes\tbackground\tinitial_contacts\tincoming\tformer_all\tearlier_rows\n";
        for (Word j = 0; j < records.size(); ++j) {
            const auto &e = records[j];
            trace.out << key << '\t' << j / 2 << '\t' << j % 2 << '\t' << steps << '\t' << ports << '\t'
                      << a.program.first << '\t' << a.program.second << '\t' << a.law << '\t' << a.wiring << '\t'
                      << a.active << '\t' << e.inlet << '\t' << e.pipes << '\t' << e.outputs << '\t' << e.hold << '\t'
                      << e.swap << '\t' << e.target << '\t' << e.sampled << '\t' << e.hits << '\t' << e.contacts << '\t'
                      << e.spikes << '\t' << e.choices << '\t' << e.draw << '\t' << e.answer << '\t' << e.proximal
                      << '\t' << e.displaced << '\t' << course.prior_steps << '\t' << course.landscape << '\t'
                      << e.landscape << '\t' << e.initial_spikes << '\t' << e.background << '\t' << e.initial_contacts
                      << '\t' << e.incoming << '\t' << course.former_all << '\t' << e.earlier_rows << '\n';
        }
        trace.publish();
    }
    if (table)
        table->publish();
    if (!o.inlet_germ.empty()) {
        const auto &a = genes[winner];
        const auto &g = subjects[winner];
        EvidenceFile record(o.inlet_germ);
        record.out << (o.inlet_cross                ? "AXION-INLET-CROSS-1 "
                       : o.inlet_generations        ? "AXION-INLET-BROOD-1 "
                       : ports & drift::recruitment ? "AXION-INLET-GERM-2 "
                                                    : "AXION-INLET-GERM-1 ")
                   << a.program.first << ' ' << a.program.second << ' ' << a.law << ' '
                   << (o.inlet_generations ? o.class_seed : h.header.configuration.seed) << ' '
                   << (o.inlet_generations ? o.inlet_generations : h.bodies[g.slot].generation) << ' ' << g.slot << ' '
                   << tick << ' ' << g.copies << ' ' << living << ' ' << a.wiring << ' ' << a.active << '\n';
        record.publish();
    }
}
