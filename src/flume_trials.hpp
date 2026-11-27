void flume_trials(std::istringstream &input) {
    Word cases = 0, seed = 0;
    std::string path, extra;
    if (!(input >> cases >> seed >> std::quoted(path)) || !cases || cases > 16384 || !at_flume || input >> extra)
        throw std::invalid_argument("use /flume-trials PAIRS SEED NEW_TSV in a flume");
    EvidenceFile file(path);
    std::array<runnel::Mind, 2> heads{local.reed.mind, partner().reed.mind};
    std::array<linkage::Tongue, 2> tongues{};
    Word organs = 0;
    for (Word i = 0; i < 2; ++i) {
        auto &p = i ? partner() : local;
        tongues[i] = {p.native_words, p.path_speech.words, p.calls.animal, p.referent.genes};
        auto body = p.resident.animal.body;
        organs |= (Word(!!body.eyes) | 2 * Word(!!body.limbs) | 4 * Word(!!body.pores)) << (3 * i);
    }
    s.d.check(s.d.upload(flume_voices.ptr, tongues.data(), sizeof(tongues)), "restore independent language systems");
    s.d.check(s.d.upload(reed_pair.ptr, heads.data(), sizeof(heads)), "isolate the two fold genotypes");
    runnel::Flume room{};
    s.d.check(s.d.download(&room, flume.ptr, sizeof(room)), "read laboratory geology");
    Word chiral = room.stone >> 31;
    Function talks{}, names{};
    s.bind(talks, "flume_debates", Image::siphons);
    s.bind(names, "flume_lexicon", Image::siphons);
    Temporary dictionary(s.d, sizeof(runnel::Lexicon));
    void *labels[]{&flume_voices.ptr, &dictionary.ptr};
    s.d.check(s.d.launch(names, 1, 1, 1, 1, 1, 1, 0, nullptr, labels, nullptr), "read public pronunciations");
    runnel::Lexicon vocabulary{};
    s.d.check(s.d.download(&vocabulary, dictionary.ptr, sizeof(vocabulary)), "read both sets of names");
    for (Word i = 0; i < 2; ++i)
        file.out << "# vocabulary " << i << '\t' << audible(vocabulary.names[i]) << '\t' << audible(vocabulary.verbs[i])
                 << '\n';
    file.out << "arm\tpair\tworld\ttarget\tlater\tcorner\treference\tdirection\tgeology\tframe\tpartner_"
                "frame\tasked\treported\tlearned\tpaid\tpose\tsuccess\tcarbon\tlater_paid\tlater_pose\tlater_"
                "success\tlater_carbon\tvia\ttransform\tlater_via\tlater_"
                "transform\torder\tfirst\tquery\treport\taction\twhy\tlater_action\n";
    std::vector<runnel::Colloquy> rows(2 * cases);
    Temporary result(s.d, rows.size() * sizeof(rows[0]));
    const char *conditions[]{"heard",         "direct_question",    "muted",      "false_report", "missing_row",
                             "missing_pivot", "uninformed_partner", "muted_motor"};
    double seconds = 0;
    for (Word arm = 0; arm < 8; ++arm) {
        auto start = std::chrono::steady_clock::now();
        void *params[]{&reed_pair.ptr, &flume_voices.ptr, &cases, &seed, &arm, &chiral, &organs, &result.ptr};
        s.d.check(s.d.launch(talks, (2 * cases + 63) / 64, 1, 1, 64, 1, 1, 0, nullptr, params, nullptr),
                  "conduct separated-reference conversations");
        s.d.check(s.d.download(rows.data(), result.ptr, rows.size() * sizeof(rows[0])), "audit all conversations");
        seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        Count tuition = 0;
        Count both = 0, success = 0, indirect = 0, retold = 0, transfer = 0, switched = 0, muted = 0, carbon = 0,
              repairs = 0, changed = 0;
        for (Word j = 0; j < 2 * cases; ++j) {
            auto &v = rows[j];
            auto condition = conditions[arm];
            file.out << condition << '\t' << j / 2 << '\t' << (j & 1) << '\t' << v.opening << '\t' << v.other << '\t'
                     << v.corner << '\t' << v.reference << '\t' << v.direction << '\t' << v.geology << '\t' << v.frame
                     << '\t' << v.other_frame << '\t' << v.consultations << '\t' << v.said << '\t' << v.learned << '\t'
                     << v.paid << '\t' << v.public_pose << '\t' << v.succeeded << '\t' << v.food << '\t' << v.later_paid
                     << '\t' << v.later_pose << '\t' << v.later_success << '\t' << v.later_food << '\t' << v.reason_via
                     << '\t' << v.rotation << '\t' << v.later_via << '\t' << v.later_rotation;
            for (auto *wave : {&v.order, &v.first, &v.enquiry, &v.testimony, &v.procedure, &v.reason, &v.later})
                file.out << '\t' << audible(*wave);
            file.out << '\n';
            success += v.succeeded;
            retold += v.learned;
            indirect += v.succeeded && v.reason_via == v.corner && v.heard;
            transfer += v.later_success;
            switched += v.different;
            if (v.tuition != 32)
                throw std::logic_error("unfunded reference contact");
            tuition += v.tuition;
            carbon += v.food + v.later_food;
            repairs += v.repaired;
            muted += v.later_ask;
            if ((j & 1) && v.succeeded && rows[j - 1].succeeded) {
                ++both;
                changed += v.public_pose != rows[j - 1].public_pose;
            }
            if (j < 2 && arm == 0)
                std::cout << "flume_example=" << j << "  target=" << v.opening << "  corner=" << v.corner
                          << "  reference=" << v.reference << "  first_pose=" << v.public_pose << "  later=" << v.other
                          << "  later_pose=" << v.later_pose << "  direction=" << v.direction
                          << "  reply=" << std::quoted(audible(v.testimony))
                          << "  procedure=" << std::quoted(audible(v.procedure)) << '\n';
        }
        std::cout << "flume_dialogue=" << conditions[arm] << "  pairs=" << cases << "  handed=" << chiral
                  << "  funded_reference_work=" << tuition << "  target_demonstrations=0  exact=" << success
                  << "  both=" << both << "  different=" << changed << "  indirect=" << indirect
                  << "  incorporated=" << retold << "  later_exact=" << transfer << "  later_referrals=" << muted
                  << "  new_motor=" << switched << "  reformulated=" << repairs << "  carbon=" << carbon << '\n';
    }
    file.publish();
    std::cout << "runnel_timing=spoken  trials=" << 16 * cases << "  seconds=" << seconds << '\n';
}
