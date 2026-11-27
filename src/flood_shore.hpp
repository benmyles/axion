// The visitor and its attendance roll are external to the habitat. Only a
// living pupil's own acquired contacts return to the sparse resident archive.
struct ShoreSchool {
    Temporary pupils, syllabus;
    Word size = 0;
    shore::Syllabus course{};
    ShoreSchool(Session &s, const Habitat &h, const Options &o, Word specific)
        : pupils(s.d, Count(o.shore_pupils) * sizeof(shore::Pupil)), syllabus(s.d, sizeof(course)) {
        std::ifstream input(o.shore_school);
        if (!input)
            throw std::invalid_argument("cannot open the shore syllabus " + o.shore_school);
        auto read = [&](braid::Speech &line, Word *hole) {
            std::string row, word;
            if (!std::getline(input, row))
                throw std::invalid_argument("supply eight names followed by two question forms");
            std::istringstream words(row);
            while (words >> word) {
                if (line.n == braid::max_phrase)
                    throw std::invalid_argument("a shore utterance exceeds the vocal range");
                if (hole && word == "*") {
                    if (*hole != braid::none)
                        throw std::invalid_argument("only one pointing position is available");
                    *hole = line.n;
                } else {
                    auto sound = acoustic::spoken_word(word);
                    if (!sound)
                        throw std::invalid_argument("an empty shore waveform cannot be taught");
                    line.wave[line.n] = sound;
                }
                ++line.n;
            }
            if (hole && *hole == braid::none)
                throw std::invalid_argument("shore questions need a * for their pointed noun");
        };
        read(course.names, nullptr);
        if (course.names.n != 8 || std::set<Count>(course.names.wave, course.names.wave + 8).size() != 8)
            throw std::invalid_argument("shore teaching needs eight distinguishable public names");
        for (Word f = 0; f < 2; ++f) {
            course.slot[f] = braid::none;
            read(course.form[f], &course.slot[f]);
        }
        std::string extra;
        if (input >> extra)
            throw std::invalid_argument("unexpected additional shore utterance");
        course.seed = o.class_seed;
        course.reversal = o.shore_reversal;
        course.channel = o.shore_channel;
        std::vector<shore::Pupil> roll;
        auto enroll = [&](Word id) {
            const auto &a = h.bodies[id];
            shore::Pupil p{id, a.born, a.founder, a.generation, a.brain_origin, a.body_origin};
            auto it = std::ranges::find(h.narrators, id, &Habitat::ResidentNarration::slot);
            if (it != h.narrators.end() && it->born == a.born)
                p.voice = it->mind;
            roll.push_back(p);
        };
        if (specific != ~0u) {
            if (specific >= h.sites() || !h.bodies[specific].energy || o.shore_pupils != 1)
                throw std::invalid_argument("a named shore pupil needs one living resident");
            enroll(specific);
        } else {
            Word n = h.sites(), first = mix(course.seed ^ 0x6483u), stride = mix(course.seed + 37) | 1;
            for (Word k = 0; k < n && roll.size() < o.shore_pupils; ++k) {
                Word id = (first + k * stride) & (n - 1);
                if (h.bodies[id].energy && h.bodies[id].body.eyes &&
                    (!o.class_organs || (h.riggers[id].genes.arbor & (3u << 14)) == (3u << 14)))
                    enroll(id);
            }
        }
        size = roll.size();
        if (size)
            s.d.check(s.d.upload(pupils.ptr, roll.data(), size * sizeof(roll[0])), "seat living school residents");
        s.d.check(s.d.upload(syllabus.ptr, &course, sizeof(course)),
                  "place the visitor's public utterances at the shore");
    }
    void finish(Session &s, Habitat &h, const Options &o) {
        std::vector<shore::Pupil> roll(size);
        if (size)
            s.d.check(s.d.download(roll.data(), pupils.ptr, Count(size) * sizeof(roll[0])),
                      "read resident field attendance");
        std::unique_ptr<EvidenceFile> trace;
        if (!o.shore_trace.empty()) {
            trace = std::make_unique<EvidenceFile>(o.shore_trace);
            trace->out << "slot\tborn\tpresent\tscenes\tnames\tforms\tpointings\tpresented\ttested\tanswered\tcorrect\t"
                          "changed\tboth\tname_edits\tform_edits\tsoma\n";
        }
        std::map<Word, size_t> index;
        for (size_t i = 0; i < h.narrators.size(); ++i)
            index[h.narrators[i].slot] = i;
        Count survivors = 0, attended = 0, learners = 0, named = 0, form_users = 0;
        shore::Ledger total{}, after{};
        auto add = [](shore::Ledger &target, const shore::Ledger &r) {
            target.scenes += r.scenes;
            target.pointings += r.pointings;
            target.presented += r.presented;
            target.tested += r.tested;
            target.answered += r.answered;
            target.correct += r.correct;
            target.changed += r.changed;
            target.both += r.both;
            target.attempts += r.attempts;
            target.agreements += r.agreements;
            target.changed_names += r.changed_names;
            target.changed_forms += r.changed_forms;
            target.soma += r.soma;
        };
        for (auto &p : roll) {
            auto r = p.book;
            add(r, p.after_switch);
            bool alive = shore::present(p, h.bodies[p.slot]);
            survivors += alive;
            attended += !!r.scenes;
            Word names = 0, forms = 0;
            for (Word j = 0; j < 8; ++j)
                names += braid::ident(p.voice, braid::intern(p.voice, course.names.wave[j], false)) == j;
            for (auto &f : p.voice.forms)
                forms += f.examples != 0 && !braid::sequence_form(f);
            named += names == 8;
            form_users += forms > 0;
            learners += bool(r.changed_names || r.changed_forms);
            const braid::Narration blank{};
            bool connected = std::memcmp(&p.voice, &blank, offsetof(braid::Narration, event)) != 0;
            if (alive && course.channel != shore::examination && r.scenes && (connected || index.contains(p.slot))) {
                if (!braid::valid_narration(p.voice))
                    throw std::logic_error("shore speech contacts are invalid");
                if (index.contains(p.slot))
                    h.narrators[index[p.slot]] = {p.slot, p.born, p.voice};
                else
                    h.narrators.push_back({p.slot, p.born, p.voice});
            }
            if (r.correct > r.answered || r.answered > r.tested || r.both > r.changed || r.agreements > r.attempts ||
                r.attempts > r.presented || r.pointings > 16 * r.scenes || r.presented > 2 * r.scenes ||
                (course.channel == shore::examination &&
                 (r.presented || r.pointings || r.changed_names || r.changed_forms || r.soma)))
                throw std::logic_error("shore ledger exceeds sensory opportunities");
            add(total, r);
            add(after, p.after_switch);
            if (trace)
                trace->out << p.slot << '\t' << p.born << '\t' << alive << '\t' << r.scenes << '\t' << names << '\t'
                           << forms << '\t' << r.pointings << '\t' << r.presented << '\t' << r.tested << '\t'
                           << r.answered << '\t' << r.correct << '\t' << r.changed << '\t' << r.both << '\t'
                           << r.changed_names << '\t' << r.changed_forms << '\t' << r.soma << '\n';
        }
        std::cout << "shore_school=" << h.header.tick << "  enrolled=" << size << "  present=" << survivors
                  << "  attended=" << attended << "  learners=" << learners << "  learned_names=" << named
                  << "  form_users=" << form_users << "  scenes=" << total.scenes << "  pointings=" << total.pointings
                  << "  examples=" << total.presented << "  tested=" << total.tested << "  answered=" << total.answered
                  << "  correct=" << total.correct << "  changed=" << total.changed << "  both=" << total.both
                  << "  attempts=" << total.attempts << "  agreements=" << total.agreements
                  << "  name_edits=" << total.changed_names << "  form_edits=" << total.changed_forms
                  << "  soma_changes=" << total.soma << "  channel=" << course.channel << "  seed=" << course.seed
                  << "  reversed_at=" << course.reversal << '\n';
        if (course.reversal != ~0u) {
            std::cout << "shore_reversal=" << course.reversal << "  before_scenes=" << total.scenes - after.scenes
                      << "  after_scenes=" << after.scenes << "  before_tested=" << total.tested - after.tested
                      << "  after_tested=" << after.tested << "  before_correct=" << total.correct - after.correct
                      << "  after_correct=" << after.correct << "  before_changed=" << total.changed - after.changed
                      << "  before_both=" << total.both - after.both << "  after_changed=" << after.changed
                      << "  after_both=" << after.both << "  after_edits=" << after.changed_forms << '\n';
        }
        if (trace)
            trace->publish();
    }
};
