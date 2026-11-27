// Included in the acoustic CUDA module after the event and speech primitives.
struct JointSight {
    Word asked = 0, heard = 0, writes = 0, ports = 0, heard_ports = 0, remote = 0, first = none, order = 0;
};
AX_FN JointSight compare_sight(Narration &self, Narration &other, Genes genes, Count voice, Word frame, Word budget,
                               Word seed, Word sampling = 0, bool revise = true) {
    JointSight out{};
    if (!voice || !self.event.stamp || visible(self.event, frame) != 15 || !(genes.arbor & (1u << 14)))
        return out;
    Word slot = acquaintance(self, voice, true);
    auto model = contact(self, slot);
    if (model.voice != voice)
        model = {voice, 0, 0};
    Word board = picture(self.event, frame);
    bool relinquished = false;
    for (Word i = 0; i < budget; ++i) {
        Word socket = sampling == 2
                          ? 0
                          : sight_to_request(model, board, out.ports, seed + i * 7, sampling != 1 && (genes.arbor & 4));
        if (socket == none || (out.ports & (1u << socket)))
            break;
        auto ask = full_question(self, frame, socket);
        if (!ask.n)
            break;
        if (!out.asked)
            out.first = socket;
        out.order |= socket << (2 * out.asked);
        ++out.asked;
        out.ports |= 1u << socket;
        auto sound = question(other, ask, false);
        if (!sound.answered || sound.words.n != 1)
            continue;
        Word feature = ident(self, intern(self, sound.words.wave[0], false));
        if (feature < 4 || feature >= 8)
            continue;
        ++out.heard;
        out.heard_ports |= 1u << socket;
        out.remote |= (feature - 4) << (2 * socket);
        bool prior = anchored(model);
        out.writes += couple_sight(model, genes, board, socket, feature - 4, revise);
        relinquished |= prior && !anchored(model);
    }
    if (relinquished && out.heard_ports == 15 && bijective(board) && bijective(out.remote)) {
        // Reuse the entire current exchange, including replies received just
        // before the old relationship became inconsistent.
        model.panel = inverse_view(board) | (out.remote << 8) | (15u << 16);
        model.excluded = 0;
    }
    if (out.heard)
        remember_contact(self, slot, model);
    return out;
}
AX_FN SpokenEvidence consult_view(Narration &self, Narration &peer, const Speech &raw, Answer original, Count voice,
                                  Word channel, bool retain, Word direction = 3) {
    SpokenEvidence turns{};
    turns.original = original;
    auto &out = turns.original;
    if (!out.queried)
        return turns;
    Word index = acquaintance(self, voice, false);
    Correspondence view{};
    if (index != none)
        view = contact(self, index);
    Word ref = index == none || !(direction & 1) ? out.source : agreed_reference(view, out.source, false);
    if (ref == none) {
        out.words = {};
        out.queried = 0;
        self.pending = 0;
        return turns;
    }
    out.words = full_question(self, out.frame, ref);
    if (!out.words.n) {
        out.queried = 0;
        self.pending = 0;
        return turns;
    }
    if (!channel)
        return turns;
    turns.witness = question(peer, channel == 3 ? raw : out.words, false);
    if (!turns.witness.answered || channel == 2)
        return turns;
    Count sound = turns.witness.words.wave[0];
    if (channel == 4) {
        Word their = turns.witness.said;
        sound = name_of(peer, (their & 4) | ((their + 1) & 3));
    }
    Word received = ident(self, intern(self, sound, false));
    Word local = index == none || !(direction & 2) ? received : agreed_reference(view, received, true);
    if (local != none)
        turns.final = accept(self, name_of(self, local), retain);
    return turns;
}
struct Audience {
    Word selected = 0, coverage[2]{};
};
AX_FN Word intelligible_answers(const Narration &speaker, const Answer &request, Count voice) {
    if (!request.queried)
        return 0;
    Word i = acquaintance(speaker, voice, false);
    if (i == none)
        return 0;
    auto model = contact(speaker, i);
    if (agreed_reference(model, request.source, false) == none)
        return 0;
    Word eligible = possible(speaker.event, request.frame, request.source), count = 0;
    while (eligible) {
        Word value = __builtin_ctz(eligible);
        eligible &= eligible - 1;
        count += agreed_reference(model, value, false) != none;
    }
    return count;
}
AX_FN Audience choose_witness(const Narration &speaker, Genes g, const Answer &query, Count a, Count b) {
    Audience choice{};
    choice.coverage[0] = intelligible_answers(speaker, query, a);
    choice.coverage[1] = intelligible_answers(speaker, query, b);
    choice.selected = (g.throat >> (query.source & 15)) & 1;
    if ((g.arbor & 4) && choice.coverage[0] != choice.coverage[1])
        choice.selected = choice.coverage[1] > choice.coverage[0];
    return choice;
}
AX_FN SpokenEvidence deliver_query(Narration *people, Mind &body, const Course &c, Answer inquiry, Receipt &receipt,
                                   Word channel) {
    Word recipient = c.peer;
    Count address = c.address;
    if (c.alternative != none && inquiry.queried) {
        auto pick = choose_witness(people[c.who], body.genes, inquiry, c.address, c.second_voice);
        receipt.coverage[0] = pick.coverage[0];
        receipt.coverage[1] = pick.coverage[1];
        if (pick.selected) {
            recipient = c.alternative;
            address = c.second_voice;
        }
    }
    if (inquiry.queried)
        receipt.approached = recipient;
    return consult_view(people[c.who], people[recipient], c.words, inquiry, address, channel, true);
}
AX_FN void study_view(Narration &self, Narration &other, Mind &body, Table &tray, const Course &c, Receipt &r) {
    Word allowance = tray.tutor;
    Word count = c.cmd == compare_views ? c.reps : 1;
    for (Word i = 0; i < count; ++i) {
        Word key = mix(c.seed + 4483 * i);
        if (c.cmd == compare_views) {
            auto event = demonstration(body, c.seed, tray.rig[c.who], i);
            if (event.work + c.budget > allowance)
                break;
            allowance -= event.work;
            r.cost += event.work;
            glimpse(self, through_glass(event.self, tray.eyes[c.who]), (c.vision >> (4 * c.who)) & 15);
            glimpse(other, through_glass(event.peer, tray.eyes[c.peer]), (c.vision >> (4 * c.peer)) & 15);
        }
        Word budget = c.budget < allowance ? c.budget : allowance;
        if (!budget)
            break;
        auto display = compare_sight(self, other, body.genes, c.address, c.frame, budget, key, c.sampling);
        allowance -= display.asked;
        r.cost += display.asked;
        r.asked += display.asked;
        r.replies += display.heard;
        r.learned += display.writes;
        r.examples += bool(display.heard);
        ++r.offered;
    }
}
