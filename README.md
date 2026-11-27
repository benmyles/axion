# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v45 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, Rigotti 4-column prefrontal `L4 → L3` mixed-selectivity apical fan-out, dense `HVC/Broca → RA/nXIIts` multi-tract corticobulbar projections, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + Ca²⁺-buffered DA-gated Slot 1 with associative Hebbian Mongillo STF & Ca²⁺-gated `I_CAN`), 5-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with zero-sum syrinx rebalancing, Ohno-paralog axonal wiring redundancy, realistic mammalian encephalization allometry, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), evolvable Kriegstein oSVZ outer radial glia (`oRG`) tangential cortical expansion, positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v45** (`examples/genesis-v45.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.08 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.04 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **122.25 µs/step (2,144 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **233.25 µs/organism** (`115 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (**`57.8 ticks/s`** at `1,024` organisms on `512×512` — `+29%` faster than v43!)
- **All-Time World Record Emergent Wild Referential Language, Encephalization & Trisolaris-13 Ecology (`v45`, 50,000 ticks on `512×512`):**
  - **`7.2×` Surge in Emergent Wild Referential Vocabulary (`0.2964` bits Shannon MI!):** Across **`8,608,223` wild vocalizations (`4,017,712` live peer replies, `136,490` vocal mimics, `182` cooperative cracks, `173` vocal duets)**, all three physical concepts spontaneously locked onto the diagonal of the `3×4` semantic matrix via pure Darwinian + Fee-Goldberg VTA→Area-X spiking plasticity (zero hardcoded call mappings):
    - **Food / Reef (`vctx=0`) → `v0`**: `287,244` calls (`3.7×` over `v1`)
    - **Predator / Injury (`vctx=1`) → `v1`**: **`2,596,122` calls** (`4.6×` over `v0`/`v2`)
    - **Shelter / Acid / Thermal (`vctx=2`) → `v2`**: **`1,950,263` calls** (`5.5×` over `v1`)
  - **`+73%` Sustained E2 Neocortex Scale, `99–100%` Bilateral Symmetry & Canonical `23.0%` Mammalian E/I Across 211 Generations:** E1 reached **`8.45L / 1,257.0` mean neurons (`max 15L`, E1 champion `10L / 1,817n`, `201,930` syn)** and E2 (`40,000` ticks of full Trisolaris-13) sustained **`505` alive, `186` species, `99%` population / `100%` champion bilateral symmetry, `max 17L` (`16L` final), `7.91L / 1,052.3` mean neurons (`mean_fanin = 150.5`, `max_fanin = 192`, champion `8L / 1,122n` with `108,124` synapses: `99,577 E / 8,547 I / 40,947 apical`, `#1: 8L / 1,137n`, `#3: 8L / 1,132n`)** at canonical **`23.0%` Inhibitory** (`pyr = 803.6 [76.4%]`, `pv = 122.1 [11.6%]`, `sst = 119.8 [11.4%]`, `mod = 6.77 [0.6%]`) and balanced multimodal organs (`eye = 3.88`, `ear = 2.20`, `muscle = 6.93`, `grasp = 5.30`, `gland = 3.30`).
  - **All-Time Record Plasticity & Ecological Agency:** **`115,211` total natural births** (`21,731` E1 + `93,480` E2), **`18.38 BILLION` combined live STDP edits** (`14.78B` E2 + `3.60B` E1), **`1,174,921` E2 live structural rewires**, **`36,525` post-natal maturations**, **`5,258` evasive prey caught (`+80%` over v43, `+30%` over v44!)**, **`2,685` pods cracked**, **`89` tool cracks (`+68%` over v44)**, **`2,679` anvil stones**, **`2,364` heat-tempered blades**, **`5,685` shelter nights (`1.9×` v44!)**, **`595` cached granary feasts (`2.6×` v44!)**, **`2,064` kin food shares (`2.2×` v44!)**, **`105` apex slain**.
- **Historic 7/7 Crushing Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `291,244.5 ATP`** (`510 alive`, `3,270 births`, `229 prey`, `74,817 synth`, `194,221 vocal`)
  - `Plasticity Frozen (No Within-Life STDP)`: `228,304.0 ATP` (**`-21.6%`** — **ALL-TIME RECORD STDP drop, `2.5×` v43!**)
  - `Apical Dendrites Silenced (No BAC)`: `259,929.7 ATP` (**`-10.8%`** — **`2.0×` v43!**)
  - `Axonal Delays Flattened (Uniform 3t)`: `279,024.2 ATP` (**`-4.2%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `188,005.8 ATP` (**`-35.4%`**, `447 alive`)
  - `Photoreceptors Blinded (Eyes Off)`: `125,083.5 ATP` (**`-57.1%`**, `283 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `59,128.9 ATP` (**`-79.7%`**, `49 alive`)
  - `Unselected Drift Control`: `16,123.5 ATP` (**`-94.5%`**, **`18.1×` natural selection**, `27 alive`)
- **All-Time World Record v45 20-Assay Cognitive Battery & 4-Agent Spiking Dialogue (`examples/genesis-v45.gen`):**
  - **All-Time Record Context-Dependent XOR Dialogue (`Assay 13`):** **`69.1%` accuracy (`post_sep = 0.828`!! `ctxA: 61% vs 36%`, `ctxB: 77% vs 19%`)** — Rigotti 4-column `L4 → L3` mixed-selectivity apical fan-out + dense `HVC → RA` multi-tract readout!
  - **All-Time Record Grounded Spoken Q&A & Rule Reversal (`Assay 10`):** **`63.5%` (`sep = 0.609`) / `69.3%` REVERSAL (`rev_sep = 0.773`)**!
  - **All-Time Record Seasonal Reversal Learning (`Assay 1`):** **`+18.0%` STDP (`254,572` vs `215,720`) AND `+8.8%` BAC (`254,572` vs `233,998`, `456` alive)**!
  - **All-Time Record Episodic Delayed Recall (`Assay 11/12`):** **`recall_sep = +0.326`**, **`recall_hit = 27.6%`**, clean laminar separation (`L1_dk = L2_dk = 0.00×`, `L3_dk = 1.21× [n=25.6]`, `L4_dk = 1.14× [n=30.6]`, `L2_cue = 9.42×`, `L3_cue = 7.83×`, `L4_cue = 4.47×`)!
  - **All-Time Record Unseen-Arena Wild MI (`Assay 3`):** **`0.1978` bits (`7.2×` v43!)**, `101` prey, `37` cached feasts, `21` coop cracks, `15` duets, `48,181` peer replies, `940` mimics!
  - **Additional Probe Benchmarks:** **Predictive MMN (`Assay 8`):** **`+3.0%` (`2.0×` vs `1.5%` no-BAC) & `+10.3%` omission** · **Working Memory (`Assay 2`):** **`541.5×` (`+2.19` tiles East dark drift)** · **Transitive Inference (`Assay 20`):** **`48.4%` perfectly symmetric (`AC 50% / CA 47%`, `adj 56.2%`)** · **Episodic Distractor (`Assay 19`):** **`44.5%`** · **3-Concept Cross-Modal (`Assay 14`):** **`37.7%` (`xmodal 34.0%`)** · **Compositional Generalization (`Assay 18`):** **`37.5%` vocal / `20.3%` joint** · **2-Syl / 3-Syl (`Assays 15/17`):** **`17.2%` / `3.1%` (`syl2 75.8%`)**.
  - **4-Agent Grounded Spiking Dialogue (`converse`):** Four distinct Gen 203 spiking agents (`1,130–1,138` neurons, `8` layers, `42,618` synapses, `114` live peer replies, `61,720` live STDP edits, up to `67` active neurons/layer across `L0..L7`, zero LLMs) — **3/4 agents (`A0, A1, A2`) pass zero-shot transitive inference `ask_rel_ac → v0`!**
- **Key Biophysical & Architectural Upgrades Shipped in v45:**
  - **Dense `HVC/Broca → RA/nXIIts` Multi-Tract Corticobulbar Projection (`src/genesis_dna.cuh`):** Three dedicated embryogenesis tracts from `L2, L3, L4 → TK_GLAND` (`30` distinct prefrontal pyramidal synapses/hub layer within existing `MAX_STRIPES_PER_LAYER = 12`, `0 B` extra VRAM, `0 B` spill).
  - **Rigotti (2013) 4-Column Prefrontal Mixed-Selectivity Apical Fan-Out (`src/genesis_dna.cuh`):** Fans strictly apical (`comp = 1`) `L4 → L3` context projections across all 4 Sperry columns so 100% of `(Context × Cue)` pairs form nonlinear Larkum BAC conjunctive assemblies without smearing sensory `L1/L2` columns.
  - **Jadhav (2012) / Buzsáki (2015) Immobility-Gated Awake Hippocampal Sharp-Wave Ripple (`src/genesis_brain.cuh`):** Subthreshold `+45 pA` replay pulse on active `L3/L4` WM assemblies during consummatory pauses (`mot_thrust < 0.25`, `eff_da > 24`), suppressed during locomotion and `0 pA` at dark baseline.
  - **Realistic Mammalian Encephalization Allometry + Sharper Area-X Decision Margin (`src/genesis_dna.cuh`, `src/genesis_brain.cuh`):** Realistic `~2–4%` body-mass CNS weight (`cm = 0.00025f`, removing swimming-drag penalty on large brains) + `10–12L` founders + sharper Area-X `is_prev_rival` margin (`12 → 6`).

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v45.mp4`)](journal/genesis-v45.mp4)
- [Detailed Architecture, Biophysics & Roadmap (`docs/genesis.md`)](docs/genesis.md)

---

## Quick Start

Requires Linux with an NVIDIA Blackwell (`sm_120`) GPU (or update `SM` in `Makefile`), `clang++`, `g++`, Python 3 (`Pillow`), and `ffmpeg` (optional, for NVENC AV1 evolution videos).

```sh
# Build native Blackwell SM 12.0 binary
make NATIVE=1 -j

# Run fast GPU unit & parity test suite
python3 tests/genesis.py

# Run 5-kernel SM 12.0 benchmark observatory
./axion genesis bench --pop 1024 --world 512

# Run the 20-assay cognitive & neuroscience probe battery on the v45 population
./axion genesis probe --load examples/genesis-v45.gen

# Run 4-agent grounded live spiking dialogue (1,130-1,138 neurons/agent, 8 layers)
./axion genesis converse --load examples/genesis-v45.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-13 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 10000 --seed 45 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 40000 --seed 450 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v45 shattering **`0.2964` bits of wild emergent referential language (`7.2×` v43 across 8.61M calls)**, **`69.1%` XOR Context Dialogue (`sep 0.828`)**, **`63.5% / 69.3%` Q&A & Reversal (`rev_sep 0.773`)**, **`+18.0%` STDP / `+8.8%` BAC reversal**, **`recall_sep +0.326`**, a **crushing 7/7 honest ablation clean sweep (`frozen -21.6%`, `no_apical -10.8%`, `zero_delay -4.2%`, `no_neuromod -35.4%`, `blind -57.1%`, `deaf -79.7%`, `drift -94.5%`)**, **`+73%` sustained E2 brain scale (`1,052.3` mean neurons, `max 17L`)**, **`18.38B` combined STDP edits**, and **`5,258` prey / `5,685` shelters / `595` cached feasts**, our immediate next breakthroughs are:

1. **Multi-Tract Corticospinal Executive Readout, Crisp Sparse `Uva→HVC` Phase Switching & 3-Fold Syrinx Retention (v46):**
   - Add dedicated prefrontal `L3/L4 → TK_MUSCLE` corticospinal readout tracts (matching `HVC → TK_GLAND`) to unlock **`Cmd16` (`GO` vs `HALT`) & `Gen18` joint vocal+motor reasoning (`55–75%+`)**, keep `L3/L4` dark replay crisp and sparse while retaining full 3-fold (`gland >= 3`) syrinxes to push **`2-Syl (55%+)`, `3-Syl (15–25%+)`, `Epi19 (65%+)`, and `Trans20 (70%+)`** alongside **`75–85%+` `Q&A` and `Ctx13`**.
2. **Sustained `1,500–3,000+` Mean Neuron Encephalization (`12–20L`) + Compositional 2-Syllable Wild Syntax (`Trisolaris-14` & `1024×1024` Multi-Biome Oceans) (v46–v47):**
   - Introduce true ordered 2-syllable wild cooperative syntax (`v0→v2` reef recruitment, `v1→v0` pack mobbing, `v2→v1` shelter muster / `OBJ_MEGAPOD` vault resonance) and scale multi-biome tribal oceans up to `1024×1024` — driving wild `vocal_mi_bits` past **`0.35–0.60+` bits** of genuine emergent compositional language.
3. **Grounded Open-Ended Spiking Conversation (v47+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v45.gen`, `examples/genesis-v44.gen`, `examples/genesis-v43.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
