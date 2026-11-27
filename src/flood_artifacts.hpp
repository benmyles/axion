Word characterize(const Habitat &h, const std::vector<material::Component> &components, Word samples) {
    if (!samples)
        return ~0u;
    std::vector<Word> ranks;
    Word skipped = 0, eligible = 0;
    for (Word i = 0; i < n; ++i) {
        auto &r = components[i];
        skipped += r.size > material::assay_size;
        if (r.size >= 3 && r.size <= material::assay_size && r.handled) {
            ranks.push_back(i);
            ++eligible;
        }
    }
    std::sort(ranks.begin(), ranks.end(), [&](Word a, Word b) {
        return components[a].size != components[b].size ? components[a].size > components[b].size : a < b;
    });
    ranks.resize(std::min<size_t>(samples, ranks.size()));
    if (ranks.empty()) {
        std::cout << "artifact_exposure=0  eligible=0  oversize=" << skipped << '\n';
        return ~0u;
    }
    std::vector<material::Assembly> sampleset;
    for (Word root : ranks) {
        std::vector<Word> members{root};
        for (size_t j = 0; j < members.size(); ++j) {
            Word at = members[j];
            for (Word d = 0; d < 4; ++d) {
                Word k = neighbor(at, d, c.side);
                if (material::bonded(h.mineral_bed[at], h.mineral_bed[k], d) &&
                    std::find(members.begin(), members.end(), k) == members.end())
                    members.push_back(k);
            }
        }
        if (members.size() != components[root].size)
            throw std::logic_error("material component changed during the census");
        material::Assembly shape;
        shape.root = root;
        shape.size = members.size();
        shape.side = c.side;
        // Deterministic ports chosen from geography alone. Keep every grain's
        // material, position and fold state, including neighbors that could
        // become connected later. Only the starting charge is discharged.
        std::sort(members.begin(), members.end());
        Word size = shape.size;
        std::vector<int> distance(size * size, 1000);
        for (Word i = 0; i < size; ++i) {
            Word at = members[i];
            auto &f = shape.fragments[i];
            f.site = at;
            f.grain = h.mineral_bed[at];
            distance[i * size + i] = 0;
            for (Word d = 0; d < 4; ++d) {
                auto k = neighbor(at, d, c.side);
                auto entry = std::lower_bound(members.begin(), members.end(), k);
                if (entry != members.end() && *entry == k) {
                    Word j = entry - members.begin();
                    f.neighbor[d] = static_cast<Byte>(j);
                    if (material::bonded(f.grain, h.mineral_bed[k], d))
                        distance[i * size + j] = 1;
                }
            }
        }
        for (Word k = 0; k < size; ++k)
            for (Word i = 0; i < size; ++i)
                for (Word j = 0; j < size; ++j)
                    distance[i * size + j] =
                        std::min(distance[i * size + j], distance[i * size + k] + distance[k * size + j]);
        int diameter = -1;
        for (Word i = 0; i < size; ++i)
            for (Word j = i + 1; j < size; ++j)
                if (distance[i * size + j] > diameter) {
                    diameter = distance[i * size + j];
                    shape.sink = i;
                    shape.first = j;
                }
        int separation = -1;
        for (Word i = 0; i < size; ++i)
            if (i != shape.first && i != shape.sink) {
                int gap = std::min(distance[shape.sink * size + i], distance[shape.first * size + i]);
                if (gap > separation) {
                    separation = gap;
                    shape.second = i;
                }
            }
        sampleset.push_back(shape);
    }
    Word count = sampleset.size();
    Temporary descriptions(s.d, Count(count) * sizeof(material::Assembly)),
        outputs(s.d, Count(count) * 21 * sizeof(material::Response));
    s.d.check(s.d.upload(descriptions.ptr, sampleset.data(), sampleset.size() * sizeof(sampleset[0])),
              "place measured assemblies in paired light chambers");
    Function run{};
    s.bind(run, "flood_artifact_response", Image::floodplain);
    launch(run, count * 21, 64, descriptions.ptr, outputs.ptr, count);
    synchronize();
    std::vector<material::Response> answers(count * 21);
    s.d.check(s.d.download(answers.data(), outputs.ptr, answers.size() * sizeof(answers[0])),
              "read actual chemical outputs under seven illuminations");
    Word photographed = ~0u, conductive = ~0u;
    Word reactive = 0, coincidence = 0, storage = 0, ordered = 0, hinged = 0;
    std::set<std::array<Word, 7>> signatures;
    for (Word i = 0; i < count; ++i) {
        auto *r = answers.data() + 21 * i;
        const auto &shape = sampleset[i];
        std::array<Word, 7> vector{};
        for (Word j = 0; j < 21; ++j)
            if (r[j].stored + r[j].product + r[j].thermal != r[j].absorbed)
                throw std::logic_error("isolated material sample lost excitation");
        for (Word j = 0; j < 7; ++j) {
            vector[j] = r[j].product;
            if (r[j + 7].product || r[j].product > r[j].absorbed)
                throw std::logic_error("isolated port received unaccounted excitation");
        }
        signatures.insert(vector);
        if (r[3].product && conductive == ~0u)
            conductive = shape.root;
        if (!r[1].product && !r[2].product && r[3].product && photographed == ~0u)
            photographed = shape.root;
        reactive += r[3].product > 0;
        coincidence += !r[1].product && !r[2].product && r[3].product;
        storage += r[4].product > 0;
        ordered += r[5].product != r[6].product;
        bool fold_effect = false;
        for (Word j = 0; j < 7; ++j)
            fold_effect |= r[j].product != r[14 + j].product;
        hinged += fold_effect;
        std::cout << "artifact_signature=" << shape.root << "  parts=" << shape.size
                  << "  lamp_a=" << shape.fragments[shape.first].site
                  << "  lamp_b=" << shape.fragments[shape.second].site << "  sink=" << shape.fragments[shape.sink].site
                  << "  dark=" << vector[0] << "  a=" << vector[1] << "  b=" << vector[2] << "  both=" << vector[3]
                  << "  retained=" << vector[4] << "  a_then_b=" << vector[5] << "  b_then_a=" << vector[6]
                  << "  rigid_change=" << fold_effect << "  absorbed_both=" << r[3].absorbed
                  << "  stored_both=" << r[3].stored << "  heat_both=" << r[3].thermal << "  insulated=0\n";
    }
    std::cout << "artifact_exposure=" << count << "  eligible=" << eligible << "  oversize=" << skipped
              << "  transmitting=" << reactive << "  coincidence=" << coincidence << "  stored=" << storage
              << "  order_sensitive=" << ordered << "  fold_sensitive=" << hinged
              << "  response_patterns=" << signatures.size() << '\n';
    return photographed != ~0u ? photographed : conductive != ~0u ? conductive : ranks.front();
}
