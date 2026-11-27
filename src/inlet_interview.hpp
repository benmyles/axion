void inlet_tuition(const braid::Course &c, Word mode) {
    auto &animal = float_actor(c.who);
    auto &body = animal.resident.animal;
    if (!body.body.limbs || !body.body.pores)
        throw std::invalid_argument("a current lesson needs a working mouth and limb");
    auto motor = connect_cortex(c.who);
    auto ear = connect_cortex(c.who, animal.braid_report.memory.auditory_route);
    apprentice::Teachers tissue{};
    tissue.motor = motor.organ;
    if (ear.index != motor.index)
        tissue.voice = ear.organ;
    Temporary live(s.d, sizeof(tissue)), receipt(s.d, sizeof(inlet::Tuition));
    s.d.check(s.d.upload(live.ptr, &tissue, sizeof(tissue)), "present the pupil's motor and auditory populations");
    inlet::Gesture b{};
    b.value = 12;
    b.energy = body.energy;
    b.mouth = 1 + std::popcount(body.body.pores);
    b.heading = body.heading;
    b.ports = drift::live | drift::lateral | drift::recruitment | drift::regrowth;
    Function teach{};
    s.bind(teach, "inlet_tuition", Image::runnels);
    auto hand = float_head.ptr + Count(c.who) * sizeof(braid::Saved),
         words = float_memories.ptr + Count(c.who) * sizeof(braid::Narration);
    braid::Course lesson = c;
    void *args[]{&hand, &words, &live.ptr, &lesson, &mode, &b, &receipt.ptr};
    s.d.check(s.d.launch(teach, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr),
              "learn a question through witnessed two-stage currents");
    inlet::Tuition r{};
    s.d.check(s.d.download(&r, receipt.ptr, sizeof(r)), "weigh the current lesson");
    s.d.check(s.d.download(&tissue, live.ptr, sizeof(tissue)), "retain the two expressed populations");
    s.d.check(s.d.download(&animal.braided, hand, sizeof(animal.braided)), "retain real practice at the teaching bed");
    if (r.food > r.released || Count(r.food) + b.energy <= r.paid || Count(r.food) + b.energy - r.paid > 65535 ||
        r.released != 4 * r.paid)
        throw std::logic_error("current tuition exceeded its carbon");
    retain_spikes(c.who, motor, tissue.motor);
    if (ear.index != motor.index)
        retain_spikes(c.who, ear, tissue.voice);
    report_memory(false);
    body.energy = b.energy + r.food - r.paid;
    body.worked += r.paid;
    body.meals += r.food;
    body.diet[1] += r.food;
    std::cout << "inlet_lesson=" << c.frame << "  speaker=" << c.who << "  scenes=" << r.scenes
              << "  offered=" << r.presented << "  attempted=" << r.attempted << "  matched=" << r.matched
              << "  edits=" << r.edits << "  motor_band=" << motor.index << "  vocal_band=" << ear.index
              << "  changed_forecasts=" << r.changed_forecasts << "  distinct_midpoints=" << r.distinct_midpoints
              << "  heat=" << r.paid << "  food=" << r.food << "  released=" << r.released
              << "  settled=" << r.released - r.food << '\n';
}
bool inlet_command(const std::string &token, std::istringstream &input) {
    if (!token.starts_with("/inlet-"))
        return false;
    if (!local.binds_floats || !local.riverine || !float_memories.ptr)
        throw std::invalid_argument("a watercourse needs a resident with a physical float-hand circuit");
    auto &body = local.resident.animal;
    if (!body.body.eyes || !body.body.limbs || !body.body.pores)
        throw std::invalid_argument("a watercourse operator needs eyes, feeding pores and a limb");
    inlet::Gesture action{};
    std::string arg;
    auto quantity = [&](Word bound) {
        if (!(input >> arg))
            throw std::invalid_argument("the watercourse needs a number");
        return parse(arg, bound);
    };
    if (token == "/inlet-table") {
        action.cmd = inlet::open_table;
        action.value = quantity(~0u);
        std::string terrain = "mixed", readout = "contextual";
        if (input >> terrain) {
            if (input >> readout) {
                if (!(input >> std::ws).eof())
                    action.twin = quantity(1);
                if (!(input >> std::ws).eof())
                    action.life = quantity(16383);
            }
        }
        if (terrain != "channels" && terrain != "basin" && terrain != "mixed" && terrain != "seep" &&
            terrain != "field")
            throw std::invalid_argument("choose channels, basin, mixed, field or seep");
        if (readout != "inherited" && readout != "local" && readout != "contextual" && readout != "misaligned")
            throw std::invalid_argument("choose inherited, contextual, local or misaligned reach wiring");
        action.landscape = terrain == "channels" ? inlet::channels
                           : terrain == "basin"  ? inlet::basin
                           : terrain == "mixed"  ? inlet::mixed
                                                 : inlet::field;
        action.ports = drift::live | drift::regrowth;
        if (terrain == "seep")
            action.ports |= drift::seepage;
        if (readout != "local")
            action.ports |= drift::lateral;
        if (readout == "contextual" || readout == "misaligned")
            action.ports |= drift::recruitment;
        if (readout == "misaligned")
            action.ports |= drift::misaligned;
    } else if (token == "/inlet-practice") {
        action.cmd = inlet::handle;
        action.value = quantity(512);
    } else if (token == "/inlet-stage") {
        action.cmd = inlet::stage;
        if (input >> arg) {
            action.noun = acoustic::spoken_word(arg);
            if (!action.noun)
                throw std::invalid_argument("the marker name must be audible");
        }
    } else if (token == "/inlet-pour")
        action.cmd = inlet::pour;
    else if (token == "/inlet-turn")
        action.cmd = inlet::turn;
    else if (token == "/inlet-clear")
        action.cmd = inlet::quiet_hand;
    else
        return false;
    if (input >> arg)
        throw std::invalid_argument("unexpected watercourse argument");
    if (!inlet_bench) {
        if (action.cmd != inlet::open_table)
            throw std::invalid_argument("first open a watercourse with /inlet-table");
        inlet_bench = std::make_unique<Temporary>(s.d, sizeof(inlet::Instrument));
        inlet_receipt = std::make_unique<Temporary>(s.d, sizeof(inlet::Receipt));
        s.bind(inlet_action, "inlet_contact", Image::runnels);
    }
    action.energy = body.energy;
    action.mouth = 1 + std::popcount(body.body.pores);
    action.heading = body.heading;
    action.stamp = 1 + local.resident.tick;
    s.d.check(s.d.upload(float_head.ptr, &local.braided, sizeof(local.braided)), "attach the speaker's own hand");
    report_memory(true);
    auto band = connect_cortex(0);
    void *args[]{&inlet_bench->ptr,    &float_head.ptr, &float_memories.ptr,
                 &float_regulator.ptr, &action,         &inlet_receipt->ptr};
    s.d.check(s.d.launch(inlet_action, 1, 1, 1, 1, 1, 1, 0, nullptr, args, nullptr), "act on the physical watercourse");
    inlet::Receipt note{};
    s.d.check(s.d.download(&note, inlet_receipt->ptr, sizeof(note)), "read the committed reach");
    Count resources = Count(action.energy) + note.eaten;
    if (note.supply + note.captured + note.settled != 4096 || note.paid >= resources || resources - note.paid > 65535 ||
        note.eaten > note.captured)
        throw std::logic_error("a public watercourse lost food or metabolic work");
    return_spikes(0, band);
    report_memory(false);
    s.d.check(s.d.download(&local.braided, float_head.ptr, sizeof(local.braided)),
              "retain the operator's learned contacts");
    body.energy = static_cast<flood::Half>(resources - note.paid);
    body.worked += note.paid;
    body.meals += note.eaten;
    body.diet[1] += note.eaten;
    std::cout << "inlet_table=" << token.substr(7) << "  status=" << note.status << "  practiced=" << note.practiced
              << "  sampled=" << note.tried << "  inlet=" << note.inlet << "  pending=" << note.pending
              << "  delivered=" << note.finished << "  contacts_before=" << note.before
              << "  contacts_after=" << note.after << "  food_remaining=" << note.supply << "  eaten=" << note.captured
              << "  settled=" << note.settled << "  heat=" << note.heat;
    if (note.marker < 4)
        std::cout << "  marker=" << note.marker << "  forecast_mask=" << note.options << "  reached=" << note.reach
                  << "  proximal=" << note.local;
    if (note.outlet < 4)
        std::cout << "  outlet=" << note.outlet;
    std::cout << "  rotations=" << note.moved << '\n';
    return true;
}
