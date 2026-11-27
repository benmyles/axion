// A bounded photograph of real pieces and bonds around the largest component.
// Every recorded cell keeps its own plugs, substance, charge and recent contact.
void material_photograph(const Habitat &h, Word root) {
    auto *journal = journal::current();
    if (!journal || root == ~0u)
        return;
    Word side = h.header.configuration.side, dim = std::min(11u, side);
    auto edge = [&](Word pos) {
        Word bits = 0;
        if (material::conducts(h.minerals.physics))
            for (Word d = 0; d < 4; ++d)
                bits |= Word(material::bonded(h.mineral_bed[pos], h.mineral_bed[neighbor(pos, d, side)], d)) << d;
        return bits;
    };
    std::vector<Word> connected{root};
    std::set<Word> seen{root};
    for (size_t j = 0; j < connected.size(); ++j) {
        Word pos = connected[j], bonds = edge(pos);
        for (Word d = 0; d < 4; ++d)
            if (bonds & (1u << d)) {
                Word n = neighbor(pos, d, side);
                if (seen.insert(n).second)
                    connected.push_back(n);
            }
    }
    auto distance = [&](Word a, Word b) {
        int d = int((a - b + side / 2) & (side - 1)) - int(side / 2);
        return d;
    };
    long sum_x = 0, sum_y = 0;
    for (auto pos : connected) {
        sum_x += distance(pos % side, root % side);
        sum_y += distance(pos / side, root / side);
    }
    auto cx = root % side + int(sum_x / long(connected.size())), cy = root / side + int(sum_y / long(connected.size()));
    constexpr char hex[] = "0123456789abcdef";
    auto byte = [&](std::string &str, Word value) {
        str += hex[(value >> 4) & 15];
        str += hex[value & 15];
    };
    for (Word y = 0; y < dim; ++y) {
        std::string pixels;
        for (Word x = 0; x < dim; ++x) {
            Word pos = cell_at(cx + x - dim / 2, cy + y - dim / 2, side);
            auto g = h.mineral_bed[pos];
            byte(pixels, g.faces);
            byte(pixels, g.substance);
            byte(pixels, g.charge);
            byte(pixels, g.marks);
            byte(pixels, edge(pos) | (seen.contains(pos) ? 16 : 0));
        }
        journal->line("mineral_row=" + std::to_string(y) + "  grains=" + pixels);
    }
    journal->metrics["mineral_window_x"]=(cx-dim/2)&(side-1);
    journal->metrics["mineral_window_y"]=(cy-dim/2)&(side-1);
    journal->metrics["mineral_focus_root"] = root;
    journal->metrics["mineral_focus_size"] = connected.size();
}
