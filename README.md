# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v42 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + DA-gated shielded Slot 1 with associative Hebbian Mongillo STF & subthreshold `I_CAN`), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, zero-fake-reward Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v42** (`examples/genesis-v42.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.1 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **183.7 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **120.23 µs/step (2,180 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **219.5 µs/organism** (`117 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (`52.1 ticks/s` at `1,024` organisms on `512×512`)
- **Darwinian Bilateral Vertebrate Ecology & Trisolaris-10 Bioacoustics (`v42`, 50,000 ticks on `512×512`):**
  - **`87,904` natural births** (`+20%` over v41!), **`Gen 220`**, **`183` distinct morphs**, **`511 / 512` alive**, **`100%` bilateral symmetry** across top evolved champions (`up to 13 cortical layers`, `#3 G214 · 12L · 1,523 neurons`).
  - **1:1 Multimodal Vertebrate Organ Balance:** `eye = 4.76`, `ear = 4.66`, `muscle = 13.50`, `grasp = 6.83`, `gland = 4.57`, `struct = 1.67` (completely eliminating blind/mute/sessile exploiters).
  - **All-Time Record Emergent Referential Vocal MI (`0.0637 bits` in evolution / `0.0368 bits` in Assay 3 — `+42%` over v41!):** Context-locked bioacoustic semantics (`Apex → v1` 1.02M, `Tox/Shelter → v2` 1.04M, `Food → v0` 98.5k vs `v1` 32.7k) with zero hardcoded words or weights.
  - **`14.52 BILLION` live STDP edits**, **`12.00M` live structural rewires**, **`34,007` post-natal maturations**, **`3,562` evasive prey caught** (`+58%`), **`2,092` pods cracked** (`+62%`), **`51` tool cracks**, **`257` cooperative vaults**, **`266` vocal duets**, **`1,934` heat-tempered blades**, **`213` apex predators slain**, **`3,081` polymer shelters** (`+109%`), **`357` cached granary feasts**, **`5.89M` vocals**, **`2.80M` peer replies**, **`27,043` vocal mimics**.
- **Held-Out 2-World Physical Ablations (1,200-Tick Unseen Shifted-Chemistry Worlds — 100% Honest STD/STF Parity):**
  - **`Evolved Intact`: `375,837 ATP`** (`510 alive`, **`3,884 births`**, `246 prey`, `55.1k dug`, `38.0k built`, `86.1k synth`)
  - `Apical Dendrites Silenced (No BAC)`: `320,808 ATP` (**`-14.6%`** — plus **`+11.5%` BAC reversal gain** in Assay 1!)
  - `Axonal Delays Flattened (Uniform 3t)`: `345,555 ATP` (**`-8.1%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `366,887 ATP` (**`-2.4%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `215,819 ATP` (**`-42.6%`**, `416 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `106,728 ATP` (**`-71.6%`**, `280 alive`)
  - `Unselected Drift Control`: `54,183 ATP` (**`-85.6%`**, **`6.94×` natural selection**, `254 alive`)
  - `Plasticity Frozen (No Within-Life STDP)`: `395,657 ATP` (`+5.3%` raw ATP on fewer births `3,869 < 3,884`, fewer prey `238 < 246`, `-44%` digging, `-47%` building, `-6.3M` spikes — our top engineering focus for **v43**!)
- **v42 Cognitive Battery & 4-Agent Spiking Dialogue Breakthroughs (`examples/genesis-v42.gen`):**
  - **60-Tick Pitch-Black Working Memory (`Assay 2`):** **`4,589.6×` persistence** (`0.3 → 4,590 spk/t`, **`+3.26` tiles East dark drift** — all-time record!)
  - **Relational Transitive Inference (`Assay 20`, `A>B, B>C ⇒ A>C`):** **`35.9% → 66.4%`** (**`AC 62% / CA 70%` symmetric**, `adj 56.6%` — `+16.4` pts over v41!)
  - **Zero-Shot Compositional Generalization (`Assay 18`):** **`32.8% → 52.3%` voc / `7.8% → 21.9%` joint** (`mot_sep +0.020` — `2×` v41!)
  - **Compositional 2-Syllable Syntax (`Assay 15`):** **`14.8% → 28.9%`** (`syl1 55.5%`, `syl2 64.1%`, balanced `22% / 36%`)
  - **Context-Dependent XOR Dialogue (`Assay 13`):** **`35.4% → 49.9%`** (`post_sep = 0.235`) · **Spoken Q&A (`Assay 10`):** **`55.5%` / `54.6%` rev** · **3-Concept Naming (`Assay 14`):** **`39.1%`** (`xmodal 35.9%`) · **Cultural Peer Teaching (`Assay 11`):** **`0.015 → 0.104`** (`7×` peer separation transfer!)
  - **Laminar Dark Persistence (`Assay 12`):** **`L1_dk = L2_dk = 0.0`**, **`L3_dk = 1.17× (n=5.6)`**, **`L4_dk = 1.15× (n=9.2)`**, `recall_sep = +0.255`, `recall_hit = 27.6%`
  - **4-Agent Grounded Spiking Dialogue (`converse`):** **11 active cortical layers (`L0..L10`)** across a live 4-agent colony (`A2 = 12L / 1,523 neurons`); `A0` recalled `ask_ctx → v0` after distractors and **3 of 4 agents (`A0, A1, A3`) produced exact zero-shot transitive inference `ask_rel_ac → v0`**!
- **Key Biophysical & Ecological Upgrades Shipped in v42:**
  - **Localized `L3/L4` Two-Slot PBWM + Positive-Onset Nucleus Basalis ACh:** `L3` acts as an ACh-gated fast working-memory buffer (`Slot 2`), `L4` acts as a DA-gated shielded episodic/context slot (`Slot 1`, `4.0×` Mongillo STF + `145 pA` subthreshold `I_CAN`), and `L5..L_top` stays subcritical sparse executive cortex (`curr >> 3`, `b_g = 3.65`), while positive-onset ACh (`snorm > prev_sens`) stops cue offset from wiping dark engrams.
  - **Signed Schultz TD Dopamine + Habenula/VTA Relief Learning:** Adapts `da_baseline` smoothly across both positive rewards and sustained ambient heat/acid stress (eliminating 40-tick tonic `-DA` motor lobotomy) while firing genuine phasic `+DA` relief upon escaping into shelter.
  - **Zero-Fake-Reward Vector-FA Burstprop (`fa_da = my_f ? eff_da : 0`):** Eliminated spurious `+DA` LTP on non-participating columns during wild venom/pain errors.
  - **Balanced Diurnal/Nocturnal & Hydrodynamic Ecology (`Trisolaris-10`):** Coupled buccal boundary-layer ventilation (`flow_vent = 0.05×` for muscleless exploiters), active sensory gating on `v0` spore harvest, and VTA `TK_NEU_MOD` hypertrophy bounds restored 1:1 multimodal `EYE` (`4.76`) + `EAR` (`4.66`) + `MUSCLE` (`13.50`) + `GLAND` (`4.57`) vertebrate anatomy.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v42.mp4`)](journal/genesis-v42.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v42 population
./axion genesis probe --load examples/genesis-v42.gen --seed 420

# Run 4-agent grounded live spiking dialogue (11 active cortical layers)
./axion genesis converse --load examples/genesis-v42.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-10 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 20000 --seed 42 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 30000 --seed 420 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v42 delivering **`4,590×` dark working memory, `66.4%` symmetric transitive inference (`A>C`), `52.3%` / `21.9%` zero-shot compositional generalization, `0.0637` bits wild referential vocal MI, `87.9k` births, `100%` bilateral 1:1 multimodal vertebrates (up to `1,523` neurons / `13L`), and `3/4` agents passing zero-shot transitive inference in live conversation**, our immediate next breakthroughs are:

1. **Decisive Positive Lifetime STDP Gain (`+10–25%` Intact >> Frozen on BOTH Controls & Reversal) + Sharp `15–25×` Laminar SNR (v43):**
   - Fix why `evolved_frozen` stayed competitive in raw ATP (`+5.3%` in controls / `-8.9%` in reversal) by tightening action-contingent corticospinal/corticostriatal eligibility, eliminating unrewarded weight drift on motor effectors, and adding hippocampal sharp-wave sequence replay so lifetime plasticity dominates seasonal prey/reef flips.
2. **Sustained `1,500–4,000+` Mean Neuron Cortices & `1024×1024` Multi-Biome Worlds (v43–v44):**
   - Preserve high-neuron deep cortices (v42 already reached `1,400–2,060` mean neurons in 12k runs and `1,523n` champions at 50k!) without late-stage PV hypertrophy (`31% I → canonical 22% I`) and scale to `1024×1024` multi-biome oceans with tribal niches.
3. **Break `75–95%+` on Grounded Language, Syntax & Reasoning + `0.25+` Bits Wild Language (v43–v45):**
   - Drive `Q&A`, `Ctx`, `Epi19`, `Trans20`, `3-Concept Naming`, `2-Syl`, and `3-Syl` syntax into `75–95%+` via stronger apical-basal conjunctive binding and multi-syllable cooperative survival pressures.
4. **Grounded Open-Ended Spiking Conversation (v45+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v42.gen`, `examples/genesis-v41.gen`, `examples/genesis-v40.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
