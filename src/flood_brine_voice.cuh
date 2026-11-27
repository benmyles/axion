// Physical labels and the public sample enter the listener. The diagnostic
// choice takes only firing strengths and visible features as inputs.
AX_FN void request_sample(const material::Samples *site, const brine::Animal *animal, const grounding::Mind *lexicon,
                          Word held, Word turn, Word hearing, Word reserve, Word planning, Word closure, Count *channel,
                          material::ListenerInput *table, material::SampleRequest *out) {
    auto &room = *site;
    auto &board = *table;
    material::SampleRequest r{};
    *channel = 0;
    if (held < 2 && lexicon && reserve > 1) {
        Word protected_band = material::catalytic_band(room.chip[held]), key = mix(room.seed + turn * 307 + 151);
        Word a = brine::lesson(turn, protected_band, mix(room.seed + 971)),
             b = (a & 12) | brine::alternate(a & 3, mix(key + 129));
        Word left = brine::cue(protected_band, room.soil[0]), right = brine::cue(protected_band, room.soil[1]);
        if (closure &&
            brine::settled(animal->genes, animal->memory, left, right, closure == 1 ? room.witnesses : ~0u)) {
            r.stop = 1;
            *out = r;
            return;
        }
        Word side = brine::request(animal->genes, animal->memory, a, b, left, right, key, planning != 0);
        r.chip = a >> 2;
        r.suggested = side;
        r.raw[0] = a & 3;
        r.raw[1] = b & 3;
        r.wave = material::field_voice::identify(*lexicon, room.pigment[2 + side], room.pigment[2 + (1 ^ side)]);
        Count foil = material::field_voice::identify(*lexicon, room.pigment[2 + (side ^ 1)], room.pigment[2 + side]);
        *channel = hearing == 2 ? foil : r.wave;
        board.piece = brine::grain(r.chip, key);
        board.solution = room.solution;
        for (Word j = 0; j < 2; ++j) {
            Word ix = j ^ (mix(key + 717) & 1);
            board.pigments[ix] = room.pigment[2 + j];
            board.sites[ix] = room.site[2 + j];
            board.soil[ix] = r.raw[j];
        }
    }
    *out = r;
}
KERNEL void brine_question(const material::Samples *site, const brine::Animal *animal, const grounding::Mind *lexicon,
                           Word held, Word turn, Word hearing, Word reserve, Word planning, Word closure,
                           Count *channel, material::ListenerInput *table, material::SampleRequest *out) {
    if (!tid())
        request_sample(site, animal, lexicon, held, turn, hearing, reserve, planning, closure, channel, table, out);
}
AX_FN void receive_evidence(brine::Animal *animal, const material::ListenerInput *board,
                            const material::ListenerResult *heard, Word vision, material::SampleRequest *note,
                            material::Samples *room) {
    if (!heard->paid || !vision)
        return;
    for (Word j = 0; j < 2; ++j)
        if (board->sites[j] == heard->chosen) {
            Word q = brine::cue(material::catalytic_band(board->piece), board->soil[j]);
            bool flash = (heard->calories != 0) ^ (vision == 2);
            auto &r = *animal;
            brine::observe(r.genes, r.memory, q, flash);
            if (room->witnesses != ~0u)
                ++room->witnesses;
            note->seen = q;
            note->outcome = flash;
        }
}
KERNEL void brine_witness(brine::Animal *animal, const material::ListenerInput *board,
                          const material::ListenerResult *heard, Word vision, material::SampleRequest *note,
                          material::Samples *room) {
    if (!tid())
        receive_evidence(animal, board, heard, vision, note, room);
}

KERNEL void brine_conversations(const brine::Animal *life, const material::Inquiry *empirical,
                                const material::Genes *hands, const grounding::Mind *speaker,
                                const grounding::Mind *listener, material::FieldLanguage *forms,
                                material::ConversationTest study, Word mode, material::ConversationCounts *out) {
    using namespace material;
    Word id = tid();
    if (id >= study.trials)
        return;
    Word key = mix(study.seed + id * 4177);
    auto scene = field_voice::scene(key);
    ConversationCounts counts{};
    Count names[4]{};
    for (Word j = 0; j < 4; ++j)
        names[j] = field_voice::identify(*speaker, scene.pigment[j], scene.pigment[j ^ 1]);
    for (Word target = 0; target < 2; ++target) {
        bool held = false;
        for (auto wave : study.held)
            held |= wave == names[target];
        counts.offered += 4;
        if (!held || !names[2] || !names[3])
            continue;
        Word coat = catalytic_band(scene.chip[target]);
        for (Word kind = 0; kind < 2; ++kind) {
            auto speech = study.form[kind];
            speech.wave[speech.length - 1] = names[target];
            Count prior_word = 0;
            bool prior_correct = false;
            for (Word bath = 0; bath < 2; ++bath) {
                auto room = scene;
                Word first = (key >> 11) & 1, active = first ^ bath, destination = active ^ kind;
                // Physically exchange which of the two deposits is responsive
                // without changing either animal's visual or acoustic inputs.
                Word shear = (brine::solution(mix(key + 2311 + bath * 311), study.species) & ~3u);
                Word ions = shear | (coat ^ brine::surface(room.soil[active], shear));
                room.solution = brine::phase(brine::dissolve(ions, key, study.species), 32);
                brine::Animal cortex = *life;
                Word energy = study.speaker_energy, other = study.peer_energy;
                Count channel = 0;
                FieldAnswer answer{};
                ListenerInput board{};
                auto speak = [&]() {
                    field_operation(empirical, hands, speaker, forms, &room, speech, 3, 0, 0, energy, &answer, &channel,
                                    &board, &cortex, room.solution, 1);
                };
                speak();
                ++counts.asked;
                counts.before += answer.wave == names[2 + destination];
                Word willing = (cortex.genes.readout >> 17) & 15,
                     attempts = study.limit < willing ? study.limit : willing;
                if ((mode != 0 && mode != 7) || (mode == 7 ? answer.prior_uncertain : answer.uncertain))
                    for (Word turn = 0;
                         turn < attempts && energy >= 2 && other >= 3 && answer.recognized && answer.referent == target;
                         ++turn) {
                        SampleRequest request{};
                        ListenerResult response{};
                        Word wire = mode == 2 ? 0 : 1;
                        request_sample(&room, &cortex, speaker, target, turn, wire, energy, Word(mode != 6),
                                       (mode == 0   ? 1u
                                        : mode == 7 ? 2u
                                                    : 0u),
                                       &channel, &board, &request);
                        if (request.stop)
                            break;
                        field_recipient(listener, &channel, &board, wire, other, &response);
                        receive_evidence(&cortex, &board, &response, mode == 3 ? 2u : 1u, &request, &room);
                        if (request.wave) {
                            --energy;
                            ++counts.requested;
                        }
                        other = other - response.paid + response.calories;
                        counts.shown += response.paid != 0;
                        counts.observed += request.seen < 16;
                        counts.held_contacts += response.paid && request.chip == coat;
                        counts.paid += response.paid;
                        counts.reference_food += response.calories;
                    }
                speak();
                if (study.intention) {
                    // An isolated physical twin uses the same material motor
                    // channel and its own endowment. Querying never updates the speaker.
                    auto twin = cortex;
                    auto measure = room;
                    FieldAnswer action{};
                    Count silent = 0;
                    ListenerInput unused{};
                    field_operation(empirical, hands, speaker, forms, &measure, speech, 4, target, 0, energy, &action,
                                    &silent, &unused, &twin, room.solution, 1);
                    counts.policy_agreement += action.paid && answer.wave == names[2 + (action.destination ^ kind)];
                }
                bool correct = answer.wave == names[2 + destination];
                counts.correct += correct;
                counts.wrong += answer.wave && !correct;
                counts.silent += !answer.wave;
                if (bath) {
                    counts.switched += answer.wave && prior_word && answer.wave != prior_word;
                    counts.genuine_switch += correct && prior_correct;
                }
                prior_word = answer.wave;
                prior_correct = correct;
                if (mode == 5)
                    channel = answer.foil;
                ListenerResult final{};
                field_recipient(listener, &channel, &board, mode == 4 ? 0u : 1u, other, &final);
                counts.acted += final.paid != 0;
                counts.arrived += final.chosen == room.site[2 + destination];
                counts.feed_requests += !kind;
                counts.harvested += !kind && final.calories;
                counts.inactive_harvests += kind && final.calories;
            }
        }
    }
    out[id] = counts;
}
