#include "axion/mill.hpp"
// Two discordant seams express two independent hidden polarities. Every
// generation hatches fresh and gets one rewarded work district. A juvenile
// can retain the other district only through the witness across the partition.
namespace axion::life::material {
AX_FN Rack seam(Word key, Word place, Word convention, Word span) {
    auto r = rack(key, span);
    r.bevel = static_cast<Byte>(place);
    roughen(r, 1 + ((convention >> place) & 1));
    return r;
}
AX_FN void local_evidence(Worker &p, Word wire, Word start, Word place, Word law, Word visits) {
    for (Word j = 0; j < visits; ++j) {
        Word key = mix(start + j * 7937);
        auto field = seam(key, place, law, 2);
        p.hand = {};
        Word motor = choose(p, cue(field, p), (key >> 15) & 7, key);
        if ((key & 3) == 1)
            motor = (key >> 9) & 7;
        auto event = adjust(field, p, motor, wire, 64, true);
        auto ready = run_rack(field, 48, 12);
        if (ready.product && (p.genes.observation & (1u << 17)))
            witness(p, event.cue, event.change);
    }
}
} // namespace axion::life::material
KERNEL void flood_guild(const material::Genes *donors, Word pool, material::Tradition *rows, Word n, Word seed,
                        Word generations, Word exposures, Word support, Word isolation) {
    Word i = tid();
    if (i >= n)
        return;
    Word base = mix(seed + 14281 * i), law = mix(base + 9371) & 3;
    material::Worker tutor{};
    tutor.genes = donors[mix(base + 919) % pool];
    Word parent = material::wire(tutor.genes, material::permutation(mix(base + 711)));
    material::play(tutor, parent, mix(base + 317), 128);
    material::local_evidence(tutor, parent, mix(base + 131), 0, law, support);
    for (Word generation = 0; generation < generations; ++generation) {
        auto key = mix(base + 23743 * generation + 9997);
        material::Worker student{};
        student.genes = donors[mix(key + 139) % pool];
        Word muscle = material::wire(student.genes, material::permutation(mix(key + 971)));
        material::play(student, muscle, mix(key + 911), 128);
        material::Tradition record;
        for (Word j = 0; j < exposures; ++j) {
            Word salt = mix(key + 193 * j), place = j & 1;
            auto assembly = material::seam(salt, place, law, 2), initial = assembly;
            tutor.hand = {};
            auto event = material::adjust(assembly, tutor,
                                          material::choose(tutor, material::cue(initial, tutor), (salt >> 8) & 7, salt),
                                          parent, 64, false);
            auto flux = material::run_rack(assembly, 48, 12);
            ++record.opportunities;
            if (flux.product) {
                record.signals += material::observe(student, initial, event.change, isolation, salt);
            }
        }
        // Independently practiced site alternates. Pupils are never given
        // the target contact, the teacher's efference, or an aperture label.
        Word personal = 1 - (generation & 1);
        material::local_evidence(student, muscle, key + 1311, personal, law, support);
        for (Word pair = 0; pair < 16; ++pair) {
            bool both = true;
            for (Word d = 0; d < 2; ++d) {
                Word salt = mix(key + pair * 7177 + d * 41 + 9571);
                auto bench = material::seam(salt, d, law, 4);
                auto probe = student;
                probe.hand = {};
                Word choice = material::choose(probe, material::cue(bench, probe), (salt >> 14) & 7, salt);
                material::adjust(bench, probe, choice, muscle, 64, false);
                bool works = material::run_rack(bench, 72, 12).product > 0;
                if (d == personal)
                    record.acquired += works;
                else
                    record.ancestral += works;
                both &= works;
            }
            record.joint += both;
        }
        record.inducted = record.joint >= 12;
        rows[generation * n + i] = record;
        tutor = student;
        parent = muscle;
    }
}
