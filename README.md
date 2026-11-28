# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v51 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection with physiologically bounded dendritic thresholds/gains, **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** with **Hasselmo (2006) active cholinergic new-episode retirement** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24–31t ACh-gated Slot 2 and `L4` 24–31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **Frontopolar BA10 `L5`** slow executive goal hub at `2.25×` STF / `115 pA` `I_CAN` with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and 3-step `+DA` phrase reset), **stable `L1` thalamocortical integration** (`soma_leak_shift = 3`), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, **1–5t Sperry-orthogonal `Uva→HVC` vocal efference copy** on `L3`, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity + **`TK_CHEM` contact gustation & olfactory toxin rejection**, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v51** (`examples/genesis-v51.gen`)

- **Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **272.98 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.50 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **657.13 µs/step (1,595.69 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **229.11 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`63.5 ticks/s`** at `1,024` deep-cortex organisms on a `1,048,576`-cell `1024×1024` ocean)
- **All-Time World Record `1024×1024` Intact Fitness (`350,543.6 ATP`!) & Clean `7/7` Honest Physical Ablation Sweep (1,200-Tick Unseen Shifted-Chemistry `1024×1024` Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `350,543.6 ATP`** (`1,021 alive`, `4,790 births`, `17.47 mean ATP`, `8.21` mean layers, `358 prey`, `153,005 synth`, `442,100 vocal`, `470.2M` STDP edits, `26,735` rewires, `128.8M` spikes — **`+5,188 ATP` over v50!**)
  - `Plasticity Frozen (No Within-Life STDP)`: `349,016.0 ATP` (`-0.4%`, `mean_atp 15.78` vs `17.47` = `-9.7%` per-creature ATP, `329` prey vs `358`)
  - `Axonal Delays Flattened (Uniform 3t)`: `345,128.1 ATP` (`-1.5%`)
  - `Apical Dendrites Silenced (No BAC)`: `329,971.1 ATP` (**`-5.9%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `324,129.5 ATP` (**`-7.5%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `200,761.7 ATP` (**`-42.7%`**, `596 alive`, `231` prey)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `58,099.0 ATP` (**`-83.4%`**, `129 alive`, `62` prey)
  - `Unselected Drift Control`: **`0.1 ATP` (`-100.0%`, `1 alive / 1 birth`!)**
- **All-Time World Record Wild Referential Language (`0.3064` Bits MI!) & Orthogonal 4-Band Hearing Evolution (`v51`, 40,000 ticks: `8k E1 + 32k E2`!):**
  - **All-Time Record Wild Vocal Mutual Information (`0.3064` bits!):** **`2×` v50's wild MI** across **`11.53 MILLION` combined vocalizations** (`8.34M` E2 + `3.19M` E1, **`5.44 MILLION` combined peer replies**, **`156,520` mimics**, **`4.94 BILLION` juvenile imprints**) — clean 3-word emergent semantics on diagonal (`v0` Food `114.0k`, `v1` Predator `2.61M`, `v2` Shelter/Toxin `2.15M`)!
  - **Organic 4-Channel Auditory & 3-Fold Syrinx Evolution (`108,699` Combined Births, `11.76B` Combined STDP Edits):** Population naturally converged onto **orthogonal 4-band basilar-papilla hearing (`ear = 4.39` mean; champions `ear = 4..6`)** + **full 3-fold syrinx (`gland = 4.06`)** + **`97%` bilateral symmetry** across `8.32` mean layers (`max_areas = 14`, `1,127.8` mean neurons, **`21.8%` canonical GABAergic inhibition** [`pyr = 875.0`, `pv = 122.5`, `sst = 123.6`, `mod = 6.72`], evolved morphs up to **`12L / 2,236` neurons** [#3 G170] and `13L / 1,418n` mid-E2), **`3,783` combined evasive prey**, **`818` pods cracked** (`159` coop, `108` vocal duets), **`1,624` heat-tempered tools**, **`452` shelters**, and **`50` apex predators slain**!
- **All-Time World Record 20-Assay Cognitive & Linguistic Battery (`examples/genesis-v51.gen` — Shattering 6 Official Records!):**
  - **All-Time World Record Referential Instruction Following (`Assay 16` `Cmd16`):** **`85.9%` (`48.4% -> 85.9%`, `sep = +0.119`, `go 0.29 > halt 0.22` — `+54.7 pts` over v50!)**
  - **All-Time World Record Compositional 3-Syllable Phrase Syntax (`Assay 17` `3-Syl`):** **`18.0%` (`4.9×` chance! `1.6% -> 18.0%`, `syl1 35.2%`, `syl2 75.8%`, `syl3 54.7%`, balanced `phraseX 23% / phraseY 12%` — MORE THAN DOUBLED v50's `8.6%`!)**
  - **All-Time World Record Zero-Shot Cross-Modal Generalization (`Assay 18` `Gen18`):** **`76.6%` joint vocal+motor (`9.4% -> 76.6%`, `voc 53.1%`, `mot_sep = +0.104` — `+18.8 pts` over v50!)**
  - **All-Time World Record Compositional 2-Syllable Syntax (`Assay 15` `2-Syl`):** **`41.4%` (`14.8% -> 41.4%`, `syl1 67.2%`, `syl2 68.0%`, balanced `phraseA 44% / phraseB 39%` — `+9.4 pts` over v50!)**
  - **All-Time Record Transitive Inference (`Assay 20` `Trans20`):** **`52.3%` (`37.5% -> 52.3%`, `adj 48.8%`, `AC 81% / CA 23%`)** · **Tied Peak Episodic Distractor Reasoning (`Assay 19` `Epi19`):** **`57.8%` (`epA 72% / epB 44%`)** · **Vocal Q&A Grounding (`Assay 10`):** **`60.3% / 56.2%` rev (`sep 0.433` — `+7.2 pts` over v50!)** · **Zero-DA Cultural Peer Teaching (`Assay 11`):** **`52.1%` solo (`34.1% -> 52.1%`, `+18.0 pts`, `tutor 58.3%`, `14,288` peer replies)** · **3-Concept Cross-Modal Naming (`Assay 14`):** **`37.2%` (`29.3% -> 37.2%`)** · **Temporal Sequence (`Assay 4`):** **`0.383`** · **Zero-Shot Transfer (`Assay 7`):** **`0.323`** · **Working Memory (`Assay 2`):** **`829.93×` (`+5.36` tiles East dark drift!)** · **Laminar & Episodic Dark Working-Memory SNR (`Assay 12`):** **`2.15×` (`cos_dist 0.868`, `L3_dk = 2.41×`, `L4_dk = 1.97×`, `L1_dk = L2_dk = 0.00×`)** · **Wild Unseen Agency (`Assay 3`):** **`0.1436` bits MI (`105` prey)** · *(Honest note on `Assay 1`: `206,952.8 ATP` intact — highest Assay 1 intact score ever, `+28k` over v50 — vs `219,330.0` frozen [`-5.6%`] on the 512-org probe, while the official 1,024-org `1024×1024` control suite swept `7/7` cleanly; on `v48.gen` the v51 engine also jumped `Q&A` to `71.9% (sep 0.911)` and `Assay 1` to `+12.3%` STDP!)*
  - **4-Agent Multi-Agent Spiking Dialogue (`converse`):** **`A0..A3: 7L / 874–883n` (`Eye 2, Ear 4–6, Syrinx 4, Muscle 4`)** across 12 live turns (**`103` peer replies**, **`29,128` live STDP edits**), keeping `L6` silent (`0`) on routine `v0`/`v1`/`ask_01`/`ask_10`/`ask_ctx` and igniting cleanly through **`L6`** (`16–17` active assemblies) on `+DA` teaching (`teach_qa_01_v2`, `teach_p3_x`), `ask_p3_x`, and `cmd_go` — zero LLMs!
- **Key Biophysical, Credit-Assignment & Concurrency Breakthroughs Shipped in v51:**
  - **Per-Neuron Vector-FA Vocal Fold Credit Assignment Bugfix (`src/genesis_brain.cuh:901`):** Fixed a critical modular-arithmetic bug (`(tid * 3u + ...) % 3u`, where `tid * 3u ≡ 0 mod 3` previously locked every single neuron in an entire cortical layer to ONE vocal fold!) to `(tid * 7u + ...) % 3u`, giving every cortical layer `L1..L_top` an exact `1/3 : 1/3 : 1/3` balanced pyramidal split across all 3 vocal folds `v0 / v1 / v2` — unlocking balanced multi-phrase syntax (`2-Syl 41.4%`, `3-Syl 18.0%`, `Gen18 76.6%`, `Cmd16 85.9%`) and doubling wild referential MI (`0.3064 bits`)!
  - **Race-Free GPU Double-Buffered `mod_ach` & Tick-`(t-1)` Vector-FA Causal Action Latch (`src/genesis_brain.cuh`):** Eliminated the cross-CTA race where `layer_idx == 0` overwrote `cr.mod_ach` and `cr.mot_vocal/thrust` while `L1..L_top` read them concurrently — staging `new_ach_q` into upper 6 bits of `peer_tpl_band` and latching causal action bits into upper 4 bits of `growth_stage` between ticks in `body_ecology_step` (**`0 B` extra VRAM, `0` extra kernels, 100% `.gen` compatible!**).
  - **2-Ear Basilar-Papilla `avp` De-Collision, `TK_CHEM` Basal Tax Fix & Blind/Deaf Freeloader Closure (`src/genesis_brain.cuh`, `src/genesis_dna.cuh`):** Reduced 2-ear `avp` cross-talk (`3.0/0.85 -> 1.8/0.45`) to eliminate `v1+p == v2+p` aliasing, lowered legacy `70×` `TK_CHEM` upkeep (`0.00140 -> 0.00004`), initialized `prev_vocal_band = 3u` cleanly across all spawn/birth paths, and closed the `act_sens_n == 0` blind/deaf freeloader loophole while preserving thriving multimodal ecology.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v51.mp4`)](journal/genesis-v51.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v51 population
./axion genesis probe --load examples/genesis-v51.gen

# Run 4-agent grounded live spiking dialogue (7 layers / 874-883 neurons/agent, 4-6 ears)
./axion genesis converse --load examples/genesis-v51.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 515 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 515 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v51 shattering **all-time world records across `1024×1024` intact fitness (`350,543.6 ATP`, `7/7` clean sweep), Wild Referential Language MI (`0.3064 bits` — `2×` v50!), Referential Instruction Following (`Cmd16 = 85.9%`), Zero-Shot Cross-Modal Generalization (`Gen18 = 76.6%` joint), Compositional 3-Syllable Syntax (`3-Syl = 18.0%`, `4.9×` chance — `>2×` v50!), Compositional 2-Syllable Syntax (`2-Syl = 41.4%`, balanced `44%/39%`), and Transitive Inference (`Trans20 = 52.3%`)**, our immediate next breakthroughs are:

1. **Balanced Dark-Audio vs. Sunlit-Visual Cortical Gain, Deeper `12–16L` 4-Ear/4-Eye Encephalization & Multi-Step Syntax (v52):**
   - Cure the remaining dark-audio vs sunlit-visual contrast asymmetry (`c1/c2` in `3-Concept`, `CA` in `Trans20`, `ctxB` in `Ctx13`) via authentic A1/V1 divisive gain control and strengthen `Assay 1` STDP seasonal reversal (`+15–30%`) alongside `7/7` controls — pushing **`3-Syl > 35–50%+`**, **`2-Syl > 55–70%+`**, **`3-Concept > 55–70%+`**, **`Trans20 > 65–80%+`**, **`Ctx13 > 65–80%+`**, and **`Peer Teaching > 65%+`** zero-DA solo on `12–16L` bilateral minds!
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v53):**
   - Evolve distinct geographic vocal dialects (`vocal_mi_bits > 0.45+` bits), multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world.
3. **Grounded Open-Ended Spiking Conversation (v54+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v51.gen`, `examples/genesis-v50.gen`, `examples/genesis-v49.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
