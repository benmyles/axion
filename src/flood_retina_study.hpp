void gaze_course(retina::Course &course, const Habitat &h, const Options &o) {
    course.gaze = o.retina_iris != ~0u                ? o.retina_iris
                  : (h.loom.mode & retina::iris_port) ? retina::voluntary
                                                      : retina::passive;
    course.inquiry = o.deliberation;
    course.query_hand = o.inquiry_hand;
    course.reversed_question = o.reversed_question;
    course.query_budget = o.gaze_purse;
    course.probes = o.retina_probes;
    course.harvests = o.gaze_foraging;
    if (course.probes) {
        if (o.retina_iris != ~0u && o.retina_iris != retina::voluntary)
            throw std::invalid_argument("stirring a tracer requires inherited gaze");
        course.gaze = retina::voluntary;
    }
    if (!course.gaze && course.harvests != 1)
        throw std::invalid_argument("multiple nursery portions need an active gaze treatment");
    course.purse = course.gaze ? o.gaze_purse : 0;
}
void retinal_portrait(journal::Specimen &p, const Habitat &h, Word id) {
    if(!h.appetites.empty()){
        p.measures["appetitive_contacts"]=std::popcount(h.appetites[id]);
        p.measures["foraging_ports"]=std::popcount(h.genomes[id].grip&255u);
    }
    if(!h.cap_histories.empty()){
        auto hst=h.cap_histories[id];p.measures["context_rows"]=capillary::count(capillary::footprint(hst));
        p.measures["signed_contexts"]=std::popcount(hst.recent&0x808080u);p.measures["context_age"]=hst.recent>>24;
    }
    if(!h.cap_memories.empty()){
        auto &m=h.cap_memories[id];
        p.measures["pressure_contacts"]=capillary::count(m.tissue.on|m.tissue.off);
        p.measures["pressure_receptors"]=std::popcount(capillary::channels(h.genomes[id].pigments[2]));
        p.measures["pressure_capacity"]=capillary::width(h.genomes[id].pigments[2]);
        p.measures["pressure_contrast"]=bool(m.after);
    }
    if (!(drift_law.ports & retina::channel_mask))
        return;
    const auto &g = h.genomes[id];
    Word band = regulation::innervated_band(g, h.conducting(id));
    Tissue soma{};
    move_neural(nullptr, &soma, Count(id) * tiles + band, 1);
    retina::Genes genes{g.pigments[0], g.pigments[1]};
    Word used = 0;
    for (Word j = 0; j < 4; ++j)
        used |= 1u << ((drift_law.ports & drift::tidal_banks) ? retina::bank(genes, j) : 0);
    auto &q = p.measures;
    if(h.loom.mode&utterance::ink_port) {
        q["retained_dye_events"]=h.trails.empty()?0:h.trails[id].size();
        if(h.loom.mode&utterance::voice_port)
            q["resonator_phases"]=1+((g.hearing.expression>>24)&7);
    }
    if (h.loom.mode & audience::connected) {
        q["public_silhouette"] = audience::silhouette(h.bodies[id].body);
        q["neighbour_contacts"] = soma.gate_b >> 16;
        q["neighbour_write_gene"] = g.hearing.low;
        q["neighbour_motor_gene"] = g.hearing.high;
    }
    if (h.loom.mode & retina::iris_port) {
        q["iris_dendrite"] = g.pigments[2];
        q["iris_motor"] = g.pigments[3];
        q["iris_quorum"] = 1 + ((g.pigments[3] >> 17) & 3);
    }
    q["optical_banks"] = used;
    q["viewing_motors"] = (genes.receptors >> 16) & 255;
    q["optical_hand"] = h.riggers[id].volley >> 16;
    q["optical_gate_a"] = soma.gate_a;
    q["optical_gate_b"] = soma.gate_b;
    q["optical_receptor"] = genes.receptors;
    q["optical_plasticity"] = genes.plasticity;
}
const char *illumination_name() const {
    if (ink_books.ptr) {
        if(ink_echo) {
            if(!ink_slots && !ink_watch.ticks)return audience_books.ptr?"flood_fast_public_echo":"flood_fast_echo";
            return audience_books.ptr?"flood_public_echo":"flood_echo";
        }
        if(!ink_slots && !ink_watch.ticks)return audience_books.ptr?"flood_fast_public_ink":"flood_fast_ink";
        return audience_books.ptr?"flood_public_ink":"flood_ink";
    }
    return audience_books.ptr ? "flood_audiences"
           : stirred.ptr      ? "flood_soundings"
           : gaze_books.ptr   ? "flood_gaze"
                              : "flood_daylight";
}
void profile_daylight(Word n) {
    if (!light_books.ptr)
        throw std::invalid_argument("select an active daylight treatment to time");
    optical_clock = std::make_unique<DriftClock>(s, n);
    int registers = 0, local = 0;
    s.d.check(s.d.func_attr(&registers, 4, illumination), "count optical registers");
    s.d.check(s.d.func_attr(&local, 3, illumination), "count optical local bytes");
    std::cout << "daylight_kernel=" << illumination_name() << "  registers=" << registers << "  local_bytes=" << local
              << "  ledger_bytes="
              << 128u * (sizeof(retina::Ledger) + (gaze_books.ptr ? sizeof(retina::GazeLedger) : 0) +
                         (stirred.ptr ? sizeof(retina::StirPatch) : 0) +
                         (audience_books.ptr ? sizeof(audience::Ledger) : 0) + (ink_books.ptr?sizeof(utterance::Ledger):0)) +
                     Count(probe_sites) *
                         (sizeof(retina::StirSample) + (public_samples ? sizeof(audience::Witness) : 0))
              << '\n';
}
void sample_tracers(Word moving) {
    rotating_samples = moving;
    if (!stirred.ptr)
        throw std::invalid_argument("a tracer record needs stirred water");
    probe_sites = std::min(4096u, n / 4);
    probe_samples = std::make_unique<Temporary>(s.d, Count(probe_sites) * sizeof(retina::StirSample));
    if (audience_books.ptr) {
        public_samples = std::make_unique<Temporary>(s.d, Count(probe_sites) * sizeof(audience::Witness));
        s.d.check(s.d.zero(public_samples->ptr, 0, Count(probe_sites) * sizeof(audience::Witness)),
                  "clear visible outlines");
    }
    s.d.check(s.d.zero(probe_samples->ptr, 0, Count(probe_sites) * sizeof(retina::StirSample)),
              "clear the sampled tracer beds");
}
void report_soundings(const std::string &trace) {
    if (!stirred.ptr) {
        if (!trace.empty())
            throw std::invalid_argument("a tracer record needs stirred water");
        return;
    }
    std::array<retina::StirPatch, 128> patches{};
    s.d.check(s.d.download(patches.data(), stirred.ptr, sizeof(patches)), "read the living animals' tracer releases");
    using Values = std::array<Count, sizeof(retina::StirCounts) / sizeof(Count)>;
    Values sum{};
    std::unique_ptr<EvidenceFile> f;
    if (!trace.empty()) {
        f = std::make_unique<EvidenceFile>(trace);
        f->out << "tick\ttile\tslot\tborn\tfirst\tother\trest\tdye\tlamp\tplaces\thues\theading\treceptors\tplasticity"
                  "\tdendrite\tiris_"
                  "motor\twiring\tports\told\tspike\ttie\tleft\tright\thand\tphysical\tstarted\tlinks\tdestination\tblo"
                  "cked\tstock\tdownstream\tstock_after\tdownstream_after\tcrossed\tlineage";
        if (public_samples)
            f->out << "\tpeer_visible\tsilhouette\taddressed\tpublic_tape\twrite_gene\tmotor_gene\texchanged_face";
        f->out << '\n';
    }
    std::vector<retina::StirSample> samples;
    std::vector<audience::Witness> faces(public_samples ? probe_sites : 0);
    if (public_samples)
        s.d.check(s.d.download(faces.data(), public_samples->ptr, faces.size() * sizeof(faces[0])),
                  "read public glints");
    if (probe_samples) {
        samples.resize(probe_sites);
        s.d.check(s.d.download(samples.data(), probe_samples->ptr, samples.size() * sizeof(samples[0])),
                  "read the sampled tracer beds");
    }
    bool exemplar = false;
    for (const auto &patch : patches) {
        auto v = std::bit_cast<Values>(patch.counts);
        for (Word j = 0; j < v.size(); ++j)
            sum[j] += v[j];
        if (!probe_samples)
            samples.push_back(patch.sample);
    }
    for (size_t i = 0; i < samples.size(); ++i) {
        const auto &sample = samples[i];
        if (!sample.tick)
            continue;
        if (f) {
            using Words = std::array<Word, sizeof(sample) / sizeof(Word)>;
            const auto fields = std::bit_cast<Words>(sample);
            for (Word j = 0; j < fields.size(); ++j) {
                if (j)
                    f->out << '\t';
                f->out << fields[j];
            }
            if (public_samples) {
                const auto &peer = faces[i];
                for (Word val :
                     {peer.visible, peer.shape, peer.face, peer.tape, peer.write_gene, peer.motor_gene, peer.exchanged})
                    f->out << '\t' << val;
            }
            f->out << '\n';
        }
        if (!exemplar && sample.started && !sample.blocked && sample.stock_after < sample.stock &&
            drift::outlet(sample.first, sample.links) != drift::outlet(sample.other, sample.links)) {
            std::cout << "tracer_witness=" << sample.tick << "  site=" << sample.tile << "  resident=" << sample.slot
                      << "  first=" << sample.first << "  second=" << sample.other << "  muscle=" << sample.hand
                      << "  source=" << (sample.physical ? sample.other : sample.first)
                      << "  outlet=" << sample.destination
                      << "  other_outlet=" << drift::outlet(sample.physical ? sample.first : sample.other, sample.links)
                      << "  stock=" << sample.stock << "  stock_after=" << sample.stock_after
                      << "  downstream=" << sample.downstream << "  downstream_after=" << sample.downstream_after
                      << '\n';
            exemplar = true;
        }
    }
    if (f)
        std::cout << "tracer_sampling=" << tick << "  slots=" << probe_sites << "  moving=" << rotating_samples
                  << "  records="
                  << std::count_if(samples.begin(), samples.end(), [](const auto &s) { return s.tick != 0; })
                  << "  sample_bytes="
                  << Count(probe_sites) *
                         (sizeof(retina::StirSample) + (public_samples ? sizeof(audience::Witness) : 0))
                  << '\n';
    if (f)
        f->publish();
    auto b = std::bit_cast<retina::StirCounts>(sum);
    if (b.opened > b.offered || b.moved + b.blocked > b.opened || b.divergent > b.opened)
        throw std::logic_error("unbalanced tracer pushes");
    std::cout << "field_soundings=" << tick << "  start=" << drift_start << "  paired_wet_stock=" << b.racks
              << "  offered=" << b.offered << "  initiated=" << b.opened << "  moved=" << b.moved
              << "  impeded=" << b.blocked << "  divergent=" << b.divergent << "  neighbour_looks=" << b.witnesses
              << "  neighbour_edits=" << b.changed << "  crossed=" << bool(world.braids.mode & retina::crossed_hands)
              << "  split=" << bool(world.braids.mode & retina::lineage_hands) << "  bytes_per_resident=0" << '\n';
}
void daylight_report(const std::string &trace = {}) {
    report_soundings(trace);
    if (audience_books.ptr) {
        using Totals = std::array<Count, sizeof(audience::Ledger) / sizeof(Count)>;
        std::array<Totals, 128> bands{};
        s.d.check(s.d.download(bands.data(), audience_books.ptr, sizeof(bands)), "read field contacts");
        Totals sums{};
        for (auto &band : bands)
            for (size_t i = 0; i < sums.size(); ++i)
                sums[i] += band[i];
        auto q = std::bit_cast<audience::Ledger>(sums);
        std::cout << "field_audience=" << tick << "  shared_views=" << q.meetings << "  visible_neighbours=" << q.writes
                  << "  contact_edits=" << q.edits << "  opportunities=" << q.choices
                  << "  public_reaches=" << q.reached << "  changed_hand=" << q.changed_hand
                  << "  face_sensitive=" << q.trace_sensitive
                  << "  crossed_face=" << bool(world.braids.mode & audience::crossed)
                  << "  split_faces=" << bool(world.braids.mode & audience::lineage) << "  new_resident_bytes=0\n";
    }
    if (optical_clock)
        optical_clock->report("daylight_profile");
    if (course_books.ptr) {
        std::array<drift::Courses, 128> pieces{};
        s.d.check(s.d.download(pieces.data(), course_books.ptr, sizeof(pieces)),
                  "count physical passages through wound fibre");
        using Fields = std::array<Count, sizeof(drift::Courses) / sizeof(Count)>;
        Fields sum{};
        for (const auto &part : pieces) {
            auto item = std::bit_cast<Fields>(part);
            for (Word i = 0; i < sum.size(); ++i)
                sum[i] += item[i];
        }
        auto q = std::bit_cast<drift::Courses>(sum);
        if (q.serial > q.pulses || q.stages < q.pulses || q.eaten > q.material || q.reversed_food > q.material ||
            q.right > q.reaches || q.order_changed > q.serial)
            throw std::logic_error("inconsistent woven watercourse");
        std::cout << "woven_courses=" << tick << "  start=" << drift_start << "  flows=" << q.pulses
                  << "  serial=" << q.serial << "  stages=" << q.stages << "  order_matters=" << q.order_changed
                  << "  beyond_first=" << q.beyond_first << "  released=" << q.material << "  eaten=" << q.eaten
                  << "  reversed_intake=" << q.reversed_food << "  reaches=" << q.reaches << "  right=" << q.right
                  << "  single=" << q.single_right << "  certain=" << q.committed << "  changed_cord=" << q.altered
                  << "  changed_routes=" << q.altered_map << '\n';
    }
    if (bottom_books.ptr) {
        std::array<drift::BottomAccess, 128> shards{};
        s.d.check(s.d.download(shards.data(), bottom_books.ptr, sizeof(shards)), "count attempts at submerged food");
        drift::BottomAccess all{};
        for (auto &b : shards) {
            all.attempted += b.attempted;
            all.obstructed += b.obstructed;
            all.supported += b.supported;
            all.immediate_food += b.immediate_food;
        }
        std::cout << "bottom_access=" << tick << "  attempted=" << all.attempted << "  too_deep=" << all.obstructed
                  << "  raised_within_reach=" << all.supported << "  immediate_food=" << all.immediate_food << '\n';
    }
    if (!light_books.ptr)
        return;
    std::array<retina::Ledger, 128> shards{};
    s.d.check(s.d.download(shards.data(), light_books.ptr, sizeof(shards)), "weigh optical experience in the habitat");
    using Fields = std::array<Count, sizeof(retina::Ledger) / sizeof(Count)>;
    Fields sum{};
    for (const auto &row : shards) {
        auto f = std::bit_cast<Fields>(row);
        for (Word j = 0; j < f.size(); ++j)
            sum[j] += f[j];
    }
    auto r = std::bit_cast<retina::Ledger>(sum);
    if (r.attended > r.eyes || r.both > std::min(r.opening, r.ending) || r.eligible > r.attended ||
        r.same_source > r.same_outlet || r.same_outlet > r.unpaired || r.unpaired > r.attended)
        throw std::logic_error("inconsistent optical exposures");
    if (gaze_books.ptr) {
        std::array<retina::GazeLedger, 128> bands{};
        retina::GazeLedger g{};
        s.d.check(s.d.download(bands.data(), gaze_books.ptr, sizeof(bands)), "read acquisition decisions");
        for (const auto &band : bands) {
            g.offered += band.offered;
            g.unique += band.unique;
            g.bought_unique += band.bought_unique;
            g.silent += band.silent;
            g.bought_silent += band.bought_silent;
        }
        if (r.attended > g.offered || g.bought_silent > g.silent || g.bought_unique > g.unique)
            throw std::logic_error("inconsistent aperture counts");
        std::cout << "voluntary_gaze=" << tick << "  offered=" << g.offered << "  followed=" << r.attended
                  << "  heat=" << r.attended << "  conserved=" << (g.offered - r.attended) << "  unique=" << g.unique
                  << "  followed_unique=" << g.bought_unique << "  invisible_onset=" << g.silent
                  << "  followed_invisible=" << g.bought_silent << "  edits=" << r.edits << "  new_resident_bytes=0\n";
    }
    std::cout << "daylight=" << tick << "  start=" << drift_start
              << "  channel=" << ((drift_law.ports & retina::channel_mask) >> retina::shift) << "  wet=" << r.wet
              << "  eyes=" << r.eyes << "  attended=" << r.attended << "  onset=" << r.opening
              << "  offset=" << r.ending << "  paired=" << r.both << "  accepted=" << r.eligible
              << "  synaptic_edits=" << r.edits << "  spike_changes=" << r.spikes << "  unpaired=" << r.unpaired
              << "  same_source=" << r.same_source << "  same_endpoint=" << r.same_outlet
              << "  independent_source=" << bool(drift_law.ports & drift::independent_glints)
              << "  displaced_water=" << r.unobstructed << "  heat=" << r.attended << "  bytes_per_resident=0\n";
}
void collect_ink(std::vector<utterance::Sample> &out) {
    Word span = std::min(ink_watch.ticks, tick - ink_watch.start);
    if (!span)
        return;
    std::vector<utterance::Sample> entries(span);
    s.d.check(s.d.download(entries.data(), ink_notes.ptr + Count(ink_slots) * sizeof(entries[0]),
                           Count(span) * sizeof(entries[0])),
              "read a resident's continuous optical record");
    for (auto &e : entries)
        if (e.tick)
            out.push_back(e);
}
void ink_report(const Habitat &h, const Options &o, std::vector<utterance::Sample> &followed) {
    if (!ink_books.ptr)
        return;
    using Fields = std::array<Count, sizeof(utterance::Ledger) / sizeof(Count)>;
    std::array<Fields, 128> patches{};
    s.d.check(s.d.download(patches.data(), ink_books.ptr, sizeof(patches)), "count physical dye and sound exchanges");
    Fields sum{};
    for (const auto &part : patches)
        for (Word j = 0; j < sum.size(); ++j)
            sum[j] += part[j];
    auto v = std::bit_cast<utterance::Ledger>(sum);
    if (v.stained > v.moves || v.moves > v.releases || v.full > v.seen || v.vocal_heat != v.syllables ||
        v.borrowed > v.calls || v.retuned > v.syllables)
        throw std::logic_error("pigment or vocal accounts disagree");
    std::vector<utterance::Sample> notes(ink_slots);
    if (ink_slots)
        s.d.check(s.d.download(notes.data(), ink_notes.ptr, notes.size() * sizeof(notes[0])),
                  "read witnessed dye transitions");
    auto write = [](const std::string &path, const std::vector<utterance::Sample> &notes) {
        if (path.empty())
            return;
        EvidenceFile file(path);
        auto &out = file.out;
        out << "tick\ttile\tslot\tborn\tactor\tfrom\tto\tground\tbefore\tafter\twet\tfibres\tplaces\thues\treceptors\tc"
               "hannel\tlight\timage\tprior_"
               "size\tsize\tdisplaced\tprevious\ttrail\tdendrites\tmotor\tsequence\twords\tpaid\theading\tanchored\tech"
               "o\tcontroller\tnatal";
        for (Word k = 0; k < 8; ++k)
            out << "\tlow_" << k << "\thigh_" << k;
        out << '\n';
        for (const auto &x : notes)
            if (x.tick) {
                out << x.tick;
                for (Word n : {x.tile,    x.slot,  x.born,  x.actor,         x.from,         x.to,   x.ground,
                               x.palette, x.after, x.wet,   x.fibres,        x.places,       x.hues, x.receptors,
                               x.channel, x.light, x.image, x.source_before, x.source_after, x.coat})
                    out << '\t' << n;
                out << '\t' << x.previous << '\t' << x.trail;
                for (Word n : {x.dendrites, x.motor, x.sequence, x.words, x.paid, x.heading, x.anchored, x.echo,
                               x.controller, x.natal})
                    out << '\t' << n;
                for (Word k = 0; k < 8; ++k)
                    out << '\t' << x.ear.low[k] << '\t' << x.ear.high[k];
                out << '\n';
            }
        file.publish();
    };
    write(o.ink_trace, notes);
    collect_ink(followed);
    write(o.ink_events, followed);
    Word longest = 0, member = 0, experienced = 0;
    for (Word j = 0; j < h.trails.size(); ++j)
        if (h.bodies[j].energy && h.trails[j].size()) {
            ++experienced;
            if (h.trails[j].size() > longest) {
                longest = h.trails[j].size();
                member = j;
            }
        }
    std::cout << "field_ink=" << tick << "  releases=" << v.releases << "  moved=" << v.moves
              << "  stained=" << v.stained << "  changed_surfaces=" << v.exchanges << "  seen=" << v.seen
              << "  partial=" << v.omitted << "  full_histories=" << v.full << "  calls=" << v.calls
              << "  syllables=" << v.syllables << "  listeners=" << v.listeners << "  spike_edits=" << v.spike_changes
              << "  vocal_heat=" << v.vocal_heat << "  ear_heat=" << v.ear_heat << "  learned_calls=" << v.borrowed
              << "  learned_syllables=" << v.retuned << "  imitating=" << ink_echo
              << "  association_edits=" << v.acoustic_edits << "  reaches=" << v.strikes
              << "  blind_reaches=" << v.blind_strikes << "  food=" << v.intercepted << "  blind_food=" << v.blind_food
              << "  heard_order_changed=" << v.altered << "  grip_heat=" << v.reaching_heat
              << "  reversed=" << bool(world.braids.mode & utterance::crossed_port)
              << "  living_witnesses=" << experienced << "  longest=" << longest << "  witness=" << member
              << "  resident_bytes=8\n";
}
void retina_study(const Habitat &h, const Options &o) {
    if (!h.braiding)
        throw std::invalid_argument("a latent learning study needs a living retinal and motor apparatus");
    std::vector<Word> slots;
    Word seed = o.class_seed, start = mix(seed + 59033), stride = mix(seed + 4049) | 1;
    for (Word i = 0; i < (o.slot == ~0u ? n : 1u) && slots.size() < o.inlet_residents; ++i) {
        Word id = o.slot == ~0u ? (start + i * stride) & (n - 1) : o.slot;
        if (id < n && h.bodies[id].energy && h.bodies[id].body.eyes &&
            (o.retina_born == ~0u || h.bodies[id].born > o.retina_born))
            slots.push_back(id);
    }
    if (slots.empty()) {
        if (o.slot != ~0u)
            throw std::invalid_argument("the requested retinal witness must be alive and sighted");
        Word alive = std::count_if(h.bodies.begin(), h.bodies.end(), [](const auto &a) { return a.energy != 0; });
        std::cout << "retinal_census=" << h.header.tick << "  living=" << alive << "  enrolled=0  courses=0\n";
        return;
    }
    Word count = slots.size();
    Temporary selected(s.d, count * sizeof(Word)), organs(s.d, Count(count) * sizeof(retina::Observer));
    s.d.check(s.d.upload(selected.ptr, slots.data(), count * sizeof(Word)), "select living visual circuits");
    Function gather{};
    s.bind(gather, "retina_residents", Image::floodplain);
    launch(gather, (count + 63) / 64, 64, world, c, selected.ptr, count, organs.ptr);
    synchronize();
    std::vector<retina::Observer> bodies(count);
    s.d.check(s.d.download(bodies.data(), organs.ptr, Count(count) * sizeof(bodies[0])),
              "read the enrolled germlines and expressed tissues");
    retina::Course course{o.inlet_practice,        o.inlet_prior,         o.retina_laps,    o.inlet_landscape,
                          h.drift_water.law.ports, Word(o.inlet_carried), o.retina_channel, o.retina_stimulus,
                          o.retina_contrast,       o.retina_beds,         o.retina_fused,   o.retina_rear_laps};
    gaze_course(course, h, o);
    if (!o.retina_host.empty()) {
        storage::AgentImage common;
        common.read(o.retina_host);
        if (!common.riverine || !common.binds_floats || !common.resident.animal.body.eyes)
            throw std::invalid_argument("select a sighted experimental host with float-hand tissue");
        auto &a = common.resident;
        const auto &hand = common.braided.mind;
        Word width = common.grows_cells ? common.cortex.active
                                        : living_tiles(common.brain.tiles, common.brain.genes.development);
        Word band = regulation::innervated_band(a.germ, width);
        for (auto &b : bodies) {
            if (course.gaze)
                b.genes = {a.germ.pigments[0], a.germ.pigments[1]};
            b.heading = a.animal.heading;
            b.muscles = (hand.genes.muscle >> 24) & 7;
            b.senses = (hand.genes.arbor >> 25) & 7;
            b.wiring = drift::wiring(hand.genes, drift::recruitment);
            b.eyes = braid::eye_wiring(braid::individual_eyes, hand.genes, b.heading);
            b.contacts = hand.volley >> 16;
            b.spikes = common.brain.tissue[band].spike;
            b.gate_a = common.brain.tissue[band].gate_a;
            b.gate_b = common.brain.tissue[band].gate_b;
        }
        s.d.check(s.d.upload(organs.ptr, bodies.data(), Count(count) * sizeof(bodies[0])),
                  "place the sampled loci in the common sensory body");
    }
    sensory_surfaces(course, o);
    if (o.inlet_generations) {
        Word population = o.inlet_cohort, groups = population / 32, selected = o.inlet_selected, mutation = 1,
             bolus = o.inlet_bolus;
        std::vector<apprentice::Animal> adults(population);
        for (Word i = 0; i < population; ++i) {
            Word which = mix(seed + i * 3617) % count;
            const auto &g = bodies[which];
            auto program = course.gaze ? g.iris : regulation::Program{g.genes.receptors, g.genes.plasticity};
            adults[i] = {program, h.genomes[slots[which]].brain.reproduction & 0x1f00u, which};
        }
        Count bytes = Count(population) * sizeof(adults[0]);
        Temporary parents(s.d, bytes), young(s.d, bytes), meals(s.d, Count(population) * sizeof(apprentice::Harvest)),
            ledger(s.d, Count(groups) * sizeof(apprentice::Rations));
        s.d.check(s.d.upload(parents.ptr, adults.data(), bytes), "found a material-fed optical population");
        s.d.check(s.d.zero(ledger.ptr, 0, Count(groups) * sizeof(apprentice::Rations)),
                  "empty the optical nursery ledger");
        Function graze{}, breed{};
        s.bind(graze, course.gaze ? "retina_active_feeding" : "retina_feeding", Image::floodplain);
        s.bind(breed, "apprentice_reproduce", Image::runnels);
        Temporary costs(s.d, course.gaze ? 128 * sizeof(Count) : 0);
        if (costs.ptr)
            s.d.check(s.d.zero(costs.ptr, 0, 128 * sizeof(Count)), "zero the nursery optical heat ledger");
        DevicePtr current = parents.ptr, next = young.ptr;
        FieldTimer time(s.d);
        time.start(stream);
        for (Word i = 0; i < o.inlet_generations; ++i) {
            Word water = mix(seed + i * 7789 + 192709), birth = mix(seed + i * 5119 + 3951);
            launch(graze, groups, 32, organs.ptr, current, population, water, course, bolus, meals.ptr, costs.ptr);
            launch(breed, groups, 32, current, meals.ptr, population, birth, selected, mutation, next, ledger.ptr);
            std::swap(current, next);
        }
        time.stop(stream);
        synchronize();
        s.d.check(s.d.download(adults.data(), current, bytes), "read food-funded offspring");
        std::vector<apprentice::Rations> district(groups);
        s.d.check(s.d.download(district.data(), ledger.ptr, Count(groups) * sizeof(district[0])),
                  "read the optical nursery carbon budget");
        Count lived = 0, born = 0, food = 0, settled = 0, purse = 0;
        for (auto &r : district) {
            lived += r.adults;
            born += r.offspring;
            food += r.carbon;
            settled += r.washed;
            purse += r.left;
        }
        Count heat = 0;
        if (costs.ptr) {
            std::array<Count, 128> parts{};
            s.d.check(s.d.download(parts.data(), costs.ptr, sizeof(parts)), "read heat released through observation");
            for (auto v : parts)
                heat += v;
            std::cout << "gaze_metabolism=" << o.inlet_generations << "  starting_stock=" << lived * course.purse
                      << "  feeds=" << course.harvests << "  catchable=" << lived * bolus * course.harvests
                      << "  intercepted=" << lived * bolus * course.harvests - settled << "  settled=" << settled
                      << "  heat=" << heat << "  parents_carbon=" << food << "  offspring_carbon=" << 8 * born
                      << "  reserve=" << purse << "  selected=" << selected << '\n';
        }
        if (lived * (bolus * course.harvests + course.purse) != food + settled + heat || food != 8 * born + purse)
            throw std::logic_error("unfunded brood in the optical nursery");
        std::vector<retina::Observer> colony;
        for (auto &a : adults)
            if (a.ancestor != ~0u) {
                auto child = bodies[a.ancestor];
                if (course.gaze)
                    child.iris = a.program;
                else
                    child.genes = {a.program.first, a.program.second};
                colony.push_back(child);
            }
        bodies.swap(colony);
        count = bodies.size();
        std::cout << "retinal_brood=" << o.inlet_generations << "  selected=" << selected << "  adult_lives=" << lived
                  << "  births=" << born << "  food=" << food << "  settled=" << settled << "  reserve=" << purse
                  << "  bolus=" << bolus << "  survivors=" << count
                  << "  bytes_per_germ=16  cuda_ms=" << time.milliseconds() << '\n';
        if (!count)
            return;
    }
    Word lives = o.retina_trials, winner = 0;
    Temporary genes(s.d, Count(count) * sizeof(bodies[0])), book(s.d, Count(count) * sizeof(retina::Score));
    s.d.check(s.d.upload(genes.ptr, bodies.data(), Count(count) * sizeof(bodies[0])), "prepare unseen visual courses");
    Function examine{};
    s.bind(examine, course.gaze ? "retina_active_panel" : "retina_panel", Image::floodplain);
    std::vector<retina::Score> scores(count);
    std::unique_ptr<EvidenceFile> table;
    if (!o.retina_table.empty()) {
        table = std::make_unique<EvidenceFile>(o.retina_table);
        table->out << "panel\tseed\tindex\tslot\treceptors\tplasticity\tright\tblank\tone_step\tdistinct\tpair_"
                      "right\tchanged_horizon\thorizon_right\tcertain\tcertain_right";
        if (course.gaze)
            table->out << "\tgaze\tprobes\tdendrite\tiris_motor\theat\tfeeds\tcaught\tendowment\tbolus";
        table->out << '\n';
    }
    Word tested_seed = 0;
    for (Word panel = 0; panel < 2; ++panel) {
        tested_seed = panel ? mix(seed ^ 0x3e79f4abu) : seed;
        DevicePtr none = 0;
        FieldTimer time(s.d);
        time.start(stream);
        launch(examine, count, 32, genes.ptr, count, tested_seed, lives, course, book.ptr, none, none);
        time.stop(stream);
        synchronize();
        s.d.check(s.d.download(scores.data(), book.ptr, Count(count) * sizeof(scores[0])),
                  "read first reaches after passive exposure");
        Count group_right = 0, blank = 0, group_spent = 0;
        auto rank = [&](const retina::Score &q) {
            // Choose a colony member by the measured payoff of independent courses.
            auto net = static_cast<long long>(q.harvested) * o.inlet_bolus - q.spent;
            return std::tuple{net, q.right, q.paired, q.different_right};
        };
        for (Word i = 0; i < count; ++i) {
            auto &r = scores[i];
            auto &b = bodies[i];
            group_spent += r.spent;
            if (!panel && rank(r) > rank(scores[winner]))
                winner = i;
            group_right += r.right;
            blank += r.blank;
            if (table) {
                table->out << panel << '\t' << tested_seed << '\t' << i << '\t' << b.slot << '\t' << b.genes.receptors
                           << '\t' << b.genes.plasticity << '\t' << r.right << '\t' << r.blank << '\t' << r.once << '\t'
                           << r.distinct << '\t' << r.paired << '\t' << r.different << '\t' << r.different_right << '\t'
                           << r.certain << '\t' << r.certain_right;
                if (course.gaze)
                    table->out << '\t' << course.gaze << '\t' << course.probes << '\t' << b.iris.first << '\t'
                               << b.iris.second << '\t' << r.spent << '\t' << course.harvests << '\t' << r.harvested
                               << '\t' << course.purse << '\t' << o.inlet_bolus;
                table->out << '\n';
            }
        }
        auto &b = bodies[winner];
        auto &r = scores[winner];
        if (course.gaze)
            std::cout << "gaze_panel=" << (panel ? "confirmation" : "selection") << "  seed=" << tested_seed
                      << "  slot=" << b.slot << "  bodies=" << count << "  courses=" << 2 * lives
                      << "  presented=" << Count(2 * lives) * (course.rounds + course.former)
                      << "  gaze=" << course.gaze << "  probes=" << course.probes
                      << "  nursery_feeds=" << course.harvests << "  purse=" << course.purse << "  heat=" << r.spent
                      << "  bolus=" << o.inlet_bolus << "  correct=" << r.right
                      << "  portions=" << 2 * lives * course.harvests << "  caught=" << r.harvested
                      << "  intercepted=" << Count(r.harvested) * o.inlet_bolus << "  energy_left="
                      << Count(2 * lives) * course.purse + Count(r.harvested) * o.inlet_bolus - r.spent
                      << "  dendrite=" << b.iris.first << "  iris_motor=" << b.iris.second
                      << "  colony_heat=" << group_spent << "  colony_correct=" << group_right << '\n';
        std::cout << "retinal_panel=" << (panel ? "confirmation" : "selection") << "  tick=" << tick
                  << "  seed=" << tested_seed << "  bodies=" << count << "  courses=" << 2 * lives
                  << "  glimpses=" << course.rounds << "  former=" << course.former << "  laps=" << course.laps
                  << "  balanced_sources=1  actual_beds=" << course.sites << "  woven_order=" << course.natural_order
                  << "  beds=" << course.beds << "  fused=" << course.fused << "  rear_laps=" << course.rear_laps
                  << "  landscape=" << course.landscape << "  channel=" << course.channel
                  << "  stimulus=" << course.stimulus << "  contrast=" << course.contrast
                  << "  carried=" << course.history << "  common_host=" << !o.retina_host.empty() << "  slot=" << b.slot
                  << "  receptors=" << b.genes.receptors << "  plasticity=" << b.genes.plasticity
                  << "  first=" << r.right << "  untouched=" << r.blank << "  one_step=" << r.once
                  << "  different_horizon=" << r.different << "  horizon_right=" << r.different_right
                  << "  different_worlds=" << r.distinct << "  pair_right=" << r.paired << "  certain=" << r.certain
                  << "  certain_right=" << r.certain_right << "  synaptic_edits=" << r.edits
                  << "  two_glimpses=" << r.perceived << "  population_correct=" << group_right
                  << "  population_blank=" << blank << "  practice_food=0  cuda_ms=" << time.milliseconds() << '\n';
    }
    if (table)
        table->publish();
    auto &b = bodies[winner];
    if (!o.retina_germ.empty()) {
        EvidenceFile f(o.retina_germ);
        f.out << (course.gaze ? "AXION-IRIS-1 " : "AXION-RETINA-1 ") << (course.gaze ? b.iris.first : b.genes.receptors)
              << ' ' << (course.gaze ? b.iris.second : b.genes.plasticity) << ' ' << b.slot << ' ' << o.class_seed
              << ' ' << o.inlet_generations << '\n';
        f.publish();
    }
    if (!o.retina_trace.empty()) {
        EvidenceFile f(o.retina_trace);
        Word one = 1, turns = course.rounds + course.former;
        std::vector<retina::Forecast> result(2 * lives);
        std::vector<Word> samples(Count(2 * lives) * turns);
        Temporary choices(s.d, result.size() * sizeof(result[0])), obs(s.d, samples.size() * sizeof(samples[0]));
        DevicePtr first = genes.ptr + Count(winner) * sizeof(b);
        launch(examine, 1, 32, first, one, tested_seed, lives, course, book.ptr, choices.ptr, obs.ptr);
        synchronize();
        s.d.check(s.d.download(result.data(), choices.ptr, result.size() * sizeof(result[0])),
                  "unseal sampled choices");
        if (turns)
            s.d.check(s.d.download(samples.data(), obs.ptr, samples.size() * sizeof(samples[0])),
                      "read the observer's retinal volleys");
        f.out
            << "life\ttwin\tseed\tports\tlaps\tlandscape\tchannel\tstimulus\tcontrast\tformer\tglimpses\treceptors\tpla"
               "sticity\tplaces\thues\theading\tsenses\tmuscles\twiring\tinitial\tinitial_spikes\tmap\tsource\tfirst_"
               "exit\ttarget\trest\tonset\tfiring\toptions\tcommand\tbare\tsingle\tdraw\tcontacts\tspikes\tbeds\tfused"
               "\tactual_beds\twoven_order\tsurface\tprivate_lamps\tbalanced_sources\tmaps\tlamps\tschedule\tinitial_"
               "a\tinitial_b\ta\tb\tglints";
        if (course.gaze)
            f.out << "\tgaze\tpurse\tdendrite\tiris_motor\theat\tprobes\tfeeds\tcaught";
        f.out << '\n';
        for (Word i = 0; i < result.size(); ++i) {
            auto &r = result[i];
            f.out << i / 2 << '\t' << i % 2 << '\t' << tested_seed << '\t' << course.ports << '\t' << r.laps << '\t'
                  << course.landscape << '\t' << course.channel << '\t' << course.stimulus << '\t' << course.contrast
                  << '\t' << course.former << '\t' << course.rounds << '\t' << b.genes.receptors << '\t'
                  << b.genes.plasticity << '\t' << Word(b.eyes.places) << '\t' << Word(b.eyes.hues) << '\t' << b.heading
                  << '\t' << b.senses << '\t' << b.muscles << '\t' << b.wiring << '\t'
                  << (course.history ? b.contacts : 0) << '\t' << (course.history ? b.spikes : 0) << '\t' << r.map
                  << '\t' << r.source << '\t' << r.first << '\t' << r.target << '\t' << r.resting << '\t' << r.opening
                  << '\t' << r.onset << '\t' << r.options << '\t' << r.command << '\t' << r.bare << '\t' << r.single
                  << '\t' << r.draw << '\t' << r.contacts << '\t' << r.spikes << '\t' << course.beds << '\t'
                  << course.fused << '\t' << course.sites << '\t' << course.natural_order << '\t' << r.surface << '\t'
                  << 1 << '\t' << 1 << '\t' << r.maps << '\t' << r.lamps << '\t' << r.schedule << '\t'
                  << (course.history ? b.gate_a : 0) << '\t' << (course.history ? b.gate_b : 0) << '\t' << r.gate_a
                  << '\t' << r.gate_b << '\t';
            for (Word j = 0; j < turns; ++j) {
                if (j)
                    f.out << ',';
                f.out << samples[Count(i) * turns + j];
            }
            if (course.gaze)
                f.out << '\t' << course.gaze << '\t' << course.purse << '\t' << b.iris.first << '\t' << b.iris.second
                      << '\t' << r.spent << '\t' << course.probes << '\t' << course.harvests << '\t' << r.harvested;
            f.out << '\n';
        }
        f.publish();
    }
}
