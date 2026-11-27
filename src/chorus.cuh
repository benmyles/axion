namespace axion::life::utterance {
AX_FN Song accent_from(Genes first, AccentCourse course, Word branch, Word image) {
    auto line = call(branch && course.neighbor ? course.alternate : first, image);
    if (branch && !course.neighbor)
        for (Word j = 0; j < line.n; ++j) {
            Word note = (line.calls >> (4 * j)) & 3;
            line.calls ^= (note ^ ((note + 1) & 3)) << (4 * j);
        }
    return line;
}
AX_FN bool same_song(Song a, Song b) {
    return a.n == b.n && a.calls == b.calls;
}
AX_FN Word catches(const Receptors &ear, Song noise, Word physical, braid::Optics glass) {
    Word votes = evoke(ear, noise, false);
    Word a = braid::pigment(glass.places, physical & 3), b = braid::pigment(glass.places, (physical >> 2) & 3);
    Word u = braid::pigment(glass.hues, (physical >> 4) & 3), v = braid::pigment(glass.hues, physical >> 6);
    return Word(located(votes, 0) == a && located(votes, 2) == u) +
           Word(located(votes, 1) == b && located(votes, 3) == v);
}
KERNEL void narrative_accents(Genes founder, Senses pupil, Senses origin, Course c, AccentCourse series, Word n,
                              Accent *out) {
    Word family = tid();
    if (family >= n)
        return;
    Word group = mix(c.seed ^ mix(family * 37561 + 41281));
    Receptors previous[2]{}, receiver[2]{};
    for (Word link = 0; link < series.links; ++link) {
        Receptors young[2]{};
        Accent r{};
        for (Word j = 0; j < series.practice; ++j) {
            Word stream = mix(group ^ mix(link * 8719 + j * 15737 + 39419));
            auto view = stage(pupil, c, stream, 1, false, &origin);
            if (!view.look.size() || !view.peer.size())
                continue;
            Word image = view.look.at(0), paired = image;
            if (series.arm == 1) {
                Word partner = (j ^ 1u) < series.practice ? j ^ 1u : j;
                Word alternate = mix(group ^ mix(link * 8719 + partner * 15737 + 39419));
                auto other = stage(pupil, c, alternate, 1, false);
                paired = other.look.size() ? other.look.at(0) : unseen;
            }
            for (Word dialect = 0; dialect < 2; ++dialect) {
                Song heard = link ? call(pupil.voice, image) : accent_from(founder, series, dialect, view.peer.at(0));
                if (link && series.arm != 2)
                    heard = imitate(previous[dialect], pupil.echo, heard, image);
                if (!link)
                    associate(receiver[dialect], heard, image, pupil.learning.law);
                r.plastic_edits += associate(young[dialect], heard, paired, pupil.learning.law);
                r.wave_count += heard.n;
                r.heat += 2 + 2 * heard.n;
            }
        }
        Word previous_scene = unseen;
        Song previous_calls[2]{};
        for (Word j = 0; j < feeds; ++j) {
            if (!(j & 1))
                previous_scene = unseen;
            auto scene = stage(pupil, c, mix(group ^ mix(j * 13381 + 86491)), 1, true, &origin);
            if (!scene.look.size() || !scene.peer.size())
                continue;
            Word image = scene.look.at(0);
            Song calls[2]{}, model[2]{};
            for (Word dialect = 0; dialect < 2; ++dialect) {
                model[dialect] = accent_from(founder, series, dialect, scene.peer.at(0));
                auto native = call(pupil.voice, image);
                calls[dialect] = series.arm == 2 ? native : imitate(young[dialect], pupil.echo, native, image);
                r.edited_notes += changed_pitches(native, calls[dialect]);
                r.within += catches(receiver[dialect], calls[dialect], scene.truth.at(0), pupil.glass);
                r.across += catches(receiver[1 - dialect], calls[dialect], scene.truth.at(0), pupil.glass);
            }
            Word physical = scene.truth.at(0);
            if (!(j & 1)) {
                previous_scene = physical;
                previous_calls[0] = calls[0];
                previous_calls[1] = calls[1];
            } else if (previous_scene != unseen) {
                for (Word d = 0; d < 2; ++d) {
                    r.paired_food += catches(receiver[d], previous_calls[d], previous_scene, pupil.glass) +
                                     catches(receiver[d], calls[d], physical, pupil.glass);
                    r.swapped_food += catches(receiver[d], previous_calls[d], physical, pupil.glass) +
                                      catches(receiver[d], calls[d], previous_scene, pupil.glass);
                    r.changed_with_scene += !same_song(previous_calls[d], calls[d]);
                }
                r.paired_portions += 8;
            }
            ++r.images;
            r.original_difference += !same_song(model[0], model[1]);
            r.different += !same_song(calls[0], calls[1]);
            r.taught_a += calls[0].n && same_song(calls[0], model[0]);
            r.taught_b += calls[1].n && same_song(calls[1], model[1]);
        }
        out[link * n + family] = r;
        previous[0] = young[0];
        previous[1] = young[1];
    }
}
} // namespace axion::life::utterance
