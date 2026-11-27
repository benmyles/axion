bool import_whole_neural(const Tissue *source, const Germ *genomes) {
    if (neural_layout.initial.empty())
        return false;
    std::vector<Word> map(neural_layout.logical);
    s.d.check(s.d.download(map.data(), nerve_index.ptr, Count(map.size()) * sizeof(Word)),
              "inspect the startup neural directory");
    bool new_directory = map == neural_layout.initial;
    std::vector<Word>().swap(neural_layout.initial);
    if (!new_directory)
        return false;
    std::vector<Tissue> compact(Count(n) * basal_bands + Count(neural_layout.hot) * nerve_page_slots);
    std::vector<Word> masks(n);
    std::vector<Byte> origins(n);
    for (Word slot = 0; slot < n; ++slot) {
        const Tissue *row = source + Count(slot) * tiles;
        auto shape = describe_neural_row(row, tiles, genomes[slot].brain.development);
        if (shape.germline) {
            origins[slot] = shape.extent;
            continue;
        }
        masks[slot] = shape.mask;
        std::copy_n(row, basal_bands, compact.data() + Count(slot) * basal_bands);
        for (Word band = basal_bands; band < tiles; ++band) {
            const auto &v = row[band];
            if (!(v.spike | v.memory | v.gate_a | v.gate_b))
                continue;
            Word entry = map[(band - basal_bands) * neural_layout.groups + slot / nerve_page_slots];
            if (!entry)
                return false;
            compact[Count(n) * basal_bands + Count(entry - 1) * nerve_page_slots + slot % nerve_page_slots] = v;
        }
    }
    s.d.check(s.d.upload(nerve_origin.ptr, origins.data(), n), "restore neuron development keys");
    s.d.check(s.d.upload(nerve_overrides.ptr, masks.data(), Count(n) * sizeof(Word)), "restore learned neural bands");
    s.d.check(s.d.upload(nerves.ptr, compact.data(), compact.size() * sizeof(Tissue)),
              "restore the represented neural bands");
    return true;
}
void export_whole_neural(Tissue *target, const Germ *genomes) {
    std::vector<Word> directory(neural_layout.logical);
    s.d.check(s.d.download(directory.data(), nerve_index.ptr, directory.size() * sizeof(Word)),
              "read the neural page directory");
    std::vector<Tissue> compact(neural_layout.tissue_bytes() / sizeof(Tissue));
    s.d.check(s.d.download(compact.data(), nerves.ptr, neural_layout.tissue_bytes()), "read the resident brain pages");
    std::vector<Word> masks(n);
    std::vector<Byte> origins(n);
    s.d.check(s.d.download(masks.data(), nerve_overrides.ptr, Count(n) * sizeof(Word)), "read acquired neural bands");
    s.d.check(s.d.download(origins.data(), nerve_origin.ptr, n), "read unvisited developmental rows");
    std::fill_n(target, Count(n) * tiles, Tissue{});
    for (Word slot = 0; slot < n; ++slot) {
        auto row = target + Count(slot) * tiles;
        for (Word j = 0; j < origins[slot]; ++j)
            if (!(masks[slot] & (1u << j)))
                row[j] = developmental_band(genomes[slot].brain.development, j);
        for (Word j = 0; j < basal_bands; ++j)
            if (masks[slot] & (1u << j))
                row[j] = compact[Count(slot) * basal_bands + j];
    }
    auto expand = [&](Word key, const Tissue *values) {
        Word band = basal_bands + key / neural_layout.groups;
        Word start = (key % neural_layout.groups) * nerve_page_slots, length = std::min(nerve_page_slots, n - start);
        for (Word j = 0; j < length; ++j)
            if (masks[start + j] & (1u << band))
                target[Count(start + j) * tiles + band] = values[j];
    };
    std::vector<Word> spilled;
    for (Word key = 0; key < directory.size(); ++key) {
        Word page = directory[key];
        if (page == spill_page)
            spilled.push_back(key);
        else if (page)
            expand(key, compact.data() + Count(n) * basal_bands + Count(page - 1) * nerve_page_slots);
    }
    if (spilled.empty())
        return;
    // Read spills with a GPU gather so a checkpoint does not pull sparse
    // working pages back to the CPU through the operating system.
    Word batch = std::min<Word>(spilled.size(), 512);
    Temporary keys(s.d, batch * sizeof(Word)), gather(s.d, Count(batch) * nerve_page_slots * sizeof(Tissue));
    std::vector<Tissue> values(Count(batch) * nerve_page_slots);
    for (Word i = 0; i < spilled.size(); i += batch) {
        Word items = std::min<Word>(batch, spilled.size() - i), count = items * nerve_page_slots;
        s.d.check(s.d.upload(keys.ptr, spilled.data() + i, items * sizeof(Word)), "select occupied spill pages");
        launch(gather_spill, (count + 127) / 128, 128, world.neural_pages, keys.ptr, count, gather.ptr);
        synchronize();
        s.d.check(s.d.download(values.data(), gather.ptr, Count(count) * sizeof(Tissue)),
                  "read the sparse neural changes");
        for (Word j = 0; j < items; ++j)
            expand(spilled[i + j], values.data() + j * nerve_page_slots);
    }
}
void move_neural(const Tissue *input, Tissue *output, Count offset, Count length, const Germ *genomes = nullptr) {
    if (!length)
        return;
    if (!neural_layout.paged) {
        auto address = nerves.ptr + offset * sizeof(Tissue);
        s.d.check(input ? s.d.upload(address, input, length * sizeof(Tissue))
                        : s.d.download(output, address, length * sizeof(Tissue)),
                  "transfer resident neural tissue");
        return;
    }
    if (genomes && !offset && length == Count(n) * tiles &&
        neural_layout.tissue_bytes() < Count(n) * tiles * sizeof(Tissue) * 3 / 4) {
        if (input && import_whole_neural(input, genomes))
            return;
        if (output) {
            export_whole_neural(output, genomes);
            return;
        }
    }
    // Bound temporary VRAM independently of the habitat size. The transfer
    // format stays a contiguous stream of canonical resident bands.
    Word batch = std::min<Count>(65536, length);
    Temporary buffer(s.d, Count(batch) * sizeof(Tissue));
    for (Count i = 0; i < length; i += batch) {
        Word count = std::min<Count>(batch, length - i);
        auto bytes = Count(count) * sizeof(Tissue);
        if (input)
            s.d.check(s.d.upload(buffer.ptr, input + i, bytes), "stage neural input");
        Count address = offset + i;
        launch(input ? import_nerve : export_nerve, (count + 127) / 128, 128, world, tiles, address, count, buffer.ptr);
        synchronize();
        if (output)
            s.d.check(s.d.download(output + i, buffer.ptr, bytes), "read logical neural bands");
    }
}
void clone_neural(DevicePtr target) {
    if (!neural_layout.paged) {
        s.d.check(s.d.copy_device(target, nerves.ptr, Count(n) * tiles * sizeof(Tissue)),
                  "restore neural benchmark state");
        return;
    }
    Word total = n * tiles;
    Count begin = 0;
    launch(export_nerve, (total + 127) / 128, 128, world, tiles, begin, total, target);
    synchronize();
}
void report_neural_memory() {
    const size_t host_page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    Count occupied = 0, implicit = 0, virgin = 0;
    Word cache_used = 0, spills = 0, host_pages = 0;
    if (neural_layout.paged) {
        std::vector<Word> map(neural_layout.logical);
        s.d.check(s.d.download(map.data(), nerve_index.ptr, map.size() * sizeof(Word)),
                  "read actual physical page use");
        std::vector<Word> masks(n);
        std::vector<Byte> widths(n);
        s.d.check(s.d.download(masks.data(), nerve_overrides.ptr, Count(n) * sizeof(Word)),
                  "count explicit neural activity");
        s.d.check(s.d.download(widths.data(), nerve_origin.ptr, n), "count developmental neural images");
        for (Word i = 0; i < n; ++i)
            if (widths[i]) {
                Word reserved = ~0u >> (32 - widths[i]);
                implicit += std::popcount(reserved & ~masks[i]);
                virgin += !masks[i];
            }
        Word previous = ~0u;
        for (Word i = 0; i < map.size(); ++i) {
            if (!map[i])
                continue;
            ++occupied;
            if (map[i] == spill_page) {
                ++spills;
                Word os_page = Count(i) * nerve_page_slots * sizeof(Tissue) / host_page_size;
                if (previous != os_page) {
                    ++host_pages;
                    previous = os_page;
                }
            } else
                cache_used += map[i] > neural_layout.hot;
        }
    }
    std::cout << "neural_storage=" << (neural_layout.paged ? "paged" : "dense") << "  sites=" << n
              << "  capacity_bands=" << tiles << "  dense_bytes=" << Count(n) * tiles * sizeof(Tissue)
              << "  device_bytes=" << neural_layout.tissue_bytes() + neural_layout.metadata_bytes()
              << "  directory_bytes=" << neural_layout.metadata_bytes()
              << "  basal_bands=" << (neural_layout.paged ? basal_bands : tiles) << "  page_slots=" << nerve_page_slots
              << "  initial_pages=" << (neural_layout.paged ? neural_layout.hot : 0)
              << "  spare_pages=" << (neural_layout.paged ? neural_layout.reserve : 0) << "  spare_used=" << cache_used
              << "  populated_pages=" << occupied << "  spill_pages=" << spills << "  spill_host_pages=" << host_pages
              << "  host_page_bytes=" << host_page_size << "  collections=" << collections << "  repacked=" << repacked
              << "  reclaimed_pages=" << retired_pages << "  discarded_host_pages=" << discarded_pages
              << "  germline_rows=" << virgin << "  implicit_bands=" << implicit
              << "  spill_address_bytes=" << neural_layout.virtual_bytes() << '\n';
}
void collect_after(Word elapsed) {
    if (!collection_period)
        return;
    collection_age += elapsed;
    if (collection_age < collection_period)
        return;
    collection_age %= collection_period;
    ++collections;
    Word logical = neural_layout.logical;
    size_t directory_bytes = Count(logical) * sizeof(Word);
    Temporary directory(s.d, directory_bytes);
    launch(inventory_nerve, (logical * 32 + 127) / 128, 128, world, logical, directory.ptr);
    synchronize();
    std::vector<Word> old(logical), next(logical), keys, claims(neural_layout.reserve);
    s.d.check(s.d.download(old.data(), nerve_index.ptr, directory_bytes), "inspect neural page ownership");
    s.d.check(s.d.download(next.data(), directory.ptr, directory_bytes), "inspect retained neural bands");
    Word filled = 0;
    const Word os_page = static_cast<Word>(sysconf(_SC_PAGESIZE));
    const size_t span = nerve_page_slots * sizeof(Tissue), length = neural_layout.virtual_bytes();
    std::vector<unsigned char> spill_state((length + os_page - 1) / os_page);
    for (Word key = 0; key < logical; ++key) {
        Word page = Count(key) * span / os_page;
        if (old[key] == spill_page)
            spill_state[page] |= 1;
        if (!next[key]) {
            retired_pages += old[key] != 0;
            continue;
        }
        keys.push_back(key);
        if (filled < neural_layout.hot) {
            next[key] = ++filled;
            continue;
        }
        next[key] = spill_page;
        if (neural_layout.reserve) {
            Word first = mix(key) % neural_layout.reserve;
            for (Word j = 0; j < std::min(4u, neural_layout.reserve); ++j) {
                Word bucket = first + j;
                if (bucket >= claims.size())
                    bucket -= claims.size();
                if (!claims[bucket]) {
                    claims[bucket] = key + 1;
                    next[key] = neural_layout.hot + bucket + 1;
                    break;
                }
            }
        }
        if (next[key] == spill_page)
            spill_state[page] |= 2;
    }
    if (old == next)
        return;
    size_t tail = Count(neural_layout.hot + neural_layout.reserve) * span;
    Temporary body(s.d, tail), live(s.d, keys.size() * sizeof(Word));
    if (tail)
        s.d.check(s.d.zero(body.ptr, 0, tail), "clear reclaimed neural bands");
    s.d.check(s.d.upload(directory.ptr, next.data(), directory_bytes), "stage compacted neural addresses");
    if (!keys.empty()) {
        s.d.check(s.d.upload(live.ptr, keys.data(), keys.size() * sizeof(Word)), "stage living neural pages");
        Word total = keys.size() * nerve_page_slots;
        launch(repack_nerve, (total + 127) / 128, 128, world, live.ptr, directory.ptr, total, body.ptr);
        synchronize();
    }
    if (tail)
        s.d.check(s.d.copy_device(nerves.ptr + Count(n) * basal_bands * sizeof(Tissue), body.ptr, tail),
                  "repack the existing neural arena");
    s.d.check(s.d.copy_device(nerve_index.ptr, directory.ptr, directory_bytes), "publish compacted neural addresses");
    if (!claims.empty())
        s.d.check(s.d.upload(nerve_claims.ptr, claims.data(), claims.size() * sizeof(Word)),
                  "release empty neural page claims");
    ++repacked;
    // Linux can release complete unused system pages after the device has
    // stopped accessing them. Logical state is already correct if it declines.
    for (size_t at = 0; at < spill_state.size();) {
        if (spill_state[at] != 1) {
            ++at;
            continue;
        }
        auto begin = at++;
        while (at < spill_state.size() && spill_state[at] == 1)
            ++at;
        auto *address = reinterpret_cast<unsigned char *>(neural_spill.data) + begin * os_page;
        if (!madvise(address, (at - begin) * os_page, MADV_DONTNEED))
            discarded_pages += at - begin;
    }
}
