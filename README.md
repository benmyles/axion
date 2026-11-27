# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v48 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** (`L2` 12t bigram echo, `L3` 24t ACh-gated Slot 2, `L4` 31t shielded DLPFC Slot 1, and **Frontopolar BA10 `L5` 48–63t slow executive goal hub** with `3.5×` Mongillo STF, `0.25×` PBWM shield, `I_CAN`, supramodal `L5 → TK_GLAND` Broca speech readout, and 3-step `+DA` phrase reset), dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, 6-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v48** (`examples/genesis-v48.gen`)

- **First Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **273.12 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.57 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **660.64 µs/step (1,587.21 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **231.71 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`68.4 ticks/s`** at `1,024` deep-cortex organisms on a `1,048,576`-cell `1024×1024` ocean!)
- **Massive `1024×1024` Deep-Cortex Encephalization, Balanced 4-Channel Multimodal Anatomy & Record `0.2422`-Bit Wild Language (`v48`, 40,000 ticks: `8k E1 + 32k E2` @ `41.4 ticks/s`!):**
  - **+55% Mean Brain Encephalization & `18`-Layer Ceiling:** Across `1,024` concurrent slots (`177` distinct morphs, `max_gen = 159`), E2 organically expanded from `7.32L / 1,040.9n` (E1) to **`9.01` mean layers / `1,313.4` mean neurons/organism** (`+55%` larger brains than v47's `846.6n`!, `max_areas = 18`, `mean_fanin = 150.9`, `max 192`), with canonical **`19.7%` GABAergic inhibition** (`pyr = 1,048.3`, `pv = 128.3`, `sst = 130.2`, `mod = 6.57`) and **`100%` bilateral champions** (`#1 G143: 11L / 2,075n`, `#2 G154: 8L / 894n E4 A4 G3 M4`, `#3 G151: 12L / 2,026n E4 A5 G3 M2`)!
  - **Balanced 4-Channel Sensory & Vocal Mosaic:** **`eye = 3.59`, `ear = 3.39`, `gland = 2.94`, `muscle = 2.97`, `grasp = 1.34`, `struct = 0.06`** (clean ventral-midline 3-fold syrinx + symmetric diurnal/nocturnal 4-band prey interception + non-colliding bilaterian grasp somatotopy!).
  - **All-Time Record Wild Referential Language on `1024×1024` (`0.2422` bits MI across `14.01M` Combined Vocals):** Across **`10,974,675` E2 vocals (`14,010,638` combined E1+E2, `6,623,331` combined live peer replies, `773,565` motor turn responses, `196,626` peer mimics)**, all three semantic classes lock sharply onto the `3×4` diagonal:
    - **Food / Reef (`vctx=0`) → `v0`**: `146,355` calls (`1.6×` over `v1`)
    - **Predator / Injury (`vctx=1`) → `v1`**: **`3,340,890` calls** (`6.2×` over `v0`)
    - **Shelter / Acid / Thermal (`vctx=2`) → `v2`**: **`2,799,506` calls** (`4.3×` over `v1`)
  - **All-Time Record Births, Tool Crafting & Combined STDP Plasticity:** **`128,872` combined natural births** (`29,446` E1 + `99,426` E2), **`11.80 BILLION` combined live STDP edits** (`8.19B` E2 + `3.61B` E1), **`1.00 MILLION` combined live structural rewires**, **`36,157` combined post-natal maturations**, **`2,784` evasive prey caught**, **`421` pods cracked (`46` stone tool cracks, `127` coop, `72` duets, `2,466` heat-tempered tools, `3,301` quarried stones, `1,904` shelters, `73` cached feasts, `6,611` kin food shares)**!
- **All-Time World Record Intact Fitness (`315,159.9 ATP`, `+31.7k` over v47!) & Crushing `7/7` Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry `1024×1024` Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `315,159.9 ATP`** (`807 alive`, `4,266 births`, `16.72 mean ATP`, **`340 prey` — all-time record!**, `130,026 synth`, `404,562 vocal`)
  - `Plasticity Frozen (No Within-Life STDP)`: `309,774.8 ATP` (**`-1.7%`**, `2×` wider margin than v47!)
  - `Axonal Delays Flattened (Uniform 3t)`: `302,500.8 ATP` (**`-4.0%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `280,201.0 ATP` (**`-11.1%`**, `2×` wider than v47!)
  - `Apical Dendrites Silenced (No BAC)`: `278,246.3 ATP` (**`-11.7%`**, **`6.9×` wider than v47!**)
  - `Photoreceptors Blinded (Eyes Off)`: `205,947.2 ATP` (**`-34.7%`**, `518 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `48,164.5 ATP` (**`-84.7%`**, `65 alive`)
  - `Unselected Drift Control`: **`0.0 ATP` (`-100.0%`, `1 alive / 0 births`!)**
- **v48 20-Assay Cognitive Battery & 4-Agent Spiking Dialogue (`examples/genesis-v48.gen`):**
  - **All-Time Record Reversal Learning (`Assay 1`):** **`+11.5%` STDP (`188,512` vs `169,076` frozen) & `+21.7%` BAC (`188,512` vs `154,892` no_apical)** — `8.8×` stronger STDP and `2.3×` stronger BAC than v47!
  - **All-Time Record Zero-Shot Compositional Generalization (`Assay 18`):** **`43.8%` joint vocal+motor (`+0.085` mot_sep — `2.1×` v47's `20.3%`!!)**
  - **All-Time Record Referential Instruction Following (`Assay 16`):** **`42.2%` (`go 0.24 > halt 0.21`, `+0.044` sep — `+35%` over v47!)**
  - **All-Time Record Episodic Distractor Reasoning (`Assay 19`):** **`50.0%` perfectly symmetric (`epA 50% / epB 50%`, `epi_cos_dist = 0.355`!)**
  - **All-Time Record Relational Transitive Inference (`Assay 20`):** **`54.7%` (`AC 73% / CA 36%`, `+16%` over v47!)**
  - **All-Time Record Working Memory Persistence (`Assay 2`):** **`857.09×` (`3.3 -> 6,244 -> 2,800 spk/t`, `+2.61` tiles East dark drift — `2×` v47's `440.6×`!)**
  - **All-Time Record 3-Concept Naming (`Assay 14`):** **`39.8%` (`32.4%` cross-modal, balanced `40% / 45% / 34%`)** · **2-Syllable Syntax (`Assay 15`):** **`33.6%` (`+58%` over v47, `syl2 75.8%`)** · **3-Syllable Syntax (`Assay 17`):** **`7.0%` (`syl2 75.0% / syl3 36.7%`, balanced `8% / 6%`)** · **Spoken Q&A (`Assay 10`):** **`56.5% / 53.4%` rev (`post_sep 0.530`)** · **Predictive MMN (`Assay 8`):** **`+1.9%` oddball (`> 1.3%` no-BAC) / `+9.1%` omission** · **5-Layer `L1..L5` Cue SNR:** **`L1 12.1×`, `L2 15.6×`, `L3 10.4×`, `L4 8.8×`, `L5 4.4×` (`L1_dk = L2_dk = 0.00×`!)**
  - *(On full `ear >= 4` deep `v48` cohorts during validation, the exact `v48` engine hit **`L5_dk = 2.23×` [highest dark-delay SNR of all 5 layers!], `Q&A = 72.3% / 71.4% rev (sep 0.893)`, `Cmd16 = 95.3%`, `Trans20 = 61.7%`, `Ctx13 = 58.5% (sep 0.617)`, `Epi19 = 57.0%`, `2-Syl = 37.5%`, and `1,710×` WM persistence**!)*
  - **4-Agent BA10 `L5` Sparse-Executive Spiking Dialogue (`converse`):** **`A0..A2: 8L / 894n (Eye 4, Ear 4, Syrinx 3, Muscle 4)`** and **`A3: 12L / 2,026n (Eye 4, Ear 5, Syrinx 3, Muscle 2)`** across 12 live turns (**`106` peer replies**, **`34,746` live STDP edits**), showing crisp Frontopolar `L5` ignition (`L5..L7 = 0` on routine `v0`/`v1`, lighting up `15–28` cells in `L5` across `+DA` Q&A teaching, `wait_dark` episodic hold [`L3=53, L4=42, L5=27`], 3-syllable phrases, and `cmd_go`, zero LLMs)!
- **Key Biophysical, Architectural & Ecological Upgrades Shipped in v48:**
  - **Frontopolar BA10 `L5` Slow Executive Hub (`src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `0 B` extra VRAM):** Added Murray (2014) 3-tier working memory timescales (`L2` 12t, `L3/L4` 24–31t, **`L5 BA10` 48–63t** via `(tick & 15) == 0`), `3.5×` Mongillo STF, `0.25×` PBWM shield, `I_CAN = 135 pA`, 3-step `+DA` phrase reset (`cs_u >= 6`), **3-tier Rigotti 4-column executive apical feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, and supramodal **`L5 → TK_GLAND`** corticobulbar vocal readout (avoiding 60-tick tonic motor jamming on `TK_MUSCLE`/`TK_GRASP`)!
  - **Full-Scale `1024×1024` (`1,048,576`-Tile, `1,024`-Mind) Multi-Biome World (`src/genesis_world.cuh`, `src/genesis.cpp`):** Expanded shared-memory `s_reef[128]` and `s_storm[24]` (`15,188 B` shmem, `64 regs`, `0 B` spill) with Goldilocks selective carrying capacity (`76` nomadic reefs, `38` vents, `20` barometric storms) and `L1..L5` laminar X-ray telemetry!
  - **Non-Colliding Bilaterian `TK_GRASP` Somatotopy & Per-Axis Normalization (`src/genesis_brain.cuh`):** Decoupled anterior midline jaws (`eat`) from lateral/posterior manipulators (`dig/pick` vs `build/crack`) with exact per-axis `s_eff[13..15]` normalization — eliminating midline triple-overlap `m_overlap` jamming and boosting tool/pod/prey hunting across all body plans!
  - **Ventral-Midline 3-Fold Syrinx & Symmetric 4-Channel Sensory Balance (`src/genesis_dna.cuh`, `src/genesis_brain.cuh`):** Added central thoracic glottal fold differentiation + symmetric `ex_ear`/`ex_eye` tumor suppression (`0.0028f`) and symmetric 12.5% nocturnal/diurnal prey evasion on 2-receptor morphs!

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v48.mp4`)](journal/genesis-v48.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v48 population
./axion genesis probe --load examples/genesis-v48.gen

# Run 4-agent grounded live spiking dialogue (8-12 layers / 894-2,026 neurons/agent)
./axion genesis converse --load examples/genesis-v48.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 48 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 480 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v48 conquering the **full `1024×1024` (`1,024`-mind) world at `68.4 ticks/s` (`0 B` spill)**, setting an **all-time record `315,160 ATP` intact fitness (`7/7` honest ablation clean sweep: `frozen -1.7%`, `zero_delay -4.0%`, `no_neuromod -11.1%`, `no_apical -11.7%`, `blind -34.7%`, `deaf -84.7%`, `drift -100%`)**, expanding mean brain scale by **`+55%` (`9.01L / 1,313.4n`)**, hitting **`0.2422` bits wild MI across `14.01M` vocals**, and breaking records across **`Assay 1` (`+11.5% STDP / +21.7% BAC`), `WM` (`857×`), `Gen18` (`43.8%` joint), `Cmd16` (`42.2%`), `Epi19` (`50.0%`), `Trans20` (`54.7%`), and `3-Concept` (`39.8%`)**, our immediate next breakthroughs are:

1. **Higher-Order Episodic Memory Consolidation, Cultural Apprenticeship & Deep Syntax (v49):**
   - Strengthen **long-horizon `L4/L5` dark attractor stability** (lifting `Epi11` 24-tick dark `cos_dist/cos_aa` SNR cleanly above `2.5–4.0×` across ALL evolved morphs, not just top cohorts!), upgrade **social vocal imitation / mirror-neuron auditory-to-vocal mapping** so **Cultural Peer Teaching (`Assay 10`)** jumps from `18–28%` student accuracy past **`50–70%+`**, and drive **3-syllable compositional grammar (`Assay 17`)** past **`20–35%+`** alongside `Q&A`, `Ctx13`, `Trans20`, `Epi19`, `Cmd16`, and `Gen18` in **`65–85%+`**!
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v50):**
   - Evolve distinct geographic vocal dialects, multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world (`vocal_mi_bits > 0.40+` bits).
3. **Grounded Open-Ended Spiking Conversation (v51+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v48.gen`, `examples/genesis-v47.gen`, `examples/genesis-v46.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
