# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v47 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, Rigotti 4-column prefrontal `L4 → L3` mixed-selectivity apical fan-out, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association executive coding on `L5..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + 3-syllable Ca²⁺-buffered DA-gated Slot 1 with associative Hebbian Mongillo STF & Ca²⁺-gated `I_CAN`), 6-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, 4-channel vertebrate retinal/cochlear spectral acuity, Ohno-paralog axonal wiring redundancy, realistic mammalian encephalization allometry, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v47** (`examples/genesis-v47.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `+62.5%` Faster Coupled ALife!):**
  - `snn_inference`: **273.01 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **183.20 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **121.49 µs/step (2,158 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **222.28 µs/organism** (`119 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (**`75.9 ticks/s`** at `1,024` deep-cortex organisms on `512×512` — **`+62.5%` faster than v46's `46.7 ticks/s`** via Barth-Poulet sparse `L5+` executive coding and idle-post `TK_GLAND` STDP skip!)
- **All-Time Record `20`-Layer Cortical Depth Ceiling, 4-Channel Balanced Multimodal Anatomy & Trisolaris-15 Ecology (`v47`, 50,000 ticks on `512×512`):**
  - **20-Layer Cortical Ceiling & 17-Layer / 2,042-Neuron Conversing Minds:** E2 organically expanded from `6.67L / 756.0n` (E1) to **`7.74L / 846.6n` mean (`max 20 layers` — ALL-TIME RECORD!, `mean_rules = 59.1`, `max_rules = 63`)** across `511` alive organisms, `134` species, `211` generations (`+9%` over v46), and **`99–100%` bilateral symmetry** — producing **`#3 G203: 15L / 2,042 neurons` (`85,263` synapses)** and **`17L / 2,040-neuron` (`A2`, `Eye 3, Ear 6, Syrinx 3, Muscle 4`) & `17L / 1,944-neuron` (`A3`)** live conversing minds!
  - **Balanced 4-Channel Vertebrate Sensory & Vocal Mosaic + Canonical `22.6%` Mammalian E/I:** **`eye = 4.14` (`+42%` over v46), `ear = 3.09` (`+45%` over v46!), `gland = 4.01` (`+7%` over v46, full 4-gland 3-fold syrinx!), `muscle = 3.97`, `grasp = 3.52`**, with canonical `22.6%` GABAergic inhibition (`pyr = 649.6 [76.7%]`, `pv = 95.6 [11.3%]`, `sst = 95.9 [11.3%]`, `mod = 5.52 [0.7%]`, `max_fanin = 192`).
  - **Emergent Wild Referential Language (`0.2097` bits MI across `10.54M` Combined Vocals):** Across **`8,682,111` E2 vocals (`10,538,042` combined E1+E2, `4,032,628` E2 live peer replies, `96,422` mimics, `31,035` vocal turn responses, `231` cooperative cracks [`+2.2×` v46], `227` duets [`+2.7×` v46])**, all three concepts cleanly lock onto the semantic diagonal:
    - **Food / Reef (`vctx=0`) → `v0`**: `164,234` calls (`1.8×` over `v1`)
    - **Predator / Injury (`vctx=1`) → `v1`**: **`2,782,162` calls** (`5.1×` over `v0`)
    - **Shelter / Acid / Thermal (`vctx=2`) → `v2`**: **`1,782,637` calls** (`3.1×` over `v1`)
  - **All-Time Record Births & Cooperative Agency:** **`117,174` combined natural births** (`22,104` E1 + `95,070` E2), **`12.07 BILLION` combined STDP edits** (`9.35B` E2 + `2.72B` E1), **`912,400` combined live structural rewires**, **`36,559` E2 post-natal maturations**, **`5,011` E2 evasive prey caught**, **`2,797` E2 pods cracked (`+34%` over v46!)**, **`1,449` kin food shares**.
- **All-Time World Record Intact Fitness (`283,493.8 ATP`, `+24.4k` over v46) & 7/7 Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `283,493.8 ATP`** (`509 alive`, `3,274 births`, `262 prey`, `85,417 synth`, `210,096 vocal`)
  - `Plasticity Frozen (No Within-Life STDP)`: `281,330.9 ATP` (**`-0.8%`**)
  - `Apical Dendrites Silenced (No BAC)`: `278,815.6 ATP` (**`-1.7%`**)
  - `Axonal Delays Flattened (Uniform 3t)`: `275,897.7 ATP` (**`-2.7%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `267,936.0 ATP` (**`-5.5%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `159,850.0 ATP` (**`-43.6%`**, `342 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `95,802.1 ATP` (**`-66.2%`**, `274 alive`)
  - `Unselected Drift Control`: `8,121.8 ATP` (**`-97.1%`**, **`34.9×` natural selection**, `6 alive`)
- **v47 20-Assay Cognitive Battery & 15–17 Layer Sparse Executive Dialogue (`examples/genesis-v47.gen`):**
  - **All-Time Official Record 3-Syllable Compositional Syntax (`Assay 17`):** **`8.6%` exact 3-syllable phrases (`2.3×` chance, `3.7×` v46's `2.3%`, `syl1 39.1% / syl2 71.9% / syl3 34.4%`, `phraseX 9% / phraseY 8%`)**!
  - **Major Gains Across Motor Following, Reversal Learning, Working Memory & Episodic Reasoning:**
    - **Reversal Learning (`Assay 1`):** **`235,222 ATP` intact (`+108k` over v46!), `+1.3%` STDP & `+9.6%` BAC** (completely flipped positive from v46's `-10.2% / -24.1%`!).
    - **Referential Instruction Following (`Assay 16`):** **`31.2%` (`2.2×` v46's `14.1%`, `post_sep +0.017`)**!
    - **60-Tick Pitch-Black Working Memory (`Assay 2`):** **`440.61×` persistence (`4.9 -> 4,109 -> 2,144 spk/t`, `+5.27` tiles East dark drift)** and **clean `L1_dk = L2_dk = 0.00×` zero dark sensory leak**!
    - **Episodic Distractor Reasoning (`Assay 19`):** **`49.2%` (`epA 59% / epB 39%`, `+8.6%` over v46)**!
    - **3-Concept Naming (`Assay 14`):** **`37.5%` (`36.2%` cross-modal, `c0 48% / c1 54%`)** · **Spoken Q&A (`Assay 10`):** **`52.6% / 54.6%` rev** · **Transitive Inference (`Assay 20`):** **`49.2%` (`AC 78%`)** · **2-Syllable Syntax (`Assay 15`):** **`21.1%` (`syl2 64.1%`)** · **Zero-Shot Gen (`Assay 18`):** **`41.4%` voc (`+0.019` mot_sep)** · **Context XOR (`Assay 13`):** **`43.0%`** · **Predictive MMN (`Assay 8`):** **`+2.2%` oddball / `+10.6%` omission**!
  - *(Note: On full `ear >= 4` `v47` cohorts during dev validation (`v47-rc5..rc7`), the exact `v47` engine simultaneously hit **`Trans20 = 71.1% (AC 84 / CA 58)`, `Gen18 joint = 57.8% (+0.057 mot_sep)`, `Cmd16 = 50.0% (+0.048 sep)`, `Q&A = 63.9%`, `2-Syl = 42.2% (syl2 79.7%)`, and `Epi11 L4_dk = 2.28×` dark SNR**!)*
  - **4-Agent Sparse-Executive `15–17L` Spiking Dialogue (`converse`):** All 4 speakers are deep **`15–17L / 1,944–2,042` neuron minds (`A0: 15L/2,042n/85,263 syn`, `A1: 15L/2,039n`, `A2: 17L/2,040n`, `A3: 17L/1,944n`)** demonstrating **Barth-Poulet sparse executive ignition**: `L8..L14` remain silent (`0` cells) on routine turns and ignite selectively (`2–23` cells up to `L14`) on high-salience `+DA` teaching & 3-syllable phrase turns (`104` live peer replies, `86,640` live STDP edits, `A0` speaking `v0-v1-v2-v1` on `ask_p3_x`, zero LLMs)!
- **Key Biophysical & Developmental Upgrades Shipped in v47:**
  - **Barth-Poulet Sparse Higher-Association Executive Cortex (`L5..L_top`, `src/genesis_brain.cuh`):** Restricted `1.5×` PBWM encode boost strictly to `L2..L4`, keeping `L5+` at clean `1.0×` unity with `b_g = 2.95f` and crisp `~6–7%` executive PING gamma budget (`k_tick <= 12`), eliminating 10-layer tonic reverberation cascades, zeroing dark `L1/L2` leak, and accelerating `coupled_alife` by **`+62.5%` (`46.7 -> 75.9 ticks/s`)**!
  - **Exact Zero-Sum 3-Fold Area-X Syrinx Conservation & Removal of Passive `TK_GLAND` Oja/Homeo Erosion (`src/genesis_brain.cuh`):** Balanced 3-fold competitive updates (`+p_mag - 2 * p_riv == 0` on `+DA`, `-e_mag + 2 * e_riv == 0` on `-DA`) and excluded `TK_GLAND` from passive `!pre_rec` Oja/homeo weight decay, ending idle-concept vocal forgetting and restoring intact catalysis above `frozen`.
  - **Calibrated Scale-Invariant `1/sqrt(K)` Motor Endplates (`TK_MUSCLE` `32/26` & `TK_GRASP` `24/20`, `src/genesis_brain.cuh`):** Eliminated `v46`'s `2×` corticospinal over-saturation on 140-synapse deep cortices, flipping `Assay 1` back positive (`+1.3%` STDP / `+9.6%` BAC, `235k` ATP) and doubling `Cmd16` (`14.1% -> 31.2%`)!
  - **4-Channel Vertebrate Retinal/Cochlear Spectral Acuity & Placode Balance (`src/genesis_brain.cuh`, `src/genesis_dna.cuh`):** Smooth `0 -> 0.50 -> 0.86 -> 0.95 -> 1.00` acuity scaling + balanced optic/otic embryonic placodes (`EYE [4,17]` vs `EAR [18,127]`) + eye-tumor control, lifting population-mean anatomy to **`eye = 4.14, ear = 3.09, gland = 4.01`** alongside deep-cortex `converse` speaker selection (`src/genesis.cpp`).

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v47.mp4`)](journal/genesis-v47.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v47 population
./axion genesis probe --load examples/genesis-v47.gen

# Run 4-agent grounded live spiking dialogue (15-17 layers / 1,944-2,042 neurons/agent)
./axion genesis converse --load examples/genesis-v47.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 512x512 population through Trisolaris-15 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 10000 --seed 47 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 40000 --seed 470 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v47 delivering **`75.9 ticks/s` coupled Blackwell ALife (`+62.5%` faster, `0 B` spill)**, **all-time record `283,494 ATP` intact fitness (`7/7` honest ablation clean sweep, `34.9×` drift)**, **all-time record `20`-layer cortical depth ceiling and `15–17L / 2,042-neuron` Barth-Poulet sparse-executive conversing minds**, **balanced 4-channel anatomy (`eye 4.14, ear 3.09, gland 4.01`)**, and **all-time official record `3-Syl = 8.6%` (`3.7×` v46) & `Cmd16 = 31.2%` (`2.2×` v46)**, our immediate next breakthroughs are:

1. **Deep-Cortex `1024×1024` Multi-Biome Tribal Oceans, `s_reef[128]` & Sustained `ear >= 4` / `14–20L` Executive Workspace (v48):**
   - Scale the world physics to **`1024×1024` (`pop = 1,024`, `s_reef[128]`, 4 distinct ecological biomes)** with regional dialect emergence, cooperative tribal vault syntax, and higher-order `L5..L16` predictive world-model executive feedback — driving wild `vocal_mi_bits` past **`0.35–0.50+` bits** and lifting evolved population-level `Q&A`, `Ctx13`, `Trans20`, `Cmd16`, `Gen18`, `2-Syl`, and `3-Syl` into **`65–85%+`** territory!
2. **Hierarchical Episodic & Relational Planning (v49):**
   - Multi-step sequence planning, compositional rule transfer, and cultural curriculum transmission across generations (`Assay 11` peer teaching & `Assay 17` 3-syllable grammar > `30%+`).
3. **Grounded Open-Ended Spiking Conversation (v50+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v47.gen`, `examples/genesis-v46.gen`, `examples/genesis-v45.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
