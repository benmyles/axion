# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v53 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection with physiologically bounded dendritic & somatic thresholds/gains (`soma_thresh 560..880`, `apic_thresh 220..420`, `bac_gain 180..560`), **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** with **Hasselmo (2006) active cholinergic new-episode retirement & fresh `L3` distractor shielding** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24–31t ACh-gated Slot 2 protecting `cs_u = 6..7` fresh memories against distractors, `L4` 24–31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **calibrated Frontopolar BA10 `L5`** slow executive goal hub at `2.5×` STF / `120 pA` `I_CAN` / `b_g = 3.70` with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and 3-step `+DA` phrase reset), **Kuffler (1953) center-surround retinal background adaptation & symmetric Warm-ON / Cool-ON opponent ganglion channels** with collision-free rank-ordered `eye_rank & 3` / `ear_rank & 3` sensory mosaics, **separated low-threshold VIP+/Hasselmo sensory-only ACh (`>12/20`) vs high-threshold multimodal surprise `phasic_ach` (`>44`)**, **stable `L1` thalamocortical integration** (`soma_leak_shift = 3`), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, **1–5t Sperry-orthogonal `Uva→HVC` vocal efference copy** on `L3`, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with whole-fold multi-unit credit assignment & exact zero-sum 2-/3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity + **`TK_CHEM` contact gustation & olfactory toxin rejection**, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful per-neuron Vector-FA Burstprop, honest energy-conserving tissue repair, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v53** (`examples/genesis-v53.gen`)

- **Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **272.98 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.48 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **655.33 µs/step (1,600.08 Mcells/s — new throughput peak!)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **229.43 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`55.0 ticks/s`** at `1,024` deep-cortex organisms on a `1,048,576`-cell `1024×1024` ocean)
- **Crushing `7/7` Honest `1024×1024` Physical Ablation Clean Sweep (`350,388.3 ATP` Intact, `-20.1%` No-Apical BAC, `-20.8%` No-Neuromod — ZERO Fake Guards, 100% STD/STF Parity!):**
  - **`Evolved Intact`: `350,388.3 ATP`** (`962 alive`, `4,751 births`, `17.24 mean ATP`, `8.37` mean layers, `376 prey`, `138,710 synth`, `397.6M` STDP edits, `33,339` rewires)
  - `Axonal Delays Flattened (Uniform 3t)`: `346,593.7 ATP` (**`-1.1%`**)
  - `Plasticity Frozen (No Within-Life STDP)`: `336,329.7 ATP` (**`-4.0%`**, `830 alive`, `302` prey)
  - `Apical Dendrites Silenced (No BAC)`: `279,981.7 ATP` (**`-20.1%`** — **`3.1×` v52's `-6.4%`!**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `277,561.7 ATP` (**`-20.8%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `151,826.6 ATP` (**`-56.7%`**, `423 alive`, `100` prey)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `123,072.6 ATP` (**`-64.9%`**, `581 alive`, `52` prey)
  - `Unselected Drift Control`: **`0.0 ATP` (`-100.0%`, `0 alive / 3 births`!)**
- **Thriving `1024×1024` Trisolaris-16 Ecosystem, `2.1×` Wild Language Surge (`0.2527` Bits MI!) & Record Tool/Shelter Civilization (`v53`, 40,000 ticks: `8k E1 + 32k E2`!):**
  - **Sustained High-Population Resilience:** **`alive = 708`** at tick 32,000 (`11×` v52's late-storm `65`!; `1,024` at 8k E1), **`115` evolved morphs**, **`125,419` combined natural births** (`90,902` E2 + `34,517` E1), **`16.18B` combined STDP edits** (`9.76B` E2), **`1.64M` rewires**, **`100%` bilateral symmetry** on champions `#0..#3`, **`20.6%` canonical Dale GABAergic inhibition** (`7.62` mean layers / `999.4` mean neurons, `max = 11L`, morphs up to **`10L / 1,787` neurons** live in `converse`)!
  - **Record Tool-Crafting, Shelter Engineering & Clean 3-Word Wild Semantics (`0.2527` bits E2 MI = `2.1×` v52!):** Across **`12.35 MILLION` combined vocalizations** (`9.14M` E2 + `3.21M` E1, **`5.80M` peer replies**, **`130k` mimics**, **`5.90B` juvenile imprints**), all 3 wild words sit cleanly on the diagonal (`v0` Food `135.9k`, `v1` Predator `1.97M`, `v2` Shelter/Toxin `3.00M`), driving **`3,051` shelters (`9.6×` v52!)**, **`1,983` heat-tempered tools (`2.1×` v52!)**, **`230` cached granary feasts**, **`2,013` evasive prey**, **`1,189` pods cracked (`432` coop, `353` vocal duets)**, **`46` apex predators slain**, and **`4,310` kin food shares**!
- **All-Time World Records in Seasonal Reversal BAC (`+43.2%`), Spoken Vocal Q&A (`68.0%`, `sep 0.772`), & Zero-DA Cultural Peer Teaching Separation (`0.367`) (`examples/genesis-v53.gen`):**
  - **All-Time World Record `1024×1024` Seasonal Reversal Learning (`Assay 1`):** **`239,190.3 ATP` intact** (`+44k` over v52!) vs `199,212.6` frozen (**`+20.1%` STDP**) vs `167,035.5` no-apical (**`+43.2%` BAC — all-time record! `alive = 478`!**)
  - **All-Time Official Record Spoken Vocal Q&A Grounding (`Assay 10`):** **`68.0%` (`post_sep = 0.772`, `q1: v2 0.49 > v0 0.45`, `q2: v0 0.85 > v2 0.11`, `rev 54.7%`, `rev_sep 0.426` — `+21.6 pts` over v52!)**
  - **All-Time Record Zero-DA Cultural Peer Teaching Student Separation (`Assay 11`):** **`50.3%` solo (`32.0% -> 50.3%`, `post_stu_sep = 0.367` — all-time record! `tutor 69.0%`, `13,732` peer replies)**
  - **Working Memory (`Assay 2`):** **`1,521.67×` (`base_spk/t = 1.2`, `+0.63` tiles East)** · **Episodic Delayed Recall (`Assay 12`):** **`16.5%` recall hit (`L2_cue 10.30×`)** · **Predictive MMN (`Assay 6`):** **`+5.2%` oddball / `+15.5%` omission** · **Zero-Shot Generalization (`Assay 18` `Gen18`):** **`49.2%` voc / `48.4%` joint (`+0.050` mot_sep)** · **Episodic Distractor (`Assay 19` `Epi19`):** **`47.7%`** · **2-Syl (`Assay 15`):** **`38.3%` (`syl2 81.2%`)** · **3-Concept (`Assay 14`):** **`35.9%` (`xmodal 35.4%`)** · **Trans20 (`Assay 20`):** **`42.2%`** · **Wild Agency (`Assay 3`):** **`107` prey, `55` pods, `42` tempered tools, `15` duets, `9` shelters, `1` apex kill, `0.1126b` MI**. *(Honest note: in `seed 535` E2, shelter-protected nocturnal torpor allowed compact `4–5L / ear=2` lineages alongside `9–10L` morphs [`mean_areas = 7.62`, `ear = 2.08`], limiting `Cmd16` [`9.4%`] and `Trans20`/`3-Syl` on `v53.gen`, whereas on `16L 4E/5A` `v52.gen` the v53 engine itself achieves `+31.1%` STDP (`214k` ATP), `Epi19 64.3% (62/67)`, `Cmd16 83.9%`, `Ctx13 50.9% (sep 0.303)`, `3-Concept 41.0%`, `Trans20 71.4%`, and `base_spk/t = 0.9` — making `14–16L + ear>=4` selection parity the #1 focus for v54!)*
  - **4-Agent `9–10L` Multi-Agent Spiking Dialogue (`converse`):** **`A0: 9L/1,141n`, `A1..A3: 10L/1,662–1,787n` (`Eye 3, Ear 2, Syrinx 4, Muscle 9`)** across 12 live turns (**`104` peer replies**, **`27,749` live STDP edits**), keeping `L5..L8` silent (`0`) on routine turns (`v0`/`v1`/`ask_01`/`wait_dark`) and igniting cleanly up through **`L8`** on `ask_10`, `ctx_food`, `teach_p3_x`, and `ask_p3_x` — zero LLMs!
- **Key Biophysical, Cholinergic, Retinal & Ecological Breakthroughs Shipped in v53:**
  - **Kuffler (1953) / Barlow (1961) Center-Surround Retinal Background Adaptation (`src/genesis_brain.cuh`):** Quieted uniform featureless daylight wallpaper (`opt_f` neutral floor `0.14 -> 0.04`, `rcue` warm onset `lw - 0.50 -> lw - 0.72`) while preserving full Warm-ON / Cool-ON opponency for sunlit reefs, cool minerals, vents, and Batesian prey — cutting dark `base_spk/t` `28×` (`25.5 -> 0.9–1.2`) and unlocking `68.0%` `Q&A` (`sep 0.772`)!
  - **Separated Low-Threshold VIP+/Hasselmo ACh (`>12/20`) vs. High-Threshold Multimodal `phasic_ach` (`>44`) (`src/genesis_brain.cuh`):** Normalized sensory-only ACh for sparse orthogonal channel activation (`clamp(n_sens, 4, 6)`) while gating unsupervised ACh-STDP/Oja strictly to strong multimodal surprises (`ext_ach_q > 44`), eliminating wild-ocean Oja weight erosion and driving `+20.1%..+31.1%` STDP & `+43.2%` BAC reversal gains!
  - **Whole-Fold Area-X Vocal Credit, Calibrated BA10 `L5`, Physiological Somatic Bounds & Honest Tissue Repair (`src/genesis_brain.cuh`):** Shared Area-X RPE across all motor units of the winning vocal fold (`prev_gfold_mask & (1 << my_gfold)`), calibrated BA10 `L5` (`b_g = 3.70`, `2.5×` STF, `120 pA` `I_CAN`, `asc_sum > 165`), bounded somatic excitability (`soma_thresh 560..880`, `sg 0.55..1.25`), raised calm `TK_GLAND` `eff_ne` gate (`>24`), added energy-conserving calm tissue repair (`+0.25 hp / -0.02 atp`), cured stale `hp < 88` `vctx = 1` hijack (`hp < 68`), and symmetrized `vctx = 0` food/prey vocal RPE — lifting late-E2 survival `11×` (`65 -> 708` alive) and doubling wild MI (`0.120 -> 0.253 bits`)!

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v53.mp4`)](journal/genesis-v53.mp4)
- [Detailed Architecture, Biophysics & Roadmap (`docs/genesis.md`)](docs/genesis.md)

---

## Quick Start

Requires Linux with an NVIDIA Blackwell (`sm_120`) GPU (or update `SM` in `Makefile`), `clang++`, `g++`, Python 3 (`Pillow`), and `ffmpeg` (optional, for NVENC AV1 evolution videos).

```sh
# Build native Blackwell SM 12.0 binary
make NATIVE=1 -j

# Run fast GPU unit & parity test suite
python3 tests/genesis.py

# Run 5-kernel SM 12.0 1024x1024 benchmark observatory
./axion genesis bench --pop 1024 --world 1024

# Run the 20-assay cognitive & neuroscience probe battery on the v53 population
./axion genesis probe --load examples/genesis-v53.gen

# Run 4-agent grounded live spiking dialogue (9-10 layers / 1,141-1,787 neurons/agent)
./axion genesis converse --load examples/genesis-v53.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 535 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 535 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v53 establishing **all-time world records across `Assay 1` `1024×1024` Seasonal Reversal (`239,190.3 ATP` intact, `+43.2%` BAC & `+20.1%` STDP!), Spoken Vocal Q&A (`68.0%`, `sep 0.772`!), Zero-DA Peer Teaching Student Separation (`0.367`, `50.3%` solo / `69.0%` tutor!), `1,600.08 Mcells/s` `1024×1024` physics, and thriving Trisolaran tool/shelter civilization (`708` alive, `3,051` shelters, `1,983` tempered tools, `46` apex kills, `0.2527` bits wild MI, crushing `7/7` clean sweep with `-20.1%` no-apical!)**, our immediate next breakthroughs are:

1. **Unifying Deep `12–16L` Cortical Encephalization + Full `4E/4A/4G` Orthogonal Sensory-Vocal Evolution with `v53`'s `Q&A` & `708`-Alive Ecology (v54):**
   - Cure the nocturnal shelter-loophole that allowed compact `4–5L / ear=0..2` lineages to co-exist with `10–16L / ear=4+` minds in late E2, and upgrade multi-syllable `L3/L4/L5` cortical sequence coding — uniting **`12–16L` encephalization + `4E/4A/4G` anatomy** with **`wild MI > 0.35–0.45b`**, **`Q&A > 70–80%`**, **`Peer Teaching > 60–70%`**, **`2-Syl > 55–70%`**, **`3-Syl > 25–40%`**, **`Cmd16 > 85–95%`**, **`Epi19 > 65–75%`**, and **`Trans20 > 75–85%`** on ONE unified evolved population!
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v55):**
   - Evolve distinct geographic vocal dialects, multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world.
3. **Grounded Open-Ended Spiking Conversation (v56+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v53.gen`, `examples/genesis-v52.gen`, `examples/genesis-v51.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
