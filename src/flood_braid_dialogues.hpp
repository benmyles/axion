void braid_dialogues(const std::vector<braid::Genes> &parents, const Options &o) {
    Word cases = o.braid_dialogues, names = o.braid_terms, repeats = o.braid_rep, length = o.braid_length,
         budget = o.braid_questions, grammar = o.braid_acquire, pattern = o.braid_pan, size = parents.size(),
         seed = o.braid_seed;
    std::vector<braid::Genes> visitors;
    if (!o.braid_workers.empty()) {
        Habitat donor;
        donor.read(o.braid_workers);
        if (donor.braiding)
            for (Word i = 0; i < donor.sites(); ++i)
                if (donor.bodies[i].energy)
                    visitors.push_back(donor.riggers[i].genes);
        if (visitors.empty())
            throw std::invalid_argument("the distant worker bank needs living float operators");
    }
    Word nw = visitors.size();
    Temporary m(s.d, Count(size) * sizeof(parents[0])), vis(s.d, Count(nw) * sizeof(parents[0])),
        raw(s.d, Count(cases) * sizeof(braid::Passage));
    s.d.check(s.d.upload(m.ptr, parents.data(), Count(size) * sizeof(parents[0])), "sample three independent bodies");
    if (nw)
        s.d.check(s.d.upload(vis.ptr, visitors.data(), Count(nw) * sizeof(parents[0])), "sample remote workers");
    Function probe{};
    s.bind(probe, "braid_dialogues", Image::runnels);
    std::unique_ptr<EvidenceFile> file;
    if (!o.braid_transcript.empty()) {
        file = std::make_unique<EvidenceFile>(o.braid_transcript);
        file->out
            << "arm\tcase\ttwin\tworld\tnames\tquery\tlength\tfocus\torders\tscreen\tdisplay\toperator\tstart\tend\tsou"
               "rce\tknown\tarticulated\tcode\tspoken\theard\tplan\tstrokes\tquestions\tclaim\tfees\tcarbon\tceiling\tb"
               "orrowed\tlocal_word\tnew_start\tnew_goal\tnew_end\tnew_plan\tnew_strokes\tnew_heard\tread_order";
        for (Word i = 0; i < 3; ++i)
            file->out << "\trig" << i << "\twrist" << i;
        for (Word i = 0; i < names; ++i)
            file->out << "\tsource" << i << "\tnative" << i;
        for (Word i = 0; i < 4; ++i)
            file->out << "\tprobe" << i << "\toutcome" << i;
        file->out << '\n';
    }
    const char *labels[]{"translated",   "one_view", "same_sample", "muted",   "garbled",
                         "untranslated", "borrowed", "unborrowed",  "screened"};
    Event a{}, b{};
    s.d.check(s.d.event_new(&a, 0), "create conversation clock");
    s.d.check(s.d.event_new(&b, 0), "create conversation finish");
    float elapsed = 0;
    std::vector<braid::Passage> results(cases);
    for (Word arm = 0; arm < braid::conversation_arms; ++arm) {
        s.d.check(s.d.record(a, stream), "begin three-body tests");
        launch(probe, (cases + 31) / 32, 32, m.ptr, size, cases, seed, names, repeats, length, budget, arm, grammar,
               pattern, vis.ptr, nw, raw.ptr);
        s.d.check(s.d.record(b, stream), "finish three-body tests");
        synchronize();
        float ms = 0;
        s.d.check(s.d.elapsed(&ms, a, b), "measure public translation");
        elapsed += ms;
        s.d.check(s.d.download(results.data(), raw.ptr, Count(cases) * sizeof(results[0])),
                  "read private hand and public speech decisions");
        Count exact = 0, legal = 0, uttered = 0, heard = 0, work = 0, source = 0, questions = 0, masked = 0,
              distinct = 0, both = 0, charges = 0, food = 0, given = 0, later = 0, distal = 0, terms = 0, actions = 0,
              reworded = 0, two = 0, claim = 0, unseen = 0;
        for (Word i = 0; i < cases; ++i) {
            auto &p = results[i];
            bool yes[2]{};
            distinct += p.twin[0].target != p.twin[1].target;
            masked += p.twin[0].screen == p.twin[1].screen && p.twin[0].observed == p.twin[1].observed;
            for (Word j = 0; j < 2; ++j) {
                auto &r = p.twin[j];
                yes[j] = r.decoded && r.after == braid::transfer(r.before, r.target) && r.claim == r.target;
                exact += yes[j];
                legal += r.ceiling;
                uttered += r.human;
                heard += r.decoded;
                source += r.source;
                questions += r.queries;
                food += r.food;
                charges += r.fee;
                given += r.borrowed;
                later += r.later_received && r.later_goal == r.later_end;
                distal += r.later_received;
                terms += r.n;
                actions += r.steps;
                claim += r.human && r.claim == r.target;
                work += r.steps != 0;
                reworded += yes[j] && r.n != p.length;
                two += yes[j] && r.n >= 2;
                unseen += r.target != braid::order;
                if (!file)
                    continue;
                auto &f = file->out;
                f << labels[arm] << '\t' << i << '\t' << j;
                for (Word x : {p.key,         p.n,          p.utterance,   p.length,
                               p.focus,       p.laws,       r.screen,      r.observed,
                               r.target,      r.before,     r.after,       r.source,
                               r.known,       r.human,      r.said,        r.n,
                               r.decoded,     r.plan,       r.steps,       r.queries,
                               r.claim,       r.fee,        r.food,        r.ceiling,
                               r.borrowed,    r.other,      r.later_start, r.later_goal,
                               r.later_end,   r.later_work, r.later_steps, r.later_received,
                               r.native_order})
                    f << '\t' << x;
                for (Word k = 0; k < 3; ++k)
                    f << '\t' << p.rig[k] << '\t' << p.wrist[k];
                for (Word k = 0; k < names; ++k)
                    f << '\t' << p.public_a[k] << '\t' << p.public_b[k];
                for (Word k = 0; k < 4; ++k)
                    f << '\t' << r.tests[k] << '\t' << r.seen[k];
                f << '\n';
            }
            both += (p.twin[0].target != p.twin[1].target) && yes[0] && yes[1];
        }
        std::cout << "braid_exchange=" << labels[arm] << "  pairs=" << cases << "  offered=" << 2 * cases
                  << "  legal=" << legal << "  reached=" << exact << "  different=" << distinct << "  both=" << both
                  << "  same_first=" << masked << "  correct_source=" << source << "  voices=" << uttered
                  << "  understood=" << heard << "  correct_meaning=" << claim << "  questions=" << questions
                  << "  budget=" << budget << "  carbon=" << food << "  cost=" << charges
                  << "  borrowed_edits=" << given << "  later_understood=" << distal << "  later_correct=" << later
                  << "  public_words=" << terms << "  different_length=" << reworded << "  combined=" << two
                  << "  motor_pulls=" << actions << "  moved=" << work << "  nonidentity=" << unseen
                  << "  convention=" << grammar << "  pan=" << pattern << '\n';
    }
    s.d.event_free(a);
    s.d.event_free(b);
    std::cout << "braid_dialogue_ms=" << elapsed << "  conversations=" << Count(2) * cases * braid::conversation_arms
              << '\n';
    if (file)
        file->publish();
}
