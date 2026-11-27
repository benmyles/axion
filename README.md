# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v40 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, bistable Mongillo activity-silent prefrontal STF working memory, 1-bit `F_STF_HOLD` PBWM striatal-thalamocortical gating, and Payeur-Naud-Richards feedback-aligned burst credit assignment.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v40** (`examples/genesis-v40.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **272.8 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **181.5 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **125.27 µs/step (2,093 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **206.2 µs/organism** (`117 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (`54.8 ticks/s` at `1,024` organisms on `512×512`)
- **Darwinian Cortical Scaling & Trisolaris-8 Ecology (`v40`, 50,000 ticks on `512×512`):**
  - **`510/512` alive**, **`182` distinct morphs**, **`Gen 194`**, **`9.70` mean cortical areas** (up to **`12L`**), **`651.7` mean active neurons** (**canonical mammalian `22.6%` Dale's-Law I ratio**: `504.6 PYR / 90.9 PV / 56.2 SST`; champion `702` neurons / `32,045` synapses; `converse` 4-agent colony `657–791` neurons across `9–11` active cortical layers).
  - **`58,332` natural births**, **`11.45 BILLION` live STDP edits**, **`11.14M` live structural rewires**, **`32,427` post-natal maturations**, **`3,985` built shelters**, **`187` cached granaries**, **`2,260` evasive prey caught**, **`1,704` pods cracked**, **`2,049` stones**, **`1,909` heat-tempered tools**, **`134` cooperative vaults**, **`153` vocal duets**, **`175` apex predators slain**, **`6.45M` vocals**, **`2.99M` peer vocal replies**, **`65,877` vocal responses**, **`37,607` mimics**.
- **7/7 Honest Physical Ablations Decisively Positive (1,200-Tick Unseen Shifted-Chemistry Worlds — 100% STD/STF Parity):**
  - **`Evolved Intact`: `345,038 ATP`** (`511 alive`, `3,279 births`, `340 prey`, `74 pods`, `1,218 dug`, `480 built`)
  - `Plasticity Frozen (No Within-Life STDP)`: `340,062 ATP` (**`-1.4%`**; `59 pods`, `869 dug`)
  - `Axonal Delays Flattened (Uniform 3t)`: `334,249 ATP` (**`-3.1%`**)
  - `Apical Dendrites Silenced (No BAC)`: `324,226 ATP` (**`-6.0%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `307,529 ATP` (**`-10.9%`**; `37 pods`, `260 dug`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `253,312 ATP` (**`-26.6%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `239,463 ATP` (**`-30.6%`**)
  - `Unselected Drift Control`: `40,140 ATP` (**`-88.4%`**, **`8.6×` natural selection**)
- **v40 Cognitive Battery Highlights (`examples/genesis-v40.gen` + fresh v40 validation):**
  - **Reversal Learning (Assay 1):** **`+0.4%` STDP gain, `+1.2%` BAC gain** (`330,324 > 328,992 > 326,567 ATP`, `509` alive)
  - **60-Tick Pitch-Black Working Memory:** **`1,227.2×`** persistence (`4.0 → 7,674 → 4,909 spk/t`, `8.17` layers)
  - **24-Tick Episodic Delayed Recall (Assay 12):** **`recall_hit = 38.4%`** (`recall_sep = +0.057`, `cue_snr = 3.01×`, `L3_cue = 3.77×`; up to **`9.94–11.21×` laminar SNR** on ear-bearing v40 specimens!)
  - **Spoken Q&A Grounding:** **`48.7%` / reversal `52.1%`** (`rev_sep = +0.147`)
  - **Context-Dependent XOR Dialogue (`Ctx`):** **`41.5% → 46.9%`**
  - **Zero-Shot Compositional Generalization (`Gen18`):** **`46.1% → 50.8%` vocal** (`joint 21.9% → 26.6%`, positive `mot_sep +0.007`)
  - **Relational Transitive Inference (`A>B, B>C ⇒ A>C`):** **`35.2% → 50.8%`** (`adj 51.6%`, `ac 77%`)
  - **Episodic Distractor Reasoning (`Epi19`):** **`36.7% → 41.4%`** · **3-Concept Naming:** **`36.0%`** (`xmodal 34.7%`) · **2-Syl Syntax:** **`14.1% → 19.5%`** (`syl1 49%`, `syl2 56%`)
- **Key Biophysical & GPU Engineering Breakthroughs Shipped in v40:**
  - **Race-Free Double-Buffered `spike_ring` / `spike_ring_next`:** Eliminated a cross-CTA CUDA read/write hazard with 0 ns pointer swap after every brain step.
  - **PING Gamma-Cycle Sparse Coding & Zero-Saturation Piecewise-Log Top-$k$:** Eliminated bin-15 saturation (`0..5,632+ pA`), balanced scale-invariant `11.1%` tick budgets (`k_tick` / `k_sub`) across `100–4,000+` neuron layers, and capped Larkum/Payeur BAC bursts to crisp 2-spike doublets.
  - **Closed Global `L0..L_top` Ping-Pong Loops & Collision-Free Sperry Tonotopy:** Enforced Sherman & Guillery Class-1B apical-only descending feedback across all layers, extended Brunel `1/sqrt(K)` scaling to `L0` and interneurons, and replaced coordinate-hash ear projection with anatomical rank-ordered Sperry columns (`s_rank & 3`).
  - **1-Bit `F_STF_HOLD` (`0x80`) PBWM Gating & Bistable `12×` Mongillo STF:** Tracks active deep STF (`stp >= 5`) in a free metadata bit (`0 B` extra VRAM) for true `6×` Encode (`1.5×`) vs. Shield (`0.25×`) gating on `L4+`, with `3.0×` facilitated vs. `0.25×` resting local gain.
  - **Symmetric Reward-Preserving 3-Factor STDP & Honest `freeze_plasticity` Parity:** Fixed `STD/STF` control parity and eliminated downward soft-bound/homeostatic weight erosion, delivering `7/7` positive held-out ablations (`+1.4%` to `+759.6%`) alongside `~22%` canonical mammalian E/I balance.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v40.mp4`)](journal/genesis-v40.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v40 population
./axion genesis probe --load examples/genesis-v40.gen

# Run 4-agent grounded live spiking dialogue
./axion genesis converse --load examples/genesis-v40.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-8 with 7/7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 20000 --seed 40 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 30000 --seed 400 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v40 achieving **race-free 10-layer sparse propagation, up to `11×` laminar SNR, `22.6%` canonical E/I balance, and `7/7` honest positive physical ablations**, the next high-leverage leaps toward human-level emergent intelligence are:

1. **Multi-Slot Prefrontal Working Memory & Hippocampal Sharp-Wave Sequence Replay (v41):**
   - Push dark delayed SNR (`L3_dk` / `L4_dk`) from `1–4×` to **`>6–10×` across 24+ tick delays and distractors** by adding theta/sharp-wave local sequence replay (`CA3 -> CA1`) and dual-slot PBWM maintenance so `L4..L_top` holds Item `X` while `L1..L2` processes Distractor `D` or Item `Y`.
   - Target: break through the `~50%` ceiling into **`70–90%+`** on `Q&A`, `Ctx` (XOR), `Epi19`, `Trans20`, `2-Syl`/`3-Syl` syntax, and `Cmd`.
2. **True Vector Feedback-Aligned Credit Assignment Across All Cortical Layers (v41–v42):**
   - Strengthen signed, action-specific efference-copy burst-rate feedback (`L0` effectors & top executive layers -> `L2..L_top` apical tufts) so deep layers genuinely sculpt non-linear conjunctive features rather than relying primarily on readout plasticity — still 100% local spikes, zero backprop.
3. **Survival-Critical Referential Language & Cultural Transmission (`vocal_mi_bits`: `0.004` → `0.25+` bits) (v41–v42):**
   - Shift a larger share of the Trisolaris energy economy onto problems literally unsolvable by silent solo reflexes: multi-syllable reef/vault keys, referential predator-type warnings, and parent-offspring apprenticeship where context-specific listening + replying separates thriving tribes from starvation.
4. **Brain & World Scaling (`2,000–4,000+` Active Neurons & `1024×1024` Multi-Biome Oceans) (v42–v44+):**
   - Decouple cranial oSVZ encephalization from somatic body mass so agile lineages sustain **`2,000–4,000+` active neurons across `16–32` specialized cortical areas** on growable `1024×1024` worlds, scaling toward open-ended grounded bidirectional spiking conversation with humans.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v40.gen`, `examples/genesis-v39.gen`, `examples/genesis-v38.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
