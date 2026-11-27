// The field owns the current episode and body. A classroom can return a change
// to durable speech contacts using the same original animal as its baseline.
constexpr size_t voice_contact_bytes = offsetof(braid::Narration, event);

bool same_field_individual(const Habitat &h, const storage::AgentImage &image) {
    Word slot = image.resident.slot;
    if (slot >= h.sites())
        return false;
    const auto &source = image.resident.animal;
    const auto &live = h.bodies[slot];
    return live.energy && live.born == source.born && live.founder == source.founder &&
           live.body_origin == source.body_origin && live.brain_origin == source.brain_origin &&
           !std::memcmp(&image.resident.germ, &h.genomes[slot], sizeof(Germ));
}
Word identify_voice(const Habitat &h, const storage::AgentImage &image) {
    if (!image.riverine || !image.binds_floats || image.brain.seed != h.header.configuration.seed ||
        image.resident.tick > h.header.tick || image.resident.slot >= h.sites())
        throw std::invalid_argument("the witness does not belong to this watershed and time");
    if (!same_field_individual(h, image))
        throw std::invalid_argument("the original witness is no longer living in this slot");
    return image.resident.slot;
}

void return_field_lesson(Habitat &h, const std::string &original_path, const std::string &taught_path, bool hand) {
    if (!h.recording_floats)
        throw std::invalid_argument("returning a field lesson needs a habitat with visual episodes");
    storage::AgentImage original, taught;
    original.read(original_path);
    taught.read(taught_path);
    const Word slot = identify_voice(h, original);
    if (identify_voice(h, taught) != slot || (!taught.narrates_floats && !taught.describes && !taught.hydraulic) ||
        std::memcmp(&h.riggers[slot].genes, &original.braided.mind.genes, sizeof(braid::Genes)) ||
        std::memcmp(&original.braided.mind.genes, &taught.braided.mind.genes, sizeof(braid::Genes)))
        throw std::invalid_argument("the lesson must come from this witness and its own learning circuit");
    const auto &initial = original.braided.mind;
    const auto &hand_memory = taught.braided.mind;
    auto &field = h.riggers[slot];
    auto same_hand = [&](const braid::Mind &other) {
        return !((field.lived ^ other.lived) & 15) &&
               !std::memcmp(field.pruned, other.pruned, braid::motors * sizeof(braid::Half));
    };
    if (hand && !same_hand(hand_memory) && !same_hand(initial))
        throw std::invalid_argument("the witness acquired another motor memory since the supplied baseline");
    const auto &before = original.braid_report.memory;
    const auto &after = taught.braid_report.memory;
    auto found = std::ranges::find(h.narrators, slot, &Habitat::ResidentNarration::slot);
    braid::Narration local = found == h.narrators.end() ? braid::Narration{} : found->mind;
    bool current = !std::memcmp(&local, &after, voice_contact_bytes);
    if (!current && std::memcmp(&local, &before, voice_contact_bytes))
        throw std::invalid_argument("the witness learned another language change since the supplied baseline");
    if (taught.describes &&
        std::memcmp(&original.accounts.memory, &taught.accounts.memory, sizeof(utterance::Memory))) {
        auto story = std::ranges::find(h.stories, slot, &Habitat::Storyteller::slot);
        const utterance::Memory empty{};
        const auto &old = story == h.stories.end() ? empty : story->memory;
        if (std::memcmp(&old, &original.accounts.memory, sizeof(old)) &&
            std::memcmp(&old, &taught.accounts.memory, sizeof(old)))
            throw std::invalid_argument("this witness acquired different speech projections in the field");
        if (story == h.stories.end())
            h.stories.push_back({slot, h.bodies[slot].born, taught.accounts.memory});
        else
            story->memory = taught.accounts.memory;
        h.loom.version = std::max(11u, h.loom.version);
        if (h.trails.empty())
            h.trails.resize(h.sites());
        std::cout << "field_narration=" << slot << "  born=" << h.bodies[slot].born
                  << "  retained_events=" << h.trails[slot].size()
                  << "  first_prompt=" << taught.accounts.memory.script[0].prompt << '\n';
    }
    if (taught.hydraulic) {
        if (h.loom.version < 14) {
            h.cap_law.raw_frame = h.loom.version == 10 || h.loom.version == 11;
            h.loom.version = 14;
            if (h.trails.empty())
                h.trails.resize(h.sites());
        }
        if (h.cap_memories.empty())
            h.cap_memories.resize(h.sites());
        auto &mine = h.cap_memories[slot].tissue;
        if (std::memcmp(&mine, &original.channels.mind.tissue, sizeof(mine)) &&
            std::memcmp(&mine, &taught.channels.mind.tissue, sizeof(mine)))
            throw std::invalid_argument(
                "the resident acquired a different capillary response since the supplied baseline");
        auto current = std::ranges::find(h.cap_speakers, slot, &Habitat::CapillarySpeaker::slot);
        const capillary::Mouth blank{};
        const auto &old = current == h.cap_speakers.end() ? blank : current->speech;
        if (std::memcmp(&old, &original.channels.speech, sizeof(old)) &&
            std::memcmp(&old, &taught.channels.speech, sizeof(old)))
            throw std::invalid_argument("the resident acquired another vocabulary since the supplied baseline");
        if (taught.voices_signs) {
            h.loom.version = std::max(23u, h.loom.version);
            auto prior = std::ranges::find(h.audible_residents, slot, &Habitat::AudibleResident::slot);
            capillary::SignalVoice blank{};
            auto &current = prior == h.audible_residents.end() ? blank : prior->speech;
            if (std::memcmp(&current, &original.signal_voice, sizeof(current)) &&
                std::memcmp(&current, &taught.signal_voice, sizeof(current)))
                throw std::invalid_argument("the resident acquired a different acoustic signalling vocabulary");
            if (prior == h.audible_residents.end())
                h.audible_residents.push_back({slot, h.bodies[slot].born, taught.signal_voice});
            else
                prior->speech = taught.signal_voice;
        }
        if (taught.compound_signals) {
            h.loom.version = std::max(21u, h.loom.version);
            auto prior = std::ranges::find(h.compound_residents, slot, &Habitat::CompoundResident::slot);
            Word current = prior == h.compound_residents.end() ? 0 : prior->signs;
            if (current != original.compound_contacts && current != taught.compound_contacts)
                throw std::invalid_argument("the resident acquired different compound contacts");
            if (prior == h.compound_residents.end())
                h.compound_residents.push_back({slot, h.bodies[slot].born, taught.compound_contacts});
            else
                prior->signs = taught.compound_contacts;
        }
        if (taught.material_signals) {
            h.loom.version = std::max(20u, h.loom.version);
            if (h.appetites.empty())
                h.appetites.resize(h.sites());
            auto prior = capillary::pack_signs(original.appetitive, h.genomes[slot].pigments[2]),
                 next = capillary::pack_signs(taught.appetitive, h.genomes[slot].pigments[2]);
            if (h.appetites[slot] != prior && h.appetites[slot] != next)
                throw std::invalid_argument(
                    "the resident acquired different appetitive contacts since the supplied lesson");
            h.appetites[slot] = next;
        }
        if (taught.channels.adaptive) {
            h.loom.version = std::max(taught.channels.adaptive == 3 ? 18u : 17u, h.loom.version);
            if (h.cap_histories.empty())
                h.cap_histories.resize(h.sites());
            auto &context = h.cap_histories[slot];
            if (std::memcmp(&context, &original.channels.history, sizeof(context)) &&
                std::memcmp(&context, &taught.channels.history, sizeof(context)))
                throw std::invalid_argument(
                    "the resident has acquired different material-context evidence since its lesson");
            context = taught.channels.history;
        }
        mine = taught.channels.mind.tissue;
        if (current != h.cap_speakers.end())
            current->speech = taught.channels.speech;
        else if (capillary::schooled(taught.channels.speech))
            h.cap_speakers.push_back({slot, h.bodies[slot].born, taught.channels.speech});
        std::cout << "capillary_home=" << slot << "  born=" << h.bodies[slot].born << "  on=" << mine.on
                  << "  off=" << mine.off << "  retained=" << h.cap_memories[slot].after << '\n';
    }
    auto &ear = h.ears[slot];
    Word auditory_slots = 0, auditory_edits = 0;
    for (Word i = 0; i < 4; ++i) {
        if (braid::contact(after, i).panel != audience::report_contact)
            continue;
        Word k = 4 + i;
        const auto &old = original.resident.ear, &next = taught.resident.ear;
        if (old.low[k] == next.low[k] && old.high[k] == next.high[k])
            continue;
        bool same = ear.low[k] == old.low[k] && ear.high[k] == old.high[k];
        bool applied = ear.low[k] == next.low[k] && ear.high[k] == next.high[k];
        if (!same && !applied)
            throw std::invalid_argument("the field's acoustic contacts changed since the supplied lesson");
        auditory_slots |= 1u << k;
    }
    for (Word k = 4; k < 8; ++k)
        if (auditory_slots & (1u << k)) {
            auditory_edits += std::popcount(ear.low[k] ^ taught.resident.ear.low[k]) +
                              std::popcount(ear.high[k] ^ taught.resident.ear.high[k]);
            ear.low[k] = taught.resident.ear.low[k];
            ear.high[k] = taught.resident.ear.high[k];
        }
    if (auditory_slots)
        std::cout << "field_acoustic_alignment=" << slot << "  records=" << std::popcount(auditory_slots)
                  << "  edits=" << auditory_edits << "  field_stamp=" << h.float_events[slot].stamp << '\n';
    Word changed = 0;
    const auto *old = reinterpret_cast<const unsigned char *>(&local);
    const auto *learned = reinterpret_cast<const unsigned char *>(&after);
    for (size_t i = 0; i < voice_contact_bytes; ++i)
        changed += std::popcount(unsigned(old[i] ^ learned[i]));
    if (changed) {
        std::copy_n(learned, voice_contact_bytes, reinterpret_cast<unsigned char *>(&local));
        // The living sight and the current conversation stay in their original records.
        // Extraction overlays the newest real sight, as it does for any resident.
        if (found == h.narrators.end())
            h.narrators.push_back({slot, h.bodies[slot].born, local});
        else
            found->mind = local;
    }
    Word nouns = 0, forms = 0;
    for (Word i = 0; i < braid::sounds_max; ++i)
        nouns += braid::ident(local, i) != braid::none;
    for (const auto &form : local.forms)
        forms += form.examples != 0 && !braid::sequence_form(form);
    if (hand) {
        Word changed = 0;
        for (Word j = 0; j < braid::motors; ++j) {
            changed += std::popcount(Word(field.pruned[j] ^ hand_memory.pruned[j]));
            field.pruned[j] = hand_memory.pruned[j];
        }
        field.lived = (field.lived & ~15u) | (hand_memory.lived & 15);
        std::cout << "field_motor_lesson=" << slot << "  tick=" << h.header.tick << "  changed_bits=" << changed
                  << "  experienced_ports=" << std::popcount(Word(field.lived & 15))
                  << "  stamp=" << h.float_events[slot].stamp << '\n';
    }
    std::cout << "field_lesson=" << slot << "  tick=" << h.header.tick << "  born=" << h.bodies[slot].born
              << "  source_tick=" << original.resident.tick << "  changed_bits=" << changed << "  nouns=" << nouns
              << "  forms=" << forms << "  field_stamp=" << h.float_events[slot].stamp
              << "  images=" << h.float_events[slot].images << "  known=" << h.float_events[slot].known
              << "  lesson_stamp=" << after.event.stamp << '\n';
}
