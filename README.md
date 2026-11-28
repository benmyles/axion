# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v49 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection, **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24t ACh-gated Slot 2 and `L4` 31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **Frontopolar BA10 `L5`** 48–63t slow executive goal hub at `2.25×` STF / `115 pA` `I_CAN` with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and 3-step `+DA` phrase reset), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, 6-tick subthreshold songbird `Uva→HVC` vocal efference copy on `L3`, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), positive-onset Nucleus Basalis ACh, signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v49** (`examples/genesis-v49.gen`)

- **Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **273.16 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.68 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **654.94 µs/step (1,601.03 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **229.19 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`65.1 ticks/s`** at `1,024` deep-cortex organisms on a `1,048,576`-cell `1024×1024` ocean!)
- **All-Time Strongest `1024×1024` `7/7` Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry `1024×1024` Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `280,347.7 ATP`** (`910 alive`, `4,232 births`, `19.68 mean ATP`, `232 prey`, `242 pods`, `109,039 synth`, `327,138 vocal`, `494.9M` STDP edits, `24,824` rewires, crisp `107.7M` spikes)
  - `Plasticity Frozen (No Within-Life STDP)`: `222,151.4 ATP` (**`-20.8%`**, `571 alive` — **`12×` wider margin than v48!**)
  - `Apical Dendrites Silenced (No BAC)`: `207,708.9 ATP` (**`-25.9%`**, `585 alive` — **`2.2×` wider than v48!**)
  - `Axonal Delays Flattened (Uniform 3t)`: `185,509.2 ATP` (**`-33.8%`**, `600 alive` — **`8.4×` wider than v48!**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `136,882.5 ATP` (**`-51.2%`**, `533 alive` — **`4.6×` wider than v48!**)
  - `Photoreceptors Blinded (Eyes Off)`: `113,273.2 ATP` (**`-59.6%`**, `497 alive`, `71 prey`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `11,172.0 ATP` (**`-96.0%`**, `9 alive`)
  - `Unselected Drift Control`: **`3.8 ATP` (`-100.0%`, `0 alive / 0 births`!)**
- **All-Time Record Births (`164,436` Combined) & Deep-Cortex `1024×1024` Evolution (`v49`, 40,000 ticks: `8k E1 + 32k E2` @ `33.1 ticks/s`!):**
  - **136,006 E2 Natural Births (`164,436` Combined E1+E2, `Gen 226`, `270` Morphs):** Evolved `8.33` mean layers (`max_areas = 13`, `mean_rules = 53.0`, `max 56`, `mean_fanin = 158.6`, `max 192`), `1,101.4` mean neurons/organism with canonical **`21.2%` GABAergic inhibition** (`pyr = 861.4`, `pv = 117.2`, `sst = 115.9`, `mod = 6.88`), **`99%` population bilateral symmetry** (`100%` bilateral champion `#1 G215: 9L / 1,008n / 4E / 4A / 3G`), and balanced multimodal organs (`eye = 3.55`, `ear = 3.63`, `gland = 2.96`, `muscle = 12.02`, `grasp = 6.14`, `struct = 4.42`).
  - **Rich Emergent Ecology & Plasticity:** **`15.20 BILLION` combined live STDP edits** (`11.29B` E2 + `3.92B` E1), **`1.12 MILLION` combined live structural rewires**, **`54,464` combined post-natal maturations**, **`3,485` combined evasive prey caught** (`2,701` E2), **`1,310` combined pods cracked** (`941` E2, `42` tool, `235` coop, `131` duets), **`2,311` combined heat-tempered tools**, **`3,177` quarried stones**, **`52` apex slain in E2 (`73` combined)**, **`9.73M` E2 vocals (`12.67M` combined, `4.27M` E2 replies, `72,754` mimics, `0.1534` bits wild MI)**!
- **v49 Engine & 20-Assay Cognitive Battery (`examples/genesis-v49.gen` + Cross-Checkpoint Zero-DA Cultural Validation):**
  - **All-Time Record Referential Instruction Following (`Assay 16`):** **`70.3%`** (`62.5% -> 70.3%`, `go 0.24 > halt 0.18` on default seed; up to **`76.6%`** across seeds — **`+67%` over v48's `42.2%`!**)
  - **All-Time Record 60-Tick Working Memory Persistence (`Assay 2`):** **`2,194.70×`** (`0.1 -> 2,265 -> 2,195 spk/t` — **`2.6×` v48!**)
  - **All-Time Record L1/L2 Cue SNR & Episodic Distractor Reasoning:** **`L1_cue = 23.61×`, `L2_cue = 14.86×`, `L3_cue = 3.07×`, `L4_cue = 2.07×`, `L5_cue = 1.78×`** (`L1_dk = L2_dk = 0.00×`!), **`Epi19 = 51.6%` (`52/52` balanced, `epi_cos_dist = 0.416`, up to `56.2%` across seeds)**, **`MMN = +3.4% > 3.1%` no-BAC (`+8.2%` omission; up to `14.4%` vs `0.5%` no-BAC across seeds)**, **`Q&A = 41.8%` (`sep 0.06 -> 0.43`)**, **`Ctx13 = 37.5%` (`sep 0.03 -> 0.49`)**, **`Trans20 = 46.9%` (`adj 50.4%`, `AC 73%`)**, **`3-Concept = 35.3%` (`28.2%` xmodal)**, **`2-Syl = 16.4%`**, **`3-Syl = 5.5–7.8%`**, **`Peer Teaching = 24.2% -> 24.7–28.9%` (`sep 0.06 -> 0.12`, `10,588` replies)**, and **`Assay 1` `BAC +10.0%` (`STDP -0.8%` on `256×256` default seed vs `+26.2%` STDP gain / `-20.8%` frozen drop on `1024×1024` controls)**.
  - **Verified Breakthrough Zero-DA Cultural Peer Teaching on the `v49` Engine (`examples/genesis-v48.gen` & `v47.gen`):** Running the exact `v49` engine on dense `L4/L5` `v48.gen` (`chem > 0`, odd `soma_leak` on `L1`) immediately achieved **`26.0% -> 47.9%` (`+21.9%`!) solo zero-DA student accuracy after paired acoustic apprenticeship** (`v47.gen`: `29.9% -> 50.3%`, `+20.4%`!), alongside **`+7.7%` STDP / `+31.9%` BAC reversal**, **`L5_dk SNR = 1.38×`**, **`Q&A = 66.4% / 57.0% rev`**, **`Ctx13 = 59.0%`**, **`Epi19 = 58.6%`**, **`Cmd16 = 57.8%`**, **`Gen18 = 45.3%` joint**, **`Trans20 = 50.0% (53/47)`**, and **`2-Syl = 31.2%`**! *(Honest diagnosis for `v50`: `v49.gen`'s evolved lineage drifted to even `bp[0]` [halving `L1` integration time -> `L1_cue n=2.5`], higher `apic_thresh = 408`, smaller `L4/L5 = 85/56p`, and `chem = 0.08`, explaining its lower `Q&A`/`Peer Teaching` relative to `v48.gen` on the same engine.)*
  - **4-Agent BA10 `L5` Sparse-Executive Spiking Dialogue (`converse`):** **`A0..A3: 8–9L / 849–1,322n` (`Eye 2–4, Ear 4, Syrinx 3–5, Muscle 14`)** across 12 live turns (**`93` peer replies**, **`42,765` live STDP edits**), showing clean edge-of-chaos dark persistence (`wait_dark`: `L2:0, L3:7, L4:3, L5:12, L6:9, L7:1, L8:0`, zero LLMs)!
- **Key Biophysical, Architectural & Ecological Upgrades Shipped in v49:**
  - **Thomson-Bannister Intra-Column Proximal Basal STF & Perin-Markram Clustered Micro-Assemblies (`src/genesis_dna.cuh`):** Fixed a hidden 10-generation `rl.param1 > 0` bug that had silently routed 85–100% of local `L2..L5` recurrent synapses to apical (`comp = 1`, zero STF) on 81% of genomes, replacing it with canonical `p_col == s_col && stp <= 2 -> comp = 0` + tight intra-column clustering (`18 - (adx+ady)*2`).
  - **Zero-DA Cultural Peer Teaching via Pure 2D Acoustic Wavefield (`include/axion/genesis.hpp`, `src/genesis_brain.cuh`, `src/genesis.cpp`):** Reused 2 padding bytes (`peer_tpl_band`, `peer_tpl_age` — `0 B` extra VRAM!) for every-tick read-only LSO spatial excess on `acoust_vX_prev[pidx] - min(up, dn)` (`0.0000` on uniform ambient prompts and own voice, `+0.72` on nearby conspecific speech), calm-water juvenile Gadagkar (2016) efference-copy mirror comparator, and non-vetoing juvenile Olveczky LMAN babbling (`age < 120`), validated in an authentic 2-phase Songbird Apprenticeship chamber (`Assay 11`, strictly zero DA to students!).
  - **Edge-of-Chaos Criticality Calibration & `1024×1024` Carrying Capacity (`src/genesis_brain.cuh`, `src/genesis.cpp`):** Calibrated clustered basal STF gains (`L3/L4 = 2.5×`, `L5 = 2.25×`, `L2 = 2.0×`), strictly self-Ca²⁺ `I_CAN` (`125/115 pA`), sensory-present STF encoding gates, zero-overshoot local basal (`12`) and motor (`42`) soft bounds, `TK_GRASP` `20/16` scaling, and `64` reefs on `1024×1024` — delivering the strongest `7/7` causal ablation clean sweep in Axion history!

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v49.mp4`)](journal/genesis-v49.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v49 population
./axion genesis probe --load examples/genesis-v49.gen

# Run 4-agent grounded live spiking dialogue (8-9 layers / 849-1,322 neurons/agent)
./axion genesis converse --load examples/genesis-v49.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 49 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 490 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v49 achieving the **strongest `7/7` causal ablation clean sweep in project history on `1024×1024` (`frozen -20.8%`, `no_apical -25.9%`, `zero_delay -33.8%`, `no_neuromod -51.2%`, `blind -59.6%`, `deaf -96.0%`, `drift -100%`)**, **`164,436` combined natural births**, **`15.20B` combined STDP edits**, **`70.3–76.6%` bidirectional instruction following (`Cmd16`)**, **`2,195×` working memory**, **`23.6×` L1 / `14.9×` L2 cue SNR**, and **verified zero-DA solo cultural peer teaching (`+21.9%` on `v48.gen`)**, our immediate next breakthroughs are:

1. **True Layer-Wide O'Reilly-Frank PBWM Gating, Clean Episode Retirement & Sensory/Apical Stability (v50):**
   - Replace per-neuron `pfc_shield` with **true layer-wide striato-thalamo-cortical PBWM input gating (`s_layer_stf_hold` on `L4`/`L5`)** and fix the `!post_recent` retirement lock so distractors (`Epi19`), second items (`Trans20`), and shared middle syllables (`3-Syl`) can no longer overwrite `L4`/`L5` — targeting **`Trans20 > 70–85%+`**, **`Epi19 > 70%+`**, **`2-Syl > 45–60%+`**, and **`3-Syl > 20–35%+`**!
   - Stabilize `L1` thalamocortical integration, apical BAC excitability, and ecological `TK_CHEM` chemosensory acuity so EVERY evolved `1024×1024` population retains dense `L1..L5` cue/dark SNR, **`Q&A > 65–75%+`**, and **`Peer Teaching > 50–65%+`** zero-DA solo cultural transmission!
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v51):**
   - Evolve distinct geographic vocal dialects, multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world (`vocal_mi_bits > 0.40+` bits).
3. **Grounded Open-Ended Spiking Conversation (v52+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v49.gen`, `examples/genesis-v48.gen`, `examples/genesis-v47.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
