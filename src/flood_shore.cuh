// A visiting teacher shares the pupil's view. It may point and supply sounds
// only after a new, naturally occurring encounter has reached this retina.
KERNEL void shore_lesson(World w, Configuration c, const Word *clock, Word offset, const shore::Syllabus *course,
                         shore::Pupil *classroom, Word size) {
    Word i = tid();
    if (i >= size)
        return;
    auto &p = classroom[i];
    const auto &a = w.animals[p.slot];
    Word stamp = *clock + offset + 1;
    if (!shore::present(p, a) || w.braids.episodes[p.slot].stamp != stamp)
        return;
    const auto &t = *course;
    const auto scene = w.braids.episodes[p.slot];
    auto &ledger = stamp >= t.reversal ? p.after_switch : p.book;
    auto &v = p.voice;
    const auto genes = w.braids.minds[p.slot].genes;
    ++ledger.scenes;
    braid::experience(v, scene);
    Word swap = stamp >= t.reversal, success[2]{};
    auto utterance = [&](Word f, Word site) {
        Word label = f ^ swap;
        auto words = t.form[label];
        words.wave[t.slot[label]] = t.names.wave[site];
        return words;
    };
    // Each probe sees the untouched start-of-encounter contacts and the same
    // public discourse. No probe response changes the next probe or lesson.
    auto topic = v.topic, prior = v.reply, pending = v.pending;
    for (Word f = 0; f < 2; ++f) {
        auto mask = braid::visible(scene, f), retina = braid::picture(scene, f);
        for (Word j = 0; j < 4; ++j)
            if (mask & (1u << j)) {
                auto reply = braid::question(v, utterance(f, j), false);
                v.topic = topic;
                v.reply = prior;
                v.pending = pending;
                ++ledger.tested;
                ledger.answered += reply.answered != 0;
                if (reply.answered && reply.words.wave[0] == t.names.wave[4 + braid::pigment(retina, j)]) {
                    ++ledger.correct;
                    success[f] |= 1u << j;
                }
            }
    }
    for (Word j = 0; j < 4; ++j)
        if ((braid::visible(scene, 0) & braid::visible(scene, 1) & (1u << j)) &&
            braid::pigment(braid::picture(scene, 0), j) != braid::pigment(braid::picture(scene, 1), j)) {
            ++ledger.changed;
            ledger.both += !!(success[0] & success[1] & (1u << j));
        }
    if (t.channel == shore::examination)
        return;
    Word key = mix(stamp ^ t.seed ^ mix(p.slot)), band = motor_band(w, c, p.slot, true);
    auto tissue = read_band(w, c.tiles, p.slot, band);
    auto organ = regulation::source(w.dna[p.slot], band, tissue);
    braid::ContactPractice regulator{organ, t.channel == shore::quiet ? apprentice::quiet : apprentice::inherited, 0, 1,
                                     nullptr};
    if (w.braids.mode & regulation::structural_port)
        regulator.mode |= apprentice::remodel;
    for (Word f = 0; f < 2; ++f) {
        auto mask = braid::visible(scene, f), retina = braid::picture(scene, f);
        if (!mask)
            continue;
        // Present genuine joint attention: the teacher both names the socket
        // and names the pigment actually seen there. Unseen dyes stay unnamed.
        for (Word z = 0; z < 4; ++z) {
            Word site = (z + key) & 3;
            if (!(mask & (1u << site)))
                continue;
            auto hue = braid::pigment(retina, site);
            ledger.changed_names += braid::notice(v, genes, t.names.wave[site], site, retina);
            ledger.changed_names += braid::notice(v, genes, t.names.wave[4 + hue], site, retina);
            ledger.pointings += 2;
        }
        Word j = (key >> (2 + 2 * f)) & 3;
        while (!(mask & (1u << j)))
            j = (j + 1) & 3;
        auto words = utterance(f, j);
        regulator.hear(v, genes, words, t.names.wave[4 + braid::pigment(retina, j)]);
        ++ledger.presented;
    }
    ledger.changed_forms += regulator.updates;
    ledger.attempts += regulator.attempts;
    ledger.agreements += regulator.matches;
    if (tissue.spike != regulator.source.cells.spike) {
        ledger.soma += nbits(tissue.spike ^ regulator.source.cells.spike);
        tissue.spike = regulator.source.cells.spike;
        assign_band(w, c.tiles, p.slot, band, tissue);
    }
}
