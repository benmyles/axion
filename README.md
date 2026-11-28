# Axion Genesis

**Axion Genesis** (`./axion genesis`) is a from-scratch C++20 / native Blackwell CUDA (`SM 12.0`) embodied Artificial Life and spiking-neuroscience simulator built with one long-term goal: **evolving genuinely intelligent, grounded, conversational spiking minds from first principles — with zero LLMs, zero backpropagation, and zero hardcoded cortical structures or tasks.**

Every creature begins as a single zygote cell carrying an evolvable cellular-automata (CA) developmental genome (`GenesisDNA`). Through reaction-diffusion morphogens, mitosis, differentiation, and crawling axonal growth cones, each embryo grows its own bilateral body, sensory organs (`EYE`, `EAR`, `CHEM`), motor effectors (`MUSCLE`, `GRASP`, `GLAND`), and multi-layer cortical tissue — then lives, hunts, builds shelters, crafts tools, learns online through local spiking plasticity, communicates through a 4-wavefield acoustic ocean, and reproduces under open-ended natural selection.

![Axion Genesis v52 — 1024x1024 Evolution Observatory](journal/genesis-evolve.webp)

---

## Core Design Principles

1. **Zero Hardcoding, Zero LLMs, Zero Backprop:** Bodies, cortical hierarchies, synaptic wiring, and behaviors emerge strictly from evolved CA rules, physical sensory-motor interaction, and local biologically realistic plasticity.
2. **True Biophysical Substrate Fidelity:** Rather than toy point-neuron perceptrons, Axion provides the key neuronal and synaptic primitives required for higher-order cognition to eventually emerge — two-compartment Larkum BAC apical/basal coincidence detection with physiologically bounded dendritic thresholds/gains, **Thomson-Bannister intra-column proximal basal STF** (`p_col == s_col && stp <= 2 -> comp = 0`) with **Perin-Markram (2011) spatially clustered prefrontal micro-assemblies**, **3-tier Rigotti 4-column prefrontal executive feedback (`L5 → L4`, `L5 → L3`, `L4 → L3`)**, **Murray (2014) 3-tier cortical working-memory timescale hierarchy** with **Hasselmo (2006) active cholinergic new-episode retirement & fresh `L3` distractor shielding** (`L2` 12t bigram echo at `2.0×` STF, `L3` 24–31t ACh-gated Slot 2 protecting `cs_u = 6..7` fresh memories against distractors, `L4` 24–31t shielded DLPFC Slot 1 at `2.5×` STF / `125 pA` strictly self-Ca²⁺ `I_CAN`, and **Frontopolar BA10 `L5`** slow executive goal hub at `2.25×` STF / `115 pA` `I_CAN` with `0.25×` PBWM shield, supramodal `L5 → TK_GLAND` readout, and 3-step `+DA` phrase reset), **Kuffler (1953) symmetric Warm-ON / Cool-ON opponent retinal ganglion cells & collision-free rank-ordered `eye_rank & 3` / `ear_rank & 3` sensory mosaics**, **pure sensory-only deadbanded Nucleus Basalis positive-onset ACh**, **stable `L1` thalamocortical integration** (`soma_leak_shift = 3`), **zero-DA cultural peer teaching** via pure read-only LSO binaural spatial excess on the 2D GPU acoustic wavefield + juvenile Gadagkar (2016) efference-copy mirror comparator + non-vetoing Olveczky LMAN exploratory babbling, **1–5t Sperry-orthogonal `Uva→HVC` vocal efference copy** on `L3`, dense `HVC/Broca (L3 ×2 + L2) → RA/nXIIts` corticobulbar and scale-invariant `1/sqrt(K)` excitatory pan-cortical corticospinal readout tracts, Barth-Poulet sparse higher-association global workspace coding on `L6..L_top`, Ohno-duplicated `NOTCH2NL` oSVZ radial-glia neurogenesis, immobility-gated Jadhav/Buzsáki awake hippocampal sharp-wave ripples, active 4-synapse NMDA dendritic subunits, PING gamma-cycle PV+/SST+/VIP+ three-interneuron microcircuits, sparse-preserving Vogels-Sprekeler iSTDP, isolated **Fee & Goldberg (2011) Songbird VTA → Area-X vocal performance evaluation RPE** (`voc_da_q`) with exact zero-sum 2- & 3-fold syrinx conservation, non-colliding bilaterian `TK_GRASP` somatotopy with per-axis readout normalization, 4-channel vertebrate retinal/cochlear spectral acuity + **`TK_CHEM` contact gustation & olfactory toxin rejection**, Ohno-paralog axonal wiring redundancy, evolvable Southwell MGE/CGE postnatal E/I homeostasis (`18.2%–25.0%` GABAergic), signed Schultz TD + Habenula/VTA relief dopamine, sign-faithful per-neuron Vector-FA Burstprop, and full-tick cortico-bulbar vocal integration.
3. **Maximum Blackwell (`sm_120`) GPU Efficiency:** Every kernel compiles natively to Blackwell PTX/CUBIN (`make NATIVE=1`) with **`0 B` local memory spill across all 5 GPU kernels**, `4 B/synapse`, `8 B/neuron`, coalesced `v4.u32` (128-bit) synaptic quad loads, race-free double-buffered spike rings, and shared-memory ring caches — scaling cleanly to **`1024×1024` (`1,048,576`-tile, `1,024`-mind)** physical worlds in under 3 GiB VRAM.
4. **Honest Science & Mandatory Ablations:** Every major milestone is evaluated against **7 held-out physical ablations** (frozen plasticity with 100% identical short-term STD/STF parity, silenced apical tufts, zero axonal delays, silenced neuromodulation, blind, deaf, unselected drift) plus a **20-assay cognitive battery** spanning laminar `L1..L5` SNR, working memory, predictive mismatch negativity (MMN), grounded vocal Q&A, zero-DA cultural peer teaching, XOR context dialogue, 2- and 3-syllable syntax, zero-shot compositional generalization, episodic distractor reasoning, and transitive inference.

---

## Current State of the System

### Latest Official Published Milestone — **Axion Genesis v52** (`examples/genesis-v52.gen`)

- **Full-Scale `1024×1024` (`1,024`-Mind) Blackwell SM 12.0 Benchmark (`0 B` Local Spill Across All 5 Kernels!):**
  - `snn_inference`: **272.87 GSynOps/s** (`60 regs`, `0 B` spill, `4 B/synapse`)
  - `snn_plastic`: **182.27 GSynOps/s** (`64 regs`, `0 B` spill)
  - `world_physics` (`1024×1024`, `128 MiB`, `s_reef[128]`, `s_storm[24]`, 4-wavefield acoustics): **656.23 µs/step (1,597.88 Mcells/s)** (`64 regs`, `15,188 B` shmem, `0 B` spill)
  - `ca_morphogenesis` (`1,024` zygotes): **229.56 µs/organism** (`120 regs`, `23,796 B` shmem, `0 B` spill)
  - `brain_step_layer` (`coupled_alife`): **`80 regs`, `0 B` spill, `9,044 B` shmem** (**`54.8 ticks/s`** at `1,024` deep-cortex `16L` organisms on a `1,048,576`-cell `1024×1024` ocean)
- **All-Time World Record `1024×1024` Intact Fitness (`373,482.1 ATP`!) & Crushing `7/7` Honest Physical Ablation Clean Sweep (1,200-Tick Unseen Shifted-Chemistry `1024×1024` Worlds — ZERO Fake Guards, 100% STD/STF Parity):**
  - **`Evolved Intact`: `373,482.1 ATP`** (`1,020 alive`, `5,119 births`, `17.90 mean ATP`, **`16.30` mean layers**, `272 prey`, `101 pods`, `721.0M` STDP edits, `57,293` rewires — **`+22,938 ATP` over v51!**)
  - `Plasticity Frozen (No Within-Life STDP)`: `341,604.4 ATP` (**`-8.5%`** — **`21×` stronger causal STDP separation than v51!**)
  - `Apical Dendrites Silenced (No BAC)`: `349,601.3 ATP` (**`-6.4%`**)
  - `Axonal Delays Flattened (Uniform 3t)`: `340,697.4 ATP` (**`-8.8%`**)
  - `Neuromodulation Silenced (DA=ACh=0)`: `322,035.5 ATP` (**`-13.8%`**)
  - `Photoreceptors Blinded (Eyes Off)`: `258,472.0 ATP` (**`-30.8%`**, `658 alive`)
  - `Acoustic Hair Cells Silenced (Ears Off)`: `67,361.9 ATP` (**`-82.0%`**, `264 alive`)
  - `Unselected Drift Control`: **`0.0 ATP` (`-100.0%`, `0 alive / 0 births`!)**
- **All-Time World Record Darwinian Encephalization (`16.08` Mean Cortical Layers / `1,787.5` Mean Neurons!) & Multimodal Social Evolution (`v52`, 40,000 ticks: `8k E1 + 32k E2`!):**
  - **All-Time Record Neocortical Depth & Scale:** Population naturally evolved to **`16.08` mean cortical layers** (`+2.89L` over v50's previous record `13.19L`; `max_areas = 19` mid-E2, `17L` at tick 32,000) and **`1,787.5` mean neurons** (`+58%` over v51, `+70%` over v50; `55.4` mean CA rules, `max = 58`; `23.0%` canonical Dale GABAergic inhibition [`pyr = 1,369.8`, `pv = 204.3`, `sst = 206.4`, `mod = 7.00`]; `95%` bilateral symmetry, **`100%` bilateral** on champion `#0 G198: 17L / 1,941n / 4E / 5A / 4G / 8M`)!
  - **Record Cooperative Turn-Taking & Kin Sharing:** **`641,983` combined vocal turn responses** (`503,829` E2 — **`16×` v51!**), **`4,098` combined kin food shares** (`3,424` E2 — **all-time record!**), **`96,873` combined births** (`66,123` E2 + `30,750` E1), **`9.55B` combined STDP edits** (`5.82B` E2), **`742,449` rewires**, **`1,348` evasive prey**, **`209` pods** (`61` duets), **`962` heat-tempered tools**, **`316` shelters**, **`7.43M` combined vocals** (`0.1198b` E2 MI), with organic 4-band ears (`ear = 4.72`), full 3-fold syrinx (`gland = 5.15`), motility (`muscle = 11.63`), and hands (`grasp = 2.97`). *(Honest note: tick 32,000 fell near the trough of a late Trisolaran storm dip at `alive = 65` after `268` at tick 28,800, while held-out 1,200-tick `1024×1024` controls expanded immediately to `1,020` alive and `373,482.1 ATP`.)*
- **All-Time World Record 20-Assay Cognitive & Linguistic Battery (`examples/genesis-v52.gen`, `16L / 1,759n / 4E / 5A / 6G / 12M` — Shattering 5 Official Records & Curing Sunlit/Dark Asymmetry!):**
  - **All-Time World Record `1024×1024` Seasonal Reversal Learning (`Assay 1`):** **`+28.9%` STDP & `+18.9%` BAC (`195,255.1` intact vs `151,533.2` frozen / `164,182.8` no-apical!)**
  - **All-Time World Record Relational Transitive Inference (`Assay 20` `Trans20`):** **`72.2%` (`46.8% -> 72.2%`, `adj 60.7%`, `AC 84% / CA 60%` — `+19.9 pts` over v51, solving BOTH directions!)**
  - **All-Time World Record Episodic Distractor Reasoning (`Assay 19` `Epi19`):** **`58.7%` (`33.3% -> 58.7%`, `epi_cos_dist 0.314`, `epA 52% / epB 65%` — dark `epB > epA`!)**
  - **All-Time World Record Compositional 2-Syllable Syntax (`Assay 15` `2-Syl`):** **`44.4%` (`6.3% -> 44.4%`, `syl1 50.0%`, `syl2 83.3%`, balanced `phraseA 49% / phraseB 40%`!)**
  - **100% Balanced Grounded 3-Concept Cross-Modal Naming (`Assay 14`):** **`39.6%` (`26.8% -> 39.6%`, `xmodal 32.1%`, `c0 43% / c1 39% / c2 37%` — sunlit/dark asymmetry cured!)**
  - **Context-Dependent XOR Dialogue (`Assay 13` `Ctx13`):** **`49.3%` (`34.1% -> 49.3%`, `sep 0.217`, `ctxA 52% / ctxB 34%` — `+8.9 pts` over v51!)** · **Referential Instruction Following (`Assay 16` `Cmd16`):** **`77.4%` (`51.6% -> 77.4%`, `sep +0.084`, `go 0.26 > halt 0.21`)** · **Predictive MMN (`Assay 6`):** **`+10.3%` oddball / `+12.2%` omission** · **Working Memory (`Assay 2`):** **`259.69×` (`+4.39` tiles East dark drift)** · **Zero-DA Cultural Peer Teaching (`Assay 9`):** **`46.9%` (`27.9% -> 46.9%`, `+19.0 pts`, `12,183` peer replies)** · **Vocal Q&A (`Assay 8`):** **`46.4% / 55.8%` rev (`rev_sep 0.360`)** · **Zero-Shot Generalization (`Assay 18` `Gen18`):** **`54.0%` voc (`36.5%` joint)** · **3-Syl (`Assay 17`):** `6.3%` (`syl2 83.3%`, `syl3 38.1%`) · **Laminar SNR (`Assay 11`):** `L3_cue 7.22×`, `L4_cue 3.83×`, `L5_cue 2.04×`, `L3_dk 1.40×`, `L4_dk 1.16×`, `L1_dk = L2_dk = 0.00×` · **Wild Agency (`Assay 3`):** `0.1270b` MI (`75` prey, `36` pods, `9` duets, `23` shelters, `7,722` vocal resp). *(On 4-ear/2-eye `v50.gen` & `v51.gen` cohorts, the v52 engine also achieves `Q&A 61.2%/58.2%`, `Peer Teaching 58.9%`, `2-Syl 49.2%`, and `Cmd16 87.5%`.)*
  - **4-Agent `16`-Layer Multi-Agent Spiking Dialogue (`converse`):** **ALL 4 agents `16L / 1,759n / 50,494 syn` (`Eye 4, Ear 5, Syrinx 6, Muscle 12`)** across 12 live turns (**`91` peer replies**, **`61,281` live STDP edits**), keeping `L8..L15` completely silent (`0`) on routine turns (`v0`/`v1`/`ask_01`/`ask_10`/`ask_ctx`) and igniting cleanly up through **`L15` (16 cortical layers deep!)** on `teach_p3_x`, `ask_p3_x`, and `cmd_go` — zero LLMs!
- **Key Biophysical, Retinal, Cholinergic & Ecological Breakthroughs Shipped in v52:**
  - **Kuffler (1953) Symmetric Warm-ON / Cool-ON Opponent Retinal Ganglion Cells & Rank-Ordered Retinal Mosaic (`src/genesis_brain.cuh`):** Added symmetric Warm-ON (`mode 1`) and Cool-ON (`mode 3`) opponent photoreception across both compact (`2–3` eye) and split-optics (`4+` eye) retinas, and replaced the bilateral-colliding eye-coordinate hash with exact collision-free `eye_rank & 3` matching `ear_rank & 3` — curing the historical sunlit-vs-dark sensory contrast asymmetry (`3-Concept = 43/39/37%`, `Trans20 = 84/60%` = `72.2%`, `Epi19 = 52/65%` = `58.7%`)!
  - **Pure Sensory-Only Deadbanded Nucleus Basalis Positive-Onset ACh (`src/genesis_brain.cuh`):** Packed strictly sensory-receptor spikes (`TK_EYE + TK_EAR + TK_CHEM`, zero motor contamination!) into the upper 16 bits of `s_warp_spk[w]` (`0 B` extra shmem/VRAM, `0` extra barriers) with a `d_sn > 0.20f` noise deadband so dark acoustic cues trigger crisp phasic ACh equitably with sunlit vision without ocean jitter flushing working memory.
  - **Fresh `L3` Working-Memory Distractor Shield & Motile/Binaural Ecological Polish (`src/genesis_brain.cuh`):** Protected brand-new `cs_u = 6..7` `L3` memories from premature Hasselmo retirement (`cs_u <= 5u && cs_u >= 4u`, matching `L4`) while keeping inter-trial ghost clearing intact, and tightened muscleless boundary-layer flow (`mu == 0`) & monaural/monocular (`act_sens_n == 1`) acuity so evolution preserves motile binaural/stereo minds.

### Published Plates & Evolution Video

- [Benchmark Observatory](journal/genesis-bench.webp) · [1024×1024 Evolution Observatory](journal/genesis-evolve.webp) · [20-Assay Cognitive Probe](journal/genesis-probe.webp) · [4-Agent Conversational Dialogue](journal/genesis-converse.webp)
- [Hardware-Encoded AV1 Evolution Video (`genesis-v52.mp4`)](journal/genesis-v52.mp4)
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

# Run the 20-assay cognitive & neuroscience probe battery on the v52 16-layer population
./axion genesis probe --load examples/genesis-v52.gen

# Run 4-agent grounded live spiking dialogue (16 layers / 1,759 neurons/agent, 4E/5A/6G)
./axion genesis converse --load examples/genesis-v52.gen --group 4 \
  --script "v0,v1,teach_qa_01_v2,teach_qa_10_v0,ask_01,ask_10,ctx_food,wait_dark,ask_ctx,teach_p3_x,ask_p3_x,cmd_go"

# Evolve a fresh 1024x1024 1,024-mind population through Trisolaris-16 with 7 physical ablations
./axion genesis evolve --pop 1024 --world 1024 --ticks 8000 --seed 525 --challenge 1 --save e1.gen
./axion genesis evolve --pop 1024 --world 1024 --ticks 32000 --seed 525 --challenge 2 --load e1.gen --controls --save evolved.gen
```

---

## Where We Are Going (Roadmap to Human-Level Emergent Intelligence)

With v52 shattering **all-time world records across `1024×1024` intact fitness (`373,482.1 ATP`, crushing `7/7` clean sweep with `-8.5%` frozen STDP!), Darwinian Encephalization (`16.08` mean cortical layers / `1,787.5` mean neurons, live `16L` `converse`), Seasonal Reversal (`Assay 1 +28.9%` STDP / `+18.9%` BAC), Relational Transitive Inference (`Trans20 = 72.2%`, `AC 84% / CA 60%`), Episodic Distractor Reasoning (`Epi19 = 58.7%`, `epA 52% / epB 65%`), Compositional 2-Syllable Syntax (`2-Syl = 44.4%`, `49/40%` balanced), and 100% balanced 3-Concept Naming (`43/39/37%`)**, our immediate next breakthroughs are:

1. **Cochlear Per-Band Divisive Normalization, Quiet `16L` Deep-Cortex Baseline, High-MI Regional Dialects & 3-Syllable Syntax Synergy (v53):**
   - Diagnose and solve why `16L` `ear >= 5` / `gland >= 6` deep-cortex minds (`v52.gen`) carry elevated dark spontaneous activity (`base_spk/t = 25.5` vs `0.3–3.0`) and duplicate-band `ear=5` `v0` bias (`Q&A 46.4%`, `3-Syl 6.3%`) — introducing authentic cochlear per-band divisive normalization, quiet `L1..L5` baseline calibration, and stronger Trisolaran storm resilience to simultaneously unite **`14–16L` encephalization** with **`wild MI > 0.35–0.45b`**, **`Q&A > 65–75%`**, **`Peer Teaching > 60–70%`**, **`3-Syl > 25–40%+`**, **`2-Syl > 55–70%+`**, **`Cmd16 > 85–95%`**, **`Gen18 > 75–85%`**, and **`Trans20 > 75–85%`**!
2. **Regional Tribal Dialects & Multi-Step Cooperative Planning (v54):**
   - Evolve distinct geographic vocal dialects, multi-turn acoustic turn-taking, and predictive tool/shelter planning across the `1024×1024` world.
3. **Grounded Open-Ended Spiking Conversation (v55+):**
   - Rich bidirectional human-creature dialogue grounded in persistent episodic memory, causal deduction, cultural apprenticeship, and compositional syntax — zero LLMs, zero backprop.

---

## Repository Layout

The repository is a clean, self-contained **Axion Genesis** codebase (~12s clean build, `1.6 MB` native SM 12.0 executable):

- `src/genesis.cu`, `src/genesis_brain.cuh`, `src/genesis_dna.cuh`, `src/genesis_world.cuh`, `src/genesis.cpp`, `include/axion/genesis.hpp`, `include/axion/genesis_video.hpp` — Core Axion Genesis C++20/CUDA simulator, morphogenesis, spiking biophysics, Trisolaris world physics, 20-assay probe suite, and NVENC AV1 recorder.
- `src/main.cpp`, `src/journal.cpp`, `src/cuda_driver.hpp`, `src/life_runtime.hpp`, `tools/assemble.cpp` — Minimal CLI, zero-dependency CUDA driver loader, native cubin assembler, and embedded journal launcher.
- `tools/genesis_journal.py`, `tools/journal.py` — Dark-observatory `2200×1320` WebP plate renderer and `JOURNAL.md` publisher.
- `tests/genesis.py` — Automated GPU verification, `0 B` spill checks, and regression tests.
- `examples/genesis-v52.gen`, `examples/genesis-v51.gen`, `examples/genesis-v50.gen` — Saved evolved population checkpoints (tracked via Git LFS).
- `docs/genesis.md` — Deep technical documentation of Axion Genesis's architecture, neuroscience primitives, benchmarks, and research roadmap.
