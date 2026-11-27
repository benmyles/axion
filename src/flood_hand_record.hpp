void connect_hands() {
    if (!hand_weights.ptr)
        return;
    world.grip_contacts = reinterpret_cast<Word *>(hand_weights.ptr);
    material_arms = {
        reinterpret_cast<unsigned char *>(hand_pending.ptr), reinterpret_cast<capillary::HandCounts *>(hand_books.ptr),
        reinterpret_cast<capillary::HandNote *>(hand_camera.ptr), hand_camera.ptr ? std::min(n, 4096u) : 0u};
    s.d.check(s.d.zero(hand_weights.ptr, 0, Count(n) * sizeof(Word)), "clear natal two-axis motor contacts");
    s.d.check(s.d.zero(hand_books.ptr, 0, 128 * sizeof(capillary::HandCounts)), "clear natural grip observations");
    if (hand_camera.ptr)
        s.d.check(s.d.zero(hand_camera.ptr, 0, material_arms.samples * sizeof(capillary::HandNote)),
                  "clear material-hand camera");
    s.bind(hand_act, "flood_grasp_material", Image::floodplain);
    s.bind(hand_learn, "flood_taste_after_grasp", Image::floodplain);
    s.bind(hand_export, "flood_hand_owners", Image::floodplain);
}
void transfer_hands(Habitat &h, bool upload) {
    if (!hand_weights.ptr)
        return;
    if (upload) {
        std::vector<Word> tissue(n);
        for (const auto &owner : h.compound_residents)
            tissue[owner.slot] = owner.signs;
        s.d.check(s.d.upload(hand_weights.ptr, tissue.data(), tissue.size() * 4),
                  "restore living two-axis motor contacts");
    } else {
        Temporary gathered(s.d, Count(n) * sizeof(capillary::HandOwner));
        launch(hand_export, (n + 127) / 128, 128, world, n, gathered.ptr);
        synchronize();
        std::vector<capillary::HandOwner> owned(n);
        s.d.check(s.d.download(owned.data(), gathered.ptr, Count(n) * sizeof(owned[0])),
                  "read living synapses with their birth identities");
        h.compound_residents.clear();
        for (Word slot = 0; slot < n; ++slot)
            if (owned[slot].signs)
                h.compound_residents.push_back({slot, owned[slot].born, owned[slot].signs});
    }
}
void report_hands(const Habitat &h, const std::string &path) {
    if (!hand_books.ptr)
        return;
    using capillary::HandCounts;
    std::array<HandCounts, 128> shards{};
    HandCounts total{};
    s.d.check(s.d.download(shards.data(), hand_books.ptr, sizeof(shards)), "read embodied material handling");
    auto sum = [&](Count HandCounts::*field) {
        for (const auto &r : shards)
            total.*field += r.*field;
    };
    for (auto k : {&HandCounts::attempts, &HandCounts::paid, &HandCounts::unavailable, &HandCounts::placed,
                   &HandCounts::lifted, &HandCounts::food, &HandCounts::edits, &HandCounts::retained_moves,
                   &HandCounts::different_part, &HandCounts::different_flow, &HandCounts::created_flow,
                   &HandCounts::removed_flow, &HandCounts::new_food, &HandCounts::young})
        sum(k);
    for (Word j = 0; j < 4; ++j)
        for (const auto &r : shards)
            total.direction[j] += r.direction[j];
    bool fixed = capillary_field.law.mode & capillary::fixed_hands,
         ghost = capillary_field.law.mode & capillary::disconnected_hands;
    if (total.paid != total.attempts ||
        (ghost ? (total.placed || total.lifted || total.different_flow)
               : total.placed + total.lifted + total.unavailable != total.attempts) ||
        (fixed && total.edits) || total.food > 4 * total.new_food)
        throw std::logic_error("inconsistent free material handling account");
    std::cout << "material_hands=" << tick << "  attempts=" << total.attempts << "  paid=" << total.paid
              << "  missing_stock=" << total.unavailable << "  installed=" << total.placed
              << "  recovered=" << total.lifted << "  food=" << total.food << "  fed_visits=" << total.new_food
              << "  edits=" << total.edits << "  retained_visits=" << total.retained_moves
              << "  different_grip=" << total.different_part << "  different_local_flow=" << total.different_flow
              << "  opened_flow=" << total.created_flow << "  closed_flow=" << total.removed_flow
              << "  young_visits=" << total.young << "  living_owners=" << h.compound_residents.size()
              << "  fixed=" << fixed << "  disconnected=" << ghost
              << "  contact_bytes=4  receipt_bytes=1  field_bytes=" << Count(n) * 5
              << "  strokes=" << total.direction[0] << ',' << total.direction[1] << ',' << total.direction[2] << ','
              << total.direction[3] << '\n';
    if (path.empty())
        return;
    std::vector<capillary::HandNote> r(material_arms.samples);
    s.d.check(s.d.download(r.data(), hand_camera.ptr, r.size() * sizeof(r[0])),
              "read individual physical hand records");
    EvidenceFile f(path);
    f.out << "tick\tsite\tslot\tborn\tfields\tsight\tgeology\tcoating\tcarrier\treactive\tstone\tbefore\tactual\tcold_"
             "row\tuntouched_flow\tflow\tcold_flow\tchoice\tcold_choice\tpossible\tcold_"
             "possible\texecuted\tdonation\tedge0\tedge1\tstock0\tstock1\tearlier_diet\tlater_"
             "diet\tfood\tedits\ttissue0\ttissue1\n";
    for (auto &v : r)
        if (v.tick) {
            f.out << v.tick;
            for (auto j :
                 {v.site,     v.slot,      v.born,    v.fields,      v.sight,    v.geology,       v.coating,
                  v.carrier,  v.reactive,  v.stone,   v.before,      v.actual,   v.cold_row,      v.untouched_flow,
                  v.flow,     v.cold_flow, v.choice,  v.cold_choice, v.possible, v.cold_possible, v.executed,
                  v.donation, v.edge0,     v.edge1,   v.stock0,      v.stock1,   v.earlier_diet,  v.later_diet,
                  v.food,     v.edits,     v.tissue0, v.tissue1})
                f.out << '\t' << j;
            f.out << '\n';
        }
    f.publish();
}
