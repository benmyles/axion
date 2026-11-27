struct DriftWatch {
    Temporary records;
    EvidenceFile file;
    drift::Capture capture{};
    DriftWatch(Session &s, const Habitat &h, const Options &o)
        : records(s.d, Count(std::min(o.steps, o.drift_window)) * std::min(h.sites() / 4, o.drift_tiles) *
                           sizeof(drift::Scene)),
          file(o.drift_trace) {
        capture = {reinterpret_cast<drift::Scene *>(records.ptr), h.header.tick, std::min(o.steps, o.drift_window),
                   std::min(h.sites() / 4, o.drift_tiles)};
        if (records.ptr)
            s.d.check(s.d.zero(records.ptr, 0, Count(capture.ticks) * capture.tiles * sizeof(drift::Scene)),
                      "clear sampled drifting food");
    }
    void publish(Session &s, drift::Law law) {
        std::vector<drift::Scene> scenes(Count(capture.ticks) * capture.tiles);
        if (records.ptr)
            s.d.check(s.d.download(scenes.data(), records.ptr, scenes.size() * sizeof(scenes[0])),
                      "read sampled reaches at the riverbed");
        file.out << "stamp\ttile\torigin\tphase\trock\tinlet\tlinks\tunbound\twater\tclay\tfibre\toutlet\tload\tpriorit"
                    "y\tsource_stock\tdest_litter\tsame_stock\tafter_stock\tafter_litter\teaten\tblank_eaten\tunbound_"
                    "eaten\tports\tplace\tslot\tborn\trow\taxes\tdraw\told\tcommand\tblank\tcapacity\tbite\tafter\tspik"
                    "es\tbackground\tfirst\tsecond\tlaw\tissued\tlearns\tspike_"
                    "after\ttag\tuses\twiring\topenings\trim\theight\tpressure\theld_stock\ttide_shift\tload_limit";
        if (law.ports & drift::winding)
            file.out << "\tupper\tturns\tlaps\tresting\tdye\toptical\treceptors\tvisual_"
                        "law\toptics\theading\thand\tgate_a\tgate_b";
        file.out << '\n';
        auto packed = [](const flood::Half(&v)[4]) {
            Count x = 0;
            for (Word j = 0; j < 4; ++j)
                x |= Count(v[j]) << (16 * j);
            return x;
        };
        if (law.ports & drift::winding) {
            for (const auto &q : scenes) {
                if (!q.tick || q.laps < 2)
                    continue;
                drift::Path path{q.links, q.upper, q.turns, q.laps};
                Word other = path.run(q.view.inlet, true);
                if (other == q.sink)
                    continue;
                std::cout << "woven_witness=" << q.tick << "  tile=" << q.tile << "  inlet=" << q.view.inlet
                          << "  lower=" << q.links << "  upper=" << q.upper << "  laps=" << q.laps
                          << "  turns=" << q.turns << "  outlet=" << q.sink << "  reverse=" << other
                          << "  clay=" << packed(q.view.bed.clay) << "  fibre=" << packed(q.view.bed.fibre)
                          << "  food=" << q.amount << "  caught=" << q.eaten << '\n';
                break;
            }
        }
        for (const auto &q : scenes)
            if (q.tick)
                for (Word j = 0; j < 4; ++j) {
                    auto &r = q.reach[j];
                    auto &b = q.view.bed;
                    auto &o = file.out;
                    o << q.tick << '\t' << q.tile << '\t' << q.origin << '\t' << q.view.phase << '\t' << q.view.rock
                      << '\t' << q.view.inlet << '\t' << q.links << '\t' << q.unbound << '\t' << packed(b.water) << '\t'
                      << packed(b.clay) << '\t' << packed(b.fibre) << '\t' << q.sink << '\t' << q.amount << '\t'
                      << q.priority << '\t' << q.source_stock << '\t' << q.dest_litter << '\t' << q.same_stock << '\t'
                      << q.after_stock << '\t' << q.after_litter << '\t' << q.eaten << '\t' << q.blank_eaten << '\t'
                      << q.unbound_eaten << '\t' << law.ports << '\t' << j << '\t' << r.id << '\t' << r.born << '\t'
                      << r.row << '\t' << r.axes << '\t' << r.draw << '\t' << r.old << '\t' << r.command << '\t'
                      << r.blank << '\t' << r.capacity << '\t' << r.bite << '\t' << r.after << '\t' << r.nerve << '\t'
                      << r.background << '\t' << r.first << '\t' << r.second << '\t' << r.law << '\t' << r.issued
                      << '\t' << r.learns << '\t' << r.spike_after << '\t' << r.tag << '\t' << r.uses << '\t'
                      << r.wiring << '\t' << r.openings << '\t' << q.rim << '\t' << q.height << '\t' << q.pressure
                      << '\t' << q.held_stock << '\t' << law.tide_shift << '\t' << law.load;
                    if (law.ports & drift::winding)
                        o << '\t' << q.upper << '\t' << q.turns << '\t' << q.laps << '\t' << q.resting << '\t' << q.dye
                          << '\t' << r.optical << '\t' << r.receptors << '\t' << r.visual_law << '\t' << r.optics
                          << '\t' << r.heading << '\t' << r.hand << '\t' << r.a << '\t' << r.b;
                    o << '\n';
                }
        file.publish();
    }
};

struct DriftSurfaces {
    Word capacity, limit, salt, previous = 0;
    Temporary marks, counters, samples;
    std::unique_ptr<EvidenceFile> file;
    drift::LegacyWatch watch{};
    DriftSurfaces(Session &s, const Habitat &h, const Options &o)
        : capacity(Word(std::min<Count>(
              o.drift_surfaces,
              2ull * std::max({h.header.configuration.side, h.header.configuration.frontier, o.extend}) *
                  std::max({h.header.configuration.side, h.header.configuration.frontier, o.extend})))),
          limit(o.drift_legacy_trace.empty() ? 0u : std::min(capacity, 16384u)), salt(mix(o.drift_surface_seed)),
          marks(s.d, Count(capacity) * sizeof(drift::Mark)), counters(s.d, 128 * sizeof(drift::Heritage)),
          samples(s.d, Count(limit) * sizeof(drift::Inheritance)) {
        s.d.check(s.d.zero(counters.ptr, 0, 128 * sizeof(drift::Heritage)), "empty the material legacy accounts");
        if (limit) {
            file = std::make_unique<EvidenceFile>(o.drift_legacy_trace);
            s.d.check(s.d.zero(samples.ptr, 0, Count(limit) * sizeof(drift::Inheritance)), "clear surface exemplars");
        }
        watch = {reinterpret_cast<drift::Heritage *>(counters.ptr), reinterpret_cast<drift::Inheritance *>(samples.ptr),
                 limit};
    }
    drift::Surfaces attach(Session &s, Word n) {
        Word count = std::min(2 * n, capacity);
        if (previous != n) {
            s.d.check(s.d.zero(marks.ptr, 0, Count(count) * sizeof(drift::Mark)),
                      "start tracing undisturbed edges in this watershed geometry");
            previous = n;
        }
        return {reinterpret_cast<drift::Mark *>(marks.ptr), count, salt};
    }
    void finish(Session &s, Word tick, Word ports) {
        std::array<drift::Heritage, 128> shards{};
        s.d.check(s.d.download(shards.data(), counters.ptr, sizeof(shards)), "read measured material legacies");
        drift::Heritage t{};
        for (auto &q : shards) {
            t.placed += q.placed;
            t.revisited += q.revisited;
            t.diverted += q.diverted;
            t.actual += q.actual;
            t.without += q.without;
            t.gained += q.gained;
            t.lost += q.lost;
            t.self += q.self;
            t.foreign += q.foreign;
            t.younger += q.younger;
            t.after_death += q.after_death;
            t.orphans += q.orphans;
            t.oldest = std::max(t.oldest, q.oldest);
        }
        if (t.actual != t.self + t.foreign || t.gained > t.actual || t.lost > t.without ||
            t.actual - t.gained != t.without - t.lost || t.diverted > t.revisited || t.younger > t.foreign ||
            t.after_death > t.foreign || t.orphans > std::min(t.younger, t.after_death))
            throw std::logic_error("material legacy exceeds intercepted carbon");
        std::cout << "drift_legacy=" << tick << "  surfaces=" << std::min(2 * previous, capacity)
                  << "  deposited=" << t.placed << "  revisited=" << t.revisited << "  redirected=" << t.diverted
                  << "  eaten=" << t.actual << "  without=" << t.without << "  gained=" << t.gained
                  << "  lost=" << t.lost << "  maker_food=" << t.self << "  foreign=" << t.foreign
                  << "  after_death=" << t.after_death << "  younger=" << t.younger
                  << "  younger_after_death=" << t.orphans << "  oldest=" << t.oldest << '\n';
        if (!file)
            return;
        std::vector<drift::Inheritance> examples(limit);
        s.d.check(s.d.download(examples.data(), samples.ptr, Count(limit) * sizeof(examples[0])),
                  "read individual downstream beneficiaries");
        auto &out = file->out;
        out << "surface\tsalt\tside\torigin\ttile\tstamp\tseam\tmaker\tborn\tlaid\tmaker_now\tmaker_"
               "energy\tinlet\tphase\trock\twater\tclay\tfibre\toutlet\talternative\tload\tpriority\taims\tcapacities\t"
               "meals\tports";
        out << "\tmaker_shape\tmaker_limbs\tmaker_eyes\tmaker_pores\tmaker_"
               "cells\tbeneficiary\tshape\tlimbs\teyes\tpores\tcells";
        for (Word j = 0; j < 4; ++j)
            out << "\tslot" << j << "\tbirth" << j;
        out << '\n';
        auto packed = [](const flood::Half(&v)[4]) {
            Count x = 0;
            for (Word j = 0; j < 4; ++j)
                x |= Count(v[j]) << (16 * j);
            return x;
        };
        Word best = ~0u;
        for (Word k = 0; k < limit; ++k)
            if (examples[k].rank) {
                auto &r = examples[k];
                auto &v = r.view;
                out << k << '\t' << salt << '\t' << r.side << '\t' << r.origin << '\t' << r.tile << '\t' << r.tick
                    << '\t' << r.edge << '\t' << r.maker.owner << '\t' << r.maker.born << '\t' << r.maker.stamp << '\t'
                    << r.maker_now << '\t' << r.maker_energy << '\t' << v.inlet << '\t' << v.phase << '\t' << v.rock
                    << '\t' << packed(v.bed.water) << '\t' << packed(v.bed.clay) << '\t' << packed(v.bed.fibre) << '\t'
                    << r.outlet << '\t' << r.alternative << '\t' << r.load << '\t' << r.priority << '\t' << r.aims
                    << '\t' << r.capacities << '\t' << r.meals << '\t' << ports;
                out << '\t' << r.maker.shape << '\t' << r.maker.limbs << '\t' << r.maker.eyes << '\t' << r.maker.pores
                    << '\t' << r.maker.cells << '\t' << r.beneficiary << '\t' << r.shape << '\t' << r.limbs << '\t'
                    << r.eyes << '\t' << r.pores << '\t' << r.cells;
                if (r.meals && (best == ~0u ||
                                std::pair{r.rank, r.tick - r.maker.stamp} >
                                    std::pair{examples[best].rank, examples[best].tick - examples[best].maker.stamp}))
                    best = k;
                for (Word j = 0; j < 4; ++j)
                    out << '\t' << r.who[j] << '\t' << r.born[j];
                out << '\n';
            }
        if (best != ~0u) {
            const auto &r = examples[best];
            Word j = r.beneficiary;
            std::cout << "drift_bequest=" << r.tick << "  placed=" << r.maker.stamp
                      << "  age=" << r.tick - r.maker.stamp << "  maker=" << r.maker.owner
                      << "  maker_born=" << r.maker.born << "  maker_shape=" << r.maker.shape
                      << "  maker_limbs=" << r.maker.limbs << "  maker_eyes=" << r.maker.eyes
                      << "  maker_pores=" << r.maker.pores << "  maker_cells=" << r.maker.cells
                      << "  maker_gone=" << (!r.maker_energy || r.maker_now != r.maker.born)
                      << "  receiver=" << r.who[j] << "  receiver_born=" << r.born[j] << "  shape=" << r.shape
                      << "  limbs=" << r.limbs << "  eyes=" << r.eyes << "  pores=" << r.pores << "  cells=" << r.cells
                      << "  inlet=" << r.view.inlet << "  outlet=" << r.outlet << "  alternative=" << r.alternative
                      << "  edge=" << r.edge << "  eaten=" << ((r.meals >> (5 * j)) & 31) << "  tick=" << tick << '\n';
        }
        file->publish();
    }
};
struct DriftSighting {
    Temporary samples, choice;
    drift::Gaze gaze{};
    drift::Exhibit witnessed{};
    DriftSighting(Session &s, const Habitat &h, const Options &o, const std::optional<apprentice::Donor> &tracer,
                  Word hearing)
        : samples(s.d, Count(std::min(h.sites() / 4, o.drift_look)) * sizeof(drift::Exhibit)),
          choice(s.d, sizeof(Word)) {
        s.d.check(s.d.zero(samples.ptr, 0, Count(std::min(h.sites() / 4, o.drift_look)) * sizeof(drift::Exhibit)),
                  "clear the sighting window");
        Word rank = ~0u;
        s.d.check(s.d.upload(choice.ptr, &rank, sizeof(rank)), "listen for a prospective current event");
        gaze = {reinterpret_cast<drift::Exhibit *>(samples.ptr),
                reinterpret_cast<Word *>(choice.ptr),
                std::min(h.sites() / 4, o.drift_look),
                o.drift_watch,
                o.drift_pupil,
                hearing,
                tracer ? signature(*tracer) : drift::Signature{}};
    }
    bool observed(Session &s) {
        Word rank = ~0u;
        s.d.check(s.d.download(&rank, choice.ptr, sizeof(rank)), "check for an observed prediction");
        if (rank == ~0u)
            return false;
        if (rank >= gaze.tiles)
            throw std::logic_error("invalid observation window");
        s.d.check(s.d.download(&witnessed, samples.ptr + Count(rank) * sizeof(witnessed), sizeof(witnessed)),
                  "read the resident's current observation");
        return true;
    }
    void report(const Habitat &h) const {
        const auto &e = witnessed;
        if (e.id == ~0u) {
            std::cout << "drift_witness=horizon  tick=" << h.header.tick << "  tiles=" << gaze.tiles
                      << "  kind=" << gaze.kind << '\n';
            return;
        }
        if (e.id >= h.sites() || !h.bodies[e.id].energy || h.bodies[e.id].born != e.born ||
            std::memcmp(&h.float_events[e.id], &e.scene, sizeof(e.scene)) || e.stamp != h.header.tick)
            throw std::logic_error("selected drift witness lost its scene or identity");
        const char *kinds[]{"", "surprise", "confirmed", "untried", "catchment"};
        std::cout << "drift_witness=" << kinds[gaze.kind] << "  tick=" << h.header.tick << "  slot=" << e.id
                  << "  born=" << e.born << "  generation=" << h.bodies[e.id].generation
                  << "  cells=" << 32 * h.conducting(e.id) << "  tile=" << e.tile << "  watched_tiles=" << gaze.tiles
                  << "  selection=" << gaze.selection << "  inlet=" << e.inlet << "  outlet=" << e.sink
                  << "  expected=" << e.aim << "  row=" << e.row << "  old=" << e.old << "  axes=" << e.axes
                  << "  rafting=" << e.rafting << "  towed=" << e.towed << "  raft_load=" << e.raft_load
                  << "  water=" << e.water << "  background=" << e.background << "  dye=" << e.dye
                  << "  retinal_places=" << e.retinal_places << "  retinal_hues=" << e.retinal_hues
                  << "  images=" << e.scene.images << "  known=" << e.scene.known << "  stamp=" << e.stamp
                  << "  wiring=" << e.wiring << "  options=" << e.choices
                  << "  local_reach=" << drift::face(e.axes, e.local) << "  prior_inlets=" << e.prior
                  << "  nearby_inlets=" << e.neighbours << "  previous_reaches=" << e.previous_reaches << '\n';
    }
};
struct InletCensus {
    static constexpr Word bins = 128 * 32;
    Temporary entries, tally;
    Word capacity, size = 0, salt, previous = 0;
    InletCensus(Session &s, const Options &o, Word seed)
        : entries(s.d, Count(o.drift_first) * sizeof(drift::History)), tally(s.d, bins * sizeof(drift::FirstChoices)),
          capacity(o.drift_first), salt(mix(seed + 5309)) {
        s.d.check(s.d.zero(tally.ptr, 0, bins * sizeof(drift::FirstChoices)), "clear the first-contact census");
    }
    drift::FirstWatch attach(Session &s, Word sites, Word tick) {
        size = std::min(capacity, sites);
        if (sites != previous) {
            s.d.check(s.d.zero(entries.ptr, 0, Count(size) * sizeof(drift::History)),
                      "start observing reachable inlets from birth");
            previous = sites;
        }
        return {reinterpret_cast<drift::History *>(entries.ptr), reinterpret_cast<drift::FirstChoices *>(tally.ptr),
                size, tick, salt};
    }
    void report(Session &s, Word tick, bool donor, bool stored) {
        std::vector<drift::FirstChoices> shards(bins);
        s.d.check(s.d.download(shards.data(), tally.ptr, bins * sizeof(shards[0])),
                  "count first reaches by measured exposure");
        using Values = std::array<Count, sizeof(drift::FirstChoices) / sizeof(Count)>;
        for (Word i = 0; i < (stored ? 32u : 16u); ++i) {
            if ((i & 4) && !donor)
                continue;
            Values total{};
            for (Word j = i; j < bins; j += 32) {
                auto part = std::bit_cast<Values>(shards[j]);
                for (Word k = 0; k < total.size(); ++k)
                    total[k] += part[k];
            }
            auto q = std::bit_cast<drift::FirstChoices>(total);
            std::cout << "inlet_first=" << (i & 8 ? "catchment" : "lifetime") << "  tick=" << tick << "  slots=" << size
                      << "  prior=" << (i & 3) << "  matched=" << !!(i & 4) << "  overflow_only=" << !!(i & 16)
                      << "  offered=" << q.total << "  right=" << q.right << "  local=" << q.local
                      << "  rotated=" << q.rotated << "  changed=" << q.changed << "  confident=" << q.certain
                      << "  confident_right=" << q.certain_right << "  eaten=" << q.food
                      << "  local_food=" << q.proximal_food << '\n';
        }
    }
};
inline braid::Speech interview_line(std::istream &input, Word *slot = nullptr) {
    braid::Speech line{};
    std::string row, word;
    if (!std::getline(input, row))
        throw std::invalid_argument("incomplete current interview");
    std::istringstream words(row);
    bool seen = false;
    while (words >> word) {
        if (line.n == braid::max_phrase)
            throw std::invalid_argument("the current interview exceeds the vocal range");
        if (word == "*") {
            if (!slot || seen)
                throw std::invalid_argument("unexpected pointing slot in current interview");
            *slot = line.n;
            seen = true;
        } else if (!(line.wave[line.n] = acoustic::spoken_word(word)))
            throw std::invalid_argument("a question contains an inaudible word");
        ++line.n;
    }
    if (!line.n || (slot && !seen))
        throw std::invalid_argument("the current interview needs an audible question and its requested pointing slot");
    return line;
}
inline braid::Speech interview_names(std::istream &input) {
    auto names = interview_line(input);
    if (names.n != 8 || std::set<Count>(names.wave, names.wave + 8).size() != 8)
        throw std::invalid_argument("a current interview needs eight distinct names");
    return names;
}
struct DriftDiary {
    Temporary notes;
    std::unique_ptr<EvidenceFile> file;
    drift::Diary diary{};
    braid::Narration speech{};
    drift::Conversation script{};
    bool asks = false;
    DriftDiary(Session &s, const Habitat &h, const Options &o, Word id)
        : notes(s.d, Count(std::min(o.steps, o.drift_diary_ticks)) * sizeof(drift::Encounter)) {
        if (id >= h.sites() || !h.bodies[id].energy)
            throw std::invalid_argument("a current diary needs a living focal resident");
        file = std::make_unique<EvidenceFile>(o.drift_diary);
        diary = {reinterpret_cast<drift::Encounter *>(notes.ptr), id, h.bodies[id].born, h.header.tick,
                 std::min(o.steps, o.drift_diary_ticks)};
        if (notes.ptr)
            s.d.check(s.d.zero(notes.ptr, 0, Count(diary.ticks) * sizeof(drift::Encounter)),
                      "clear private river encounters");
        if (o.drift_questions.empty())
            return;
        std::ifstream input(o.drift_questions);
        if (!input)
            throw std::invalid_argument("cannot open the current interview");
        script.names = interview_names(input);
        script.anchor = interview_line(input, &script.slot);
        script.actual = interview_line(input);
        script.before = interview_line(input);
        script.inverse = interview_line(input);
        std::string extra;
        if (input >> extra)
            throw std::invalid_argument("unexpected additional current question");
        auto it = std::ranges::find(h.narrators, id, &Habitat::ResidentNarration::slot);
        if (it != h.narrators.end() && it->born == diary.born)
            speech = it->mind;
        asks = true;
    }
    void finish(Session &s, Word tick) {
        Word size = diary.ticks;
        std::vector<drift::Encounter> events(size);
        std::vector<drift::Answers> replies(size);
        if (size)
            s.d.check(s.d.download(events.data(), notes.ptr, Count(size) * sizeof(events[0])),
                      "read the focal resident's observed currents");
        if (asks && size) {
            Temporary memory(s.d, sizeof(speech)), questions(s.d, sizeof(script)),
                answers(s.d, Count(size) * sizeof(replies[0]));
            s.d.check(s.d.upload(memory.ptr, &speech, sizeof(speech)), "present acquired contact state");
            s.d.check(s.d.upload(questions.ptr, &script, sizeof(script)), "present the public acoustic interview");
            s.d.check(s.d.zero(answers.ptr, 0, Count(size) * sizeof(replies[0])), "clear unheard replies");
            Function ask{};
            s.bind(ask, "drift_dialogue", Image::runnels);
            void *args[]{&notes.ptr, &size, &memory.ptr, &questions.ptr, &answers.ptr};
            s.d.check(s.d.launch(ask, (size * 12 + 31) / 32, 1, 1, 32, 1, 1, 0, nullptr, args, nullptr),
                      "replay conversations about experienced currents");
            s.d.check(s.d.download(replies.data(), answers.ptr, Count(size) * sizeof(replies[0])),
                      "read acoustic replies in order");
        }
        auto &out = file->out;
        out << "slot\tborn\tstamp\ttile\tinlet\tsink\taim\trow\told\tafter\taxes\tcommand\tblank\tload\tbite\toptions\t"
               "background\tdye\traft\ttowed\traft_"
               "load\twater\tplaces\thues\timages\tknown\texamined\tsite\tconversation\tno_"
               "topic\tno_event\tmidpoint\n";
        Count seen = 0, predicted = 0, wrong = 0, food = 0, changed = 0, complete = 0, paired = 0, repair = 0,
              repaired = 0, quiet = 0, blind = 0, inverse = 0, inverse_right = 0, anchor_right = 0, actual_right = 0,
              before_right = 0, cleared = 0;
        for (Word i = 0; i < size; ++i) {
            const auto &n = events[i];
            const auto &e = n.view;
            if (!e.stamp)
                continue;
            ++seen;
            food += n.bite;
            changed += n.command != n.blank;
            bool certain = std::has_single_bit(n.available);
            predicted += certain;
            wrong += certain && e.aim != e.sink;
            auto first = braid::picture(e.scene, 0), last = braid::picture(e.scene, 1),
                 forecast = braid::picture(e.scene, 2);
            Word answered_sites = 0;
            for (Word j = 0; j < 4; ++j) {
                Word words = asks ? replies[i].turns[0][j] : 0x8888u;
                Word cut = asks ? replies[i].turns[1][j] : 0x8888u;
                Word blank = asks ? replies[i].turns[2][j] : 0x8888u;
                out << e.id << '\t' << e.born << '\t' << e.stamp << '\t' << e.tile << '\t' << e.inlet << '\t' << e.sink
                    << '\t' << e.aim << '\t' << e.row << '\t' << e.old << '\t' << n.after << '\t' << e.axes << '\t'
                    << n.command << '\t' << n.blank << '\t' << n.load << '\t' << n.bite << '\t' << n.available << '\t'
                    << e.background << '\t' << e.dye << '\t' << e.rafting << '\t' << e.towed << '\t' << e.raft_load
                    << '\t' << e.water << '\t' << e.retinal_places << '\t' << e.retinal_hues << '\t' << e.scene.images
                    << '\t' << e.scene.known << '\t' << asks << '\t' << j << '\t' << words << '\t' << cut << '\t'
                    << blank << '\t' << e.midpoint << '\n';
                if (!asks)
                    continue;
                bool known = braid::visible(e.scene, 2) & (1u << j);
                Word expectation = 4 + braid::pigment(forecast, j), actual = 4 + braid::pigment(last, j),
                     before = 4 + braid::pigment(first, j);
                bool anchor = known && (words & 15) == expectation, later = ((words >> 4) & 15) == actual,
                     prior = ((words >> 8) & 15) == before;
                anchor_right += anchor;
                actual_right += later;
                before_right += prior;
                complete += anchor && later && prior;
                if (later)
                    answered_sites |= 1u << j;
                quiet += ((cut >> 4) & 15) == actual;
                cleared += ((cut >> 4) & 15) < 8;
                if (known && expectation != actual) {
                    ++repair;
                    repaired += anchor && later;
                }
                for (Word turn = 0; turn < 4; ++turn)
                    blind += ((blank >> (4 * turn)) & 15) < 8;
                Word locations = braid::lamps(first, before - 4, 3);
                if (std::has_single_bit(locations)) {
                    ++inverse;
                    inverse_right += prior && ((words >> 12) & 15) == Word(std::countr_zero(locations));
                }
            }
            for (Word j = 0; j < 4; ++j)
                for (Word k = j + 1; k < 4; ++k)
                    if (braid::pigment(last, j) != braid::pigment(last, k))
                        paired += !!(answered_sites & (1u << j)) && !!(answered_sites & (1u << k));
        }
        std::cout << "drift_diary=" << diary.slot << "  born=" << diary.born << "  tick=" << tick << "  events=" << seen
                  << "  committed=" << predicted << "  mistaken=" << wrong << "  eaten=" << food
                  << "  recalled_choices=" << changed << '\n';
        if (asks)
            std::cout << "drift_conversation=" << diary.slot << "  events=" << seen << "  anchors=" << 4 * seen
                      << "  expected_right=" << anchor_right << "  actual_right=" << actual_right
                      << "  before_right=" << before_right << "  three_turns=" << complete
                      << "  contrasting_pairs=" << paired << "  discrepancies=" << repair << "  both_sides=" << repaired
                      << "  erased_topic_answers=" << cleared << "  erased_topic_right=" << quiet
                      << "  missing_scene_answers=" << blind << "  unique_replies=" << inverse
                      << "  locations_right=" << inverse_right << '\n';
        file->publish();
    }
};

struct DriftClock {
    Driver &driver;
    struct Sample {
        cuda::Event before{}, after{};
        Word tiles = 0;
    };
    std::vector<Sample> samples;
    Word used = 0;
    DriftClock(Session &s, Word n) : driver(s.d), samples(n) {
        try {
            for (auto &q : samples) {
                driver.check(driver.event_new(&q.before, 0), "create a current timer");
                driver.check(driver.event_new(&q.after, 0), "create a current finish timer");
            }
        } catch (...) {
            clear();
            throw;
        }
    }
    void clear() {
        for (auto &q : samples) {
            if (q.before)
                driver.event_free(q.before);
            if (q.after)
                driver.event_free(q.after);
        }
    }
    ~DriftClock() { clear(); }
    Word remaining() const { return Word(samples.size()) - used; }
    bool start(Stream stream, Word cells) {
        if (!remaining())
            return false;
        samples[used].tiles = cells;
        driver.check(driver.record(samples[used].before, stream), "start current kernel timer");
        return true;
    }
    void stop(Stream stream) { driver.check(driver.record(samples[used++].after, stream), "end current kernel timer"); }
    void report(const char *label = "drift_profile") {
        if (!used)
            return;
        driver.check(driver.wait(samples[used - 1].after), "finish current measurements");
        std::vector<float> times;
        double total = 0;
        Count tiles = 0;
        for (Word i = 0; i < used; ++i) {
            float ms = 0;
            driver.check(driver.elapsed(&ms, samples[i].before, samples[i].after), "measure suspended-food kernel");
            times.push_back(ms);
            total += ms;
            tiles += samples[i].tiles;
        }
        std::sort(times.begin(), times.end());
        std::cout << label << '=' << used << "  tested_tiles=" << tiles << "  cuda_ms=" << total
                  << "  median_us=" << 1000 * times[used / 2] << "  p90_us=" << 1000 * times[(9 * (used - 1)) / 10]
                  << "  Mtiles_per_s=" << (total ? tiles / (1000 * total) : 0) << '\n';
    }
};
