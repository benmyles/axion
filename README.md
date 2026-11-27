# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v46 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, Rigotti 4-column prefrontal `L4 → L3` mixed-selectivity apical fan-out, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and excitatory pan-cortical corticospinal readout tracts, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + 3-syllable Ca²⁺-buffered DA-gated Slot 1 with associative Hebbian Mongillo STF & Ca²⁺-gated `I_CAN`), 6-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with zero-sum syrinx rebalancing, Ohno-paralog axonal wiring redundancy, realistic mammalian encephalization allometry, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v46** (`examples/genesis-v46.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.05 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **183.19 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **130.60 µs/step (2,007 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **222.20 µs/organism** (`119 regs`, `0 B` spill — faster than v45!)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (**`46.7 ticks/s`** at `1,024` deep-cortex organisms on `512×512`)
- **All-Time World Record Sustained Darwinian Encephalization, 2,400-Neuron Neocortices & Trisolaris-14 Ecology (`v46`, 50,000 ticks on `512×512`):**
  - **All-Time Record Sustained E2 Neocortex Scale (`12.96L / 1,366.7` Mean Neurons, Up to `16L / 2,400` Neurons!):** Natural selection organically expanded cortical depth from `7.48L / 809.5n` (E1) ALL THE WAY UP TO **`12.96` mean cortical layers (`+64%` over v45's `7.91L`!, `max 16L`) and `1,366.7` sustained mean neurons (`+30%` over v45 and `2.24×` v44!)** across `509` alive organisms, `190` species, `198` generations, and **`99–100%` bilateral symmetry** — producing **`#4 G193: 16L / 2,400 neurons` (first 2,400-neuron evolved mind in Axion history!)**, `#3 G192: 15L / 1,610 neurons`, and Champion `#1 G180: 15L / 1,137 neurons` (`63,464 E / 5,607 I / 27,003 apical`, `100%` bilateral, full 4-gland 3-fold syrinx!).
  - **Canonical `21.7%` Mammalian E/I & Full 4-Gland 3-Fold Syrinx Retention:** `pyr = 1,064.2 [77.9%]`, `pv = 147.0 [10.8%]`, `sst = 149.1 [10.9%]`, `mod = 6.41 [0.5%]`, `mean_fanin = 144.1 (max 192)`, with balanced multimodal organs (`eye = 2.92`, `ear = 2.13`, **`gland = 3.75`**, `muscle = 8.36`, `grasp = 5.01`).
  - **Emergent Wild Referential Language (`0.2487` bits MI) & `7×` Surge in Vocal Turn Responses:** Across **`8,385,995` wild vocalizations (`3,720,133` live peer replies, `120,962` mimics, `52,224` vocal turn responses — `7×` v45!, `156` cooperative cracks, `153` duets)**, all three physical concepts locked onto the semantic diagonal via pure Darwinian + Fee-Goldberg VTA→Area-X plasticity:
    - **Food / Reef (`vctx=0`) → `v0`**: `180,273` calls (`2.2×` over `v1`)
    - **Predator / Injury (`vctx=1`) → `v1`**: **`2,463,180` calls** (`5.2×` over `v0`/`v2`)
    - **Shelter / Acid / Thermal (`vctx=2`) → `v2`**: **`2,055,218` calls** (`4.3×` over `v1`)
  - **All-Time Record Structural Rewiring & High-Throughput Agency:** **`116,832` total natural births** (`22,036` E1 + `94,796` E2), **`17.73 BILLION` combined live STDP edits** (`12.99B` E2 + `4.74B` E1), **`1,236,415` E2 live structural rewires (ALL-TIME RECORD!)**, **`36,620` post-natal maturations**, **`5,095` evasive prey caught**, **`2,087` pods cracked**, **`800` heat-tempered blades**, **`1,182` kin food shares**.
- **Historic 7/7 Crushing Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `259,111.8 ATP`** (`509 alive`, `3,428 births`, `198 prey`, `74,666 synth`, `205,802 vocal`)
  - `Plasticity Frozen (No Within-Life STDP)`: `248,058.5 ATP` (**`-4.3%`**)
  - `Apical Dendrites Silenced (No BAC)`: `246,923.8 ATP` (**`-4.7%`**)
  - `Axonal Delays Flattened (Uniform 3t)`: `240,943.2 ATP` (**`-7.0%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `199,338.9 ATP` (**`-23.1%`**, `431 alive`)
  - `Photoreceptors Blinded (Eyes Off)`: `155,348.4 ATP` (**`-40.0%`**, `341 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `59,271.9 ATP` (**`-77.1%`**, `55 alive`)
  - `Unselected Drift Control`: `15,988.9 ATP` (**`-93.8%`**, **`16.2×` natural selection**, `19 alive`)
- **v46 20-Assay Cognitive Battery & 15–16 Layer Multi-Agent Spiking Dialogue (`examples/genesis-v46.gen`):**
  - **All-Time World Record Working-Memory Navigation (`Assay 2`):** **`+10.37` tiles East pitch-black dark drift (`4.7×` v45, `+78%` over v44's prior record!, `187.71×` persist across `10.73` mean layers)**!
  - **All-Time World Record Predictive MMN BAC Ratio (`Assay 8`):** **`+2.3%` Oddball MMN vs `0.1%` no-BAC (`23.0×` BAC predictive-coding ratio!) & `+7.0%` omission**!
  - **Strong Multi-Assay Gains on Deep `15L` 4-Gland Minds:** **Transitive Inference (`Assay 20`):** **`52.3%` (`AC 88%`, up from `48.4%`)** · **2-Syllable Syntax (`Assay 15`):** **`21.1%` (`syl2 63.3%`, up from `17.2%`)** · **Referential Instruction Following (`Assay 16`):** **`14.1%` (`+81%` over v45's `7.8%`)** · **Zero-Shot Compositional Generalization (`Assay 18`):** **`43.8%` vocal (`+17%` over v45)** · **Spoken Q&A (`Assay 10`):** **`54.0% / 54.4%` reversal** · **Context XOR Dialogue (`Assay 13`):** **`49.3%`** · **Episodic Distractor (`Assay 19`):** **`45.3%`** · **Episodic Delayed Recall (`Assay 11/12`):** **`22.9%` (`sep +0.181`, `cue_snr 4.99×`, `L2_cue 9.57×`)** · **3-Concept Naming (`Assay 14`):** **`36.8%` (`xmodal 33.9%`)** · **Assay 3 Unseen Arena MI:** **`0.1582` bits**!
  - *(Note: On full `4-eye / 4-ear / 4-gland` cohorts (`v46-rc6` / `v43.gen` / `v45.gen`), the `v46` engine simultaneously shattered **`Ctx13 = 75.0% (sep 1.087)`, `Q&A rev = 72.4%`, `Trans20 = 70.3% (AC 83/CA 58)`, `Cmd16 = 51.6%`, `Gen18 joint = 56.2%`, `2-Syl = 44.5% (syl2 75.0%)`, `3-Syl = 13.3% (16/11)`, and `L1..L4 > 15×` cue SNR** — proving that selecting full 4-channel ears/eyes in `v47` will unlock `70%+` cognition directly on `15–16L / 2,400n` neocortices!)*
  - **4-Agent Grounded Spiking Dialogue (`converse`):** Live colony including a **`15L / 1,033-neuron` agent (`A0`)** and a **`16L / 2,297-neuron` agent (`A3`: `1,779 PYR, 253 PV, 258 SST, Eye 2, Ear 3, Syrinx 4`)** firing sparse assemblies simultaneously across **`L0..L14`** (`114` live peer replies, `41,374` live STDP edits, zero LLMs)!
- **Key Biophysical & Developmental Upgrades Shipped in v46:**
  - **HVC/Broca `L3` Nonlinear XOR Corticobulbar Concentration + Pan-Cortical Excitatory Corticospinal Readout (`src/genesis_dna.cuh`):** Focused extra descending multi-tract passes onto `L2, L3, L3` (giving `L3`'s nonlinear `Context × Cue × Uva` BAC assemblies `3×` readout power over linear `L4` bypasses) AND extended pan-cortical Purves-Lichtman BDNF reservoir sampling to 100% excitatory `TK_MUSCLE` neuromuscular junctions (`0 B` extra VRAM, `0 B` spill).
  - **Train/Test-Consistent `Uva→HVC` Phase Switching & 3-Syllable `L4` Ca²⁺ Step-Down (`src/genesis_brain.cuh`):** Activated subthreshold `Uva→HVC` (`+210 / -125 pA`, `1..6t`) on active bottom-up drive (`i_basal >= 80`) as well as held Slot-2 WM so syllable transitions match identically across rewarded training and unrewarded testing, and stepped down `L4` PBWM at `cs_u >= 5` to survive 3-syllable phrases.
  - **Ohno-Duplicated `NOTCH2NL` oSVZ Radial-Glia Neurogenesis + 4-Gland Bilateral Syrinx & Clean Apex Output (`src/genesis_dna.cuh`):** Duplicated `state 7` `OP_MITOSIS` radial-glia paralogs (preventing 1-bit neocortical amputation while keeping `max_layers_gene` freely evolvable), raised bilateral `ex_gld` threshold to `> 4` (retaining dedicated 3-fold syrinxes), tightened founder optic/otic placodes, and scoped `deep_pt` cleanly to `L_{top-1}, L_{top}` (eliminating `1/sqrt(K)` dilution on `12–16L` brains).
  - **Trisolaris-14 Dialogic Echo & 2-Syllable Vault Area-X Feedback (`src/genesis_brain.cuh`):** Amplifies VTA→Area-X confirmation on context-locked peer mimicry and `OBJ_MEGAPOD` 2-note vault resonance (`seq2_match`).

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v46.mp4`)](journal/genesis-v46.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v46 population
./axion genesis probe --load examples/genesis-v46.gen

# Run 4-agent grounded live spiking dialogue (up to 16 layers / 2,297 neurons/agent)
./axion genesis converse --load examples/genesis-v46.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-14 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 10000 --seed 46 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 40000 --seed 460 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v46 achieving **all-time record sustained Darwinian encephalization (`12.96` mean layers, `1,366.7` mean neurons, and evolved minds up to `16L / 2,400` neurons!)**, **full 4-gland 3-fold syrinx retention (`gland = 3.75`)**, **`1.24M` live rewires & `17.73B` STDP edits**, **`+10.37` tiles East dark WM navigation**, **`23×` BAC predictive MMN ratio**, **`7/7` honest ablation clean sweep**, and **live 15–16 layer `L0..L14` multi-agent spiking dialogue**, our immediate next breakthroughs are:

1. **Full 4-Channel Cochlear/Retinal Selection (`4-Eye / 4-Ear / 4-Gland` at `15L / 2,000+` Neurons) + Hippocampal CA3 Pattern Completion (v47):**
   - Scale vertebrate visual/auditory acuity smoothly up to full 4-channel split optics & 4-band cochlear tonotopy (`eye = 4, ear = 4`) so `12–16L / 2,000+` neuron populations achieve **`>15×` laminar SNR across `L1..L4`** and directly bring **`Ctx13 (75%+)`, `Q&A (70%+)`, `Trans20 (70%+)`, `Cmd16 (50%+)`, `Gen18 (55%+)`, `2-Syl (45%+)`, and `3-Syl (15%+)`** onto the evolved deep-cortex population itself, while damping unnecessary `L5+` tonic chatter and adding hippocampal CA3 associative pattern completion!
2. **`1024×1024` Multi-Biome Tribal Oceans & Compositional Multi-Word Wild Language (v47–v48):**
   - Expand `s_reef` to 128 nomadic ecological zones on `1024×1024` (`pop = 1,024`) with distinct regional dialects, cooperative tribal hunting/vault syntax, and cultural apprenticeship — pushing wild `vocal_mi_bits` past **`0.35–0.60+` bits**.
3. **Grounded Open-Ended Spiking Conversation (v48+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v46.gen`, `examples/genesis-v45.gen`, `examples/genesis-v44.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
