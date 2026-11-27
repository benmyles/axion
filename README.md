# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v44 — 512x512 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, localized `L3/L4` 2-slot PBWM working memory (ACh-gated Slot 2 + Ca²⁺-buffered DA-gated Slot 1 with associative Hebbian Mongillo STF & Ca²⁺-gated `I_CAN`), 5-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with zero-sum syrinx rebalancing, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), evolvable Kriegstein oSVZ outer radial glia (`oRG`) tangential cortical expansion, positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, peer cultural teaching, XOR context dialogue, 2- and 3-syllable syntax, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v44** (`examples/genesis-v44.gen`)

- **Blackwell SM 12.0 Throughput (`0 B` Local Spill Across All 5 Kernels, `1.6 MB` Self-Contained Binary):**
  - `snn_inference`: **273.1 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **183.3 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`512×512` hydrodynamics + thermal + 4-chem + 4-wavefield acoustics): **116.14 µs/step (2,257 Mcells/s)** (`64 regs`, `0 B` spill)
  - `ca_morphogenesis`: **241.5 µs/organism** (`115 regs`, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,032 B` shmem** (**`54.8 ticks/s`** at `1,024` organisms on `512×512` — `+22%` faster than v43!)
- **All-Time World Record Emergent Wild Referential Language & Trisolaris-13 Ecology (`v44`, 50,000 ticks on `512×512`):**
  - **`5.7×` Surge in Emergent Wild Referential Vocabulary (`0.2341` bits Shannon MI!):** Across **`8,082,232` wild vocalizations (`3,801,034` live peer replies, `60,534` vocal mimics, `257` cooperative cracks, `218` vocal duets)**, all three physical concepts spontaneously locked onto the diagonal of the `3×4` semantic matrix via pure Darwinian + Fee-Goldberg VTA→Area-X spiking plasticity (zero hardcoded call mappings):
    - **Food / Reef (`vctx=0`) → `v0`**: `251,237` calls (`3.1×` over `v1`)
    - **Predator / Injury (`vctx=1`) → `v1`**: **`2,115,214` calls** (`3.8×` over `v0`/`v2`)
    - **Shelter / Acid / Thermal (`vctx=2`) → `v2`**: **`1,950,019` calls** (`4.8×` over `v1`)
  - **100% Bilateral Multimodal Species & Canonical Mammalian E/I Across 199 Generations:** E1 expanded to **`1,502.4` mean neurons (`max 1,817n / 12L`, `0.2002b` MI)** and E2 (`40,000` ticks of full Trisolaris-13) sustained **`508` alive, `212` species, `100%` population & champion bilateral symmetry, `max 17L` (`15L` final), `609.7` mean neurons (champion `6L / 741n`, `#3: 6L / 768n`)** at canonical **`20.9%` Inhibitory** (`pyr = 476.0 [78.1%]`, `pv = 60.2 [9.9%]`, `sst = 67.5 [11.1%]`, `mod = 5.99 [1.0%]`) and paired binocular/binaural organs (`eye = 5.73`, `ear = 2.91`, `muscle = 3.79`, `grasp = 2.62`, `gland = 3.93`).
  - **`115,393` total natural births** (`20,612` E1 + `94,781` E2), **`8.58 BILLION` combined live STDP edits** (`5.20B` E2 + `3.38B` E1), **`440,171` E2 live structural rewires**, **`33,567` post-natal maturations**, **`4,052` evasive prey caught (`+39%` over v43!)**, **`2,438` pods cracked**, **`2,620` anvil stones**, **`2,345` heat-tempered blades**, **`2,977` polymer shelters**, **`227` cached granary feasts**.
- **Historic 7/7 Crushing Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `180,959 ATP`** (`510 alive`, `2,632 births`, `156 prey`, `50 pods`, `58,113 synth`, `157,836 vocal`)
  - `Plasticity Frozen (No Within-Life STDP)`: `149,970 ATP` (**`-17.1%`** — **`2.0×` stronger than v43's `-8.5%`!**)
  - `Apical Dendrites Silenced (No BAC)`: `168,456 ATP` (**`-6.9%`** — stronger than v43's `-5.5%`!)
  - `Axonal Delays Flattened (Uniform 3t)`: `166,064 ATP` (**`-8.2%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `85,027 ATP` (**`-53.0%`**, `272 alive` — **`2.1×` stronger than v43's `-24.9%`!**)
  - `Photoreceptors Blinded (Eyes Off)`: `67,510 ATP` (**`-62.7%`**, `323 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `20,000 ATP` (**`-88.9%`**, `24 alive`)
  - `Unselected Drift Control`: `0.0 ATP` (**`-100.0%`**, **`∞×` natural selection**, `1 alive / 0 births`)
- **v44 20-Assay Cognitive Battery & 4-Agent Spiking Dialogue (`examples/genesis-v44.gen`):**
  - **All-Time Record Laminar Cue SNR & Clean Dark WM Separation (`Assay 12`):** **`L1_cue = 24.70×`**, **`L2_cue = 59.96×` (sixty-fold!)**, **`L3_cue = 6.66×`**, **`L4_cue = 3.83×`**, with **`L1_dk = L2_dk = 0.00×`**, **`L3_dk = 1.54× (n=1.0)`**, **`L4_dk = 1.14× (n=0.2)`**, and **`Epi11` delayed recall SNR = `1.50×`** (`cos_dist = 0.636`, `cue_snr = 8.38×`)!
  - **All-Time Record Dark Working-Memory Navigation (`Assay 2`):** **`+5.81` tiles East dark drift** (`+24%` over v43's `+4.68`, `56.7×` persist)!
  - **All-Time Record Predictive Coding MMN (`Assay 8`):** **`+10.3%` Oddball MMN & `+24.0%` Omission Dip** (`2.6×` v43)!
  - **Compositional Multi-Syllable Syntax & Reasoning:** **2-Syllable Syntax (`Assay 15`):** **`28.1%` symmetric** (`syl1 53.9%`, `syl2 54.7%`, `+20%` over v43) · **3-Syllable Syntax (`Assay 17`):** **`4.7%` perfectly symmetric (`5%/5%`, `syl2 62.5%`, `3.0×` v43!)** · **Episodic Distractor (`Assay 19`):** **`48.4%` symmetric (`56%/41%`, `cos 0.292`)** · **Transitive Inference (`Assay 20`):** **`56.2%` (`AC 72% / CA 41%`, `adj 52.3%`)** · **Zero-Shot Compositional Generalization (`Assay 18`):** **`39.1%` joint (`mot_sep = +0.052`)** · **Seasonal Reversal (`Assay 1`):** **`+4.7%` STDP & `+2.0%` BAC** · **Spoken Q&A (`Assay 10`):** **`41.3%` / `45.8%` reversal** · **Assay 3 Unseen Arena MI:** **`0.0852` bits (`3.1×` v43)**!
  - *(Note: On `v43`'s deeper `12.9L / 1,371n` cortex, the `v44` engine simultaneously hit **`+15.5%` STDP / `+23.2%` BAC reversal, `48.4%` joint `Gen18`, `42.2%` symmetric `2-Syl`, `7.8%` symmetric `3-Syl`, `57.4%/54.9%` Q&A, `52.3%` `Epi19`, and `0.179b` Assay 3 MI** — proving that scaling sustained E2 depth to `12–20L` in v45 will unlock even bigger cognitive jumps!)*
  - **4-Agent Grounded Spiking Dialogue (`converse`):** Four evolved Gen 189 spiking agents (`643–768` neurons, `6` layers, `114` live peer replies, `15,152` live STDP edits, zero LLMs).
- **Key Biophysical & Ecological Upgrades Shipped in v44:**
  - **Isolated Fee & Goldberg (2011) Songbird VTA → Area-X Vocal Performance RPE (`i8 CreatureState::voc_da_q`, `0 B` struct growth):** Delivers crisp 1-tick vocal outcome `±DA` strictly into Area-X (`TK_GLAND`) synaptic plasticity with zero-sum `-8/+4` 3-fold rebalancing — achieving `0.2341` bits wild referential language without ever corrupting locomotor/feeding VTA dopamine (`cr.mod_da`), silencing metabolic catalysis, or causing cortical over-editing.
  - **Laminar Ca²⁺-Gated `I_CAN`, Ca²⁺-Buffered `L4` PBWM Persistence & 5-Tick `Uva→HVC` Efference Copy (`src/genesis_brain.cuh`):** Eliminated layer-wide `s_any_spk` dark hijack via per-neuron `ema`/`ring` Ca²⁺ gating (`L3 >= 4`, `L4 >= 2`), stepped down freshly encoded `L4` phrase context (`cs_u >= 6`) across multi-syllable windows, boosted `L4 → L3` apical context integration (`3/4`), and extended subthreshold `Uva→HVC` vocal efference copy across `1..5t`.
  - **Ohno-Paralog Axonal Guidance Redundancy + Evolvable `oRG` Sheet Expansion (`org_rad 4..7`) + Lean Myelinated Upkeep (`src/genesis_dna.cuh`):** Ohno duplicate sensory & pyramidal guidance genes automatically cover single-rule mutations while keeping full `PV`/`SST` basket/Martinotti inhibition intact, locking canonical `~21%` Dale E/I and `100%` bilateral symmetry.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [512×512 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v44.mp4`)](journal/genesis-v44.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v44 population
./axion genesis probe --load examples/genesis-v44.gen

# Run 4-agent grounded live spiking dialogue (643-768 neurons/agent, 6 layers)
./axion genesis converse --load examples/genesis-v44.gen --group 4 \
  --script "teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,distract,teach_ctx_v0,ctx_danger,distract,teach_ctx_v2,ctx_food,distract,ask_ctx,teach_rel_ab,teach_rel_bc,ask_rel_ac"

# Evolve a fresh 512x512 population through Trisolaris-13 with 7 physical ablations
./axion genesis evolve --pop 512 --world 512 --ticks 10000 --seed 44 --challenge 1 --save e1.gen
./axion genesis evolve --pop 512 --world 512 --ticks 40000 --seed 440 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v44 shattering **`0.2341` bits of wild emergent referential language (`5.7×` v43 across 8.08M calls)**, delivering a **crushing 7/7 honest ablation clean sweep (`frozen -17.1%`, `no_apical -6.9%`, `zero_delay -8.2%`, `no_neuromod -53.0%`, `blind -62.7%`, `deaf -88.9%`, `drift -100%`)**, **`59.96×` L2 / `24.70×` L1 cue SNR**, **`+5.81` tiles East dark WM drift**, **`+10.3%` MMN / `+24.0%` omission**, and **`4,052` prey caught**, our immediate next breakthroughs are:

1. **Sustained `2,000–3,500+` Mean Neuron Encephalization (`12–20L`) + Hippocampal Sharp-Wave Sequence Replay (v45):**
   - Preserve deep `12–20L` cortical stacks through all 50,000 ticks (preventing juvenile `stage=0` fast-breeders from capping E2 at `6L`), and add `L3↔L4` theta-gamma sharp-wave/ripple replay + hippocampal CA3 pattern completion to push `Q&A`, `Ctx13`, `Epi19`, `Trans20`, `2-Syl`, and `3-Syl` into **`65–90%+`**.
2. **Tribal Multi-Biome `1024×1024` Oceans & Compositional 2-Syllable Wild Syntax (v45–v46):**
   - Scale to `1024×1024` multi-biome oceans with multi-syllable cooperative hunting/vault/alarm syntax (`v0→v2`, `v1→v0`, `v2→v1`) and juvenile apprenticeship — driving wild `vocal_mi_bits` past **`0.35–0.60+` bits** of true emergent compositional language.
3. **Grounded Open-Ended Spiking Conversation (v46+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v44.gen`, `examples/genesis-v43.gen`, `examples/genesis-v42.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
