std::optional<regulation::Program> audience_nursery(const Habitat &h, const Options &o, DevicePtr couple,
                                                    audience::Stock stock) {
    Word population = o.inlet_cohort, groups = population / 32, seed = o.class_seed;
    std::vector<Word> living;
    for (Word i = 0; i < n; ++i)
        if (h.bodies[i].energy && h.bodies[i].body.eyes)
            living.push_back(i);
    if (living.empty())
        throw std::invalid_argument("the nursery has no living founders");
    std::vector<apprentice::Animal> adults(population);
    for (Word i = 0; i < population; ++i) {
        Word id = living[mix(seed + 3617 * i) % living.size()];
        auto &g = h.genomes[id];
        adults[i] = {{g.hearing.low, g.hearing.high}, g.brain.reproduction & 0x1f00u, id};
    }
    Count bytes = Count(population) * sizeof(adults[0]);
    Temporary a(s.d, bytes), b(s.d, bytes), food(s.d, Count(population) * sizeof(apprentice::Harvest)),
        ledger(s.d, Count(groups) * sizeof(apprentice::Rations)), heat(s.d, 256 * sizeof(Count)),
        scores(s.d, Count(population) * sizeof(Word));
    s.d.check(s.d.upload(a.ptr, adults.data(), bytes), "enroll living audience loci");
    s.d.check(s.d.zero(ledger.ptr, 0, Count(groups) * sizeof(apprentice::Rations)), "clear brood accounts");
    s.d.check(s.d.zero(heat.ptr, 0, 256 * sizeof(Count)), "clear sensory and vocal expenditure");
    Function graze{}, reproduce{}, examine{};
    s.bind(graze, "audience_feeding", Image::runnels);
    s.bind(reproduce, "apprentice_reproduce", Image::runnels);
    s.bind(examine, "audience_screen", Image::runnels);
    DevicePtr current = a.ptr, next = b.ptr;
    Word mutation = 1, screen_layouts = 24, selection = o.inlet_selected;
    FieldTimer time(s.d);
    time.start(stream);
    for (Word i = 0; i < o.audience_generations; ++i) {
        stock.seed = mix(seed + i * 31259 + 98711);
        Word births = mix(seed + i * 7499 + 17191);
        launch(graze, groups, 32, couple, current, population, stock, food.ptr, heat.ptr);
        launch(reproduce, groups, 32, current, food.ptr, population, births, selection, mutation, next, ledger.ptr);
        std::swap(current, next);
    }
    stock.seed = mix(seed ^ 0xac7eb167u);
    s.d.check(s.d.zero(scores.ptr, 0, Count(population) * sizeof(Word)), "clear common-garden returns");
    launch(examine, groups, 32, couple, current, population, stock, screen_layouts, scores.ptr);
    time.stop(stream);
    synchronize();
    std::vector<Word> returns(population);
    std::array<Count, 256> costs{};
    std::vector<apprentice::Rations> bands(groups);
    s.d.check(s.d.download(adults.data(), current, bytes), "recover surviving circuits");
    s.d.check(s.d.download(returns.data(), scores.ptr, population * sizeof(Word)), "weigh family returns");
    s.d.check(s.d.download(bands.data(), ledger.ptr, groups * sizeof(bands[0])), "read family balances");
    s.d.check(s.d.download(costs.data(), heat.ptr, sizeof(costs)), "weigh heat from speech");
    Count lived = 0, born = 0, carbon = 0, settled = 0, reserve = 0, spent = 0;
    for (auto &r : bands) {
        lived += r.adults;
        born += r.offspring;
        carbon += r.carbon;
        settled += r.washed;
        reserve += r.left;
    }
    Count viewing = 0;
    for (Word i = 0; i < 128; ++i) {
        spent += costs[i];
        viewing += costs[128 + i];
    }
    if (lived * (2 * stock.bolus + audience::allowance(stock)) != carbon + settled + spent + viewing ||
        carbon != 8 * born + reserve)
        throw std::logic_error("the audience brood lost carbon");
    Word winner = ~0u, survivors = 0;
    for (Word i = 0; i < population; ++i)
        if (adults[i].ancestor != ~0u) {
            ++survivors;
            if (winner == ~0u || returns[i] > returns[winner])
                winner = i;
        }
    std::cout << "audience_brood=" << o.audience_generations << "  founders=" << population << "  adult_lives=" << lived
              << "  births=" << born << "  survivors=" << survivors << "  carbon=" << carbon << "  settled=" << settled
              << "  offer_heat=" << spent << "  vocal_heat=" << (stock.tracer ? 0 : spent)
              << "  manual_heat=" << (stock.tracer ? spent : 0) << "  sensory_heat=" << viewing
              << "  reserve=" << reserve << "  parental_food=" << o.inlet_selected << "  bolus=" << stock.bolus
              << "  seed=" << seed << "  cuda_ms=" << time.milliseconds() << '\n';
    if (winner == ~0u)
        return {};
    std::cout << "audience_descendant=" << winner << "  ancestor=" << adults[winner].ancestor
              << "  screen_carbon=" << returns[winner]
              << "  screen_encounters=48  write_gene=" << adults[winner].program.first
              << "  motor_gene=" << adults[winner].program.second << '\n';
    return adults[winner].program;
}
void audience_study(const Habitat &h, const Options &o) {
    if (o.audience_juveniles && !h.braiding)
        throw std::invalid_argument("a native juvenile screen needs expressed waterside hand circuits");
    storage::AgentImage guide, child;
    child.read(o.audience_pupil);
    audience::Pair pair{};
    pair.pupil = audience::embodied(child);
    if (!o.audience_juveniles) {
        guide.read(o.audience_guide);
        pair.guide = audience::embodied(guide);
    }
    if (!o.audience_germ.empty())
        pair.guide.genes = audience::read_germ(o.audience_germ);
    retina::Course water{};
    sensory_surfaces(water, o);
    audience::Stock stock{mix(o.class_seed ^ 0x6394d38bu),
                          o.audience_rounds,
                          o.audience_arm,
                          h.drift_water.law.ports,
                          o.inlet_bolus,
                          water.surfaces,
                          water.sites,
                          o.audience_tracer};
    stock.access = o.audience_access;
    Word count = 2 * o.audience_trials, start = 0;
    bool verbose = !o.audience_trace.empty();
    Word detailed = verbose ? count : 2, groups = (o.audience_trials + 31) / 32;
    Temporary couple(s.d, sizeof(pair)), words(s.d, 2 * sizeof(braid::Narration)),
        kept(s.d, Count(detailed) * sizeof(audience::Contact)), landed(s.d, Count(detailed) * sizeof(audience::Catch));
    Temporary sealed(s.d, verbose ? 0 : Count(count) * sizeof(audience::Commitment)),
        reduced(s.d, verbose ? 0 : Count(groups) * sizeof(audience::Sweep));
    std::array<braid::Narration, 2> terms{guide.braid_report.memory, child.braid_report.memory};
    s.d.check(s.d.upload(couple.ptr, &pair, sizeof(pair)), "seat the guide and pupil in the same light");
    s.d.check(s.d.upload(words.ptr, terms.data(), sizeof(terms)), "restore independent grounded sound contacts");
    Function names{}, meet{}, release{};
    s.bind(names, "audience_names", Image::runnels);
    s.bind(meet, "audience_meet", Image::runnels);
    s.bind(release, "audience_pour", Image::runnels);
    launch(names, 1, 8, couple.ptr, words.ptr);
    synchronize();
    if (o.audience_juveniles) {
        std::vector<Word> ids;
        Word base = mix(o.class_seed + 37123), stride = mix(o.class_seed + 9851) | 1;
        for (Word j = 0; j < n && ids.size() < o.audience_juveniles; ++j) {
            Word id = (base + j * stride) & (n - 1);
            const auto &a = h.bodies[id];
            if (a.energy > 4 * stock.rounds + 3 && a.body.limbs && a.body.eyes && a.body.pores &&
                (o.retina_born == ~0u || a.born > o.retina_born))
                ids.push_back(id);
        }
        if (ids.empty()) {
            std::cout << "audience_juveniles=0  tested=0  selected=0\n";
            return;
        }
        Word total = ids.size();
        Temporary chosen(s.d, ids.size() * sizeof(Word)), eyes(s.d, ids.size() * sizeof(retina::Observer)),
            animals(s.d, ids.size() * sizeof(audience::Animal)), scores(s.d, ids.size() * sizeof(Word));
        Function gather{}, panel{};
        s.bind(gather, "retina_residents", Image::floodplain);
        s.bind(panel, "audience_juveniles", Image::runnels);
        s.d.check(s.d.upload(chosen.ptr, ids.data(), ids.size() * sizeof(Word)),
                  "enroll sighted free-living residents");
        launch(gather, (total + 63) / 64, 64, world, c, chosen.ptr, total, eyes.ptr);
        synchronize();
        std::vector<retina::Observer> retinas(total);
        s.d.check(s.d.download(retinas.data(), eyes.ptr, retinas.size() * sizeof(retinas[0])),
                  "read the enrolled eyes");
        std::vector<audience::Animal> adults(total);
        for (Word j = 0; j < total; ++j) {
            auto &g = h.genomes[ids[j]].hearing;
            adults[j] = {retinas[j], {g.low, g.high}, 1, 1, h.bodies[ids[j]].energy, 1};
        }
        s.d.check(s.d.upload(animals.ptr, adults.data(), adults.size() * sizeof(adults[0])),
                  "place actual juvenile circuits");
        auto garden = stock;
        garden.seed = mix(o.class_seed ^ 0x731ce201u);
        launch(panel, total, 32, couple.ptr, animals.ptr, total, garden, scores.ptr);
        synchronize();
        std::vector<Word> carbon(total);
        s.d.check(s.d.download(carbon.data(), scores.ptr, carbon.size() * sizeof(Word)), "weigh juvenile family food");
        Word winner = std::max_element(carbon.begin(), carbon.end()) - carbon.begin();
        pair.guide = adults[winner];
        s.d.check(s.d.upload(couple.ptr, &pair.guide, sizeof(pair.guide)), "enroll the selected living guide");
        auto &a = h.bodies[ids[winner]];
        std::cout << "audience_juveniles=" << total << "  tested=" << 64u * total
                  << "  selected=1  slot=" << ids[winner] << "  born=" << a.born << "  generation=" << a.generation
                  << "  active_cells=" << 32 * h.conducting(ids[winner]) << "  energy=" << a.energy
                  << "  screen_carbon=" << carbon[winner] << "  screen_seed=" << garden.seed << '\n';
    }
    if (o.audience_generations) {
        auto offspring = audience_nursery(h, o, couple.ptr, stock);
        if (!offspring)
            return;
        pair.guide.genes = *offspring;
        s.d.check(s.d.upload(couple.ptr + offsetof(audience::Pair, guide) + offsetof(audience::Animal, genes),
                             &*offspring, sizeof(*offspring)),
                  "enroll the descendant's inherited circuit");
    }
    Count actual_voice = o.audience_juveniles ? 0 : audience::public_voice(guide), heard_voice = actual_voice;
    if (!o.audience_timbre.empty()) {
        storage::AgentImage visitor;
        visitor.read(o.audience_timbre);
        if (!visitor.binds_floats)
            throw std::invalid_argument("the substituted voice must have a resonator");
        heard_voice = audience::public_voice(visitor);
    }
    if (!o.audience_juveniles)
        stock.calibrated = audience::recall(child, heard_voice, stock.dialect);
    if (o.audience_lessons) {
        Temporary history(s.d, sizeof(audience::Practice)),
            track(s.d,
                  o.audience_lessons_trace.empty() ? 0 : Count(o.audience_lessons) * sizeof(audience::Demonstration));
        Function observe{};
        s.bind(observe, "audience_practice", Image::runnels);
        Word taught = o.audience_lessons, method = o.audience_pairing, demonstration_seed = o.audience_lesson_seed;
        Word guide_budget = 7 * taught, pupil_budget = 3 * taught;
        auto garden = stock;
        garden.paths = nullptr;
        garden.sites = 0;
        launch(observe, 1, 1, couple.ptr, garden, demonstration_seed, taught, method, guide_budget, pupil_budget,
               history.ptr, track.ptr);
        synchronize();
        audience::Practice r{};
        s.d.check(s.d.download(&r, history.ptr, sizeof(r)), "read the pupil's own learned acoustic correspondences");
        stock.dialect = r.learned;
        stock.calibrated = method != 2 || stock.calibrated;
        std::cout << "audience_practice=" << r.offers << "  seed=" << demonstration_seed << "  pairing=" << method
                  << "  paired=" << r.paired << "  grains=" << r.grains << "  moved=" << r.moved
                  << "  edits=" << r.writes << "  guide_heat=" << r.guide_heat << "  pupil_heat=" << r.pupil_heat
                  << "  guide_ration=" << guide_budget << "  pupil_ration=" << pupil_budget
                  << "  pictures=" << r.pictures << "  sounds=" << r.heard << "  first=" << r.learned.bank[0]
                  << "  second=" << r.learned.bank[1] << "  guide_contacts=" << r.guide.contacts
                  << "  pupil_contacts=" << r.pupil.contacts << "  guide_spikes=" << r.guide.spikes
                  << "  pupil_spikes=" << r.pupil.spikes << "  public_tape=" << r.tape
                  << "  inherited_law=" << pair.pupil.lexical_law << "  receptive=" << pair.pupil.receptive << '\n';
        if (track.ptr) {
            std::vector<audience::Demonstration> events(r.offers);
            s.d.check(s.d.download(events.data(), track.ptr, events.size() * sizeof(events[0])),
                      "read witnessed demonstrations");
            EvidenceFile f(o.audience_lessons_trace);
            f.out << "event\tfirst\tsecond\tsource\tdest\topening\tending\tbefore_a\tbefore_b\tafter_a\tafter_"
                     "b\tlaw\treceptive\tpairing\n";
            for (Word i = 0; i < events.size(); ++i) {
                f.out << i;
                for (Word value : std::bit_cast<std::array<Word, 10>>(events[i]))
                    f.out << '\t' << value;
                f.out << '\t' << pair.pupil.lexical_law << '\t' << pair.pupil.receptive << '\t' << method << '\n';
            }
            f.publish();
        }
    }
    if (!o.audience_output.empty()) {
        EvidenceFile f(o.audience_output);
        f.out << "AXION-AUDIENCE-1 " << pair.guide.genes.first << ' ' << pair.guide.genes.second << '\n';
        f.publish();
    }
    std::vector<audience::Contact> contacts(detailed);
    std::vector<audience::Catch> food(detailed);
    s.d.check(s.d.download(&pair, couple.ptr, sizeof(pair)), "read the presented anatomy and acquired nouns");
    audience::Sweep tally{};
    FieldTimer time(s.d);
    if (!verbose) {
        Function commit{}, settle{};
        s.bind(commit, "audience_commit", Image::runnels);
        s.bind(settle, "audience_settle", Image::runnels);
        time.start(stream);
        launch(commit, (count + 63) / 64, 64, couple.ptr, stock, count, sealed.ptr);
        launch(settle, groups, 32, couple.ptr, stock, count, sealed.ptr, reduced.ptr);
        time.stop(stream);
        synchronize();
        std::vector<audience::Sweep> shards(groups);
        s.d.check(s.d.download(shards.data(), reduced.ptr, shards.size() * sizeof(shards[0])),
                  "read warp-level social outcomes");
        for (const auto &g : shards) {
            for (Word k = 0; k < audience::metrics; ++k)
                tally.values[k] += g.values[k];
            tally.example = std::min(tally.example, g.example);
        }
        start = tally.example == ~0u ? 0u : 2 * tally.example;
    } else
        time.start(stream);
    launch(meet, (detailed + 63) / 64, 64, couple.ptr, stock, start, detailed, kept.ptr);
    launch(release, (detailed + 63) / 64, 64, couple.ptr, stock, start, detailed, kept.ptr, landed.ptr);
    if (verbose)
        time.stop(stream);
    synchronize();
    s.d.check(s.d.download(contacts.data(), kept.ptr, contacts.size() * sizeof(contacts[0])),
              "read pre-release choices");
    s.d.check(s.d.download(food.data(), landed.ptr, food.size() * sizeof(food[0])), "weigh independent releases");
    std::unique_ptr<EvidenceFile> trace;
    if (!o.audience_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.audience_trace);
        trace->out
            << "life\tkey\tsite\tmap\ttwin\tfirst\tsecond\tcoat_a\tcoat_b\tviewed\tseen\ttape_before\ttape\t"
               "addressed\twrite_gene\tmotor_gene\tpre_a\tpre_b\tforecast_a\tforecast_b\ttie\tleft\tright\tpicked\t"
               "opened\tsaid_from\tsaid_to\theard_from\theard_to\ttaught\tbefore\tafter\tguide_contacts\tguide_spikes\t"
               "pupil_spikes\tstarting_spikes\tprior_aim\tfinal_aim\tprior_voice\tfinal_voice\twork\tguide_looks\t"
               "pupil_looks\tspoken_order\treceived_order\trelease_from\trelease_to\tfirst_wave\tsecond_wave\tbefore_"
               "catches\tafter_catches\tbefore_words\tafter_words\t"
               "truthful\tfresh\tseed\tarm\trounds\ttracer\tports\tsites\tcalibrated\tcode_a\tcode_b\taccess";
        for (auto prefix : {"guide", "pupil"})
            for (auto name :
                 {"places", "hues", "heading", "muscles", "receptors", "plasticity", "wiring", "initial_contacts",
                  "initial_spikes", "energy", "mouth", "ears", "limbs", "name0", "name1", "name2", "name3"})
                trace->out << '\t' << prefix << '_' << name;
        trace->out << '\n';
    }
    for (Word i = 0; i < detailed; ++i) {
        const auto &r = contacts[i];
        const auto &f = food[i];
        if (verbose)
            audience::weigh(tally, r, f, std::popcount(r.before ^ r.after));
        if (i & 1) {
            auto &other = contacts[i - 1];
            if (r.tape != other.tape || r.guide_spikes != other.guide_spikes ||
                r.guide_contacts != other.guide_contacts || r.tie != other.tie || r.pre_a != other.pre_a ||
                r.pre_b != other.pre_b)
                throw std::logic_error("the listener changed the expert's prior experience");
            if (verbose) {
                tally.values[audience::changed] += r.picked != other.picked && r.opened && other.opened;
                tally.values[audience::both] += f.fresh && food[i - 1].fresh;
            }
        }
        if (trace) {
            auto &out = trace->out;
            out << i / 2;
            const std::array fields{r.key,         r.site,           r.map,          r.twin,         r.first,
                                    r.second,      r.coat_a,         r.coat_b,       r.viewed,       r.seen,
                                    r.tape_before, r.tape,           r.addressed,    r.write_gene,   r.motor_gene,
                                    r.pre_a,       r.pre_b,          r.forecast_a,   r.forecast_b,   r.tie,
                                    r.left,        r.right,          r.picked,       r.opened,       r.said_from,
                                    r.said_to,     r.heard_from,     r.heard_to,     r.taught,       r.before,
                                    r.after,       r.guide_contacts, r.guide_spikes, r.pupil_spikes, r.starting_spikes,
                                    r.prior_aim,   r.final_aim,      r.prior_voice,  r.final_voice,  r.work,
                                    r.guide_looks, r.pupil_looks};
            for (auto x : fields)
                out << '\t' << x;
            out << '\t' << r.spoken_order << '\t' << r.received_order << '\t' << r.release_from << '\t' << r.release_to
                << '\t' << r.first_wave << '\t' << r.second_wave;
            for (Word x : {f.prior, f.final, f.voiced_before, f.voiced_after, f.truthful, f.fresh})
                out << '\t' << x;
            for (Word x : {stock.seed, stock.arm, stock.rounds, stock.tracer, stock.ports, stock.sites,
                           stock.calibrated, stock.dialect.bank[0], stock.dialect.bank[1], stock.access})
                out << '\t' << x;
            for (const auto *agent : {&pair.guide, &pair.pupil}) {
                auto &e = agent->eye;
                for (Word x : {Word(e.eyes.places), Word(e.eyes.hues), e.heading, e.muscles, e.genes.receptors,
                               e.genes.plasticity, e.wiring, e.contacts, e.spikes, agent->energy, agent->mouth,
                               agent->ears, agent->limbs})
                    out << '\t' << x;
                for (Count call : agent->names)
                    out << '\t' << call;
            }
            out << '\n';
        }
    }
    if (trace)
        trace->publish();
    Word example = 0;
    if (verbose)
        for (Word i = 0; i < count; i += 2) {
            auto &r = contacts[i];
            if (food[i].fresh && food[i + 1].fresh && r.picked != contacts[i + 1].picked &&
                braid::port(r.map, r.first) != braid::port(r.map, r.second)) {
                example = i;
                break;
            }
        }
    auto &a = contacts[example], &b = contacts[example + 1];
    std::cout << "audience_scene=" << (start + example) / 2 << "  first=" << a.first << "  second=" << a.second
              << "  map=" << a.map << "  coat_a=" << a.coat_a << "  coat_b=" << a.coat_b << "  viewed_a=" << a.viewed
              << "  viewed_b=" << b.viewed << "  chosen_a=" << a.picked << "  chosen_b=" << b.picked
              << "  heard_a=" << a.taught << "  heard_b=" << b.taught << "  opened_a=" << a.opened
              << "  opened_b=" << b.opened << "  prior_a=" << food[example].prior
              << "  prior_b=" << food[example + 1].prior << "  final_a=" << food[example].final
              << "  final_b=" << food[example + 1].final << '\n';
    std::cout << "audience_exchange=" << pair.guide.eye.slot << "  pupil=" << pair.pupil.eye.slot
              << "  seed=" << stock.seed << "  arm=" << stock.arm << "  layouts=" << o.audience_trials
              << "  encounters=" << count << "  stages=" << stock.rounds << "  surfaces=" << stock.sites
              << "  portions=" << 2 * count << "  caught_before=" << tally.values[audience::prior]
              << "  caught_after=" << tally.values[audience::final]
              << "  words_before=" << tally.values[audience::prior_words]
              << "  words_after=" << tally.values[audience::final_words]
              << "  statements=" << tally.values[audience::statements]
              << "  truthful=" << tally.values[audience::truthful] << "  heard=" << tally.values[audience::heard]
              << "  unseen_source=" << tally.values[audience::fresh]
              << "  different_offers=" << tally.values[audience::changed]
              << "  both_unseen=" << tally.values[audience::both] << "  improved=" << tally.values[audience::helped]
              << "  impaired=" << tally.values[audience::harmed] << "  edits=" << tally.values[audience::edits]
              << "  offer_heat=" << tally.values[audience::effort]
              << "  vocal_heat=" << (stock.tracer ? 0 : tally.values[audience::effort])
              << "  manual_heat=" << (stock.tracer ? tally.values[audience::effort] : 0)
              << "  write_gene=" << pair.guide.genes.first << "  motor_gene=" << pair.guide.genes.second
              << "  guide_places=" << Word(pair.guide.eye.eyes.places)
              << "  guide_hues=" << Word(pair.guide.eye.eyes.hues)
              << "  pupil_places=" << Word(pair.pupil.eye.eyes.places) << "  ports=" << stock.ports
              << "  tracer=" << stock.tracer << "  calibrated=" << stock.calibrated << "  guide_voice=" << actual_voice
              << "  heard_voice=" << heard_voice << "  lessons=" << o.audience_lessons
              << "  pairing=" << o.audience_pairing << "  access=" << stock.access
              << "  seen_neither=" << tally.values[audience::neither_seen + 0]
              << "  seen_one=" << tally.values[audience::neither_seen + 1]
              << "  seen_both=" << tally.values[audience::neither_seen + 2]
              << "  offered_neither=" << tally.values[audience::offer_neither + 0]
              << "  offered_one=" << tally.values[audience::offer_neither + 1]
              << "  offered_both=" << tally.values[audience::offer_neither + 2]
              << "  useful_neither=" << tally.values[audience::fresh_neither + 0]
              << "  useful_one=" << tally.values[audience::fresh_neither + 1] << "  guide_reserve=" << pair.guide.energy
              << "  pupil_reserve=" << pair.pupil.energy
              << "  commitment_bytes=" << (verbose ? sizeof(audience::Contact) : sizeof(audience::Commitment))
              << "  assessment_bytes="
              << (verbose ? Count(count) * sizeof(audience::Catch) : Count(groups) * sizeof(audience::Sweep))
              << "  detail_rows=" << detailed << "  cuda_ms=" << time.milliseconds() << '\n';
}
