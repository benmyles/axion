#include "axion/signal_voice.hpp"
#include "axion/material_hands.hpp"
#pragma once
#include "axion/regulation.hpp"
#include "axion/drift.hpp"
#include "axion/siphon_words.hpp"
#include "axion/rattle.hpp"
#include "axion/braid_gate.hpp"
#include "axion/braid_discourse.hpp"
#include "axion/narrative.hpp"
#include "axion/capillary_voice.hpp"
#include "axion/hollow_words.hpp"
#include "axion/floodplain.hpp"
#include "life_io.hpp"
#include "field_voice_storage.hpp"
#include "grounding_storage.hpp"
#include <bit>
#include <array>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <cstring>
#include <vector>

namespace axion::life::flood {
struct Habitat {
    struct Header {
        char magic[8] = "AXFLOOD";
        Word version = 3, tick = 0;
        Configuration configuration{};
        Balance initial{};
        Count founders = 0;
        Counters account{};
    } header;
    std::vector<Silt> land;
    std::vector<Word> slots;
    std::vector<Germ> genomes;
    std::vector<Colony> bodies;
    std::vector<Tissue> nerves;
    bool short_neural_rows = false;
    Count file_bands = 0, file_developmental = 0;
    std::vector<Ear> ears;
    struct PodHeader {
        pods::Physics physics{};
        Word entry_mass = 0, reserved = 0;
        pods::Census account{};
    } crop;
    bool enclosed = false;
    Word pod_origin = 0;
    pods::Inheritance pod_donor{};
    std::vector<pods::Animal> pod_animals;
    std::vector<pods::Surface> rinds;
    struct CortexHeader {
        Word open = 1, cohort_controls = 0;
        Count initial = 0;
        GrowthTotals account{};
    } neuro;
    bool developing = false;
    std::vector<Cortex> cortices;
    struct MaterialHeader {
        material::Conditions physics{};
        Count pieces = 0, charge = 0, terminals[3]{};
        material::Ledger account{};
    } minerals;
    bool articulated = false, inherit_hands = false;
    material::Genes hand_donor{};
    std::vector<material::Grain> mineral_bed;
    std::vector<material::Worker> workers;
    bool foresight = false;
    material::TideState tide{};
    std::vector<material::Foresight> forecasts;
    bool investigatory = false;
    material::WorksiteState worksite{};
    std::vector<material::Inquiry> enquiries;
    std::vector<material::Imprint> patina;
    bool sedimentary = false;
    material::SedimentState seam{};
    std::vector<Half> macromolecules;
    bool overhearing = false;
    siphon::Chorus audience{};
    std::vector<siphon::Echo> acoustics;
    bool pottery = false;
    clay::State workshop{};
    std::vector<clay::Mind> potters;
    std::vector<clay::Tile> ware, cargo;
    drift::State drift_water{};
    bool braiding=false;braid::State loom{};Count float_imports=0,float_exports=0;Word gate_phase=0;braid::GateBook sluice_book{};braid::SedimentBook sediment_book{};std::vector<braid::GateMemory> sluice_contacts;
    bool recording_floats=false;
    struct ResidentNarration {Word slot,born;braid::Narration mind;};
    std::vector<braid::Episode> float_events;
    std::vector<ResidentNarration> narrators;
    std::vector<utterance::Trail> trails;
    struct Storyteller { Word slot,born; utterance::Memory memory; };
    static constexpr size_t first_story=offsetof(Storyteller,memory)+offsetof(utterance::Memory,questions);
    std::vector<Storyteller> stories;
    capillary::Settings cap_law{};
    std::vector<capillary::Mind> cap_memories;
    std::vector<Word> cap_waves;
    struct CapillarySpeaker {Word slot=0,born=0;capillary::Mouth speech{};};
    std::vector<capillary::History> cap_histories;
    std::vector<unsigned char> appetites;
    struct AudibleResident {Word slot,born;capillary::SignalVoice speech;};
    static constexpr size_t initial_audible=offsetof(AudibleResident,speech)+offsetof(capillary::SignalVoice,episodes);
    std::vector<AudibleResident> audible_residents;
    struct CompoundResident { Word slot, born, signs; };
    std::vector<CompoundResident> compound_residents;
    std::vector<CapillarySpeaker> cap_speakers;
    bool steady_eyes() const {return loom.version>=12 && !cap_law.raw_frame;}
    struct NamedFloats {Word slot,born;braid::Words lexicon;};
    std::vector<braid::Mind> riggers;std::vector<braid::Half> floats;std::vector<NamedFloats> flute_words;
    bool rattling = false;
    rattle::State backwater{}; rattle::Attention queries{};rattle::Load mesh{};
    std::vector<rattle::Mind> listeners;
    bool reeding=false; runnel::State rush{};runnel::Social reed_audit{}; std::vector<runnel::Mind> cutters; std::vector<Byte> folds;
    bool wicking=false;wicks::State wicking_state{};std::vector<Word> liquid;
    bool practising=false;
    practice::State learning{};practice::Credits legacy_work{};std::vector<practice::Mark> strands;
    std::vector<practice::Mind> pupils;
    std::vector<practice::Context> memories;
    bool linking=false;
    linkage::State millwork{};
    std::vector<linkage::Mind> makers;
    std::vector<linkage::Tissue> mechanisms;
    std::vector<linkage::Provenance> seals;
    bool appraising=false;
    effort::State economics{};
    std::vector<effort::Mind> economies;
    std::vector<unsigned char> treads;
    bool ranging = false;
    hollow::State recess{};
    std::vector<hollow::Mind> wanderers;
    std::vector<hollow::Bed> hollows;
    std::vector<Word> bearings, sightings;
    bool calling = false;
    siphon::State reeds{};
    std::vector<siphon::Animal> callers;
    std::vector<Byte> apertures;
    bool saline = false;
    Word founder_arbors = 1;
    brine::State solution{};
    std::vector<brine::Animal> saline_animals;
    std::vector<brine::Salts> salts;
    std::array<Count, 5> salt_inventory() const {
        std::array<Count, 5> inventory{};
        for (auto q : salts)
            for (Word j = 0; j < 5; ++j)
                inventory[j] += brine::amount(q, j);
        return inventory;
    }
    struct ResidentSpeech {
        Word slot = 0, born = 0;
        grounding::Mind vocabulary{};
        material::FieldLanguage questions{};
    };
    std::vector<ResidentSpeech> spoken;
    struct NativeSpeaker {
        Word slot = 0, born = 0;
        RecollectionGenes genes{};
        Word reserved = 0;
        siphon::Vocabulary words{};
    };
    std::vector<NativeSpeaker> captions;
    std::vector<NativeSpeaker> place_words;

    void retain_speakers() {
        std::erase_if(audible_residents,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(compound_residents,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(cap_speakers,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(stories,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(narrators,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(flute_words,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(place_words,[&](const auto &p){return p.slot>=bodies.size()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born;});
        std::erase_if(captions, [&](const auto &p) {
            return p.slot >= bodies.size() || !bodies[p.slot].energy || bodies[p.slot].born != p.born;
        });
        std::erase_if(spoken, [&](const auto &q) {
            return q.slot >= bodies.size() || !bodies[q.slot].energy || bodies[q.slot].born != q.born;
        });
    }
    Word format() const {
        return base_format() + (calling ? 512 : 0) + (overhearing ? 1024 : 0) + (captions.empty() ? 0 : 2048) +
               (pottery ? 12288 : 0) + (pottery && (workshop.use & press::installed) ? 49152 : 0) +
               (pottery && (workshop.use & press::feel_set) ? 65536 : 0) + (ranging ? 131072u : 0u) +
               (place_words.empty() ? 0u : 262144u) + (appraising ? 524288u : 0u) + (linking ? 1048576u : 0u) +
               (practising ? 6291456u : 0u) + (wicking ? 8388608u : 0u) + (reeding ? 16777216u : 0u) +
               (rattling ? 33554432u : 0u) + (braiding ? 67108864u : 0u) + (recording_floats ? 134217728u : 0u) +
               (narrators.empty() ? 0u : 268435456u) + (short_neural_rows ? 536870912u : 0u);
    }
    Word base_format() const {
        if (saline)
            return 25 + Word(enclosed) + 2 * Word(developing) + (spoken.empty() ? 0 : 256);
        if (sedimentary)
            return 21 + Word(enclosed) + 2 * Word(developing) + (spoken.empty() ? 0 : 256);
        if (investigatory)
            return 17 + Word(enclosed) + 2 * Word(developing) + (spoken.empty() ? 0 : 256);
        if (foresight)
            return 13 + Word(enclosed) + 2 * Word(developing);
        if (articulated)
            return 9 + Word(enclosed) + 2 * Word(developing);
        return developing ? (enclosed ? 8 : 7) : enclosed ? 6 : 3;
    }
    Word conducting(Word slot) const {
        if (!developing)
            return living_tiles(header.configuration.tiles, genomes[slot].brain.development);
        Word tag = bodies[slot].cohort;
        return conducting_bands(cortices[slot], tag && tag == (neuro.cohort_controls >> 2));
    }
    Word sites() const { return header.configuration.side * header.configuration.side; }
    void allocate() {
        Word n = sites();
        land.resize(n);
        if(recording_floats)float_events.resize(n);
        if(braiding){riggers.resize(n);floats.resize(n);if(loom.version>=3)sluice_contacts.resize(n);}
        if(rattling) listeners.resize(n);
        if(reeding){cutters.resize(n);folds.resize(n);}
        if(wicking)liquid.resize(n);
        if(practising){strands.resize(n/4);pupils.resize(n);memories.resize(Count(n)*learning.arena);}
        if(linking){makers.resize(n);mechanisms.resize(Count(n)*millwork.arena);seals.resize(n/4);}
        if(appraising){economies.resize(n);treads.resize(n);}
        if (ranging) {
            wanderers.resize(n); hollows.resize(n); bearings.resize(Count(n)*recess.slots);
            sightings.resize(Count((n+15)/16)*recess.slots);
        }
        if (pottery) {
            potters.resize(n);
            ware.resize(n);
            cargo.resize(n);
        }
        if (overhearing)
            acoustics.resize(n);
        if (calling) {
            callers.resize(n);
            apertures.resize(n);
        }
        if (saline) {
            salts.resize(n);
            saline_animals.resize(n);
        }
        if (sedimentary)
            macromolecules.resize(n);
        slots.resize(n);
        genomes.resize(n);
        bodies.resize(n);
        nerves.resize(Count(n) * header.configuration.tiles);
        ears.resize(n);
        if (articulated) {
            mineral_bed.resize(n);
            workers.resize(n);
        }
        if (investigatory) {
            enquiries.resize(n);
            patina.resize(n);
        }
        if (foresight)
            forecasts.resize(n);
        if (developing)
            cortices.resize(n);
        if (enclosed) {
            pod_animals.resize(n);
            rinds.resize(n);
        }
    }
    template <class F> void arrays(F f) {
        f(land);
        f(slots);
        f(genomes);
        f(bodies);
        f(nerves);
        f(ears);
    }
    template <class Writer> void emit_neural_rows(Writer &write) {
        Word n = sites(), bands = header.configuration.tiles;
        std::vector<Byte> lengths(n);
        Count total = 0;
        file_developmental = 0;
        for (Word i = 0; i < n; ++i) {
            auto row = describe_neural_row(nerves.data() + Count(i) * bands, bands, genomes[i].brain.development);
            lengths[i] = row.extent | (row.germline ? 128u : 0u);
            if (row.germline)
                ++file_developmental;
            else
                total += row.extent;
        }
        file_bands = total;
        write(&total, sizeof(total));
        write(lengths.data(), n);
        std::vector<Tissue> buffer;
        buffer.reserve(65536);
        for (Word i = 0; i < n; ++i) {
            Word count = lengths[i];
            if (count & 128)
                continue;
            const auto *row = nerves.data() + Count(i) * bands;
            if (buffer.size() + count > buffer.capacity()) {
                write(buffer.data(), buffer.size() * sizeof(Tissue));
                buffer.clear();
            }
            buffer.insert(buffer.end(), row, row + count);
        }
        if (!buffer.empty())
            write(buffer.data(), buffer.size() * sizeof(Tissue));
    }
    template <class F> void chunks(F f) {
        retain_speakers();
        header.version = format();
        f(&header, sizeof(header));
        arrays([&](auto &v) {
            if constexpr (std::is_same_v<typename std::decay_t<decltype(v)>::value_type, Tissue>) {
                if (short_neural_rows) {
                    emit_neural_rows(f);
                    return;
                }
            }
            f(v.data(), v.size() * sizeof(v[0]));
        });
        if (enclosed) {
            f(&crop, sizeof(crop));
            f(pod_animals.data(), pod_animals.size() * sizeof(pods::Animal));
            f(rinds.data(), rinds.size() * sizeof(pods::Surface));
        }
        if (developing) {
            f(&neuro, sizeof(neuro));
            f(cortices.data(), cortices.size() * sizeof(Cortex));
        }
        if (articulated) {
            f(&minerals, sizeof(minerals));
            f(mineral_bed.data(), mineral_bed.size() * sizeof(material::Grain));
            f(workers.data(), workers.size() * sizeof(material::Worker));
        }
        if (foresight) {
            f(&tide, sizeof(tide));
            f(forecasts.data(), forecasts.size() * sizeof(forecasts[0]));
        }
        if (investigatory) {
            f(&worksite, sizeof(worksite));
            f(enquiries.data(), enquiries.size() * sizeof(enquiries[0]));
            f(patina.data(), patina.size() * sizeof(patina[0]));
        }
        if (sedimentary) {
            f(&seam, sizeof(seam));
            f(macromolecules.data(), macromolecules.size() * sizeof(Half));
        }
        if (saline) {
            f(&solution, sizeof(solution));
            f(salts.data(), salts.size() * sizeof(salts[0]));
            f(saline_animals.data(), saline_animals.size() * sizeof(saline_animals[0]));
        }
        if (calling) {
            f(&reeds, sizeof(reeds));
            f(apertures.data(), apertures.size());
            f(callers.data(), callers.size() * sizeof(callers[0]));
        }
        if (overhearing) {
            f(&audience, sizeof(audience));
            f(acoustics.data(), acoustics.size() * sizeof(acoustics[0]));
        }
        if (pottery) {
            constexpr size_t original = offsetof(clay::State, book) + offsetof(clay::Totals, press);
            f(&workshop, (workshop.use & press::feel_set)    ? sizeof(workshop)
                         : (workshop.use & press::installed) ? original + offsetof(clay::PressLedger, rims)
                                                             : original);
            f(potters.data(), potters.size() * sizeof(potters[0]));
            f(ware.data(), ware.size() * sizeof(ware[0]));
            f(cargo.data(), cargo.size() * sizeof(cargo[0]));
        }
        if (ranging) {
            f(&recess,sizeof(recess));
            f(hollows.data(),hollows.size()*sizeof(hollows[0]));
            f(wanderers.data(),wanderers.size()*sizeof(wanderers[0]));
            f(bearings.data(),bearings.size()*sizeof(Word));
            f(sightings.data(),sightings.size()*sizeof(Word));
        }
        if(appraising){
            f(&economics,sizeof(economics));f(economies.data(),economies.size()*sizeof(economies[0]));f(treads.data(),treads.size());
        }
        if(linking){
            f(&millwork,sizeof(millwork));f(makers.data(),makers.size()*sizeof(makers[0]));
            f(mechanisms.data(),mechanisms.size()*sizeof(mechanisms[0]));f(seals.data(),seals.size()*sizeof(seals[0]));
        }
        if(practising){f(&learning,sizeof(learning));f(pupils.data(),pupils.size()*sizeof(pupils[0]));f(memories.data(),memories.size()*sizeof(memories[0]));f(&legacy_work,sizeof(legacy_work));f(strands.data(),strands.size()*sizeof(strands[0]));}
        if(wicking){f(&wicking_state,sizeof(wicking_state));f(liquid.data(),liquid.size()*sizeof(liquid[0]));}
        if(reeding){f(&rush,sizeof(rush));f(cutters.data(),cutters.size()*sizeof(cutters[0]));f(folds.data(),folds.size());if(rush.version>=2)f(&reed_audit,sizeof(reed_audit));}
        if(rattling){f(&backwater,sizeof(backwater));f(listeners.data(),listeners.size()*sizeof(listeners[0]));if(backwater.format>=2)f(&queries,sizeof(queries));if(backwater.format>=3)f(&mesh,sizeof(mesh));}
        if(braiding){
            auto physical=loom;if(physical.version==14 && !cap_speakers.empty())physical.version=15;physical.reserved=flute_words.size();f(&physical,sizeof(physical));
            if(physical.version>=14){
                f(&cap_law,sizeof(cap_law));Word named=cap_speakers.size();f(&named,sizeof(named));
                if(named)f(cap_speakers.data(),Count(named)*sizeof(CapillarySpeaker));
                f(cap_memories.data(),cap_memories.size()*sizeof(capillary::Mind));
                f(cap_waves.data(),cap_waves.size()*sizeof(Word));
                if(physical.version>=17)f(cap_histories.data(),cap_histories.size()*sizeof(capillary::History));
                if(physical.version>=20)f(appetites.data(),appetites.size());
                if(physical.version>=21){Word total=compound_residents.size();f(&total,4);if(total)f(compound_residents.data(),Count(total)*sizeof(CompoundResident));}
                if(physical.version>=22){Word total=audible_residents.size();f(&total,4);if(physical.version==22){for(auto &p:audible_residents)f(&p,initial_audible);}else if(total)f(audible_residents.data(),Count(total)*sizeof(AudibleResident));}
            }
            if (physical.version>=10) {
                Word n=stories.size(); f(&n,sizeof(n));
                if(physical.version==10) {for(auto &story:stories) f(&story,first_story);}
                else if(n) f(stories.data(),Count(n)*sizeof(Storyteller));
                f(trails.data(),trails.size()*sizeof(trails[0]));
            }
            f(riggers.data(),riggers.size()*sizeof(riggers[0]));f(floats.data(),floats.size()*sizeof(floats[0]));
            if(physical.version>=2){f(&float_imports,sizeof(float_imports));f(&float_exports,sizeof(float_exports));}
            if(physical.version>=3){f(&gate_phase,sizeof(gate_phase));f(&sluice_book,sizeof(sluice_book));f(sluice_contacts.data(),sluice_contacts.size()*sizeof(sluice_contacts[0]));}
            if(physical.version>=4)f(&sediment_book,sizeof(sediment_book));
            if(physical.version>=7)f(&drift_water,sizeof(drift_water));
            if(physical.reserved)f(flute_words.data(),flute_words.size()*sizeof(NamedFloats));
        }
        if(recording_floats)f(float_events.data(),float_events.size()*sizeof(braid::Episode));
        if(!narrators.empty()){Word count=narrators.size();f(&count,4);f(narrators.data(),Count(count)*sizeof(ResidentNarration));}
        if (!place_words.empty()) {
            Word count=place_words.size();f(&count,sizeof(count));f(place_words.data(),Count(count)*sizeof(NativeSpeaker));
        }
        if (!captions.empty()) {
            Word n = captions.size();
            f(&n, sizeof(n));
            f(captions.data(), Count(n) * sizeof(NativeSpeaker));
        }
        if (!spoken.empty()) {
            Word count = spoken.size();
            f(&count, sizeof(count));
            f(spoken.data(), spoken.size() * sizeof(ResidentSpeech));
        }
    }
    void expand_brains(Word tiles) {
        Word prior = header.configuration.tiles;
        if (tiles == prior)
            return;
        if (tiles < prior)
            throw std::invalid_argument("reducing a saved neural arena would discard tissue");
        if (!developing)
            throw std::invalid_argument("enlarge a saved neural arena with --neurogenesis inherited");
        // A legacy resident's birth size refers to the old physical capacity.
        bool legacy = cortices.empty();
        if (legacy) {
            cortices.resize(sites());
            for (Word i = 0; i < sites(); ++i)
                cortices[i] = hatch_cortex(growth_founder(mix(header.configuration.seed + i + 19561)),
                                           living_tiles(prior, genomes[i].brain.development));
        }
        std::vector<Tissue> wider(Count(sites()) * tiles);
        for (Word i = 0; i < sites(); ++i) {
            Word retained = legacy ? cortices[i].active : prior;
            std::copy_n(nerves.data() + Count(i) * prior, retained, wider.data() + Count(i) * tiles);
        }
        nerves.swap(wider);
        header.configuration.tiles = tiles;
    }
    void read(const std::string &path) {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in)
            throw std::runtime_error("cannot open floodplain " + path);
        auto extent = in.tellg();
        in.seekg(0);
        Word version = 0;
        in.seekg(8);
        in.read(reinterpret_cast<char *>(&version), sizeof(version));
        in.seekg(0);
        bool language = version & 256u;
        bool annotated = version & 2048u;
        calling = version & 512u;
        overhearing = version & 1024u;
        pottery = version & 4096u;
        bool grasped = version & 8192u;
        bool pressing = version & 16384u;
        bool pressure_readers = version & 32768u;
        bool feeling = version & 65536u;
        ranging=version&131072u;
        bool named_places=version&262144u;
        appraising=version&524288u;
        linking=version&1048576u;
        practising = version & 2097152u;
        bool signed_works = version & 4194304u;
        wicking = version & 8388608u;
        reeding = version & 16777216u;
        rattling = version & 33554432u;
        braiding = version & 67108864u;
        recording_floats = version & 134217728u;
        bool expressed = version & 268435456u;
        short_neural_rows = version & 536870912u;
        version &= ~1073741568u;
        if((recording_floats&&!braiding)||(expressed&&!recording_floats))throw std::runtime_error("missing resident float observations");
        if (!in || (reeding&&!wicking) || (wicking&&!ranging) || (signed_works&&!practising) || (practising&&!linking) || (linking && (!ranging || version<9)) || (appraising && !ranging) || (named_places && !ranging) || (version < 1 || version > 28) || (calling && version < 9) ||
            ((overhearing || annotated || pottery) && !calling) || (grasped && !pottery) || (pressing && !grasped) || (pressure_readers && !pressing) || (feeling && !pressure_readers) || (language && version < 17))
            throw std::runtime_error("invalid floodplain header");
        // The previous water/earth format has the same prefix. Its spare word
        // becomes the weaving switch, and the five new ledger counters follow.
        constexpr size_t old_header = offsetof(Header, account) + offsetof(Counters, fibre_lifted);
        static_assert(old_header == 304 && offsetof(Header, initial) == 72 && offsetof(Colony, fibre) == 76);
        constexpr size_t woven_header = offsetof(Header, account) + offsetof(Counters, recruits);
        static_assert(woven_header == 344);
        size_t header_bytes = version == 1 ? old_header : version == 2 ? woven_header : sizeof(Header);
        header = {};
        if (!in.read(reinterpret_cast<char *>(&header), header_bytes) || std::memcmp(header.magic, "AXFLOOD", 8))
            throw std::runtime_error("invalid floodplain header");
        Count hash = storage::digest(storage::hash_start, &header, header_bytes);
        if (version == 1)
            header.configuration.weaving = 0;
        if (version < 3)
            header.configuration.flora = 0;
        saline = version >= 25;
        sedimentary = version >= 21;
        investigatory = version >= 17;
        foresight = version >= 13;
        articulated = version >= 9;
        enclosed = articulated ? ((version - 9) & 1) : (version >= 4 && version != 7);
        developing = articulated ? ((version - 9) & 2) : version >= 7;
        header.version = format();
        const auto &c = header.configuration;
        if (!std::has_single_bit(c.side) || c.side < 4 || c.side > 1024 || !std::has_single_bit(c.tiles) ||
            c.tiles < 2 || c.tiles > 32 || !c.season ||
            (c.frontier && (!std::has_single_bit(c.frontier) || c.frontier < c.side || c.frontier > 1024)) ||
            c.density > 100 || c.terrain > 1 || c.architecture > 1 || c.morph > 1 || c.wire > 7 || c.predation > 1 ||
            c.plastic > 1 || c.weaving > 1 || c.flora > 2)
            throw std::runtime_error("invalid floodplain dimensions");
        Count soil_bytes = version == 1   ? offsetof(Silt, fibre_east)
                           : version == 2 ? offsetof(Silt, root)
                                          : sizeof(Silt);
        Count wanted = header_bytes + sizeof(Count) +
                       Count(sites()) * (soil_bytes + sizeof(Word) + sizeof(Germ) + sizeof(Colony) +
                                         Count(c.tiles) * sizeof(Tissue) + sizeof(Ear));
        Count stored_bands = 0;
        if (short_neural_rows) {
            auto resume = in.tellg();
            Count prior = header_bytes + Count(sites()) * (soil_bytes + sizeof(Word) + sizeof(Germ) + sizeof(Colony));
            in.seekg(static_cast<std::streamoff>(prior), std::ios::beg);
            if (!in.read(reinterpret_cast<char *>(&stored_bands), sizeof(stored_bands)) ||
                stored_bands > Count(sites()) * c.tiles)
                throw std::runtime_error("invalid sparse neural bank extent");
            wanted -= Count(sites()) * c.tiles * sizeof(Tissue);
            wanted += sizeof(Count) + sites() + stored_bands * sizeof(Tissue);
            in.seekg(resume);
        }
        size_t crop_bytes =
            version == 4 ? offsetof(PodHeader, account) + offsetof(pods::Census, withheld) : sizeof(crop);
        if (enclosed)
            wanted += crop_bytes +
                      Count(sites()) * ((version < 6 ? offsetof(pods::Animal, frame_genes) : sizeof(pods::Animal)) +
                                        sizeof(pods::Surface));
        if (developing)
            wanted += sizeof(neuro) + Count(sites()) * sizeof(Cortex);
        if (articulated)
            wanted += sizeof(minerals) + Count(sites()) * (sizeof(material::Grain) + sizeof(material::Worker));
        if (foresight)
            wanted += sizeof(tide) + Count(sites()) * sizeof(material::Foresight);
        if (investigatory)
            wanted += sizeof(worksite) + Count(sites()) * (sizeof(material::Inquiry) + sizeof(material::Imprint));
        if (sedimentary)
            wanted += sizeof(seam) + Count(sites()) * sizeof(Half);
        if (saline)
            wanted += sizeof(solution) + Count(sites()) * (sizeof(brine::Salts) + sizeof(brine::Animal));
        if (calling)
            wanted += sizeof(reeds) + Count(sites()) * (sizeof(Byte) + sizeof(siphon::Animal));
        if (overhearing)
            wanted += sizeof(audience) + Count(sites()) * sizeof(siphon::Echo);
        constexpr size_t first_workshop = offsetof(clay::State, book) + offsetof(clay::Totals, press);
        size_t workshop_bytes = feeling ? sizeof(workshop) : pressure_readers ? first_workshop+offsetof(clay::PressLedger,rims) : pressing ? first_workshop + offsetof(clay::PressLedger, studied) : grasped ? first_workshop : first_workshop - 2 * sizeof(Count);
        if (pottery)
            wanted += workshop_bytes + Count(sites()) * (sizeof(clay::Mind) + 2 * sizeof(clay::Tile));
        if (ranging) {
            auto resume=in.tellg(); in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if (!in.read(reinterpret_cast<char *>(&recess),sizeof(recess)) || !recess.slots ||
                recess.slots>hollow::limit || (recess.mode&~8191u)) throw std::runtime_error("invalid hollow arena");
            wanted+=sizeof(recess)+Count(sites())*(sizeof(hollow::Mind)+sizeof(hollow::Bed)+recess.slots*sizeof(Word))
                    +Count((sites()+15)/16)*recess.slots*sizeof(Word);
            in.seekg(resume);
        }
        if(appraising)wanted+=sizeof(economics)+Count(sites())*(sizeof(effort::Mind)+1);
        if(linking){
            auto save=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&millwork),sizeof(millwork)) || !std::has_single_bit(millwork.arena) ||
               millwork.arena>4 || !std::has_single_bit(millwork.period) || millwork.period>64 || (millwork.mode&~65535u))
                throw std::runtime_error("invalid linkage arena");
            in.seekg(save);
            wanted+=sizeof(millwork)+Count(sites())*(sizeof(linkage::Mind)+millwork.arena*sizeof(linkage::Tissue))+
                    Count(sites()/4)*sizeof(linkage::Provenance);
        }
        if(practising){
            auto save=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&learning),sizeof(learning)) || learning.version!=1 || !learning.arena || learning.arena>4 || (learning.mode&~131071u))
                throw std::runtime_error("invalid practice arena");
            wanted+=sizeof(learning)+Count(sites())*(sizeof(practice::Mind)+learning.arena*sizeof(practice::Context));
            if(signed_works)wanted+=sizeof(legacy_work)+Count(sites()/4)*sizeof(practice::Mark);
            in.seekg(save);
        }
        if(wicking)wanted+=sizeof(wicking_state)+Count(sites())*sizeof(Word);
        if(reeding){
            auto place=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&rush),sizeof(rush))||(rush.version!=1&&rush.version!=2))throw std::runtime_error("invalid reed version");
            wanted+=sizeof(rush)+Count(sites())*(sizeof(runnel::Mind)+1)+(rush.version==2?sizeof(reed_audit):0);in.seekg(place);
        }
        if(rattling){
            auto place=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&backwater),sizeof(backwater)) || (backwater.format<1 || backwater.format>3))
                throw std::runtime_error("unknown bank acoustic tissue format");
            wanted+=sizeof(backwater)+Count(sites())*sizeof(rattle::Mind)+(backwater.format>=2?sizeof(queries):0)+(backwater.format>=3?sizeof(mesh):0);
            in.seekg(place);
        }
        Word flotilla_names=0, story_count=0, hydraulic_names=0;
        if(braiding){
            auto save=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if (!in.read(reinterpret_cast<char *>(&loom), sizeof(loom)) || (!loom.version || loom.version > 24) ||
                loom.reserved > sites() ||
                (loom.mode & ~(loom.version >= 10 ? ~0u
                               : loom.version >= 9 ? (2097151u | (127u << 22))
                               : loom.version >= 8 ? (2097151u | (15u << 22))
                               : loom.version >= 7 ? 2097151u
                               : loom.version >= 6 ? 1048575u
                               : loom.version >= 5 ? 262143u
                               : loom.version >= 4 ? 131071u
                               : loom.version >= 3 ? 32767u
                                                   : 1023u)))
                throw std::runtime_error("invalid float-work header");
            if(loom.version>=14){
                if(!in.read(reinterpret_cast<char*>(&cap_law),sizeof(cap_law)) || ((cap_law.mode&7)>4) || (cap_law.mode && !(cap_law.mode&7)) || cap_law.mode>32767 || ((cap_law.mode&capillary::material_hands)&&loom.version<24) || ((cap_law.mode&(capillary::fixed_hands|capillary::disconnected_hands))&&!(cap_law.mode&capillary::material_hands)) || ((cap_law.mode&1024)&&loom.version<20) || ((cap_law.mode&2048)&&!(cap_law.mode&1024)) || ((cap_law.mode&512)&&loom.version<19) || ((cap_law.mode&256) && (!(cap_law.mode&128)||loom.version<18)) || ((cap_law.mode&128)&&loom.version<17) || (cap_law.mode&48)==48 || ((cap_law.mode&64)&&!(cap_law.mode&48)) || cap_law.shift>8 || cap_law.mineral>32 ||
                   cap_law.raw_frame>1 || !in.read(reinterpret_cast<char*>(&hydraulic_names),4) || hydraulic_names>sites())
                    throw std::runtime_error("invalid capillary habitat");
                wanted+=sizeof(cap_law)+4+Count(hydraulic_names)*(loom.version==14?offsetof(CapillarySpeaker,speech)+offsetof(capillary::Mouth,manual):sizeof(CapillarySpeaker))+Count(sites())*(sizeof(capillary::Mind)+(loom.version>=17?sizeof(capillary::History):0)+(loom.version>=20?1:0))+3*Count((sites()+31)/32)*sizeof(Word);
                in.seekg(Count(hydraulic_names)*(loom.version==14?offsetof(CapillarySpeaker,speech)+offsetof(capillary::Mouth,manual):sizeof(CapillarySpeaker))+Count(sites())*(sizeof(capillary::Mind)+(loom.version>=17?sizeof(capillary::History):0)+(loom.version>=20?1:0))+3*Count((sites()+31)/32)*sizeof(Word),std::ios::cur);
                if(loom.version>=21){Word count=0;if(!in.read(reinterpret_cast<char*>(&count),4)||count>sites())throw std::runtime_error("invalid two-port resident count");
                    wanted+=4+Count(count)*sizeof(CompoundResident);in.seekg(Count(count)*sizeof(CompoundResident),std::ios::cur);}
                if(loom.version>=22){Word count=0;if(!in.read(reinterpret_cast<char*>(&count),4)||count>sites())throw std::runtime_error("invalid number of acoustic signallers");
                    auto stride=loom.version==22?initial_audible:sizeof(AudibleResident);wanted+=4+Count(count)*stride;in.seekg(Count(count)*stride,std::ios::cur);}
            }
            if (loom.version>=10) {
                if (!in.read(reinterpret_cast<char*>(&story_count),4) || story_count>sites() ||
                    ((loom.mode&utterance::voice_port) && !(loom.mode&utterance::ink_port)) ||
                    ((loom.mode&(utterance::crossed_port|utterance::stored_vocal_feedback)) && !(loom.mode&utterance::voice_port)) ||
                    ((loom.mode&utterance::ink_port) && !(loom.mode&(1u<<23))))throw std::runtime_error("invalid narrative extent");
                wanted+=4+Count(story_count)*(loom.version==10?first_story:sizeof(Storyteller))+Count(sites())*sizeof(utterance::Trail);
            }
            if ((loom.mode & (7u << 24)) && !(loom.mode & (1u << 23)))
                throw std::runtime_error("unconnected underwater probe");
            if ((loom.mode & (3u << 27)) && !(loom.mode & (1u << 26)))
                throw std::runtime_error("unconnected audience port");
            if((loom.mode&(regulation::silent_feedback|regulation::structural_port))&&!(loom.mode&regulation::motor_port))throw std::runtime_error("missing cortical motor port");
            if((loom.mode&braid::mobile_bed)&&!(loom.mode&braid::mineral_bed))throw std::runtime_error("missing erodible substrate");
            if(loom.version>=4)wanted+=sizeof(sediment_book);
            if(loom.version>=7)wanted+=sizeof(drift_water);
            flotilla_names=loom.reserved;
            wanted+=sizeof(loom)+Count(sites())*(sizeof(braid::Mind)+sizeof(braid::Half))+Count(flotilla_names)*sizeof(NamedFloats)+(loom.version>=2?2*sizeof(Count):0)+(loom.version>=3?sizeof(gate_phase)+sizeof(sluice_book)+Count(sites())*sizeof(braid::GateMemory):0);
            in.seekg(save);
        }
        if(recording_floats)wanted+=Count(sites())*sizeof(braid::Episode);
        Word speakers_count=0;
        if(expressed){
            auto resume=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&speakers_count),4)||!speakers_count||speakers_count>sites())throw std::runtime_error("invalid event speakers");
            wanted+=4+Count(speakers_count)*sizeof(ResidentNarration);in.seekg(resume);
        }
        Word names=0;
        if(named_places) {
            auto resume=in.tellg();in.seekg(std::streamoff(wanted-sizeof(Count)),std::ios::beg);
            if(!in.read(reinterpret_cast<char*>(&names),4)||!names||names>sites())throw std::runtime_error("invalid spatial speakers");
            wanted+=4+Count(names)*sizeof(NativeSpeaker);in.seekg(resume);
        }
        Word annotations = 0;
        if (annotated) {
            auto resume = in.tellg();
            in.seekg(std::streamoff(wanted - sizeof(Count)), std::ios::beg);
            if (!in.read(reinterpret_cast<char *>(&annotations), 4) || !annotations || annotations > sites())
                throw std::runtime_error("invalid annotated residents");
            wanted += 4 + Count(annotations) * sizeof(NativeSpeaker);
            in.seekg(resume);
        }
        Count speakers = 0;
        if (language) {
            if (extent <= std::streamoff(wanted + sizeof(Word)))
                throw std::runtime_error("missing resident lexicons");
            Count bytes = Count(extent) - wanted - sizeof(Word);
            speakers = bytes / sizeof(ResidentSpeech);
            if (speakers > sites() || speakers * sizeof(ResidentSpeech) != bytes)
                throw std::runtime_error("invalid resident lexicon table");
            wanted += sizeof(Word) + bytes;
        }
        if (extent != std::streamoff(wanted))
            throw std::runtime_error("invalid floodplain extent");
        allocate();
        auto read = [&](void *ptr, size_t bytes) {
            if (!in.read(static_cast<char *>(ptr), bytes))
                throw std::runtime_error("short floodplain state");
            hash = storage::digest(hash, ptr, bytes);
        };
        if (version < 3) {
            std::vector<unsigned char> old(sites() * soil_bytes);
            read(old.data(), old.size());
            for (Word i = 0; i < sites(); ++i)
                std::copy_n(old.data() + i * soil_bytes, soil_bytes, reinterpret_cast<unsigned char *>(&land[i]));
        } else
            read(land.data(), land.size() * sizeof(Silt));
        auto array = [&](auto &v) { read(v.data(), v.size() * sizeof(v[0])); };
        array(slots);
        array(genomes);
        array(bodies);
        if (short_neural_rows) {
            Count count = 0;
            read(&count, sizeof(count));
            if (count != stored_bands)
                throw std::runtime_error("changed sparse neural extent");
            std::vector<Byte> lengths(sites());
            array(lengths);
            Count seen = 0;
            for (Word length : lengths) {
                if ((length & 127) > c.tiles || length == 128)
                    throw std::runtime_error("invalid sparse neural row");
                if (!(length & 128))
                    seen += length;
            }
            if (seen != count)
                throw std::runtime_error("inconsistent sparse neural row lengths");
            std::fill(nerves.begin(), nerves.end(), Tissue{});
            // Bound the decode buffer even for a fully populated old reserve.
            std::vector<Tissue> buffer(std::min<Count>(count, 65536));
            Word row = 0;
            while (row < sites()) {
                Word first = row, used = 0;
                while (row < sites()) {
                    Word length = lengths[row] & 128 ? 0 : lengths[row];
                    if (used + length > buffer.size())
                        break;
                    used += length;
                    ++row;
                }
                if (used)
                    read(buffer.data(), Count(used) * sizeof(Tissue));
                Word start = 0;
                for (Word i = first; i < row; ++i) {
                    Word length = lengths[i];
                    auto *bands = nerves.data() + Count(i) * c.tiles;
                    if (length & 128) {
                        for (Word j = 0; j < (length & 127); ++j)
                            bands[j] = developmental_band(genomes[i].brain.development, j);
                    } else if (length) {
                        std::copy_n(buffer.data() + start, length, bands);
                        start += length;
                    }
                }
            }
        } else
            array(nerves);
        array(ears);
        if (enclosed) {
            crop = {};
            read(&crop, crop_bytes);
            auto &p = crop.physics;
            if (!p.lag || p.lag > 8 || !p.period || p.variable > 1 || p.credit > 2 || p.background > 95 ||
                p.cut_cohort > 2 || crop.reserved)
                throw std::runtime_error("invalid pod physics");
            if (version >= 6)
                array(pod_animals);
            else {
                constexpr size_t old_stride = offsetof(pods::Animal, frame_genes);
                std::vector<unsigned char> old(Count(sites()) * old_stride);
                read(old.data(), old.size());
                for (Word i = 0; i < sites(); ++i)
                    std::copy_n(old.data() + Count(i) * old_stride, old_stride,
                                reinterpret_cast<unsigned char *>(&pod_animals[i]));
            }
            array(rinds);
        }
        if (developing) {
            read(&neuro, sizeof(neuro));
            array(cortices);
            if (neuro.open > 1 || (neuro.cohort_controls & 3) > 2 || (neuro.cohort_controls >> 2) > 2)
                throw std::runtime_error("invalid neural development mode");
        }
        if (articulated) {
            read(&minerals, sizeof(minerals));
            array(mineral_bed);
            array(workers);
            const auto &p = minerals.physics;
            if (p.density > 100 || p.bonded > 3 || p.agency > 1 || p.vision > 2)
                throw std::runtime_error("invalid material field laws");
        }
        if (foresight) {
            read(&tide, sizeof(tide));
            array(forecasts);
            if (!tide.period || tide.period > 65536 || tide.active > 11 || c.side < 8)
                throw std::runtime_error("invalid construction tide");
        }
        if (investigatory) {
            read(&worksite, sizeof(worksite));
            array(enquiries);
            array(patina);
            if (worksite.active > 1 || worksite.marks > 4)
                throw std::runtime_error("invalid worksite conditions");
            for (auto slot : patina) {
                if (slot.tick != ~0u && slot.tick > header.tick)
                    throw std::runtime_error("worksite trace from a later tide");
                for (Word j = 0; j < 4; ++j) {
                    Word mark = (slot.events >> (8 * j)) & 255;
                    if (mark && !(mark & 128))
                        throw std::runtime_error("invalid physical imprint");
                }
            }
        }
        if (sedimentary) {
            read(&seam, sizeof(seam));
            array(macromolecules);
            Count remaining = 0;
            for (auto amount : macromolecules)
                remaining += amount;
            if (seam.pattern > 3 || seam.guidance > 1 ||
                seam.opening + seam.totals.adsorbed != remaining + seam.totals.released)
                throw std::runtime_error("invalid sediment carbon stock");
        }
        if (saline) {
            read(&solution, sizeof(solution));
            array(salts);
            array(saline_animals);
            if (solution.species < 2 || solution.species > 5 || solution.flowing > 7)
                throw std::runtime_error("invalid dissolved material conditions");
            auto inventory = salt_inventory();
            if (!std::equal(inventory.begin(), inventory.end(), solution.stock))
                throw std::runtime_error("unbalanced dissolved material");
            for (auto q : salts)
                if (q.ions >> (6 * solution.species))
                    throw std::runtime_error("unrecognized solute");
            for (auto &a : saline_animals)
                if ((a.memory.low | a.memory.high) & ~a.genes.active)
                    throw std::runtime_error("inactive brine neuron holds a trace");
        }
        if (calling) {
            read(&reeds, sizeof(reeds));
            array(apertures);
            array(callers);
            if (!std::has_single_bit(reeds.bands) || reeds.bands > 4 || reeds.wire > 3 || (reeds.windows & 3) > 2 ||
                reeds.windows > 255 || !std::has_single_bit(reeds.pulse) || reeds.pulse > 65536 ||
                reeds.total.inquiries > reeds.total.pairs || reeds.total.replies > reeds.total.pairs ||
                reeds.total.strokes > reeds.total.pairs || reeds.total.consumed > reeds.total.released ||
                reeds.total.native_work < reeds.total.strokes ||
                reeds.total.native_work > reeds.total.strokes + 2 * reeds.total.pairs)
                throw std::runtime_error("invalid resonant pore record");
        }
        if (overhearing) {
            read(&audience, sizeof(audience));
            array(acoustics);
            if (audience.coupling > 5 || audience.access > 3 || audience.total.paired > audience.total.heard ||
                audience.total.young > audience.total.heard)
                throw std::runtime_error("invalid audience history");
            for (auto e : acoustics)
                if ((e.tick > header.tick && (0u - e.tick) > 32768) || (e.pulse && !(e.pulse >> 12)))
                    throw std::runtime_error("invalid acoustic exposure");
        }
        if (pottery) {
            workshop = {};
            read(&workshop, workshop_bytes);
            array(potters);
            array(ware);
            array(cargo);
            if (bool(workshop.use & press::installed) != pressing || bool(workshop.use & press::feel_set) != feeling ||
                workshop.use > (feeling    ? 524287u
                                : pressing ? 32767u
                                           : 127u) ||
                ((workshop.use & press::tidal) && !(workshop.use & press::bedded)) ||
                workshop.book.press.predating > workshop.book.press.studied ||
                workshop.book.press.heat != workshop.book.press.strokes + workshop.book.press.probe_heat ||
                workshop.book.press.rims > workshop.book.press.strokes ||
                workshop.book.press.open > workshop.book.press.wet ||
                workshop.book.press.wet > workshop.book.press.strokes ||
                workshop.book.press.printed > workshop.book.press.strokes ||
                workshop.book.press.eaten > workshop.book.press.released ||
                workshop.book.press.recast > workshop.book.press.printed || (workshop.material & 3) > 2 ||
                (workshop.material >> 2) > 2 || workshop.book.old > workshop.book.recovered ||
                workshop.book.recovered > workshop.book.taps)
                throw std::runtime_error("invalid clay workshop");
            for (Word i = 0; i < sites(); ++i)
                if (!clay::valid(potters[i]) || !clay::valid(ware[i], header.tick) ||
                    !clay::valid(cargo[i], header.tick) || (clay::size(ware[i]) && !mineral_bed[i].faces) ||
                    (clay::size(cargo[i]) && !workers[i].hand.faces))
                    throw std::runtime_error("invalid clay object or associative assembly");
        }
        if (ranging) {
            read(&recess,sizeof(recess));array(hollows);array(wanderers);array(bearings);array(sightings);
            Count stock=0;
            for (Word id=0;id<sites();++id) {
                if (!hollow::viable(wanderers[id],recess.slots) || !hollow::viable(hollows[id]))
                    throw std::runtime_error("invalid hollow or spatial cortex");
                stock+=hollows[id].carbon;
                for (Word j=0;j<recess.slots;++j) {
                    Word mark=bearings[Count(j)*sites()+id];
                    if (!hollow::valid_mark(mark) || (j>=wanderers[id].used && mark))
                        throw std::runtime_error("invalid spatial memory cell");
                }
            }
            if (recess.opening+recess.book.settled+recess.book.placed!=stock+recess.book.released ||
                recess.book.eaten>recess.book.released || std::ranges::any_of(sightings,[&](Word p){return p==0x80000000u || (p&0x7fffffffu)>sites();}))
                throw std::runtime_error("invalid hollow carbon or survey record");
        }
        if(appraising){
            read(&economics,sizeof(economics));array(economies);array(treads);
            const auto &b=economics.ledger;
            Count compressed=0;for(Word edge:treads)compressed+=(edge&15)+(edge>>4);
            if(b.pressed!=compressed+b.washed)throw std::runtime_error("unbalanced trail compaction");
            if(economics.mode&~(32767u|(127u<<16)|effort::appraisal|effort::foreign_surface) || b.settled>b.offers || b.trips+b.stays!=b.offers || b.credited>b.settled ||
               b.saved+b.price!=b.fresh_price || !std::ranges::all_of(economies,effort::viable))
                throw std::runtime_error("invalid effort memory or footing ledger");
        }
        if(linking){
            read(&millwork,sizeof(millwork));array(makers);array(mechanisms);array(seals);const auto &b=millwork.totals;
            if(b.eaten+b.loose!=b.flow || b.taps+2*b.loops_in+b.loops_out!=b.heat || b.invented>b.attempts || b.solved>b.invented || b.older_carbon>b.other_carbon || b.other_carbon>b.eaten)
                throw std::runtime_error("unbalanced linkage works");
            for(Word j=0;j<sites();++j){
                if(!linkage::valid(makers[j],millwork.arena))throw std::runtime_error("invalid learned material contexts");
                for(Word c=0;c<millwork.arena;++c)if(!linkage::key_at(makers[j],c)){
                    const auto &t=mechanisms[Count(c)*sites()+j];linkage::Tissue blank{};
                    if(std::memcmp(&t,&blank,sizeof(t)))throw std::runtime_error("unowned linkage contacts");
                }
            }
            for(auto &s:seals)if((s.maker>sites() && s.maker!=~0u) || (s.dated>header.tick && s.dated!=~0u) ||
                  (!s.maker && (s.born||s.dated||s.stamp||s.pattern)))throw std::runtime_error("invalid workshop provenance");
        }
        if(practising){
            read(&learning,sizeof(learning));array(pupils);array(memories);
            if(signed_works){read(&legacy_work,sizeof(legacy_work));array(strands);
                if(legacy_work.older_food>legacy_work.other_food||legacy_work.other_food>legacy_work.eaten||legacy_work.eaten>legacy_work.transport||
                    legacy_work.older_events>legacy_work.other_events||legacy_work.necessary>legacy_work.channel_open||legacy_work.immediate>legacy_work.installed)throw std::runtime_error("invalid attributed catchment");
                for(auto r:strands)if(r.who && ((r.who>sites()&&r.who!=~0u)||r.edge>15||!((r.edge>>2)>(r.edge&3))||
                    (r.date>header.tick&&r.date!=~0u)))throw std::runtime_error("invalid standing strand");
            }else{legacy_work={};std::fill(strands.begin(),strands.end(),practice::Mark{});}
            const auto &book=learning.book;
            if(book.direct_exact>book.direct_known || book.direct_known>book.strokes || book.alignments>book.proposals || book.fittings>book.proposals || book.proposals>book.searches ||
               book.work<book.strokes+2*book.placed+book.removed+book.collections)throw std::runtime_error("invalid practice accounts");
            for(Word i=0;i<sites();++i){
                auto &m=pupils[i];if(!practice::valid(m,learning.arena)||(m.focus>>12)>sites()/4)throw std::runtime_error("invalid private practice state");
                for(Word j=0;j<learning.arena;++j){auto &c=memories[Count(j)*sites()+i];if(!practice::valid(c,j<practice::size(m)))throw std::runtime_error("invalid material hypotheses");}
            }
        }
        if(wicking){
            read(&wicking_state,sizeof(wicking_state));array(liquid);const auto &b=wicking_state.book;
            Word period=wicking_state.mode>>8;
            if((wicking_state.mode&252u)||(period&&(!std::has_single_bit(period)||period>header.configuration.side))||b.transverse_food>b.carbon||b.worked_food>b.carbon||b.later_hops>b.tracer_moves||b.bridged_hops>b.tracer_moves)
                throw std::runtime_error("invalid capillary flow");
            std::vector<bool> origin(sites());Word marked=0;
            for(Word p:liquid){if(!wicks::valid(p,sites()))throw std::runtime_error("invalid capillary parcel");
                if(p&wicks::present){auto id=wicks::origin(p);if(origin[id])throw std::runtime_error("duplicated capillary tracer");origin[id]=true;++marked;}}
            if(marked!=b.tracers)throw std::runtime_error("unbalanced capillary tracers");
        }
        if(reeding){
            read(&rush,sizeof(rush));array(cutters);array(folds);const auto &r=rush.book;reed_audit={};
            if(rush.version==2){read(&reed_audit,sizeof(reed_audit));auto &v=reed_audit;
                if(v.better+v.worse>v.altered||v.marked>v.better||v.lifted_carbon>v.carbon||v.lifted_water>v.water||v.swallowed>v.carbon||v.inferred_better+v.inferred_worse>v.inferred_changes)
                    throw std::runtime_error("invalid observed-work attribution");}
            if((rush.version!=1&&rush.version!=2) || (rush.mode&~32767u) || !rush.period || rush.period>256 || r.accurate>r.predicted ||
               r.social_use>r.recalled || r.probes>r.opportunities || r.work<r.folds+r.probes || r.lifted_food>r.thumped_carbon ||
               r.swallowed>r.thumped_carbon || r.lifted_water>r.thumped_water)throw std::runtime_error("invalid reed accounts");
            for(Word i=0;i<sites();++i){
                auto a=runnel::fold_at(folds[i],0),b=runnel::fold_at(folds[i],1);
                if((runnel::traced(folds[i],0)&&!a)||(runnel::traced(folds[i],1)&&!b)||a>4||b>4||(a&&!land[i].fibre_east)||(b&&!land[i].fibre_south)||!runnel::valid(cutters[i]))
                    throw std::runtime_error("damaged reed or unowned flex contacts");
            }
        }
        if(rattling) {
            read(&backwater,sizeof(backwater));array(listeners);
            queries={};mesh={};if(backwater.format>=2)read(&queries,sizeof(queries));if(backwater.format>=3)read(&mesh,sizeof(mesh));
            const auto &r=backwater.total;
            if((backwater.mode&~(backwater.format==3?255u:backwater.format==2?127u:31u)) || backwater.phase>3 || queries.changed>queries.solicited || queries.replies>queries.solicited || queries.solicited>r.pulls ||
               r.mixed>r.voiced || r.voiced>r.pulls || r.edits>r.settled ||
               r.switches>r.settled || r.resident_changes>r.voiced || r.built_changes>r.voiced ||
               mesh.pinched>mesh.opposed || mesh.opposed>r.pulls || mesh.to_gripper>r.return_food || mesh.to_worker>r.food || r.conducted>r.pulls || r.cost!=r.calls+r.pulls ||
               !std::all_of(listeners.begin(),listeners.end(),rattle::valid))
                throw std::runtime_error("invalid bank acoustics or private sensory trace");
        }
        flute_words.clear();
        stories.clear();trails.clear();cap_speakers.clear();compound_residents.clear();audible_residents.clear();appetites.clear();cap_histories.clear();cap_memories.clear();cap_waves.clear();cap_law={};
        if(braiding){
            read(&loom,sizeof(loom));
            if(loom.version>=14){
                read(&cap_law,sizeof(cap_law));Word named=0;read(&named,4);if(named!=hydraulic_names)throw std::runtime_error("changed capillary speakers");
                cap_speakers.resize(named);if(loom.version==14){for(auto &p:cap_speakers)read(&p,offsetof(CapillarySpeaker,speech)+offsetof(capillary::Mouth,manual));}else array(cap_speakers);
                std::vector<bool> used(named?sites():0);
                for(auto &p:cap_speakers){
                    if(p.slot>=sites()||!bodies[p.slot].energy||bodies[p.slot].born!=p.born||used[p.slot]||!capillary::valid(p.speech))
                        throw std::runtime_error("damaged capillary speaker");
                    used[p.slot]=true;
                }
                cap_memories.resize(sites());array(cap_memories);cap_waves.resize(3*((sites()+31)/32));array(cap_waves);
                if(loom.version>=17){cap_histories.resize(sites());array(cap_histories);
                    if(!std::ranges::all_of(cap_histories,[](auto h){return capillary::valid(h);}))throw std::runtime_error("invalid remembered material context");
                }
                if(loom.version>=20){
                    appetites.resize(sites());array(appetites);
                    for(Word slot=0;slot<sites();++slot)if(appetites[slot]&&bodies[slot].energy){
                        Word gene=genomes[slot].pigments[2];
                        if(capillary::pack_signs(capillary::expand_signs(appetites[slot],gene),gene)!=appetites[slot])
                            throw std::runtime_error("invalid inhabited chemical contact projection");
                    }
                }
                if(loom.version>=21){
                    Word n=0;read(&n,4);if(n>sites())throw std::runtime_error("invalid two-port records");
                    compound_residents.resize(n);array(compound_residents);std::vector<bool> used(n?sites():0);
                    for(auto &p:compound_residents){
                        if(p.slot>=sites()||used[p.slot]||!bodies[p.slot].energy||bodies[p.slot].born!=p.born)throw std::runtime_error("invalid two-port resident identity");
                        used[p.slot]=true;
                    }
                }
                if(loom.version>=22){
                    Word n=0;read(&n,4);if(n>sites())throw std::runtime_error("invalid acoustic signallers");
                    audible_residents.resize(n);if(loom.version==22){for(auto &p:audible_residents)read(&p,initial_audible);}else array(audible_residents);std::vector<bool> used(n?sites():0);
                    for(auto &p:audible_residents){if(p.slot>=sites()||used[p.slot]||!bodies[p.slot].energy||bodies[p.slot].born!=p.born||!capillary::valid(p.speech))throw std::runtime_error("invalid acoustic signalling resident");used[p.slot]=true;}
                }
                if(!std::ranges::all_of(cap_memories,[](auto m){return capillary::valid(m);}))throw std::runtime_error("damaged capillary memories");
            }
            if (loom.version>=10) {
                Word size; read(&size,4);
                if (size!=story_count) throw std::runtime_error("changed narrative extent");
                stories.resize(size);
                if(loom.version==10) {for(auto &story:stories)read(&story,first_story);}
                else array(stories);
                std::vector<bool> occupied(size?sites():0);
                for (const auto &b:stories) {
                    if (b.slot>=sites() || !bodies[b.slot].energy || bodies[b.slot].born!=b.born || occupied[b.slot] || !utterance::valid(b.memory))
                        throw std::runtime_error("invalid resident narrative tissue");
                    occupied[b.slot]=true;
                }
                trails.resize(sites());array(trails);
                if (!std::ranges::all_of(trails,[](auto t){return t.valid();})) throw std::runtime_error("invalid retained visual history");
            }
            array(riggers);array(floats);float_imports=float_exports=0;
            if(loom.version>=2){read(&float_imports,sizeof(Count));read(&float_exports,sizeof(Count));}
            gate_phase=0;sluice_book={};
            if(loom.version>=3){read(&gate_phase,sizeof(gate_phase));read(&sluice_book,sizeof(sluice_book));array(sluice_contacts);
                auto &g=sluice_book;if(gate_phase>3 || g.crossings>g.wet || g.picked>g.flux || g.writes>g.observed ||
                    g.predicted_requests>g.requests || g.understood>g.responses || g.syllables<g.requests ||
                    !std::all_of(sluice_contacts.begin(),sluice_contacts.end(),braid::valid_gate))
                    throw std::runtime_error("damaged pressure contacts or canal accounting");
            }
            sediment_book={};
            if(loom.version>=4){
                read(&sediment_book,sizeof(sediment_book));auto &s=sediment_book;
                if(s.relieved>s.wet||s.opened>s.relieved||s.closed>s.relieved-s.opened||s.gained<s.opened||s.lost<s.closed||
                   s.lodged>s.wet||s.scoured>s.wet-s.lodged||s.turned>s.lodged)throw std::runtime_error("invalid mineral flow records");
            }
            drift_water={};
            if(loom.version>=7){
                read(&drift_water,sizeof(drift_water));
                if(!drift::valid(drift_water.law) || ((drift_water.law.ports&drift::sight) && !recording_floats) || !drift::balanced(drift_water.total))throw std::runtime_error("invalid drifting-food accounts");
            }
            flute_words.resize(flotilla_names);array(flute_words);
            if(loom.version<7 && std::ranges::any_of(riggers,[](const braid::Mind &m){return braid::drift_tag(m.lived)!=0;}))
                throw std::runtime_error("unexpected drift lineage in a pre-drift population");
            auto &a=loom.book;Count stock=0;for(auto f:floats)stock+=braid::mass(f);
            if(a.trapped+float_imports<a.trapped || a.meals+float_exports<a.meals || a.meals+float_exports>a.trapped+float_imports || a.trapped+float_imports-a.meals-float_exports!=stock || a.engaged>a.strokes || a.spent<a.strokes ||
               a.transferred<2*a.engaged || a.across>a.carried || a.traced_food>a.others || a.others>a.meals ||
               a.soma_moves>a.soma_events || a.ear_moves>a.ear_events || !std::all_of(riggers.begin(),riggers.end(),braid::valid))
                throw std::runtime_error("invalid float tissue or carbon account");
            std::vector<bool> occupied(sites());
            for(auto &s:flute_words){
                if(s.slot>=sites()||!bodies[s.slot].energy||bodies[s.slot].born!=s.born||occupied[s.slot]||!braid::valid_words(s.lexicon))
                    throw std::runtime_error("invalid resident acoustic receptors");
                occupied[s.slot]=true;
            }
        }
        if(recording_floats){
            array(float_events);
            if(!std::all_of(float_events.begin(),float_events.end(),braid::valid_episode))throw std::runtime_error("invalid float episode");
        }else float_events.clear();
        narrators.clear();
        if(expressed){
            Word count=0;read(&count,4);if(count!=speakers_count)throw std::runtime_error("invalid event speaker extent");
            narrators.resize(count);array(narrators);std::vector<bool> taken(sites());
            for(auto &p:narrators){
                if(p.slot>=sites()||taken[p.slot]||!bodies[p.slot].energy||bodies[p.slot].born!=p.born||!braid::valid_narration(p.mind))
                    throw std::runtime_error("invalid resident event speaker");
                taken[p.slot]=true;
            }
        }
        place_words.clear();
        if(named_places) {
            Word count=0;read(&count,4);if(count!=names)throw std::runtime_error("invalid spatial word extent");
            place_words.resize(count);array(place_words);std::vector<bool> taken(sites());
            for(auto &p:place_words) {
                if(p.slot>=sites() || taken[p.slot] || !bodies[p.slot].energy || bodies[p.slot].born!=p.born || p.reserved || !siphon::valid(p.words))
                    throw std::runtime_error("invalid spatial speaker");
                taken[p.slot]=true;
            }
        }
        captions.clear();
        if (annotated) {
            Word n = 0;
            read(&n, 4);
            if (n != annotations)
                throw std::runtime_error("invalid annotation extent");
            captions.resize(n);
            array(captions);
            std::vector<bool> found(sites());
            for (auto &a : captions) {
                if (a.slot >= sites() || found[a.slot] || !bodies[a.slot].energy || a.born != bodies[a.slot].born ||
                    !siphon::valid(a.words))
                    throw std::runtime_error("invalid owner of acoustic vocabulary");
                found[a.slot] = true;
            }
        }
        spoken.clear();
        if (language) {
            Word count = 0;
            read(&count, sizeof(count));
            if (!count || count != speakers)
                throw std::runtime_error("invalid number of resident lexicons");
            spoken.resize(count);
            array(spoken);
            std::vector<bool> occupied(sites());
            for (const auto &r : spoken) {
                if (r.slot >= sites() || occupied[r.slot] || !bodies[r.slot].energy || bodies[r.slot].born != r.born)
                    throw std::runtime_error("lexicon belongs to a missing resident");
                occupied[r.slot] = true;
                grounding::validate(r.vocabulary);
                material::validate(r.questions);
            }
        }
        if (version == 1)
            for (auto &a : bodies) {
                a.fibre = 0;
                a.cohort = 0;
            }
        Count check;
        if (!in.read(reinterpret_cast<char *>(&check), sizeof(check)) || check != hash)
            throw std::runtime_error("floodplain checksum mismatch");
        std::vector<bool> found(sites());
        Balance total;
        Count alive = 0, pending = 0, cells = 0, pieces = 0, charge = 0, terminal[3]{};
        for (Word i = 0; i < sites(); ++i) {
            auto slot = slots[i];
            if (slot >= sites() || found[slot])
                throw std::runtime_error("invalid floodplain occupancy");
            found[slot] = true;
            auto a = bodies[slot];
            auto p = land[i];
            auto shape = a.body;
            if (!genomes[slot].module_count || genomes[slot].module_count > 4 ||
                Word(a.body_origin >> 32) > header.tick || Word(a.brain_origin >> 32) > header.tick ||
                a.founder >= sites() || a.born > header.tick || a.cohort > 2 || a.heading > 3 || a.action > 7 ||
                a.signal > call_types || a.heard > call_types || (shape.shape >> body_sites) ||
                !(shape.shape & (1u << center)) || ((shape.pores | shape.eyes | shape.limbs) & ~shape.shape))
                throw std::runtime_error("invalid floodplain anatomy");
            if (enclosed) {
                if (rinds[i].winding > 3)
                    throw std::runtime_error("invalid pod surface");
                const auto &layer = pod_animals[slot];
                if (layer.frame.active > 3 || (layer.frame.anchors >> 16) || (layer.frame.certainty >> 16))
                    throw std::runtime_error("invalid motor frame");
                for (auto &row : layer.brain.support)
                    for (auto value : row)
                        if (value < -63 || value > 63)
                            throw std::runtime_error("invalid temporal contacts");
                Word mass = 0;
                for (Word k = 0; k < 8; ++k)
                    mass += layer.gut.food[0][k] + layer.gut.food[1][k] + layer.gut.food[2][k] + layer.gut.chaff[k];
                if (mass > 120 || (!a.energy && mass))
                    throw std::runtime_error("invalid retained pod stock");
                total.energy += mass;
                pending += mass;
            }
            if (developing) {
                auto cortex = cortices[slot];
                if (!viable_cortex(cortex, c.tiles))
                    throw std::runtime_error("invalid spiking population size");
                if (a.energy)
                    cells += 32 * cortex.active;
            }
            if (articulated) {
                auto h = workers[slot].hand, m = mineral_bed[i];
                if (!material::valid(m) || !material::valid(h) || (workers[slot].recent >> 17))
                    throw std::runtime_error("invalid material tissue or grain");
                for (Word k = 0; k < 3; ++k)
                    terminal[k] += material::inventory(m, k + 1) + material::inventory(h, k + 1);
                pieces += material::quantity(m) + material::quantity(h);
                charge += m.charge + h.charge;
                total.energy += m.charge + h.charge;
            }
            total.water += p.water;
            total.clay += p.grains + p.east + p.south + a.load;
            if (sedimentary)
                total.energy += macromolecules[i];
            if (ranging) total.energy+=hollows[i].carbon;
            if (braiding) total.energy+=braid::mass(floats[i]);
            total.energy += p.leaf + p.fruit + p.litter + a.energy + p.fibre_east + p.fibre_south + a.fibre;
            alive += a.energy != 0;
        }
        if (total.water != header.initial.water || total.clay != header.initial.clay ||
            total.energy + header.account.heat != header.initial.energy + header.account.light ||
            alive + header.account.deaths != header.founders + header.account.births)
            throw std::runtime_error("invalid floodplain conservation ledger");
        if (articulated && (minerals.pieces != pieces ||
                            minerals.charge + minerals.account.light + (investigatory ? worksite.totals.pumped : 0) !=
                                charge + minerals.account.reactions + minerals.account.thermal ||
                            !std::equal(terminal, terminal + 3, minerals.terminals)))
            throw std::runtime_error("invalid conserved material stocks");
        auto &g = neuro.account;
        if (developing && (neuro.initial + g.births + g.recruited != cells + g.deaths + g.retired ||
                           g.investment != g.recruited / 16))
            throw std::runtime_error("invalid neural development ledger");
        if (enclosed &&
            Count(crop.entry_mass) + crop.account.swallowed != crop.account.returned + crop.account.waste + pending)
            throw std::runtime_error("invalid crop conservation ledger");
        journal::checkpoint(path, hash, false);
    }
};
} // namespace axion::life::flood
