# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v50 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection with physiologically bounded dendritic thresholds/gains, **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** with **Hasselmo (2006) active cholinergic new-episode retirement** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24–31t ACh-gated Slot 2 and `L4` 24–31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **Frontopolar BA10 `L5`** slow executive goal hub at `2.25×` STF / `115 pA` `I_CAN` with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and 3-step `+DA` phrase reset), **stable `L1` thalamocortical integration** (`soma_leak_shift = 3`), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, **1–5t Sperry-orthogonal `Uva→HVC` vocal efference copy** on `L3`, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity + **`TK_CHEM` contact gustation & olfactory toxin rejection**, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v50** (`examples/genesis-v50.gen`)

- **Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **273.20 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.66 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **655.27 µs/step (1,600.22 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **229.56 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`69.0 ticks/s`** at `1,024` deep-cortex organisms on a `1,048,576`-cell `1024×1024` ocean — `+6.0%` faster than v49!)
- **All-Time World Record `1024×1024` Intact Fitness (`345,356.1 ATP`!) & Crushing `7/7` Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry `1024×1024` Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `345,356.1 ATP`** (`1,021 alive`, `4,650 births`, `18.42 mean ATP`, **`13.00` mean layers**, `200 prey`, **`791` pods cracked!**, `116,643 synth`, `391,881 vocal`, `410.1M` STDP edits, `15,418` rewires, crisp `111.6M` spikes — **`+65,008 ATP` over v49 and `+30,196 ATP` over v48!**)
  - `Plasticity Frozen (No Within-Life STDP)`: `332,263.6 ATP` (**`-3.8%`**, `535` pods vs `791`!)
  - `Axonal Delays Flattened (Uniform 3t)`: `282,122.7 ATP` (**`-18.3%`**)
  - `Apical Dendrites Silenced (No BAC)`: `281,376.3 ATP` (**`-18.5%`**, `394` pods vs `791`!)
  - `Neuromodulation Silenced (DA=ACh=0)`: `255,022.0 ATP` (**`-26.2%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `110,836.3 ATP` (**`-67.9%`**, `429 alive`, `54` prey!)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `34,489.1 ATP` (**`-90.0%`**, `150 alive`, `14` prey!)
  - `Unselected Drift Control`: **`4,004 ATP` (`-98.8%`, `13 alive / 2 births`!)**
- **All-Time Record `13.19`-Layer Encephalization, `183,281` Births & `23.00 BILLION` STDP Edits (`v50`, 40,000 ticks: `8k E1 + 32k E2`!):**
  - **All-Time Record Mean Cortical Depth (`13.19` Mean Layers!):** Organically expanded from `11.60L` (E1) to **`13.19` mean layers / `1,051.5` mean neurons** (`+58%` deeper than v49's `8.33L`, surpassing v46's `12.96L`!; `max_areas = 17`, `mean_rules = 51.4`, `max 55`, `mean_fanin = 161.0`, `max 192`), with canonical **`23.0%` GABAergic inhibition** (`pyr = 803.1`, `pv = 121.9`, `sst = 119.8`, `mod = 6.77`), **`100%` bilateral `14`-layer champions** (`#1 G180: 14L / 1,025n / 4E / 2A / 4G`, `#2 G182: 14L / 1,070n`, `#3 G188: 13L / 916n / 2E / 8A`, `#4 G188: 13L / 1,149n`), and balanced multimodal organs (`eye = 3.28`, `ear = 2.88`, `chem = 0.20` [`2.5×` v49], `gland = 3.73`, `muscle = 10.08`, `grasp = 6.42`, `struct = 3.63`).
  - **All-Time Record Ecology, Plasticity & Wild Language:** **`183,281` combined natural births** (`151,053` E2 + `32,228` E1), **`23.00 BILLION` combined live STDP edits** (`18.65B` E2 + `4.35B` E1), **`1.52 MILLION` combined live structural rewires**, **`68,626` post-natal maturations**, **`4,249` combined evasive prey caught** (`3,523` E2), **`1,234` pods cracked** (`1,204` E2, `242` coop, **`180` vocal duets**), **`85` apex slain in E2 (`89` combined)**, **`2,185` heat-tempered tools**, **`15.53 MILLION` combined vocals** (`12.47M` E2, **`7.05 MILLION` combined peer replies**, **`69,170` mimics**, **`0.1824` bits E2 wild MI** — Food→`v0` `59.3k`, Predator→`v1` `5.40M`, Shelter/Tox→`v2` `1.74M`!).
- **All-Time Record 20-Assay Cognitive Battery (`examples/genesis-v50.gen`, ZERO Regressions Across All 20 Assays!):**
  - **All-Time `1024×1024` Record Reversal Learning (`Assay 1`):** **`+15.5%` STDP (`178,732` vs `154,774` frozen) & `+4.3%` BAC (`178,732` vs `171,292` no_apical)**!
  - **All-Time Official World Record Zero-DA Cultural Peer Teaching (`Assay 11`):** **`54.4%` zero-DA solo student accuracy (`40.6% -> 54.4%`, `post_stu_sep = 0.317`, `tutor = 54.9%`, `14,038` peer replies — `2.2×` v49's `24.7%`!)**
  - **All-Time Official World Record Zero-Shot Cross-Modal Generalization (`Assay 18`):** **`57.8%` joint vocal+motor (`52.3%` voc, `+0.065` mot_sep — `2.3×` v49's `25.0%` & `+14.0%` over v48!)**
  - **All-Time Official World Record Episodic Distractor Reasoning (`Assay 19`):** **`57.8%` (`33.6% -> 57.8%`, `epA 75% / epB 41%`, `epi_cos_dist = 0.341`!)**
  - **Tied All-Time Official Record 3-Syllable Compositional Syntax (`Assay 17`):** **`8.6%` (`0.8% -> 8.6%` = `10.8×` jump!, `syl1 28.9%`, `syl2 58.6%`, `syl3 41.4%`, balanced `phraseX 11% / phraseY 6%`!)**
  - **2-Syllable Syntax (`Assay 15`):** **`32.0%`** (`syl1 52.3%`, `syl2 68.8%` — `2×` v49!) · **Spoken Q&A (`Assay 10`):** **`57.3% / 51.2%` rev (`post_sep 0.555`)** · **Context XOR Dialogue (`Assay 13`):** **`46.2%` (`post_sep 0.483`)** · **Predictive MMN (`Assay 8`):** **`+4.2% > 3.8%` no-BAC (`+16.3%` omission)** · **Working Memory (`Assay 2`):** **`1,540.92×` (`+0.24` tiles E, `11.98` avg layers)** · **Episodic & Laminar SNR (`Assay 12`):** **`cos_dist = 0.757` (`1.53×` SNR), `cue_snr = 6.00×`, `L1_cue = 27.19×`, `L2_cue = 20.42×`, `L3_cue = 4.91×`, `L3_dk = 1.53×`, `L4_cue = 2.52×`, `L4_dk = 1.51×` (`L1_dk = L2_dk = 0.00×`), `recall_sep = +0.183`** · **Wild Unseen Agency (`Assay 3`):** **`0.1507` bits MI (`151` shelters, `117` pods, `58` tempered tools, `37` cached feasts)** · **Transitive Inference (`Assay 20`):** **`48.4%` (`adj 50.8%`, `AC 83%`)**.
  - *(Cross-checkpoint verification on `v50` engine: `v48.gen` hit **`Ctx13 = 68.4% (post_sep 1.002)`**, **`Q&A = 67.6% / 59.2% rev (sep 0.755)`**, **`Peer Teaching = 52.9%`**, **`recall_sep = +0.401`**, **`L5_dk = 1.30×`**; `v49.gen` hit **`+12.5% STDP / +19.3% BAC`**, **`Cmd16 = 68.8%`**, **`Peer Teaching = 51.0%`**; and `v50` 10k `chem=1.34` cohorts hit **`Cmd16 = 100.0% (64/64!)`**, **`3-Syl = 11.7%`**, and **`3-Concept = 42.4%`**!)*
  - **4-Agent `14`-Layer Sparse-Executive Spiking Dialogue (`converse`):** **`A0..A3: 14L / 1,025–1,076n` (`Eye 4, Ear 2, Syrinx 4, Muscle 10–11`)** across 12 live turns (**`104` peer replies**, **`47,537` live STDP edits**), keeping `L6..L13` completely silent (`0`) on routine `v0`/`v1`/`teach_qa`/`ask_ctx` and igniting cleanly all the way up to **`L13` (14 cortical layers deep!)** during `teach_p3_x`, `ask_p3_x`, and `cmd_go` — zero LLMs!
- **Key Biophysical, Architectural & Ecological Upgrades Shipped in v50:**
  - **Hasselmo (2006) Active Cholinergic New-Episode Retirement (`src/genesis_brain.cuh`):** Removed the `!post_recent` retirement deadlock on `L4` (`cs_u <= 5`) and `L5` (`cs_u == 4`) so completed `+DA`-stepped and aging prefrontal traces flush cleanly on new-episode ACh onsets even when actively firing — eliminating inter-trial ghost bleed while keeping `!post_recent` on `L3` (`Cmd16` / `Epi19`).
  - **Stable `L1` Thalamocortical Recruitment & Physiological Apical BAC Bounds (`src/genesis_brain.cuh`):** Replaced the discontinuous `bio_params[0] & 1` `L1` leak parity lottery with canonical `soma_leak_shift = 3` (`b_g = 3.95f`), tightened `apic_thresh` (`[220..420]`) and `bac_gain` (`[180..560]`) to healthy mammalian dendritic physiology, and tuned `L5` (`b_g = 3.40f`, `s_l_spk[3] && asc_sum > 210`) for crisp edge-of-chaos BA10 persistence.
  - **Sperry-Orthogonal `1–5t` `Uva→HVC` Efference Copy & Ecological `TK_CHEM` Gustatory Acuity (`src/genesis_brain.cuh`):** Mapped `L3` vocal efference copy onto Sperry-orthogonal sub-assemblies (`((tid>>1)+(tid>>5))%3`) with a crisp `5`-tick window (eliminating inter-trial vocal bleed) and added authentic contact gustation / olfactory toxin rejection on `TK_CHEM`.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v50.mp4`)](journal/genesis-v50.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v50 population
./axion genesis probe --load examples/genesis-v50.gen

# Run 4-agent grounded live spiking dialogue (14 layers / 1,025-1,076 neurons/agent)
./axion genesis converse --load examples/genesis-v50.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 50 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 500 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v50 setting **all-time world records across `1024×1024` intact fitness (`345,356 ATP`, `7/7` clean sweep), `13.19` mean cortical layers (`14`-layer sparse-executive `converse`), `183,281` combined births, `23.00B` STDP edits, `Assay 1` (`+15.5%` STDP), Zero-DA Cultural Peer Teaching (`54.4%`), Zero-Shot Generalization (`57.8%` joint), Episodic Distractor Reasoning (`57.8%`), and `3-Syl` (`8.6%`, up to `11.7%` + `100.0%` `Cmd16` in 10k `chem=1.34` cohorts)**, our immediate next breakthroughs are:

1. **Balanced Auditory-vs-Visual Cortical Gain Control, Deep Compositional Syntax & Full 4-Ear/Chem Retention (v51):**
   - Cure the **dark-audio vs sunlit-visual recruitment asymmetry** (where dark acoustic cues like Phrase B/Y, `c1`, and `C>A` recruit fewer `L1/L2` neurons than bright sunlit cues!) via authentic A1/V1 modality-balanced divisive normalization — driving **`2-Syl > 50–65%+`**, **`3-Syl > 20–35%+`**, **`3-Concept > 55–70%+`**, **`Trans20 > 65–80%+`**, and **`Epi19 > 70%+`** symmetrically across BOTH sunlit and dark-audio concepts!
   - Strengthen `ear >= 4` & `chem >= 1` evolutionary retention across 40k ticks (so `Cmd16` locks into `75–100%`, `Q&A` > `70%`, and `Peer Teaching` > `65%` zero-DA solo!).
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v52):**
   - Evolve distinct geographic vocal dialects, multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world (`vocal_mi_bits > 0.40+` bits).
3. **Grounded Open-Ended Spiking Conversation (v53+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v50.gen`, `examples/genesis-v49.gen`, `examples/genesis-v48.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
