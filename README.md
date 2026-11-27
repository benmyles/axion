# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PV+/SST+/VIP+ three-interneuron microcircuits, Mongillo activity-silent prefrontal STF working memory, PBWM striatal-thalamocortical gating, and Payeur-Naud-Richards feedback-aligned burst credit assignment.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v39** (`examples/genesis-v39.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.0 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **181.3 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`256×256` / `512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **22.98 µs/step (2,851 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **206.0 µs/organism** (`109 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem**
- **Darwinian Cortical Scaling & Ecology (`v39`, 50,000 ticks on `512×512` Trisolaris-7):**
  - **`510/512` alive**, **`255` distinct morphs**, **`Gen 240`**, **`972` mean active neurons** (`1,488` E1 mean; mid-run champion peaked at **`1,634` neurons / `178,437` synapses**; final adult champion `1,084` neurons / `38,526` synapses / `9` layers).
  - **`68,762` natural births**, **`100.51 BILLION` live STDP edits (`9.3×` v38)**, **`65.87M` live structural rewires (`12.8×` v38)**, **`4,152` evasive prey caught (`2.8×` v38)**, **`1,516` pods cracked (`2.3×` v38)**, **`322` apex predators slain (`4.3×` v38)**, **`212` vocal duets (`8.8×` v38)**, **`40,066` vocal responses (`5.5×` v38)**, **`3.08M` peer vocal replies (`5.1×` v38)**.
- **v39 Cognitive Battery Breakthroughs (`911`-neuron fresh specimen, `examples/genesis-v39.gen`):**
  - **60-Tick Pitch-Black Working Memory:** **`1,802×`** persistence (`6.9 → 14,068 → 12,375 spk/t`, `+3.44` tiles East drift)
  - **Spoken Q&A Grounding:** **`45.6%` / reversal `49.2%`** (`sep 0.015 → 0.196 → rev 0.307`)
  - **Context-Dependent XOR Dialogue (`Ctx`):** **`28.2% → 41.3%`** (`+13.1%`)
  - **Episodic Distractor Reasoning (`Epi19`):** **`27.8% → 40.2%`** (`+12.4%`, `epA 36% / epB 44%`)
  - **Transitive Inference (`A>B, B>C ⇒ A>C`):** **`27.6% → 41.4%`** (`+13.8%`, `ac 67%`)
  - **3-Concept Cross-Modal Naming:** **`34.3%`** (`xmodal 33.2%`), **Predictive MMN:** **`8.1%`**, **Zero-Shot 02/12:** `post_div = 0.293`, **Gen18 Vocal:** `31.2% → 39.8%`
- **Honest 7-Lesion Audit (v39 → v40 Target):**
  - `evolved_intact` (`279.3k ATP`) beat `evolved_zero_delay` (`276.3k`, `-1.1%`), `evolved_blind` (`239.5k`, `-14.2%`), `evolved_deaf` (`219.7k`, `-21.3%`), and `drift_unselected` (`14.4k`, `-94.8%`).
  - Removing delayed PV shunting without tightening excitatory gain caused `1,000`-neuron v39 brains to fire `4×` hotter (`166.9M` spikes vs `41.5M` in v38), letting calmer `frozen` (`302k`), `no_apical` (`298k`), and `no_neuromod` (`284k`) conserve ATP. Calibrating sparse laminar rate control (`~8–12%` duty cycle, high `SNR`) is **Priority #1 in v40**.
- **Biophysical Upgrades Shipped in v39:**
  - **Mongillo Prefrontal STF & PBWM `L4+` Shielding:** Calcium facilitation (`stp: 1 → 5`) on deep local recurrent synapses + occupied `L4+` distractor shielding + VTA DA trial-boundary flush.
  - **Vector Feedback-Aligned Efference-Copy Burstprop:** Routes physical tick-`(t-1)` vocal/motor efference copies onto `L1..L_top` apical tufts (Payeur-Naud-Richards 2021 + Lillicrap 2016 — 100% local spikes, zero backprop).
  - **Mg²⁺-Thresholded 1-Tick NMDA Dendritic Subunits & Same-Substep PV+ Top-$k$:** Eliminated single-substep collision bias and PV lag bias against real heterogeneous axonal delays.
  - **Pan-Cortical BDNF Trophic Competition & Stride-2 Sperry Gliding:** Uniform reservoir sampling on `TK_GLAND` (`~88%` exc / `~12%` D2) + dense cortico-cortical ascending tracts (`exc_syn/pyr` doubled).
  - **Trisolaris-7 Ecology:** 2-syllable seasonal nocturnal reef precursors (`v0→v2` vs `v2→v0`), predator acoustic eavesdropping, `3.5×` Zahavi chatter cost, and high-payoff cooperative vaults/mobbing.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v39.mp4`)](journal/genesis-v39.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v39 champion
./axion genesis probe --load examples/genesis-v39.gen

# Run 4-agent grounded live spiking dialogue
./axion genesis converse --load examples/genesis-v39.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-7 with 7/7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 20000 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 30000 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

Our measurements across v25–v39 pinpointed four fundamental ceilings that separate worm/insect-scale spiking circuits (~150–1,000 neurons, ~45–55% reasoning) from true higher-order cognition and grounded language:

1. **Cortical Scale & Growable Worlds (v38 → v42):**
   - Current: up to `32` layers / `8,192` neurons / `192` synapses/cell per organism on `512×512` arenas (already sustaining `934–1,730` neurons natively).
   - Next: push sustainable evolved brain sizes to **`2,000–4,000+` active neurons** with hierarchical cortical areas and **runtime-growable `1024×1024` oceans** supporting multi-biome migration and larger social tribes.
2. **High-SNR Sparse Orthogonal Representations (`cos(A, B) > 0.65`, `cos(A, A') < 0.08`):**
   - Current: `L1` achieves **`SNR = 8–10×` (`cos = 0.51–0.55`, `noise = 0.05`)**; deep `L2..L_top` are being stabilized into strictly subcritical local recurrence with dense ascending convergence.
   - Next: push `SNR > 6–10×` uniformly all the way through `L_top` (`L8..L16`) so downstream plasticity reads clean, repeatable high-dimensional semantic codes.
3. **Deep Multi-Layer Credit Assignment & Prefrontal Sequence Memory (v39 → v43):**
   - Current: Vector feedback-aligned efference-copy Burstprop + Mongillo STF + PBWM `L4+` shielding.
   - Next: **hippocampal sharp-wave sequence replay** and **multi-slot working-memory binding** so brains break past the ~50% ceiling into **80–95%+** across XOR context switching (`Ctx`), distractor-resistant episodic reasoning (`Epi19`), transitive inference (`Trans20`), and compositional 2- and 3-syllable syntax (`2-Syl` / `3-Syl`).
4. **Survival-Critical Language & Culture (`vocal_mi_bits`: `0.004` → `0.25+` bits):**
   - Current: Trisolaris-7 introduces 2-syllable seasonal reef precursors, predator eavesdropping, Zahavi silence discipline, and cooperative vaults/mobbing.
   - Next: make complex environmental navigation, multi-step foraging, predator defense, and multi-generational apprenticeship **literally unsolvable without referential signaling, memory, and shared grammar** — driving genuine emergent language and grounded bidirectional conversation with humans.

---

## Repository Layout

The repository has been stripped of all pre-genesis legacy prototypes into a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v39.gen`, `examples/genesis-v38.gen`, `examples/genesis-v36.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
