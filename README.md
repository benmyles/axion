# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v43 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + DA-gated shielded Slot 1 with associative Hebbian Mongillo STF & subthreshold `I_CAN`), subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), evolvable Kriegstein oSVZ outer radial glia (`oRG`) tangential cortical expansion, positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v43** (`examples/genesis-v43.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.1 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **183.8 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **122.69 µs/step (2,137 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **239.0 µs/organism** (`113 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (`44.7 ticks/s` at `1,024` organisms on `512×512`)
- **Massive Darwinian Encephalization & Multimodal Vertebrate Ecology (`v43`, 50,000 ticks on `512×512`):**
  - **All-Time Record Sustained Mean Cortex Size (`2.4×` over v42!):** Real Darwinian encephalization climbed from **`9.45L / 1,218 neurons`** in E1 to **`12.90L / 1,370.7 mean neurons` (`max 15L`)** in E2 across `171` distinct morphs (`#1 G191: 9L / 1,421n`, `#2 G193: 15L / 1,167n`, `#3 G194: 10L / 1,586n`, `#4 G203: 14L / 1,660n`) with **`98%` population / `100%` champion bilateral symmetry**!
  - **Canonical Mammalian E/I Balance Locked Across 206 Generations:** **`21.1%` Inhibitory** (`pyr = 1,074.8 [78.4%]`, `pv = 145.0 [10.6%]`, `sst = 143.8 [10.5%]`, `mod = 7.08 [0.5%]`) — permanently eliminating both `0%-I` epilepsy and late-stage `43%-I` PV hypertrophy.
  - **Binocular / Binaural Multimodal Anatomy:** `eye = 2.59`, `ear = 4.16`, `muscle = 10.91`, `grasp = 1.92`, `gland = 4.02`, `mod = 7.08`.
  - **`119,104` total natural births** (`39,962` E1 + `79,142` E2), **`7.74 BILLION` live STDP edits** in E2 (`13.8B` combined), **`627,828` live structural rewires**, **`24,545` post-natal maturations**, **`2,925` evasive prey caught**, **`2,268` pods cracked**, **`57` tool cracks**, **`319` cooperative vaults**, **`307` vocal duets**, **`1,779` heat-tempered blades**, **`242` apex predators slain**, **`4,045` polymer shelters**, **`465` cached granary feasts**, **`6.04M` vocals (`2.93M` replies, `42,886` mimics, `0.0410b` wild MI)**.
- **Historic 7/7 Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `230,187 ATP`** (`510 alive`, `3,172 births`, `182 prey`, `115 pods`, `3,674 built`)
  - `Plasticity Frozen (No Within-Life STDP)`: `210,627 ATP` (**`-8.5%`** — plus **`+19.1%` STDP reversal gain** in Assay 1! `prey -29%`, `built -45%`)
  - `Apical Dendrites Silenced (No BAC)`: `217,629 ATP` (**`-5.5%`** — plus **`+4.6%` BAC reversal gain** in Assay 1! `prey -26%`, `built -45%`)
  - `Axonal Delays Flattened (Uniform 3t)`: `155,692 ATP` (**`-32.4%`**, `367 alive`)
  - `Neuromodulation Silenced (DA=ACh=0)`: `172,983 ATP` (**`-24.9%`**, `443 alive`, `prey -43%`, `built -74%`)
  - `Photoreceptors Blinded (Eyes Off)`: `121,313 ATP` (**`-47.3%`**, `269 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `26,804 ATP` (**`-88.4%`**, `51 alive`)
  - `Unselected Drift Control`: `0 ATP` (**`-100.0%`**, **`∞×` natural selection**, `0 alive`)
- **v43 20-Assay Cognitive Battery & 4-Agent Spiking Dialogue (`examples/genesis-v43.gen`):**
  - **Seasonal Reversal Learning (`Assay 1`):** **`+19.1%` STDP gain** (`247,196` vs `207,519` ATP) and **`+4.6%` BAC gain** (`alive = 506`) — flipping v42's `-0.9%` STDP reversal into a decisive `+19.1%` win!
  - **60-Tick Pitch-Black Working Memory (`Assay 2`):** **`2,329.7×` persistence** (`2.5 → 8,475 → 5,747 spk/t`) with **all-time record `+4.68` tiles East dark drift** (`+44%` over v42!)
  - **Zero-Shot Compositional Generalization (`Assay 18`):** **`10.9% → 39.1%` joint vocal+motor hit** (`mot_sep = +0.108` — **`1.8×` v42!**)
  - **Spoken Q&A & Reversal (`Assay 10`):** **`56.0%` (`sep = 0.406`) / `53.3%` reversal (`rev_sep = 0.340`)** (`+5.8` pts over v42!)
  - **Context-Dependent XOR Dialogue (`Assay 13`):** **`28.9% → 46.2%` (`post_sep = 0.347`)** · **3-Concept Naming (`Assay 14`):** **`41.3%`** (`xmodal 35.3%`, balanced `35/34/55%`) · **Transitive Inference (`Assay 20`):** **`57.8%` symmetric (`AC 61% / CA 55%`)** · **Episodic Distractor (`Assay 19`):** **`46.9%` symmetric (`47/47%`)** · **2-Syllable Syntax (`Assay 15`):** **`23.4%` symmetric (`23/23%`, `syl1 54.7%`, `syl2 53.9%`)** · **Predictive MMN (`Assay 8`):** **`+4.0%` MMN (`4.4×` vs `0.9%` no-BAC)** & **`+6.6%` Omission** · **Laminar Dark Persistence (`Assay 12`):** `L1_dk = L2_dk = 0.0`, **`L3_dk = 1.30×`**, **`L4_dk = 1.02×`**, `cos_dist = 0.590`, `recall_sep = +0.106`
  - **4-Agent Grounded Spiking Dialogue (`converse`):** Four distinct `1,167–1,666`-neuron evolved agents (`9–15` cortical layers, up to `54` active neurons/layer, `113` peer replies, `19,430` live STDP edits), answering `ask_ctx → v0` after distractors and zero-shot `ask_rel_ac → v0`.
- **Key Biophysical & Ecological Upgrades Shipped in v43:**
  - **Sparse-Preserving Vogels-Sprekeler iSTDP & Homeostatic Scaling (`ema == 0`):** Stopped passive `do_ltp` from dismantling PV/SST inhibitory weights on 90% of sparse PING neurons, preserving sharp cortical tuning.
  - **Sign-Faithful Burstprop & Apical Predictive Coding + Cortical Hebbian Sprouting:** Stopped potentiating basal/apical excitatory weights on `-DA` punishment and eliminated random `L0` motor noise sprouts (`layer_idx >= 1`, `post_recent != 0`, `mod_pos || ext_ach_q > 18`).
  - **Evolvable Southwell (2012) MGE/CGE Postnatal E/I Homeostasis + VTA/BF Nuclei + Kriegstein oSVZ `oRG` Tangential Expansion (`src/genesis_dna.cuh`):** Locks canonical evolvable `18.2%–25.0%` GABAergic balance (`1:1 PV+:SST+`), preserves medial `z <= 2` VTA/BF `MOD` nuclei, and expands deep neocortical sheets tangentially (`org_rad` evolvable `3..6`) to sustain `1,200–1,660+` neuron brains without narrow-column collapse.
  - **Subthreshold Songbird `Uva→HVC` Vocal Efference Copy (`L3`) + Binocular Stereopsis / Binaural Echolocation + Trisolaris-12 Cognitive Economy:** Paired-receptor physics (`>= 2` eyes/ears for full acuity) plus rebalanced Trisolaris-12 grazing vs. seasonal Batesian prey/pod/vault payoffs forced lifetime plasticity, apical BAC, axonal delays, and neuromodulation to win decisively (`7/7` clean sweep).

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v43.mp4`)](journal/genesis-v43.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v43 population
./axion genesis probe --load examples/genesis-v43.gen --seed 430

# Run 4-agent grounded live spiking dialogue (1,167-1,666 neurons/agent, 9-15 layers)
./axion genesis converse --load examples/genesis-v43.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-12 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 20000 --seed 43 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 30000 --seed 430 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v43 delivering a **7/7 honest ablation clean sweep (`frozen -8.5%`, `no_apical -5.5%`, `zero_delay -32.4%`, `no_neuromod -24.9%`, `blind -47.3%`, `deaf -88.4%`, `drift -100%`), `+19.1%` STDP reversal learning, `1,371` mean neurons (`up to 1,666n / 15L`) at canonical `21.1%` E/I, `+4.68` tiles dark WM drift, `39.1%` joint zero-shot compositional generalization, and `56.0%` spoken Q&A**, our immediate next breakthroughs are:

1. **Hippocampal Sharp-Wave Sequence Replay & Higher-Order Compositional Binding (v44):**
   - Push `Q&A`, `Ctx`, `Epi19`, `Trans20`, `3-Concept Naming`, `2-Syl`, and `3-Syl` syntax from `40–68%` into **`75–95%+`** via dark-period sharp-wave/ripple replay (`L3↔L4` theta-gamma phase coupling), stronger apical-basal conjunctive binding, and clean `15–25×` laminar SNR across `1,500–3,000+` neurons.
2. **Tribal Multi-Biome `1024×1024` Oceans & `0.25+` Bits Wild Referential Syntax (v44–v45):**
   - Scale the physical world to `1024×1024` multi-biome oceans with regional resources, multi-syllable cooperative hunting/vault syntax, and juvenile apprenticeship — driving wild `vocal_mi_bits` from `0.04–0.08` bits toward `0.25+` bits of true emergent language.
3. **2,000–4,000+ Mean Neuron Cortical Hierarchies (v44–v45):**
   - Scale sustained mean brain size to `2,000–4,000+` active spiking neurons (`12–24` specialized cortical areas) while keeping `0 B` local memory spill on Blackwell SM 12.0.
4. **Grounded Open-Ended Spiking Conversation (v45+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v43.gen`, `examples/genesis-v42.gen`, `examples/genesis-v41.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
