// Same local conductance, storage, uptake and catalytic threshold as the
// habitat. The apparatus isolates a connected specimen and supplies two lamps.
KERNEL void flood_artifact_response(const material::Assembly *assemblies, material::Response *out, Word n) {
    Word local = tid() & 63, experiment = tid() / 64, kind = experiment % 7, arm = (experiment / 7) % 3,
         part = experiment / 21;
    if (part >= n)
        return;
    const auto &shape = assemblies[part];
    auto fragment = shape.fragments[local];
    __attribute__((shared)) static Word storage[64];
    auto *pieces = reinterpret_cast<material::Grain *>(storage);
    __attribute__((shared)) static Word census[64];
    auto grain = fragment.grain;
    grain.charge = 0;
    pieces[local] = grain;
    asm volatile("bar.sync 0;" ::: "memory");
    Word absorbed = 0, folds = 0, heat = 0;
    material::Half product = 0, raw = 96;
    material::Response r;
    for (Word t = 0; t < 64; ++t) {
        Word axis = (t >> 1) & 1, direction = axis ? 2 : 1, other = fragment.neighbor[direction];
        Word coordinate = axis ? fragment.site / shape.side : fragment.site % shape.side;
        if (local < shape.size && other < 64 && ((coordinate ^ t) & 1) == 0) {
            auto &a = pieces[local], &b = pieces[other];
            material::conduct(a, b, direction, arm != 1);
            if (arm != 1 && ((t + 3 * fragment.site) & 31) == 0 && material::bonded(a, b, direction) &&
                a.charge + 1 >= material::capacity(a) && b.charge && material::exchange(a, b, direction)) {
                --b.charge;
                ++heat;
            }
        }
        asm volatile("bar.sync 0;" ::: "memory");
        if (local < shape.size) {
            // Conditions 0..3 are the four input combinations. Condition 4
            // probes delayed discharge, 5 and 6 exchange the two pulse orders.
            bool early = t < 16 && !(t & 1), later = t >= 16 && t < 32 && !(t & 1);
            bool A = kind <= 3 ? early && (kind & 1) : kind == 4 ? early : kind == 5 ? early : later;
            bool B = kind <= 3 ? early && (kind & 2) : kind == 4 ? early : kind == 5 ? later : early;
            Word lamp =
                ((local == shape.first && A) || (local == shape.second && B)) ? material::light_band(pieces[local]) : 4;
            // During a retention test the reagent first enters after both
            // light pulses have ended. The two order tests have equal inputs.
            bool expose = local == shape.sink && (kind < 4 || t >= 40);
            material::Half empty = 0, waste = 0;
            auto &source = expose ? raw : empty;
            auto &food = expose ? product : waste;
            auto &p = pieces[local];
            Word fixed = p.marks & 1;
            auto q = material::excite(p, lamp, material::catalytic_band(p), source, food);
            if (arm == 2)
                p.marks = static_cast<material::Byte>((p.marks & ~1u) | fixed);
            absorbed += q.light;
            folds += arm == 2 ? 0 : q.folds;
            if (q.reaction) {
                r.product += q.reaction;
                r.late += t >= 40;
                if (!r.first)
                    r.first = t + 1;
            }
        }
        asm volatile("bar.sync 0;" ::: "memory");
    }
    if (local == shape.sink)
        out[experiment] = r;
    asm volatile("bar.sync 0;" ::: "memory");
    census[local] = absorbed | (folds << 8) | (Word(pieces[local].charge) << 16) | (heat << 24);
    asm volatile("bar.sync 0;" ::: "memory");
    if (!local) {
        auto &v = out[experiment];
        for (Word i = 0; i < shape.size; ++i) {
            v.absorbed += census[i] & 255;
            v.redirections += (census[i] >> 8) & 255;
            v.stored += (census[i] >> 16) & 255;
            v.thermal += census[i] >> 24;
        }
    }
}
