struct NeuralShape {
    Count living = 0, expressed = 0, scheduled = 0, rounds = 0, irregular = 0, fixed_warps = 0;
    bool prefer_packed(Word bands) const { return living && living * bands * 4 >= rounds * 32 * 7; }
};
using CellWidths = std::array<Word, 33>;
inline NeuralShape neural_shape(const Habitat &h, CellWidths *distribution = nullptr) {
    NeuralShape q{};
    Word n = h.sites(), tiles = h.header.configuration.tiles;
    if (h.cortices.size() != n || h.slots.size() != n || h.bodies.size() != n)
        return q;
    for (Word first = 0; first < n; first += 32) {
        Word used = 0, fixed = 0;
        for (Word i = first; i < std::min(first + 32, n); ++i) {
            Word slot = h.slots[i];
            if (!h.bodies[slot].energy)
                continue;
            Word width = h.cortices[slot].active;
            if (distribution)
                ++(*distribution)[width];
            ++q.living;
            q.expressed += h.conducting(slot);
            q.scheduled += width;
            q.irregular += bool(width & (width - 1));
            fixed |= 1u << ((i - first) * tiles / 32);
            if (used + width > 32) {
                ++q.rounds;
                used = 0;
            }
            used += width;
        }
        q.rounds += used != 0;
        q.fixed_warps += std::popcount(fixed);
    }
    return q;
}
inline bool pack_cortex(const Habitat &h, NeuralDispatch dispatch) {
    if (!h.developing || dispatch == NeuralDispatch::fixed)
        return false;
    if (dispatch == NeuralDispatch::packed)
        return true;
    Word tiles = h.header.configuration.tiles, n = h.sites();
    if (tiles < 8 || n < 4096)
        return false;
    if (h.cortices.size() != n)
        return tiles == 32 && n >= 16384;
    return neural_shape(h).prefer_packed(tiles);
}
struct NeuralPageLayout {
    Word sites = 0, bands = 0, groups = 0, logical = 0, hot = 0, reserve = 0;
    bool paged = false;
    std::vector<Word> initial, overrides;
    std::vector<unsigned char> origin;
    NeuralPageLayout(Session &s, const Habitat &h, NeuralMemory policy, Word requested_cache)
        : sites(h.sites()), bands(h.header.configuration.tiles) {
        if (policy == NeuralMemory::dense || bands <= basal_bands)
            return;
        if (policy == NeuralMemory::automatic && (!h.developing || h.nerves.empty() || sites < 16384 || bands < 8))
            return;
        int pageable = 0;
        auto status = s.d.attribute(&pageable, 88, s.device);
        if (status || !pageable) {
            if (policy == NeuralMemory::paged) {
                s.d.check(status, "query GPU pageable memory access");
                throw std::runtime_error("paged neural storage requires CUDA pageable memory access on this host");
            }
            return;
        }
        groups = (sites + nerve_page_slots - 1) / nerve_page_slots;
        logical = groups * (bands - basal_bands);
        initial.resize(logical);
        // Entire unvisited rows may still be the exact germline image.
        // Preserve arbitrary nonzero bands, including state from extinct bodies.
        origin.resize(sites);
        overrides.resize(sites);
        for (Word slot = 0; slot < h.nerves.size() / bands; ++slot) {
            auto *row = h.nerves.data() + Count(slot) * bands;
            auto shape = describe_neural_row(row, bands, h.genomes[slot].brain.development);
            if (shape.germline) {
                origin[slot] = shape.extent;
                continue;
            }
            overrides[slot] = shape.mask;
            for (Word j = basal_bands; j < shape.extent; ++j)
                if (shape.mask & (1u << j))
                    initial[(j - basal_bands) * groups + slot / nerve_page_slots] = 1;
        }
        for (auto &v : initial)
            if (v)
                v = ++hot;
        Word wanted = requested_cache == ~0u ? std::max(32u, hot / 2 + sites / 256) : requested_cache;
        reserve = std::min(logical - hot, requested_cache == ~0u ? std::bit_ceil(wanted) : wanted);
        const Count bytes = (Count(sites) * basal_bands + Count(hot + reserve) * nerve_page_slots) * sizeof(Tissue) +
                            Count(logical + reserve) * sizeof(Word) + Count(sites) * (sizeof(Word) + 1);
        if (policy == NeuralMemory::automatic && bytes * 4 >= Count(sites) * bands * sizeof(Tissue) * 3) {
            std::vector<Word>().swap(initial);
            std::vector<Word>().swap(overrides);
            std::vector<unsigned char>().swap(origin);
            return;
        }
        paged = true;
    }
    Count tissue_bytes() const {
        return sizeof(Tissue) *
               (paged ? Count(sites) * basal_bands + Count(hot + reserve) * nerve_page_slots : Count(sites) * bands);
    }
    Count metadata_bytes() const {
        return paged ? Count(logical + reserve) * sizeof(Word) + Count(sites) * (sizeof(Word) + 1) : 0;
    }
    Count virtual_bytes() const { return paged ? Count(logical) * nerve_page_slots * sizeof(Tissue) : 0; }
};
class NeuralSpill {
    size_t bytes = 0;

  public:
    Tissue *data = nullptr;
    explicit NeuralSpill(size_t n) : bytes(n) {
        if (!bytes)
            return;
        void *address =
            mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (address == MAP_FAILED)
            throw std::system_error(errno, std::generic_category(), "reserve sparse neural spill");
        data = static_cast<Tissue *>(address);
    }
    ~NeuralSpill() {
        if (data)
            munmap(data, bytes);
    }
    NeuralSpill(const NeuralSpill &) = delete;
    NeuralSpill &operator=(const NeuralSpill &) = delete;
};
