namespace axion::life::capillary {
AX_FN bool audible_workshop(Workshop &t, Body *bodies, const Request &q, Reply &r) {
    Word who = q.who, peer = 1 - who;
    auto &body = bodies[who];
    Count ear = (t.ears & (1u << who)) ? q.cochlea[who] : 0;
    if (q.action == set_acoustic_link) {
        t.channel_noise = q.a;
        t.questions = q.b;
        t.reply_mode = q.c;
        return true;
    }
    if (q.action == acoustic_shutter) {
        t.ears = (t.ears & ~(1u << who)) | (q.a << who);
        return true;
    }
    if (q.action == teach_resonator || q.action == teach_heading) {
        bool motor = q.action == teach_heading;
        if (!body.eyes || !body.limbs || !ear || q.statement.n != 1 || q.amount[who] < q.question.n + 4 ||
            (!motor && (!t.seal || (q.a && !t.second_seal)))) {
            r.status = 1;
            return true;
        }
        Word state = q.b;
        if (motor) {
            t.foot[who] = (t.foot[who] & ~(1u << q.a)) | (state << q.a);
        } else {
            auto w = current_work(t);
            state = press_port(t, q.a, q.a ? t.second_row : address(w.feed, w.bed, 18));
        }
        auto m = q.voices[who];
        VoiceTuition spent{};
        acquire_sound(m, body, ear, motor, 2 * q.a + state, q.statement.wave[0], q.question, spent);
        r.heat[who] = q.question.n + 3;
        r.spikes = spent.spikes;
        r.edits = spent.edits;
        r.voices[who] = m;
        r.voice_changes = 1u << who;
        r.contact = 1u << who;
        r.new_spike[who] = body.cells.spike;
        r.voice_action = 4;
        return true;
    }
    if (q.action == private_feeder) {
        if (!t.ready || !t.seal || !t.second_seal || t.feeder_stock < 2 || !body.eyes || !body.limbs ||
            !(t.shutters & (1u << who)) || q.amount[who] < 5) {
            r.status = 1;
            return true;
        }
        t.goal = q.a;
        t.witness = who;
        t.stocked = 1;
        t.auditory_seed = q.seed;
        auto a = compound_body(body, 0, q.compound[who]);
        Word mark = compose_move(a, q.a, mix(q.seed + 49277));
        auto w = current_work(t);
        Word paths[2]{address(w.feed, w.bed, 18), t.second_row}, sight = 0;
        Word optics =
            projector(sealed_port(t, 0, paths[0]), false) | (projector(sealed_port(t, 1, paths[1]), false) << 2);
        t.alternate_light = compound_light(optics, mark ^ 3u);
        for (Word port = 0; port < 2; ++port) {
            if (mark & (1u << port))
                press_port(t, port, paths[port]);
            sight |= press_port(t, port, paths[port]) << port;
        }
        auto m = q.voices[who];
        remember_sensation(m, a.body, 0, sight, r.spikes);
        r.voices[who] = m;
        r.voice_changes = 1u << who;
        r.contact = 1u << who;
        r.new_spike[who] = a.body.cells.spike;
        r.heat[who] = 2 + __builtin_popcount(mark);
        r.marked = mark;
        r.lamp = sight;
        r.sound_state = last_sensation(m, 0);
        r.voice_action = 1;
        return true;
    }
    if (q.action != answer_question && q.action != send_answer)
        return false;
    auto &voice = q.voices[who];
    Word organ = addressed(voice, ear, 0, q.question) ? 0u : addressed(voice, ear, 1, q.question) ? 1u : 2u;
    if (organ == 2)
        return false;
    r.voice_action = 2 + organ;
    Word state = last_sensation(voice, organ);
    r.sound_state = state;
    if (state >= 4 || q.amount[who] < 4)
        return true;
    r.words = relate(voice, body, ear, organ, state, q.question);
    r.heat[who] = r.words.n;
    if (organ || !(t.ears & (1u << peer)) || !r.words.n || q.amount[peer] < 5)
        return true;
    auto incoming = damaged_call(r.words, t.channel_noise, t.auditory_seed);
    r.delivered = incoming.n;
    r.heat[peer] = incoming.n;
    Word heard = recognise(q.voices[peer], q.cochlea[peer], 0, incoming);
    r.listener_state = heard;
    if (!t.stocked || t.witness != who || !bodies[peer].limbs || !bodies[peer].mouth)
        return true;
    Word input = heard;
    if (t.channel_noise != 1) {
        Word room = (q.amount[who] - r.heat[who] - 1) / 2;
        Word reserve = q.amount[peer] - r.heat[peer];
        Word allowed = t.questions < room ? t.questions : room;
        room = reserve > 3 ? (reserve - 3) / 2 : 0;
        allowed = allowed < room ? allowed : room;
        auto learner = compound_body(bodies[peer], 0, q.compound[peer]);
        r.exchange = resolve_audio(voice, q.voices[peer], ear, q.cochlea[peer], learner, incoming,
                                   mix(t.auditory_seed + 24407), allowed, t.reply_mode, t.alternate_light);
        r.heat[who] += r.exchange.effort;
        r.heat[peer] += r.exchange.effort;
        input = r.exchange.committed;
        r.listener_state = __builtin_popcount(r.exchange.remaining) == 1 ? __builtin_ctz(r.exchange.remaining) : 4;
    }
    if (input >= 4)
        return true;
    auto reader = compound_body(bodies[peer], 0, q.compound[peer]);
    Word choice = compose_move(reader, input, mix(t.auditory_seed + 24407));
    t.foot[peer] = choice;
    auto m = q.voices[peer];
    remember_sensation(m, reader.body, 1, choice, r.spikes);
    r.voices[peer] = m;
    r.voice_changes = 1u << peer;
    r.contact = 1u << peer;
    r.new_spike[peer] = reader.body.cells.spike;
    r.heat[peer] += 2;
    r.feeder = choice;
    r.ready = 1;
    t.stocked = 0;
    if (choice == t.goal && q.amount[who] - r.heat[who] < 65535 && q.amount[peer] - r.heat[peer] < 65535) {
        r.share = 1;
        t.feeder_stock -= 2;
        t.shared_food += 2;
    }
    return true;
}
} // namespace axion::life::capillary
