#include "flood_capillary_speech.hpp"
#include "flood_context_study.hpp"
#include "flood_context_cohort.hpp"
#include "flood_signal_study.hpp"
#include "flood_compound_study.hpp"
#include "flood_capillary_speed.hpp"
std::unique_ptr<Temporary> capillary_works(const Habitat &h, const Options &o, capillary::Course &course,
                                           Word requested = 0) {
    if (!o.cap_natural)
        return {};
    Word size = requested ? requested : o.cap_trials;
    auto store = std::make_unique<Temporary>(s.d, Count(size) * sizeof(capillary::Work));
    Function mine{};
    s.bind(mine, "capillary_quarry", Image::floodplain);
    Word now = tick;
    auto weather = h.cap_law;
    launch(mine, (size + 63) / 64, 64, world, c, weather, now, course.seed, course.depth, course.scarce, size,
           store->ptr);
    return store;
}
void capillary_screen(const Habitat &h, const Options &o) {
    if (!h.braiding)
        throw std::invalid_argument("a capillary cohort needs inherited sensory and motor equipment");
    std::unique_ptr<storage::AgentImage> tutor;
    if (!o.signal_tutor.empty()) {
        tutor = std::make_unique<storage::AgentImage>();
        tutor->read(o.signal_tutor);
        auto body = capillary::phenotype(*tutor);
        if (!body.eyes || !body.limbs || !body.mouth)
            throw std::invalid_argument("the tutor needs eyes, feeding pores and manipulators");
    }
    std::vector<Word> slots;
    Word start = mix(o.class_seed + 4783), stride = mix(o.class_seed + 11321) | 1;
    for (Word k = 0; k < n && slots.size() < o.cap_members; ++k) {
        Word slot = (start + k * stride) & (n - 1);
        auto &a = h.bodies[slot];
        if (!a.energy || !a.body.limbs || !a.body.eyes || (o.retina_born != ~0u && a.born <= o.retina_born))
            continue;
        if (tutor && slot == tutor->resident.slot && a.born == tutor->resident.animal.born &&
            h.genomes[slot].grip == tutor->resident.germ.grip)
            continue;
        if (o.signal.lessons && !a.body.pores)
            continue;
        auto m = h.cap_memories.empty() ? capillary::Mind{} : h.cap_memories[slot];
        if (capillary::count(m.tissue.on | m.tissue.off) >= o.cap_min)
            slots.push_back(slot);
    }
    if (o.signal.lessons && slots.size() < 2) {
        std::cout << "material_relay=" << tick << "  pairs=0\n";
        return;
    }
    if (slots.empty()) {
        if (o.context_series.streams)
            std::cout << "context_population=" << tick << "  studied=0\n";
        else
            std::cout << "capillary_cohort=0  enrolled=0\n";
        return;
    }
    Word animals = slots.size(), trials = o.cap_trials;
    capillary::Course course = o.cap_course;
    course.seed = o.class_seed;
    course.gate = o.story_gate;
    course.adaptive = o.cap_context == ~0u ? bool(h.cap_law.mode & capillary::contextual) : bool(o.cap_context);
    course.regional = o.cap_context == ~0u ? bool(h.cap_law.mode & capillary::regional_synapses) : o.cap_context == 3;
    auto boards = capillary_works(h, o, course, o.context_series.streams * o.context_series.rounds);
    DevicePtr natural = boards ? boards->ptr : 0;
    Temporary histories(s.d, course.adaptive ? Count(animals) * sizeof(capillary::History) : 0);
    Temporary appetites(s.d, o.signal.lessons && o.signal.retained ? animals : 0);
    Temporary ids(s.d, animals * 4), bodies(s.d, Count(animals) * sizeof(capillary::Body)),
        priors(s.d, Count(animals) * sizeof(capillary::Mind)),
        grades(s.d,
               (o.context_series.streams || o.signal.lessons) ? 0 : Count(animals) * sizeof(capillary::CohortGrade));
    s.d.check(s.d.upload(ids.ptr, slots.data(), animals * 4), "enroll living causal learners");
    Function gather{}, test{};
    s.bind(gather, "capillary_residents", Image::floodplain);
    s.bind(test, "capillary_population", Image::runnels);
    launch(gather, (animals + 63) / 64, 64, world, c, ids.ptr, animals, bodies.ptr, priors.ptr, histories.ptr,
           appetites.ptr);
    if (o.signal.lessons) {
        if(o.signal_channels==2)compound_cohort_study(h,o,slots,bodies.ptr,tutor.get());
        else signal_study(h,o,slots,bodies.ptr,appetites.ptr,tutor.get());
        return;
    }
    if (o.context_series.streams) {
        context_cohort_study(h, o, slots, course, bodies.ptr, priors.ptr, histories.ptr, natural);
        return;
    }
    FieldTimer clock(s.d);
    clock.start(stream);
    launch(test, animals, 32, bodies.ptr, priors.ptr, histories.ptr, animals, course, trials, natural, grades.ptr);
    clock.stop(stream);
    synchronize();
    std::vector<capillary::CohortGrade> scores(animals);
    s.d.check(s.d.download(scores.data(), grades.ptr, scores.size() * sizeof(scores[0])),
              "count real repairs by cohort");
    std::vector<capillary::Body> expressed(o.cap_screen.empty() ? 0 : animals);
    if (!expressed.empty())
        s.d.check(s.d.download(expressed.data(), bodies.ptr, expressed.size() * sizeof(expressed[0])),
                  "record the actual regulatory programs expressed by the cohort");
    std::unique_ptr<EvidenceFile> file;
    if (!o.cap_screen.empty()) {
        file = std::make_unique<EvidenceFile>(o.cap_screen);
        file->out
            << "slot\tborn\tgeneration\tcells\tcontacts\tpairs\tcaught\tcommitted\tfull_law\tprobes\tfounder\tbody_"
               "origin\tbrain_origin\toffspring\tmeals\tdendrites\tgrip\tplasticity\tcapacity\tlocal_"
               "rule\tarbor\tprogram_first\tprogram_second\tspike_release\tbasal_rule\tmodule_mask\n";
    }
    Word best = 0, full = 0;
    Count eaten = 0;
    for (Word j = 0; j < animals; ++j) {
        Word slot = slots[j];
        auto &a = h.bodies[slot];
        auto &g = scores[j];
        eaten += g.caught;
        full += g.strong == 2 * trials;
        auto rank = [](auto q) { return std::tuple{q.caught, q.first, q.strong}; };
        if (rank(g) > rank(scores[best]))
            best = j;
        if (file)
            file->out << slot << '\t' << a.born << '\t' << a.generation << '\t' << 32 * h.conducting(slot) << '\t'
                      << (h.cap_memories.empty()
                              ? 0
                              : capillary::count(h.cap_memories[slot].tissue.on | h.cap_memories[slot].tissue.off))
                      << '\t' << g.first << '\t' << g.caught << '\t' << g.opens << '\t' << g.strong << '\t'
                      << g.disagreed << '\t' << a.founder << '\t' << a.body_origin << '\t' << a.brain_origin << '\t'
                      << a.offspring << '\t' << a.meals << '\t' << h.genomes[slot].pigments[2] << '\t'
                      << h.genomes[slot].grip << '\t' << (h.riggers[slot].genes.plastic >> 8) << '\t'
                      << capillary::width(h.genomes[slot].pigments[2]) << '\t'
                      << ((expressed[j].growth.law ^ (course.regional ? expressed[j].module_mask : 0)) & 15) << '\t'
                      << expressed[j].growth.arbor << '\t' << expressed[j].growth.program.first << '\t'
                      << expressed[j].growth.program.second << '\t' << expressed[j].growth.release << '\t'
                      << (expressed[j].growth.law & 15) << '\t' << expressed[j].module_mask << '\n';
    }
    if (file)
        file->publish();
    Word slot = slots[best];
    auto &a = h.bodies[slot];
    std::cout << "capillary_cohort=" << tick << "  enrolled=" << animals << "  trials=" << trials
              << "  empty_contacts=" << course.empty_contacts << "  completely_predicted=" << full
              << "  earned=" << eaten << "  slot=" << slot << "  born=" << a.born << "  generation=" << a.generation
              << "  cells=" << 32 * h.conducting(slot) << "  best=" << scores[best].caught
              << "  complete_local=" << scores[best].strong << "  cuda_ms=" << clock.milliseconds() << '\n';
}
void capillary_study(const Habitat &h, const Options &o) {
    if (o.cap_members) {
        capillary_screen(h, o);
        return;
    }
    using namespace capillary;
    storage::AgentImage pupil;
    pupil.read(o.cap_pupil);
    auto body = capillary::phenotype(pupil);
    Course c = o.cap_course;
    c.seed = o.class_seed;
    c.gate = o.story_gate;
    c.adaptive = o.cap_context == ~0u ? bool(pupil.channels.adaptive & 1) : bool(o.cap_context);
    c.regional = o.cap_context == ~0u ? pupil.channels.adaptive == 3 : o.cap_context == 3;
    body.pressure_xor = c.regional ? body.module_mask : 0;
    if (o.context_series.streams) {
        context_study(h, pupil, body, c, o);
        if (!o.cap_trials)
            return;
    }
    Word total = o.cap_trials * 2;
    capillary::Tissue natal = c.empty_contacts ? capillary::Tissue{} : pupil.channels.mind.tissue;
    auto history = c.empty_contacts ? History{} : pupil.channels.history;
    auto boards = capillary_works(h, o, c);
    DevicePtr natural = boards ? boards->ptr : 0;
    Temporary proposals(s.d, Count(total) * sizeof(Output)), grades(s.d, Count(total) * sizeof(Result)),
        exposure(s.d, o.cap_lesson.empty() ? 0 : 2 * Count(c.probes + c.views) * sizeof(Step));
    Function learn{}, open{};
    s.bind(learn, "capillary_learn", Image::runnels);
    s.bind(open, "capillary_release", Image::runnels);
    FieldTimer tuition(s.d);
    tuition.start(stream);
    launch(learn, (total + 127) / 128, 128, body, c, total, natural, natal, history, proposals.ptr, exposure.ptr);
    tuition.stop(stream);
    FieldTimer grading(s.d);
    grading.start(stream);
    launch(open, (total + 127) / 128, 128, c, total, natural, proposals.ptr, grades.ptr);
    grading.stop(stream);
    synchronize();
    std::vector<Output> plans(total);
    std::vector<Result> results(total);
    s.d.check(s.d.download(plans.data(), proposals.ptr, plans.size() * sizeof(Output)),
              "read the sealed capillary designs");
    s.d.check(s.d.download(results.data(), grades.ptr, results.size() * sizeof(Result)),
              "weigh deposits after the hardware opens");
    std::vector<Work> layouts(boards ? o.cap_trials : 0);
    if (boards)
        s.d.check(s.d.download(layouts.data(), boards->ptr, layouts.size() * sizeof(Work)),
                  "read sampled physical capillaries");
    std::unique_ptr<EvidenceFile> trace;
    if (!o.cap_trace.empty()) {
        trace = std::make_unique<EvidenceFile>(o.cap_trace);
        trace->out << "pair\trock\tseed\tdepth\tmoves\tdendrites\tmotor\tsenses\twick\twet\tsill\tcoating\tfeed\tcatche"
                      "r\twaste\ttools\tinitial_on\tinitial_off\ton\toff\tmodel_on\tmodel_"
                      "off\tsupports\trivalry\tknown\tpredictions\tplans\tshortest\tprior_moves\tprior_"
                      "ready\taction\tready\talternatives\tcaught\tleaked\theat\tlooks\tacts\tdiagnostic\tfingerprint\t"
                      "supply\tfeasible\torder\tstrokes\tfinal\tremaining\tmoved\tdenied\tcontext_mode\tcontext_"
                      "passive\tcontext_receipts\n";
    }
    Count caught = 0, before = 0, possible = 0, shortest = 0, refused = 0, invented = 0, motions = 0, heat = 0,
          edits = 0, spikes = 0, tests = 0, investigations = 0, diagnostic = 0, local = 0;
    Count queried = 0, correct = 0, spurious = 0, conflicts = 0, both = 0, diverged = 0, impossible = 0;
    Count denied = 0, salvaged = 0, ordered = 0, needed_stock = 0;
    Count passed = 0, settled = 0;
    Count affordable = 0;
    for (Word m = 0; m < (1u << grips); ++m)
        if (Word(std::popcount(m)) <= c.moves)
            affordable |= bit(m);
    for (Word i = 0; i < total; ++i) {
        auto &p = plans[i];
        auto &r = results[i];
        Word pair = i >> 1, rock = c.rock ^ ((i & 1) << 4);
        auto w = boards ? layouts[pair] : channel(mix(c.seed ^ mix(pair * 3517 + 77893)), c.depth, c.scarce);
        possible += r.shortest <= c.moves;
        caught += r.caught;
        impossible += r.shortest > c.moves;
        refused += !p.ready;
        denied += r.actual.denied;
        bool prerequisite = perform(w, r.commands, true).denied != 0;
        ordered += p.ready && prerequisite;
        salvaged += r.caught && prerequisite;
        needed_stock += r.caught && r.commands.n >= 3 && prerequisite;
        before += p.prior_ready && bool(r.solutions & bit(p.prior_actions));
        invented += p.ready && !r.caught;
        shortest += r.caught && Word(std::popcount(p.action)) == r.shortest;
        if (p.ready)
            motions += std::popcount(p.action);
        heat += p.heat;
        edits += p.edits;
        spikes += p.spikes;
        tests += p.looks;
        investigations += p.acts;
        diagnostic += p.informative;
        local += p.single_known;
        queried += count(p.known);
        correct += count(~(p.predictions ^ r.solutions) & p.known);
        spurious += count((p.predictions ^ r.solutions) & p.known);
        if (i & 1) {
            auto a = results[i - 1].solutions & results[i - 1].feasible & affordable,
                 b = r.solutions & r.feasible & affordable;
            if (a && b && !(a & b)) {
                ++conflicts;
                both += r.caught && results[i - 1].caught;
            }
            diverged += plans[i - 1].action != p.action;
        }
        if (trace) {
            Count tools = 0;
            for (Word k = 0; k < w.n; ++k)
                tools |= Count(w.handle[k].site | (w.handle[k].material << 6)) << (8 * k);
            auto &out = trace->out;
            out << pair;
            for (Count v : std::initializer_list<Count>{rock,
                                                        c.seed,
                                                        c.depth,
                                                        c.moves,
                                                        body.dendrites,
                                                        body.motor,
                                                        channels(body.dendrites),
                                                        w.bed.wick,
                                                        w.bed.wet,
                                                        w.bed.sill,
                                                        w.bed.paint,
                                                        w.feed,
                                                        w.catcher,
                                                        w.waste,
                                                        tools,
                                                        p.initial_on,
                                                        p.initial_off,
                                                        p.tissue.on,
                                                        p.tissue.off,
                                                        p.belief.on,
                                                        p.belief.off,
                                                        p.supports,
                                                        p.rivalry,
                                                        p.known,
                                                        p.predictions,
                                                        r.solutions,
                                                        r.shortest,
                                                        p.prior_actions,
                                                        p.prior_ready,
                                                        p.action,
                                                        p.ready,
                                                        p.alternatives,
                                                        r.caught,
                                                        r.leaked,
                                                        p.heat,
                                                        p.looks,
                                                        p.acts,
                                                        p.informative,
                                                        p.fingerprint,
                                                        w.stock,
                                                        r.feasible,
                                                        r.commands.order,
                                                        r.commands.n,
                                                        r.actual.position,
                                                        r.actual.supply,
                                                        r.actual.moved,
                                                        r.actual.denied,
                                                        c.adaptive && c.arm != lookup && c.arm != erased,
                                                        footprint(p.history),
                                                        p.history.recent})
                out << '\t' << v;
            out << '\n';
        }
        passed += p.passed;
        settled += p.settled;
        if (p.passed + p.settled != p.looks)
            throw std::logic_error("a released probe has no outlet");
        if (p.heat > c.budget || (p.predictions & ~p.known))
            throw std::logic_error("invalid capillary cost or prediction");
    }
    if (trace)
        trace->publish();
    Word example = 0;
    for (Word i = 0; i < total; ++i)
        if (results[i].caught && results[i].shortest >= 2 && results[i].shortest <= c.moves) {
            example = i;
            break;
        }
    Word pair = example >> 1, stone = c.rock ^ ((example & 1) << 4), motion = results[example].actual.position;
    auto sample = boards ? layouts[pair] : channel(mix(c.seed ^ mix(pair * 3517 + 77893)), c.depth, c.scarce);
    Function plate{};
    s.bind(plate, "capillary_plate", Image::runnels);
    Temporary scene(s.d, sizeof(Plate));
    launch(plate, 1, 1, sample, motion, stone, scene.ptr);
    synchronize();
    Plate illustration{};
    s.d.check(s.d.download(&illustration, scene.ptr, sizeof(illustration)),
              "read the two recorded physical pressure paths");
    Count handles = 0;
    for (Word i = 0; i < sample.n; ++i)
        handles |= Count(sample.handle[i].site | (sample.handle[i].material << 6)) << (8 * i);
    std::cout << "capillary_scene=" << pair << "  rock=" << stone << "  changes=" << motion << "  handles=" << handles
              << "  wet=" << sample.bed.wet << "  wick=" << sample.bed.wick << "  sill=" << sample.bed.sill
              << "  coating=" << sample.bed.paint << "  source=" << sample.feed << "  goal=" << sample.catcher
              << "  waste=" << sample.waste << "  prior_path=" << illustration.before
              << "  later_path=" << illustration.after << '\n';
    if (o.cap_bench)
        capillary_speed(plans[example].belief, channels(body.dendrites), sample, c.seed, o.cap_bench);
    if (!o.cap_lesson.empty()) {
        EvidenceFile e(o.cap_lesson);
        e.out << "twin\ttime\tprior_on\tprior_off\trivals\tmodel_on\tmodel_"
                 "off\tprevious\tintended\tphysical\tsensed\tfiring\tcost\tspike_before\tspike_after\tedits\n";
        Word k = c.views + c.probes;
        std::vector<Step> rows(2 * k);
        if (k)
            s.d.check(s.d.download(rows.data(), exposure.ptr, rows.size() * sizeof(rows[0])),
                      "read individual paid observations");
        for (Word branch = 0; branch < 2; ++branch)
            for (Word j = 0; j < plans[branch].looks; ++j) {
                auto &a = rows[branch * k + j];
                e.out << branch << '\t' << j;
                for (Count v : std::initializer_list<Count>{a.old_on, a.old_off, a.rivals, a.positive, a.negative,
                                                            a.previous, a.intended, a.physical, a.seen, a.result,
                                                            a.cost, a.initial, a.terminal, a.admitted})
                    e.out << '\t' << v;
                e.out << '\n';
            }
        e.publish();
    }
    std::cout << "capillary_school=" << pupil.resident.slot << "  born=" << pupil.resident.animal.born
              << "  cases=" << total << "  seed=" << c.seed << "  views=" << c.views << "  probes=" << c.probes
              << "  observations=" << tests << "  passed=" << passed << "  settled=" << settled
              << "  grips=" << investigations << "  discriminating=" << diagnostic << "  heat=" << heat
              << "  edits=" << edits << "  spike_changes=" << spikes << "  predicted_local=" << local
              << "  receptors=" << channels(body.dendrites) << "  capacity=" << width(body.dendrites)
              << "  empty_contacts=" << c.empty_contacts << "  law=" << body.growth.law
              << "  search_gene=" << body.motor << "  gate=" << c.gate << "  tissue_bytes=" << sizeof(capillary::Tissue)
              << "  cuda_ms=" << tuition.milliseconds() << '\n';
    std::cout << "capillary_repair=" << pupil.resident.slot << "  pairs=" << o.cap_trials << "  depth=" << c.depth
              << "  arm=" << c.arm << "  moves=" << c.moves << "  possible=" << possible
              << "  impossible=" << impossible << "  before=" << before << "  earned=" << caught
              << "  scarce=" << c.scarce << "  order_sensitive=" << ordered << "  recovered=" << salvaged
              << "  long_recoveries=" << needed_stock << "  denied_strokes=" << denied << "  shortest=" << shortest
              << "  wasted=" << invented << "  abstained=" << refused << "  motions=" << motions
              << "  irreconcilable=" << conflicts << "  both=" << both << "  different=" << diverged
              << "  definite=" << queried << "  correct=" << correct << "  wrong=" << spurious
              << "  forecasts=" << Count(total) * (1u << grips) << "  living_works=" << o.cap_natural
              << "  physical_ms=" << grading.milliseconds() << '\n';
    if (!o.cap_save.empty() || o.cap_vocab || o.cap_questions || o.cap_meetings || o.cap_recipes || o.cap_orders) {
        Temporary training(s.d, sizeof(Lessons));
        Function school{};
        s.bind(school, "capillary_graduate", Image::runnels);
        launch(school, 1, 1, body, c, natal, history, training.ptr);
        synchronize();
        Lessons lived{};
        s.d.check(s.d.download(&lived, training.ptr, sizeof(lived)), "retain a lifetime of independent interventions");
        pupil.channels.mind.tissue = lived.tissue;
        if (c.adaptive) {
            pupil.channels.adaptive = c.regional ? 3 : 1;
            pupil.channels.history = lived.history;
        } else if (o.cap_context != ~0u && pupil.channels.adaptive)
            pupil.channels.adaptive = 2;
        pupil.hydraulic = true;
        pupil.brain.tissue[utterance::band_of(pupil)].spike = lived.terminal;
        body.cells.spike = lived.terminal;
        if (o.cap_vocab || o.cap_questions || o.cap_meetings || o.cap_recipes || o.cap_orders)
            capillary_speech_study(pupil, body, c, o);
        if (!o.cap_save.empty())
            storage::write_checked(o.cap_save, pupil);
    }
}
