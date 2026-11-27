void cascade_interview(const Habitat &h, const Options &o, Word slot) {
    if (slot >= n || !h.braiding || !h.bodies[slot].energy || !h.bodies[slot].body.eyes)
        throw std::invalid_argument("select a living sighted pupil for the acoustic sequence interview");
    std::ifstream f(o.retina_questions);
    if (!f)
        throw std::invalid_argument("cannot open the cascade interview");
    inlet::Questions q{};
    q.names = interview_names(f);
    q.predict = interview_line(f, &q.variable);
    q.actual = interview_line(f);
    q.source = interview_line(f, &q.source_variable);
    std::string extra;
    if (f >> extra)
        throw std::invalid_argument("unexpected additional cascade question");
    retina::Terms terms{};
    if (!o.retina_terms.empty()) {
        std::ifstream language(o.retina_terms);
        if (!language)
            throw std::invalid_argument("cannot read public sequence names");
        std::string line;
        while (std::getline(language, line)) {
            std::istringstream words(line);
            std::string word;
            if (!(words >> word))
                continue;
            auto i = terms.size;
            if (i == 4)
                throw std::invalid_argument("the external speaker accepts up to four temporal names");
            auto call = acoustic::spoken_word(word);
            if (!call || std::find(terms.wave, terms.wave + i, call) != terms.wave + i ||
                std::find(q.names.wave, q.names.wave + 8, call) != q.names.wave + 8)
                throw std::invalid_argument("each narrated sequence needs a distinct audible name");
            while (words >> word) {
                auto ref = std::find(q.names.wave + 4, q.names.wave + 8, acoustic::spoken_word(word));
                if (ref == q.names.wave + 8 || terms.width[i] >= 4)
                    throw std::invalid_argument("each external name describes one to four lamp colors");
                terms.pulses[i] |= Word(ref - (q.names.wave + 4)) << (2 * terms.width[i]++);
            }
            if (!terms.width[i])
                throw std::invalid_argument("a temporal name has no physical sequence");
            terms.wave[i] = call;
            ++terms.size;
        }
    }
    braid::Narration learned{};
    auto saved = std::ranges::find(h.narrators, slot, &Habitat::ResidentNarration::slot);
    if (saved != h.narrators.end() && saved->born == h.bodies[slot].born)
        learned = saved->mind;
    const auto &a = h.bodies[slot];
    const auto &g = h.genomes[slot];
    const auto &hand = h.riggers[slot];
    Word band = regulation::innervated_band(g, h.conducting(slot));
    Tissue tissue{};
    move_neural(nullptr, &tissue, Count(slot) * tiles + band, 1);
    retina::Observer body{{g.pigments[0], g.pigments[1]},
                          a.heading,
                          (hand.genes.muscle >> 24) & 7,
                          (hand.genes.arbor >> 25) & 7,
                          drift::wiring(hand.genes, drift::recruitment),
                          braid::eye_wiring(h.loom.mode, hand.genes, a.heading),
                          hand.volley >> 16,
                          tissue.spike,
                          slot,
                          tissue.gate_a,
                          tissue.gate_b};
    retina::Course c{o.inlet_practice,        o.inlet_prior,         o.retina_laps,    o.inlet_landscape,
                     h.drift_water.law.ports, Word(o.inlet_carried), o.retina_channel, o.retina_stimulus,
                     o.retina_contrast,       o.retina_beds,         o.retina_fused};
    gaze_course(c, h, o);
    body.iris = {g.pigments[2], g.pigments[3]};
    sensory_surfaces(c, o);
    Word seed = mix(o.class_seed ^ 0x4ae6b92du), cases = 2 * o.retina_trials, hearing = o.retina_hearing;
    Temporary vocab(s.d, sizeof(learned)), script(s.d, sizeof(q)), early(s.d, Count(cases) * sizeof(retina::Spoken)),
        later(s.d, Count(2 * cases) * sizeof(retina::Delivered));
    s.d.check(s.d.upload(vocab.ptr, &learned, sizeof(learned)), "present acquired sound-to-color connections");
    s.d.check(s.d.upload(script.ptr, &q, sizeof(q)), "read the external speaker's vocabulary and questions");
    Temporary inquiry(s.d, c.inquiry ? Count(cases) * sizeof(retina::Inquiry) : 0);
    std::vector<retina::Inquiry> receipts(c.inquiry ? cases : 0);
    Function first{}, second{};
    s.bind(first, c.gaze || c.inquiry ? "cascade_active_before" : "cascade_before", Image::runnels);
    s.bind(second, "cascade_after", Image::runnels);
    std::vector<retina::Spoken> initial(cases);
    std::vector<retina::Delivered> ending(2 * cases);
    FieldTimer time(s.d);
    time.start(stream);
    launch(first, (cases + 63) / 64, 64, vocab.ptr, body, script.ptr, seed, cases, c, hearing, terms, early.ptr,
           inquiry.ptr);
    synchronize();
    s.d.check(s.d.download(initial.data(), early.ptr, initial.size() * sizeof(initial[0])),
              "seal the words before either release");
    launch(second, (2 * cases + 63) / 64, 64, vocab.ptr, body, script.ptr, seed, cases, c, early.ptr, later.ptr);
    time.stop(stream);
    synchronize();
    s.d.check(s.d.download(ending.data(), later.ptr, ending.size() * sizeof(ending[0])),
              "read the observed natural and diverted outcomes");
    if (inquiry.ptr)
        s.d.check(s.d.download(receipts.data(), inquiry.ptr, receipts.size() * sizeof(receipts[0])),
                  "read the pupil's sealed trials");
    std::unique_ptr<EvidenceFile> log;
    if (!o.retina_voice.empty()) {
        log = std::make_unique<EvidenceFile>(o.retina_voice);
        log->out << "seed\tlife\ttwin\tturn\tbeds\tlaps\thearing\tlandscape\tplaces\thues\tinitial_"
                    "row\theard\tsounds\toptions\tmotor\tsaid\tpremature\tno_"
                    "scene\ttopic\tactual\treported\torigin\tcleared\thand\ta\tb\tactual_beds\twoven_"
                    "order\tsurface\tbalanced_sources\tsound_units\tlearned_units";
        if (c.inquiry)
            log->out << "\tinquiry\tquery_hand\tbudget\tstart\tprior_heat\tinitial_spikes\tprior_options\tprior_"
                        "word\toffers\tpaid\tstopped\tdigest\tfinal_spikes\tfinal_hand\tfinal_a\tfinal_"
                        "b\treceptors\tplasticity\tdendrite\tiris_"
                        "motor\theading\tsenses\twiring\tports\tstimulus\tchannel\tcontrast\tfused\tactions\tquery_"
                        "context\tremoved\tdeposited";
        log->out << '\n';
    }
    Count gaze_heat = 0, study_work = 0, offered = 0, prior_right = 0, learned_right = 0, unlearned = 0, committed = 0,
          unanswered = 0, investigated = 0, orders_differ = 0;
    Count recognized = 0, expected = 0, answered = 0, correct = 0, actual = 0, origins = 0, both = 0, diverted = 0,
          early_words = 0, empty = 0, cleared = 0;
    Count opposed = 0, paired = 0, compressed = 0, mixed = 0, compressed_right = 0, mixed_right = 0, sound_units = 0,
          wave_stages = 0;
    for (Word i = 0; i < cases; ++i) {
        const auto &s = initial[i];
        const auto &u = ending[2 * i], &v = ending[2 * i + 1];
        if (braid::visible(s.event, braid::result) || u.outlet == v.outlet)
            throw std::logic_error("the current result leaked into the cascade forecast");
        gaze_heat += s.heat;
        if (c.inquiry) {
            auto &q = receipts[i];
            if (retina::grain_count(q.removed) != q.paid || retina::grain_count(q.deposited) != q.paid)
                throw std::logic_error("a question altered the total tracer stock");
            study_work += q.paid;
            offered += q.offers;
            committed += q.stopped;
            investigated += q.paid > 0;
            bool past = q.prior_word == u.outlet, present = s.forecast == u.outlet;
            prior_right += past;
            learned_right += !past && present;
            unlearned += past && !present;
            unanswered += q.prior_word >= 8 && present;
            if ((i & 1) && initial[i - 1].cues != s.cues) {
                auto &other = receipts[i - 1];
                orders_differ += q.paid != other.paid || q.actions != other.actions;
            }
        }
        compressed += s.learned != 0;
        mixed += s.learned > 1;
        compressed_right += s.learned && s.forecast == u.outlet;
        mixed_right += s.learned > 1 && s.forecast == u.outlet;
        sound_units += s.units;
        wave_stages += s.laps;
        recognized += s.heard == s.laps;
        answered += s.forecast < 8;
        expected += s.motor == u.outlet;
        correct += s.forecast == u.outlet;
        both += u.word == u.outlet && v.word == v.outlet;
        diverted += s.forecast == u.outlet && v.word == v.outlet;
        early_words += s.early < 8;
        empty += s.silent < 8;
        if ((i & 1) && c.beds > 1) {
            auto &p = initial[i - 1];
            if (p.hand != s.hand || p.a != s.a || p.b != s.b || p.origin != s.origin)
                throw std::logic_error("reordering the question changed its earlier education");
            const auto &other = ending[2 * i - 2];
            if (other.outlet != u.outlet) {
                ++opposed;
                paired += p.forecast == other.outlet && s.forecast == u.outlet;
            }
        }
        for (Word t = 0; t < 2; ++t) {
            auto &x = ending[2 * i + t];
            actual += x.word == x.outlet;
            origins += x.source == s.origin;
            cleared += x.cut < 8;
            if (log) {
                log->out << seed << '\t' << i / 2 << '\t' << (i & 1) << '\t' << t << '\t' << c.beds << '\t' << s.laps
                         << '\t' << hearing << '\t' << c.landscape << '\t' << Word(body.eyes.places) << '\t'
                         << Word(body.eyes.hues) << '\t' << s.origin << '\t' << s.heard << '\t' << s.cues << '\t'
                         << s.options << '\t' << s.motor << '\t' << s.forecast << '\t' << s.early << '\t' << s.silent
                         << '\t' << s.topic << '\t' << x.outlet << '\t' << x.word << '\t' << x.source << '\t' << x.cut
                         << '\t' << s.hand << '\t' << s.a << '\t' << s.b << '\t' << c.sites << '\t' << c.natural_order
                         << '\t' << s.surface << '\t' << 1 << '\t' << s.units << '\t' << s.learned;
                if (c.inquiry) {
                    auto &q = receipts[i];
                    log->out << '\t' << c.inquiry << '\t' << c.query_hand << '\t' << c.query_budget << '\t' << c.rounds
                             << '\t' << s.heat << '\t' << q.initial_spike << '\t' << q.prior << '\t' << q.prior_word
                             << '\t' << q.offers << '\t' << q.paid << '\t' << q.stopped << '\t' << q.digest << '\t'
                             << q.final_spike << '\t' << q.after.hand << '\t' << q.after.a << '\t' << q.after.b << '\t'
                             << body.genes.receptors << '\t' << body.genes.plasticity << '\t' << body.iris.first << '\t'
                             << body.iris.second << '\t' << body.heading << '\t' << body.senses << '\t' << body.wiring
                             << '\t' << c.ports << '\t' << c.stimulus << '\t' << c.channel << '\t' << c.contrast << '\t'
                             << c.fused << '\t' << q.actions << '\t' << c.reversed_question << '\t' << q.removed << '\t'
                             << q.deposited;
                }
                log->out << '\n';
            }
        }
    }
    if (log)
        log->publish();
    if (cases >= 2) {
        Word pick = 0;
        while (pick + 2 < cases && ending[2 * pick].outlet == ending[2 * pick + 2].outlet)
            pick += 2;
        const auto &a = initial[pick], &b = initial[pick + 1];
        std::cout << "cascade_scene=" << pick / 2 << "  laps=" << a.laps << "  origin=" << a.origin
                  << "  first_cues=" << a.cues << "  other_cues=" << b.cues << "  heard=" << a.heard
                  << "  other_heard=" << b.heard << "  prediction=" << a.forecast << "  alternative=" << b.forecast
                  << "  outlet=" << ending[2 * pick].outlet << "  other_outlet=" << ending[2 * pick + 2].outlet
                  << "  names=";
        for (Word j = 0; j < 8; ++j) {
            if (j)
                std::cout << ',';
            std::cout << acoustic::spell(q.names.wave[j]);
        }
        std::cout << '\n';
    }
    if (c.inquiry)
        std::cout << "spoken_inquiry=" << slot << "  seed=" << seed << "  courses=" << cases << "  beds=" << c.beds
                  << "  actual_beds=" << c.sites << "  laps=" << c.laps << "  free_views=" << c.rounds + c.former
                  << "  rack=" << c.inquiry << "  budget=" << c.query_budget << "  coupling=" << c.query_hand
                  << "  query_context=" << c.reversed_question << "  action_witness=1"
                  << "  correct_before=" << prior_right << "  correct_after=" << correct
                  << "  improved=" << learned_right << "  lost_correct=" << unlearned << "  from_silence=" << unanswered
                  << "  investigated=" << investigated << "  offered=" << offered << "  grain_releases=" << study_work
                  << "  stopped=" << committed << "  order_changed_inquiry=" << orders_differ
                  << "  early_outcomes=" << early_words << "  changed_order_pairs=" << opposed
                  << "  both_orders=" << paired << '\n';
    if (c.gaze)
        std::cout << "gaze_speech=" << slot << "  gaze=" << c.gaze << "  probes=" << c.probes << "  purse=" << c.purse
                  << "  heat=" << gaze_heat << "  opportunities=" << Count(cases) * (c.rounds + c.former)
                  << "  courses=" << cases << "  correct=" << correct << "  both=" << paired << "  opposed=" << opposed
                  << '\n';
    std::cout << "cascade_interview=" << slot << "  tick=" << tick << "  born=" << a.born
              << "  generation=" << a.generation << "  seed=" << seed << "  balanced_sources=1  actual_beds=" << c.sites
              << "  woven_order=" << c.natural_order << "  beds=" << c.beds << "  laps=" << c.laps
              << "  glimpses=" << c.rounds << "  channel=" << c.channel << "  hearing=" << hearing
              << "  carried=" << c.history << "  courses=" << cases << "  heard=" << recognized
              << "  motor=" << expected << "  voiced=" << answered << "  correct=" << correct
              << "  opposite_orders=" << opposed << "  both_orders=" << paired << "  named_sequences=" << terms.size
              << "  shortened=" << compressed << "  shortened_right=" << compressed_right << "  combined=" << mixed
              << "  combined_right=" << mixed_right << "  sound_units=" << sound_units
              << "  physical_steps=" << wave_stages << "  premature=" << early_words << "  no_scene=" << empty
              << "  releases=" << 2 * cases << "  outcomes=" << actual << "  origins=" << origins
              << "  both_releases=" << both << "  revised=" << diverted << "  erased_topic=" << cleared
              << "  cuda_ms=" << time.milliseconds() << '\n';
}
