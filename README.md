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

### Latest Official Published Milestone — **Axion Genesis v38** (v39 In Validation)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels):**
  - `snn_inference`: **273.0 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **181.4 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **119–121 µs/step (~2,170–2,200 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **178–205 µs/organism** (`108–109 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (`~105 ticks/s` at 1,024 organisms on v39 substrate)
- **Darwinian Cortical Scaling (`v38`, 50,000 ticks on `512×512`):**
  - Shattered the **1,000-neuron** barrier purely through natural selection: **`933.7` mean active neurons** across `508/512` living creatures at tick 30,000 (`Gen 197`, `182` species, `mean_areas = 8.43`, `max_areas = 16`, `max_fanin = 192`, `98%` bilateral symmetry).
  - Peak evolved champion reached **`1,730` neurons / `13` layers / `109,994` synapses**; final saved champion (`examples/genesis-v38.gen`) has **`1,365` neurons / `65,861` synapses / `8` layers / `100%` symmetry**.
  - **All 7/7 Held-Out Physical Ablations Strongly Positive:** Intact `237,404 ATP` vs Frozen (`-2.4%`), No-Apical (`-48.0%`), Zero-Delay (`-2.8%`), No-Neuromod (`-60.1%`), Blind (`-32.1%`), Deaf (`-18.3%`), Drift (`-90.4%`).
- **Active Structural Upgrades (v39 Prefrontal STF & Vector Credit — In Dev/Validation):**
  - **Laminar STP & Activity-Silent Working Memory:** Mongillo-Barak-Tsodyks (2008) calcium facilitation (`STF`) on deep prefrontal local recurrent synapses (`L2+`), paired with O'Reilly & Frank (2006) **PBWM occupancy shielding** on `L4..L_top` and VTA dopamine gate flush.
  - **Vector Feedback-Aligned Efference-Copy Burstprop:** Moves beyond scalar global dopamine by routing tick-`(t-1)` vocal and motor efference copies through fixed anatomical feedback projections onto `L1..L_top` apical tufts (Payeur-Naud-Richards 2021 + Lillicrap 2016 — 100% local spikes, zero backprop).
  - **Pan-Cortical BDNF Trophic Wiring & Stride-2 Sperry Column Gliding:** Eliminated inhibitory flooding and checkerboard bouncing in 1,000+ neuron brains (`exc_syn/pyr` doubled `18.5 -> 32.5`, vocal fold silence cut `42% -> 4–11%`, `MMN` surged to **`21.3%`**, `L1 SNR` reached **`9.84×`**, `2-Syl` syntax hit **`23.4%`**, `Trans20` hit **`53.1%`**).
  - **Trisolaris-7 Ecology (Three-Body Inspired Selection Pressure):** Erratic multi-star Chaotic vs Stable eras, 2-syllable temporal-order nocturnal reef precursors (`v0→v2` Warm vs `v2→v0` Cool, flipping seasonally), predator acoustic eavesdropping, Zahavi open-ocean chatter cost, and high-payoff cooperative vaults & alarm mobbing.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v38.mp4`)](journal/genesis-v38.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v38 champion
./axion genesis probe --load examples/genesis-v38.gen

# Run 4-agent grounded live spiking dialogue
./axion genesis converse --load examples/genesis-v38.gen

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

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, and 20-assay probe suite.
- `tools/genesis_journal.py` — Dark-observatory plate renderer and NVENC AV1 video pipeline.
- `tests/genesis.py` — Automated GPU verification, spill checks, and regression tests.
- `examples/genesis-v38.gen`, `examples/genesis-v36.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
- `docs/` — Historical research logs and earlier prototype studies (Floodplain & Mind architectures, preserved for reference).
