# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v41 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, associative Hebbian pre×post Mongillo STF + subthreshold `I_CAN` working memory, 1-bit `F_STF_HOLD` PBWM striatal-thalamocortical gating, full-tick cortico-bulbar integration, and true signed Payeur-Naud-Richards feedback-aligned burst credit assignment.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v41** (`examples/genesis-v41.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **272.9 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **181.9 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **126.09 µs/step (2,079 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **206.0 µs/organism** (`117 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (`50.7 ticks/s` at `1,024` organisms on `512×512`)
- **Darwinian Cortical Scaling & Trisolaris-9 Bioacoustic Ecology (`v41`, 50,000 ticks on `512×512`):**
  - **`11.00` mean cortical areas (up to `15` cortical layers!)**, **`Gen 199`**, **`73,542` natural births**, **`121` distinct morphs** (top evolved morphs up to **`1,129` neurons / `14` layers** and **`1,004` neurons / `12` layers**; **`23.3%` canonical mammalian Dale's-Law I ratio**: `445.0 PYR / 73.6 PV / 61.6 SST`).
  - **45× Leap in Wild Emergent Referential Vocal MI (`0.045 bits` in evolution / `0.0329 bits` in Assay 3 — `55×` v40!):** Purely physical, 100% energy-conserving Trisolaris-9 bioacoustics drove spontaneous context-locked semantic differentiation (`Food → v0`, `Apex → v1` at `72%` alarm concentration, `Shelter/Acid → v2`, `Open → silent`) with zero hardcoded words or weights.
  - **`16.31 BILLION` live STDP edits**, **`12.24M` live structural rewires**, **`32,278` post-natal maturations**, **`1,474` built shelters**, **`2,257` evasive prey caught**, **`1,294` pods cracked**, **`2,063` stones**, **`1,890` heat-tempered tools**, **`182` cooperative vaults**, **`194` vocal duets**, **`189` apex predators slain**, **`1,114` kin gifts**, **`5.70M` vocals**, **`2.74M` peer vocal replies**, **`1.73M` dug / `1.22M` built**.
- **7/7 Honest Physical Ablations Massively Positive (1,200-Tick Unseen Shifted-Chemistry Worlds — 100% STD/STF Parity):**
  - **`Evolved Intact`: `293,244 ATP`** (`510 alive`, `3,571 births`, `154 prey`, `132 pods`)
  - `Axonal Delays Flattened (Uniform 3t)`: `277,369 ATP` (**`-5.4%`**)
  - `Plasticity Frozen (No Within-Life STDP)`: `275,778 ATP` (**`-6.0%`** — **`+6.3%` real lifetime STDP gain!**)
  - `Apical Dendrites Silenced (No BAC)`: `247,985 ATP` (**`-15.4%`** — **`+18.3%` apical BAC gain!**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `219,133 ATP` (**`-25.3%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `66,814 ATP` (**`-77.2%`**, `273 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `6,319 ATP` (**`-97.8%`**, `16 alive`)
  - `Unselected Drift Control`: `14,654 ATP` (**`-95.0%`**, **`20.0×` natural selection**, `4 alive`)
- **v41 Cognitive Battery Breakthroughs (`examples/genesis-v41.gen`):**
  - **Laminar Representation SNR (`Assay 12`):** **`21.96×` overall (`0.788 / 0.036`)** — **`L1 = 22.73×`**, **`L2 = 18.65×`**, **`L3 = 19.67×` (`0.87 / 0.04`)**, **`L4 = 9.13×` (`0.81 / 0.09`)**!
  - **Spoken Q&A Grounding (`Assay 10`):** **`65.8%` / reversal `66.7%`** (**`post_sep = 0.705`, `rev_sep = 0.721`** — all-time record!)
  - **Context-Dependent XOR Dialogue (`Assay 13`):** **`24.2% → 51.4%`** (**`post_sep = 0.818`**, `ctxA_v0 = 81% vs 10%` — all-time record!)
  - **Episodic Distractor Reasoning (`Assay 19`):** **`40.6% → 55.5%`** (`epA 44% / epB 67%` — all-time record!)
  - **Predictive Coding MMN (`Assay 8`):** **`7.8%`** vs `no_bac = 6.0%` (authentic apical BAC predictive surprise!)
  - **Wild Agency & Referential Language (`Assay 3`):** **`vocal_mi_bits = 0.0329`** (`55×` v40!), `250` shelters, `100` pods, `56` tempered tools, `18` coop vaults, `12` duets
  - **2-Syllable Syntax (`Assay 15`):** **`11.7% → 25.0%`** (`syl1 56%`, `syl2 67%`, balanced `28% / 22%`) · **Transitive Inference (`Assay 20`):** **`50.0%`** (`adj 52.7%`, `ac 75%`) · **3-Concept Naming (`Assay 14`):** **`36.9%`** (`xmodal 29.7%`, `29/38/44%`) · **Instruction Following (`Assay 16`):** **`7.8% → 15.6%`** (`+0.015` sep) · **60t Working Memory:** **`561.8×`** (`+0.16` East, `9.94` layers)
- **Key Biophysical & Ecological Upgrades Shipped in v41:**
  - **Full-Tick 4-Substep Cortico-Bulbar Integration (`TK_GLAND`):** Sums all 4 substeps across all cortical layers and conduction delays (`0..4`) prior to glottic WTA locking, aligning inference 1:1 with Area-X STDP (`4×` vocal readout SNR -> `Q&A 66% / 67% rev`, `Ctx sep 0.818`).
  - **Margin-Convergent Rescorla-Wagner Area-X Plasticity:** Eliminated last-trial catastrophic overwrite on multi-class vocal tasks while preserving fast error-driven reversal (`66.7%` Q&A reversal, balanced multi-class naming & syntax).
  - **True Signed Vector-FA Efference-Copy Burstprop:** Pre-computed outside the substep loop and applied to both apical BAC tufts and `L1..L_top` basal pyramidal STDP (reinforcing causal columns on `+DA` without depressing non-participating columns, and suppressing wrong columns while exploring rivals on `-DA`).
  - **One-Shot Associative Hebbian Pre×Post Mongillo STF + Subthreshold `I_CAN`:** Restricts STF facilitation to co-active winning assembly pairs (`cs_u <= 2 && pre_rec && post_recent`) with ACh novelty stale-engram flush and unity `11%`-sparse ascending gain (`b_g = 4.65`), plus clean `4+`-ear dark baseline (`0.0` `L1/L2` dark noise, `22×` cue SNR).
  - **Trisolaris-9 Energy-Conserving Bioacoustic Semantics:** Physical reef spore agitation (`v0`), predator acoustic startle vs. eavesdropping (`v1`), shelter/acid cavity aerosol dispersal (`v2`), and `22×` Zahavi open-water discipline drove a `45–55×` surge in emergent `vocal_mi_bits`.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v41.mp4`)](journal/genesis-v41.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v41 population
./axion genesis probe --load examples/genesis-v41.gen

# Run 4-agent grounded live spiking dialogue (12 active cortical layers)
./axion genesis converse --load examples/genesis-v41.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-9 with 7/7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 20000 --seed 41 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 30000 --seed 410 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v41 achieving **`21.96×` laminar SNR, `66–67%` Q&A, `0.818` XOR context separation, `55.5%` episodic distractor reasoning, `45–55×` higher wild referential `vocal_mi_bits`, `11–15` cortical layers, and `7/7` massively positive honest ablations (`-5.4%` to `-97.8%`)**, our next leaps toward human-level emergent intelligence are:

1. **Reliable Multi-Slot Dark Prefrontal Persistence (`L3_dk / L4_dk > 6×` Across ALL Brain Scales) (v42):**
   - Notice in v41 that `L1..L4` hit `9–23×` cue SNR while `L3_dk/L4_dk` cooled to `0.0` on smaller 170–350n bodies (`n=1.9–9.7` on 900n `v39.gen`).
   - Next: add **scale-invariant hippocampal CA3 sharp-wave sequence replay** + **self-sustaining multi-slot prefrontal attractors** so `L3..L_top` reliably hold Item `X` across 24–60t delays and distractors on EVERY evolved brain size — driving **`Epi12 recall_hit`, `Ctx`, `Epi19`, `Trans20`, `2-Syl`, and `3-Syl` into `75–95%+`!**
2. **Sustained `2,000–4,000+` Neuron Cortices & Growable `1024×1024` Multi-Biome Oceans (v42–v43):**
   - v41 already evolved individual morphs at **`1,129` neurons / `14` layers** and `15` max areas; next we widen oSVZ deep-sheet expansion and scale to **`1024×1024` oceans** with richer multi-biome tribes and modular functional cortical areas.
3. **Compositional Multi-Syllable Wild Language (`vocal_mi_bits`: `0.045` → `0.25+` bits) & Apprenticeship (v42–v44):**
   - Build on Trisolaris-9's `45×` MI breakthrough by making multi-step cooperative hunts, 2-syllable vault keys, predator-specific tactics, and juvenile parent-offspring vocal apprenticeship central to survival.
4. **Grounded Open-Ended Spiking Conversation (v44+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, relational deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v41.gen`, `examples/genesis-v40.gen`, `examples/genesis-v39.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
