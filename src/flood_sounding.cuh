// Two inlets can be tested by stirring a grain out of the litter. All four sites
// belong to one CUDA lane this tick, including the deposit at the final outlet.
template <bool Public>
AX_FN bool stir_water(World w, Configuration c, const DriftSquare &sites, Word tick, Word tile, Word key, Word lamp,
                      Word law, Word links, Word rest, Word dye, retina::StirPatch *patches, retina::StirSample *survey,
                      Word sample_count, Word rotating, Word &source, Word &actual, Word &owner,
                      audience::Ledger *visits, audience::Witness *witnesses) {
    Word other = (source + 1 + mix(key ^ 0x297b79u) % 3) & 3;
    auto &book = patches[(tile >> 5) & 127].counts;
    if (!w.land[sites[other]].water || !w.land[sites[source]].litter || !w.land[sites[other]].litter)
        return false;
    count(book.racks, 1);
    Word seat = (key >> 22) & 3, slot = w.slots[sites[seat]];
    auto &animal = w.animals[slot];
    const auto &dna = w.dna[slot];
    if (animal.energy < 2 || !animal.body.eyes || !animal.body.limbs ||
        !(dna.pigments[0] & (1u << (16 + (w.motor[slot] & 7)))))
        return false;
    count(book.offered, 1);
    auto glass = braid::eyes_of(w, slot);
    auto &hand = w.braids.minds[slot];
    auto tissue = read_band(w, c.tiles, slot, motor_band(w, c, slot));
    retina::Observer b{};
    b.genes = {dna.pigments[0], dna.pigments[1]};
    b.heading = animal.heading;
    b.iris = {dna.pigments[2], dna.pigments[3]};
    b.wiring = drift::wiring(hand.genes, law);
    Word spectrum = braid::pigment(glass.hues, lamp);
    Word matrix = law & drift::tidal_banks ? retina::bank(b.genes, spectrum) : 0;
    retina::Banks banks{hand.volley >> 16, tissue.gate_a, tissue.gate_b};
    Word old = banks.get(matrix), ports = drift::branch(law, braid::drift_tag(hand.lived));
    auto first = retina::project(retina::reflected(rest, dye, source, source), glass);
    auto second = retina::project(retina::reflected(rest, dye, other, other), glass);
    Word tie = mix(key ^ mix(slot + 17041u));
    auto action = retina::reach(b, first, second, spectrum, old, tissue.spike, ports, tie);
    audience::Witness public_view{};
    if constexpr (Public) {
        Word front = (seat + 1 + 2 * (animal.heading & 1)) & 3;
        const auto &neighbour = w.animals[w.slots[sites[front]]];
        if (neighbour.energy && neighbour.body.eyes) {
            Word face = braid::pigment(glass.hues, audience::silhouette(neighbour.body));
            bool other_face = w.braids.mode & audience::crossed;
            if (w.braids.mode & audience::lineage) {
                Word mark = braid::drift_tag(hand.lived);
                other_face = mark && mark != (other_face ? 2u : 1u);
            }
            face ^= Word(other_face);
            regulation::Program genes{dna.hearing.low, dna.hearing.high};
            public_view = {
                1, neighbour.body.shape, face, tissue.gate_b >> 16, genes.first, genes.second, Word(other_face)};
            auto m = audience::offer(genes, tissue.gate_b >> 16, face, action.eye[0].onset, action.eye[1].onset,
                                     action.eye[0].prediction, action.eye[1].prediction, tie);
            auto &v = visits[(tile >> 5) & 127];
            count(v.choices, 1);
            count(v.changed_hand, m.opened && m.hand != action.hand);
            count(v.reached, m.opened);
            auto control =
                audience::offer(genes, tissue.gate_b >> 16, face ^ 1u, action.eye[0].onset, action.eye[1].onset,
                                action.eye[0].prediction, action.eye[1].prediction, tie);
            count(v.trace_sensitive, m.opened != control.opened || (m.opened && m.hand != control.hand));
            action.eye[0].volley = m.first;
            action.eye[1].volley = m.second;
            action.hand = m.hand;
            action.started = m.opened;
        }
    }
    Word mark = braid::drift_tag(hand.lived);
    bool crossed = w.braids.mode & retina::crossed_hands;
    if (w.braids.mode & retina::lineage_hands)
        crossed = mark && mark != (crossed ? 2u : 1u);
    Word physical = action.hand ^ Word(crossed);
    Word picked = physical ? other : source;
    Word destination = drift::outlet(picked, links);
    Word before = w.land[sites[picked]].litter, downstream = w.land[sites[destination]].litter;
    bool blocked = picked != destination && downstream == 65535u;
    if (action.started) {
        owner = slot;
        count(book.opened, 1);
        if (picked != destination && !blocked) {
            --w.land[sites[picked]].litter;
            ++w.land[sites[destination]].litter;
            count(book.moved, 1);
        }
        if (blocked)
            count(book.blocked, 1);
        if (drift::outlet(source, links) != drift::outlet(other, links))
            count(book.divergent, 1);
    }
    // An odd spatial stride assigns at most one lane per recording slot per tick.
    // With moving sites the shift changes independently of the animal's choice.
    Word salt = rotating ? mix(c.seed ^ mix(tick + 0x71e07b2du)) : mix(c.seed);
    Word rank = (tile * 16127u + salt) & (c.side * c.side / 4 - 1);
    if (rank < (survey ? sample_count : 128u)) {
        if constexpr (Public)
            if (witnesses)
                witnesses[rank] = public_view;
        auto &record = survey ? survey[rank] : patches[rank].sample;
        record = {tick + 1,
                  tile,
                  slot,
                  animal.born,
                  source,
                  other,
                  rest,
                  dye,
                  spectrum,
                  glass.places,
                  glass.hues,
                  animal.heading,
                  dna.pigments[0],
                  dna.pigments[1],
                  b.iris.first,
                  b.iris.second,
                  b.wiring,
                  ports,
                  old,
                  tissue.spike,
                  tie,
                  action.eye[0].volley,
                  action.eye[1].volley,
                  action.hand,
                  physical,
                  action.started,
                  links,
                  destination,
                  Word(blocked),
                  before,
                  downstream,
                  w.land[sites[picked]].litter,
                  w.land[sites[destination]].litter,
                  Word(crossed),
                  mark};
    }
    source = picked;
    actual = blocked ? picked : destination;
    return action.started;
}
