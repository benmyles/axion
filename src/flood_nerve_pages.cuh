// A logical page contains one neural band for 64 consecutive slot IDs. The
// basal bands are dense. Unrepresented pages read as zero without faulting.
AX_FN Word page_entry(const Word *address) {
    Word value;
#if __CUDA_ARCH__ >= 700
    asm volatile("ld.relaxed.gpu.global.u32 %0,[%1];" : "=r"(value) : "l"(address) : "memory");
#else
    value = *reinterpret_cast<const volatile Word *>(address);
#endif
    return value;
}
AX_FN Word claim_word(Word *address, Word old, Word replacement) {
    Word prior;
    asm volatile("atom.global.cas.b32 %0,[%1],%2,%3;"
                 : "=r"(prior)
                 : "l"(address), "r"(old), "r"(replacement)
                 : "memory");
    return prior;
}
AX_FN Word additional_page(NervePages pages, Word key) {
    Word entry = spill_page;
    if (pages.reserve) {
        Word first = mix(key) % pages.reserve;
        for (Word probe = 0; probe < 4 && probe < pages.reserve; ++probe) {
            Word bucket = first + probe;
            if (bucket >= pages.reserve)
                bucket -= pages.reserve;
            Word owner = claim_word(pages.claims + bucket, 0, key + 1);
            if (!owner || owner == key + 1) {
                entry = pages.hot + bucket + 1;
                break;
            }
        }
    }
    Word old = claim_word(pages.index + key, 0, entry);
    return old ? old : entry;
}
AX_FN Count nerve_page_key(NervePages pages, Word slot, Word band) {
    return Count(band - basal_bands) * pages.groups + slot / nerve_page_slots;
}
AX_FN Tissue *page_address(World w, Word slot, Word key, Word entry) {
    auto p = w.neural_pages;
    Word offset = slot & (nerve_page_slots - 1);
    if (entry == spill_page)
        return p.spill + Count(key) * nerve_page_slots + offset;
    return w.nerve + Count(p.sites) * basal_bands + Count(entry - 1) * nerve_page_slots + offset;
}
AX_FN Tissue stored_band(World w, Word capacity, Word slot, Word band) {
    auto p = w.neural_pages;
    if (!p.index)
        return w.nerve[Count(slot) * capacity + band];
    if (band < basal_bands)
        return w.nerve[Count(slot) * basal_bands + band];
    Word key = nerve_page_key(p, slot, band), entry = page_entry(p.index + key);
    return entry ? *page_address(w, slot, key, entry) : Tissue{};
}
AX_FN void write_band(World w, Word capacity, Word slot, Word band, Tissue value) {
    auto p = w.neural_pages;
    if (!p.index) {
        w.nerve[Count(slot) * capacity + band] = value;
        return;
    }
    if (band < basal_bands) {
        w.nerve[Count(slot) * basal_bands + band] = value;
        return;
    }
    Word key = nerve_page_key(p, slot, band), entry = page_entry(p.index + key);
    if (!entry) {
        if (!(value.spike | value.memory | value.gate_a | value.gate_b))
            return;
        entry = additional_page(p, key);
    }
    *page_address(w, slot, key, entry) = value;
}
AX_FN Tissue read_band(World w, Word capacity, Word slot, Word band) {
    auto p = w.neural_pages;
    if (p.index && !(p.overrides[slot] & (1u << band)))
        return band < p.origin[slot] ? developmental_band(w.dna[slot].brain.development, band) : Tissue{};
    return stored_band(w, capacity, slot, band);
}
AX_FN void assign_band(World w, Word capacity, Word slot, Word band, Tissue value) {
    write_band(w, capacity, slot, band, value);
    if (w.neural_pages.index)
        w.neural_pages.overrides[slot] |= 1u << band;
}
AX_FN void import_band(World w, Word capacity, Word slot, Word band, Tissue value) {
    auto p = w.neural_pages;
    if (!p.index) {
        write_band(w, capacity, slot, band, value);
        return;
    }
    Tissue base = band < p.origin[slot] ? developmental_band(w.dna[slot].brain.development, band) : Tissue{};
    Word bit = 1u << band;
    if (same_band(value, base)) {
        asm volatile("red.global.and.b32 [%0],%1;" ::"l"(p.overrides + slot), "r"(~bit) : "memory");
    } else {
        write_band(w, capacity, slot, band, value);
        asm volatile("red.global.or.b32 [%0],%1;" ::"l"(p.overrides + slot), "r"(bit) : "memory");
    }
}
AX_FN Word nonzero_nerve(Tissue t) { return t.spike | t.memory | t.gate_a | t.gate_b; }
