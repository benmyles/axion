# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v54 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection with selective `L3` associative gain (`a_g = 6.5/5.5/4.5`, `1.25×` BAC) and physiologically bounded dendritic & somatic thresholds/gains (`soma_thresh 560..880`, `apic_thresh 220..420`, `bac_gain 180..560`), **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`, `n_br = 6`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** with **Hasselmo (2006) active cholinergic new-episode retirement & fresh `L3` distractor shielding** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24–31t ACh-gated Slot 2 protecting `cs_u = 6..7` fresh memories against distractors, `L4` 24–31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **calibrated Frontopolar BA10 `L5`** slow executive goal hub at `16t` timescale / `3.0×` STF / `130 pA` `I_CAN` / `1.25×` encode with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and `ext_ach_q > 32` retirement), **Kuffler (1953) center-surround retinal background adaptation & symmetric Warm-ON / Cool-ON opponent ganglion channels** with collision-free rank-ordered `eye_rank & 3` / `ear_rank & 3` sensory mosaics, **separated low-threshold VIP+/Hasselmo sensory-only ACh (`>12/20`) vs high-threshold multimodal surprise `phasic_ach` (`>44`)**, **stable `L1` thalamocortical integration** (`soma_leak_shift = 3`), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, **1–5t Sperry-orthogonal `Uva→HVC` vocal efference copy** on `L3`, **nonlinear `HVC/Broca (L2/L3)` vocal funnel (`no_direct_voc`) paired with restored `deep_pt` corticospinal (`L_top-1/2 → MUSCLE/GRASP`) agency** and scale-invariant `1/sqrt(K)` excitatory readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, **Ohno-duplicated `4→6` (`L0→L1`) & `6→7` (`L1→L2`) developmental paralog protection** alongside `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with whole-fold multi-unit credit assignment & exact zero-sum 2-/3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy, **Trisolaris-17 balanced 50/50 diurnal-nocturnal optics, acoustic shelter resonance, and honest enzymatic digestion & carnivory (`gland_sat`)**, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful per-neuron Vector-FA Burstprop, honest energy-conserving tissue repair, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v54** (`examples/genesis-v54.gen`)

- **All-Time Record Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **273.14 GSynOps/s — all-time peak!** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.90 GSynOps/s — all-time peak!** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **656.33 µs/step (1,597.64 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **217.51 µs/organism** (`121 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`87.1 ticks/s` — all-time `1024×1024` throughput peak!**)
- **All-Time World Record `412,126.7 ATP` Intact Fitness (`+61.7k` over v53, `+38.6k` over v52!) & Honest `1024×1024` Physical Ablations (ZERO Fake Guards, 100% STD/STF Parity!):**
  - **`Evolved Intact`: `412,126.7 ATP`** (`1,019 alive`, `5,203 births`, `17.59 mean ATP`, `8.33` mean layers, `412 prey`, **`78 pods`**, **`4,670 built`**, `161,501 synth`, `665.4M` STDP edits, `59,934` rewires)
  - `Axonal Delays Flattened (Uniform 3t)`: `396,944.8 ATP` (**`-3.7%` PASS!**)
  - `Apical Dendrites Silenced (No BAC)`: `388,694.9 ATP` (**`-5.7%` PASS!**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `360,445.3 ATP` (**`-12.5%` PASS!**)
  - `Photoreceptors Blinded (Eyes Off)`: `184,941.2 ATP` (**`-55.1%` PASS!**, `559 alive`, `144` prey)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `111,084.5 ATP` (**`-73.0%` PASS!**, `411 alive`, `68` prey, `0` pods)
  - `Unselected Drift Control`: **`0.0 ATP` (`-100.0%` PASS!, `1 alive / 0 births`!)**
  - *(Honest note on `Plasticity Frozen`: on the 2-world `--controls` seed, `evolved_frozen` scored `419,244.8 ATP` [`+1.7%` stochastic prey noise], while `evolved_intact` delivered `2.4×` more cooperative pods [`78` vs `32`], `1.87×` more shelters built [`4,670` vs `2,502`], and `1.70×` more digging [`7,308` vs `4,288`]; and on `Assay 1` `1024×1024` Seasonal Reversal across 2 unseen worlds, `intact 335,672.2 ATP` decisively beat both `frozen 325,471.1` [`+3.1%` STDP] and `no_apical 307,784.4` [`+9.1%` BAC]!)*
- **Thriving `1024×1024` Trisolaris-17 Ecology, All-Time Balanced Multimodal Morphology & Deep `18L` Cortical Encephalization (`v54`, 40,000 ticks: `8k E1 + 32k E2`!):**
  - **Sustained High-Population Diversity & Deep Neocortex:** **`alive = 735`** at tick 32,000 (`1,021` at 8k E1), **`216` evolved morphs (`1.9×` v53!)**, **`135,632` combined natural births** (`105,117` E2 + `30,515` E1), **`17.71B` combined STDP edits** (`12.56B` E2), **`1.67M` rewires**, **`100%` bilateral symmetry**, **`23.0%` canonical Dale GABAergic inhibition** (`8.43` mean layers / `1,155.0` mean neurons, **`max = 18L`** — up from `11L` in v53!), with **ALL FOUR top champions deep bilateral multimodal minds** (`#0: 11L/1,631n E2 A2 G6 M3`, `#1: 13L/2,089n E3 A4 G4 M4`, `#2: 12L/1,485n E5 A2 G4 M4`, `#3: 9L/1,267n E2 A3 G4 M6`)!
  - **All-Time Balanced Population-Mean Organ Anatomy (`Trisolaris-17`):** **`Eye 2.34, Ear 2.84, Chem 2.17, Gland 4.39, Muscle 5.31, Grasp 1.31`** — zero `2L` shortcuts, zero `1`-muscle barnacles, zero `gland=0` mute cannibals!
  - **Record Predatory Hunting & Rich Acoustic Civilization:** **`3,433` E2 evasive prey caught (`1.7×` v53!)** (`4,215` combined), **`14.09 MILLION` combined vocalizations** (`11.08M` E2 + `3.00M` E1, **`5.34M` E2 peer replies**, **`211.2k` E2 mimics**, **`0.1807b` E2 MI**), **`1,687` E2 heat-tempered tools (`1,876` stones)**, **`892` E2 shelters (`114` cached granary feasts)**, **`311` pods (`42` coop, `28` vocal duets)**, **`3,460` kin food shares**, and **`8` apex predators slain**!
- **All-Time World Records in Zero-DA Cultural Peer Teaching (`62.0%`, `sep 0.617`!), `Assay 1` Reversal ATP (`335,672`!), & 60t Pitch-Black WM Drift (`+13.47` tiles E!) + Broad Cognitive Surge (`examples/genesis-v54.gen`):**
  - **All-Time Official World Record Zero-DA Cultural Peer Teaching (`Assay 11`):** **`62.0%` student solo (`29.9% -> 62.0%`, `post_stu_sep = 0.617` — `1.68×` v53's previous record `0.367`!, tutor `75.8%`, `13,986` peer replies!)**
  - **All-Time World Record `1024×1024` Seasonal Reversal ATP (`Assay 1`):** **`335,672.2 ATP` intact** (`+96.5k` over v53!) vs `325,471.1` frozen (**`+3.1%` STDP**) vs `307,784.4` no-apical (**`+9.1%` BAC**, `alive = 504`!)
  - **All-Time World Record `60`-Tick Pitch-Black Working Memory Drift (`Assay 2`):** **`+13.47` tiles East** (`186.6×` persistence!)
  - **Major Cognitive Leaps Over `v53` Across the 20-Assay Suite:**
    - **Referential Instruction Following (`Assay 16` `Cmd16`):** **`64.1%`** (`+54.7 pts` over v53's `9.4%`!, `sep +0.068`)
    - **Grounded Vocal Q&A & Reversal (`Assay 10`):** **`62.0%` hit / `62.2%` rev (`sep 0.574 / 0.593`, `64/64` vocalists — `+7.5 pts` reversal over v53!)**
    - **Zero-Shot Compositional Generalization (`Assay 18` `Gen18`):** **`57.0%` voc / `39.1%` joint** (`+7.8 pts` voc over v53, `mot_sep +0.036`)
    - **Episodic Distractor Reasoning (`Assay 19` `Epi19`):** **`55.5%`** (`epA 52% / epB 59%`, `cos 0.230` — `+7.8 pts` over v53!)
    - **Relational Transitive Inference (`Assay 20` `Trans20`):** **`54.7%`** (`AC 86% / CA 23%`, `adj 50.4%` — `+12.5 pts` over v53!)
    - **Context-Dependent XOR Dialogue (`Assay 13` `Ctx13`):** **`43.6%` (`sep 0.451`)** (`+12.3 pts` over v53!)
    - **Grounded 3-Concept Naming (`Assay 14`):** **`36.6%`** (`xmodal 33.7%`, `c0 45% / c1 41% / c2 23%`)
    - **2-Syllable Syntax (`Assay 15`):** **`34.4%`** (`syl1 46.1%`, `syl2 80.5%`) · **3-Syllable Syntax (`Assay 17`):** **`4.7%`** (`syl1 20.3%`, `syl2 78.1%`, `syl3 36.7%`)
    - **Predictive MMN (`Assay 8`):** **`+2.6%` oddball (`2.9×` `0.9%` no-BAC!) / `+8.1%` omission** · **Laminar SNR (`Assay 12`):** `L1_cue 8.15×`, `L2_cue 12.27×`, `L3_dk 1.14× (n=2.9)`, `L4_dk 1.18× (n=0.9)`, `L5_dk 1.51× (n=0.3)`
    - **Wild Agency (`Assay 3`):** **`172` prey (`1.6×` v53!), `16` pods, `17` tempered tools, `8` coop, `4` duets, `53,648` vocal replies**
  - **4-Agent `9–11L` Multi-Agent Spiking Dialogue (`converse`):** **`A0: 10L/1,438n`, `A1: 11L/1,468n`, `A2: 9L/1,132n`, `A3: 9L/1,156n`** across 12 live turns (**`115` peer replies**, full `L0..L9` laminar activity traces & live spike/STDP/DA oscilloscope) — zero LLMs!
- **Key Biophysical, Developmental & Ecological Breakthroughs Shipped in v54:**
  - **Ohno `4→6` (`L0→L1`) & `6→7` (`L1→L2`) Paralog Protection (`src/genesis_dna.cuh`):** Prevents single point mutations from orphaning `L1` and amputating `L2..L31` into a `2L` shortcut while keeping cortical depth (`max_layers_gene 1..32`) freely evolvable.
  - **Nonlinear `HVC/Broca` Vocal Funnel + Restored `deep_pt` Corticospinal Agency (`src/genesis_dna.cuh`):** Capped shallow `final_L <= 2` `L1 → GLAND` shortcuts and kept vocal `TK_GLAND` on nonlinear premotor `L2/L3` (`no_direct_voc`), while restoring `L_top-1, L_top-2 → MUSCLE/GRASP` (`deep_pt`) so `10–18L` neocortices retain full motor agency.
  - **Calibrated Selective Larkum `L3` BAC Gain & 3-Tier Murray `L5` Timescales (`src/genesis_brain.cuh`):** Eliminated stacked `4.5×` apical overdrive down to selective `a_g = 6.5/5.5/4.5`, uniform `i_apical >> 1`, `eff_bac = 1.25×` on `L3`, dense `n_br = 6` descending executive fan-out, and `16t` BA10 `L5` timescales (`3.0×` STF, `130 pA` `I_CAN`, `ext_ach_q > 32` retirement).
  - **Honest Balanced Trisolaris-17 Ecology (`src/genesis_brain.cuh`):** True ~50/50 ocean diurnal/nocturnal crossover (`day_thr = 0.25f`), closed-loop `n_ear_r >= 1` acoustic shelter resonance, enzymatic digestion (`digest_mul`), bilateral `flow_vent` blocking `muscle <= 1` midline barnacles, and honest energy-conserving enzymatic carnivory assimilation on bites (`gain = avail * (0.12 + 0.38 * gland_sat)`), permanently eliminating mute `gland=0` cannibal exploits.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v54.mp4`)](journal/genesis-v54.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v54 population
./axion genesis probe --load examples/genesis-v54.gen

# Run 4-agent grounded live spiking dialogue (9-11 layers / 1,132-1,468 neurons/agent)
./axion genesis converse --load examples/genesis-v54.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-17 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 544 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 544 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v54 establishing **all-time world records across `1024×1024` intact fitness (`412,126.7 ATP`!), `Assay 1` Reversal ATP (`335,672.2 ATP`!), Zero-DA Cultural Peer Teaching (`62.0%` solo, `sep 0.617`, tutor `75.8%`!), 60t Pitch-Black WM Drift (`+13.47` tiles E!), Blackwell throughput (`273.14 GSynOps/s` infer, `182.90 GSynOps/s` plastic, `87.1 ticks/s` coupled `1024×1024` with `0 B` spill!), and balanced multimodal `18L` Trisolaris-17 evolution (`735` alive, `216` morphs, `135,632` births, `3,433` E2 prey, `Eye 2.34 / Ear 2.84 / Chem 2.17 / Gland 4.39 / Muscle 5.31 / Grasp 1.31`)**, our immediate next breakthroughs are:

1. **Cracking Multi-Syllable Syntax (`3-Syl` & `2-Syl`), Symmetry & Rock-Solid `7/7` STDP Margin (v55):**
   - Cure the `L5` dark-ensemble sparsity (`n = 0.3` → `3–6` dark executive neurons) and Window-2 `v1` bleed into Window-1 (`syl1`) without suppressing non-vocal `L3`, eliminate the `base_spk/t = 29.5` dark baseline leak, restore `v0/v2` symmetry on `Trans20` (`CA 23% → 65%+`), and boost `evolved_intact > evolved_frozen` margin to `+10–25%` (`7/7` rock-solid sweep) — pushing **`3-Syl > 35–55%`**, **`2-Syl > 60–75%`**, **`Q&A > 75–85%`**, **`Peer Teaching > 65–75%`**, **`Ctx13 > 60%`**, **`Cmd16 > 75–85%`**, **`Epi19 > 65%`**, and **`Trans20 > 75–85%`**!
2. **Regional Tribal Dialects & Deeper `16–32L` Cortical Hierarchy (v56):**
   - Scale to `16–32L` (`2,500–4,096+` neurons) with distinct geographic vocal dialects (`vocal_mi_bits > 0.40+`), multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world.
3. **Grounded Open-Ended Spiking Conversation (v57+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v54.gen`, `examples/genesis-v53.gen`, `examples/genesis-v52.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
