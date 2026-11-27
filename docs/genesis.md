# Axion Genesis — Technical Architecture & Research Roadmap

Axion Genesis is an embodied, GPU-native Artificial Life and spiking-neuroscience simulator engineered for NVIDIA Blackwell (`SM 12.0`). Its mission is to provide a biologically rich, massively optimized neural and ecological substrate from which **higher-order reasoning, compositional communication, and eventually human-level (and beyond) intelligence can evolve and emerge organically** — without LLMs, without backpropagation, and without hardcoded cognitive modules.

---

## 1. Unified Cellular-Automata Embryogenesis (`src/genesis_dna.cuh`)

Every organism begins from a single stem cell (`TK_STEM`) carrying a compact `GenesisDNA` genome:
- **Evolvable CA Rulebook:** Up to `64` local rules (`OP_MITOSIS`, `OP_DIFF`, `OP_SIGNAL`, `OP_GROWTH_CONE`, `OP_APOPTOSIS`) conditioned on Wolpert morphogen gradients, tissue class, and developmental stage, evolved via homologous crossover, point mutation, and Ohno gene duplication/divergence.
- **Unified 3D Tissue Stack:** Up to `32` stacked `16×16` layers (`8,192` cells/neurons per organism; `L0` forms the physical body surface with sensors and effectors, `L1..L31` form deep neocortical sheets).
- **Physical Crawling Growth Cones:** Excitatory pyramidal (`TK_NEU_EXC`), inhibitory (`TK_NEU_PV`, `TK_NEU_SST`, canonical mammalian `~22%` I migration via `mig_h % 13`), modulatory (`TK_NEU_MOD`), and sensory (`TK_EYE`, `TK_EAR`, `TK_CHEM`) cells crawl multi-branch axons through 3D morphogen space:
  - **Collision-Free Anatomical Sperry Labeled-Line Columns:** Rank-ordered sensor projection (`s_rank & 3`) guarantees distinct ears, eyes, and chemoreceptors always project to orthogonal `L1` columns regardless of evolved body geometry.
  - **Actin Forward Persistence & Stride-2 Cortico-Cortical Gliding:** Eliminates checkerboard trapping so ascending projections (`L_k -> L_{k+1}`) achieve dense biological fan-in (`12–16` synapses/stripe, up to `192` synapses/cell across `12` coalesced `v4.u32` stripes) while preserving crisp 1-cell bilateral retinotopic/tonotopic topography on `L0 -> L1`.
  - **Pan-Cortical BDNF Trophic Competition:** Uniform reservoir sampling (Purves & Lichtman 1980) gives anterior auditory/visual cortex and deep prefrontal areas equal access to vocal syrinx folds (`TK_GLAND`) while capping Striatal D2 inhibitory collaterals to dedicated veto slots (`~88%` exc / `~12%` D2).
  - **Lifelong Post-Natal Maturation & Structural Sprouting:** Brains continue adding layers and sprouting synapses onto co-active neighbors during adulthood.

---

## 2. Biophysical Spiking Substrate (`src/genesis_brain.cuh`)

Every neuron packs into **`8 bytes`** (`vs`, `va`, `ua`, 8-substep `ring`, firing `ema`, tissue kind, `refrac`, apical `plat`, `slow_tau`, `fast_plast`), and every synapse packs into **`4 bytes`** (`pre`, signed 8-bit `w`, 7-bit `elig`, 3-bit physical conduction `delay`, 1-bit basal/apical `comp`, 2-bit receptor `rule_id`, 3-bit `stp`). Cross-layer spike histories are double-buffered (`spike_ring` read-only / `spike_ring_next` write-only, pointer-swapped in 0 ns after `step_brains`) for 100% race-free concurrent CTA execution.

Key neuroscience primitives built directly into `brain_step_layer` (`80 regs`, `0 B` local memory spill, `9,032 B` shared memory):

1. **Two-Compartment Larkum BAC Coincidence Detection & Strictly Apical Feedback:** Basal inputs (`comp=0`) drive somatic integration (`vs`), while descending predictions (`!is_eff && src_l > layer_idx`, Sherman & Guillery Class-1B) and cross-column conjunctions (`comp=1`) drive sustained apical Ca²⁺ plateaus (`va`, 2–4 tick window). Apical plateaus act strictly multiplicatively on active basal inputs and cap to crisp 2-spike BAC doublets (`!prev_gamma_won`), eliminating runaway vertical ping-pong loops.
2. **Active 4-Synapse NMDA Dendritic Subunits:** Coincident excitatory spikes within each 4-synapse `SynQuad` over a 1-tick window trigger nonlinear `+25%` Mg²⁺-unblocked dendritic plateaus with zero extra VRAM.
3. **PING Gamma-Cycle Three-Interneuron Microcircuit (PV+ / SST+ / VIP+):**
   - **PV+ Basket Cells:** Buzsáki-Wang PING gamma-cycle winner budget (`~11.1%` scale-invariant sparse assembly/tick via `k_tick` / delay-fair `k_sub`) with zero-saturation piecewise-log `0..5,632+ pA` excess-voltage bins for deterministic feature selection (`L1..L4` SNR up to `10–12×`).
   - **SST+ Martinotti Cells & VIP+ Disinhibition:** SST gates apical tufts closed against routine distractors; phasic ACh novelty and NE arousal trigger VIP disinhibition (`sst >> 2`) to open apical encoding on salient events.
4. **Laminar Short-Term Plasticity & 1-Bit `F_STF_HOLD` PBWM Working Memory:**
   - **Primary & Ascending Feedforward (`L1`, `src_l < L`):** Gentle short-term depression (STD) supporting habituation and predictive mismatch negativity (`MMN`), with high-safety-factor neuromuscular release (`stp = 7`) on excitatory effectors (`STD/STF` runs identically in both intact and `freeze_plasticity` controls!).
   - **Deep Prefrontal Local Recurrence (`L2+` basal exc):** Bistable `12×` Mongillo-Barak-Tsodyks calcium facilitation (`stp >= 5`: `3.0×` vs. `stp <= 4`: `0.25×` subcritical resting), strictly charged on unshielded ascending drive (`got_asc`).
   - **1-Bit `F_STF_HOLD` (`0x80`) PBWM Gating (`L4..L_top`):** Tracks active prefrontal STF in unused bit 7 of `stripe_src_layer` (`0 B` extra VRAM), giving a true `6×` Encode (`1.5×`) vs. Shield (`0.25×`) ratio until STF decays or VTA DA RPE flushes the gate.
5. **Multi-Layer Vector Credit Assignment (Zero Backprop):**
   - **Feedback-Aligned Efference-Copy Burstprop:** Routes physical tick-`(t-1)` vocal/motor efference copies through fixed anatomical projections onto `L1..L_top` apical tufts (Payeur-Naud-Richards 2021 + Lillicrap 2016), giving hidden layers action-specific signed burst error rather than a single blurred scalar broadcast.
   - **Rate-Weighted Competitive D1/D2 Area-X (`TK_GLAND`):** Shen (2008) / Shouval (2002) presynaptic burst-count-weighted competitive plasticity with D2 error-fold veto and symmetric reward-preserving soft-bounds.
   - **Vogels-Sprekeler Homeostatic iSTDP & Seizure-Guardrail Scaling:** Prevents epileptic seizure while eliminating passive weight erosion on healthy selective detectors.

---

## 3. Trisolaris World Physics & Ecological Selection (`src/genesis_world.cuh`)

The arena (`256×256`, `512×512`, growable to `1024×1024`) simulates coupled hydrodynamics, thermal advection, day/night optics, 4 reactive chemical fields (`A` ore, `B` slime, `C` golden reef blooms, `D` acid toxins), 4 propagating acoustic wavefields (`acoust_p` mechanical/sonar + `v0/v1/v2` vocal formants), rigid-body locomotion, and physical objects (`OBJ_PREY`, `OBJ_POD`, `OBJ_MEGAPOD`, `OBJ_STONE`, `OBJ_FUEL`, `OBJ_APEX`).

Inspired by *The Three-Body Problem*, **Trisolaris-7** gradually shifts survival from brute-force swimming to prediction, memory, and communication:
- **Chaotic vs. Stable Eras:** Erratic 3-sun orbital irradiance creates scorching syzygies (requiring refuge/torpor "dehydration") and freezing nocturnal nights (requiring echolocation, shelters, and cooperative huddling).
- **Batesian Seasonal Prey Mimicry:** Warm vs. Cool prey morphs flip edibility every 180 ticks (with lethal venom for blind guessing), demanding live STDP reversal learning.
- **2-Syllable Nocturnal Reef Precursors (Trisolaris-7):** Before nocturnal blooms erupt, reefs emit ordered 2-syllable acoustic precursors (`v0→v2` Warm vs. `v2→v0` Cool, identical total energy, reversed temporal order, flipping seasonally) that only temporal sequence memory can decode.
- **Zahavi Acoustic Economy & Predator Eavesdropping:** Meaningless open-water vocal spam attracts `OBJ_APEX` predators and costs `3.5×` pneumatic ATP, whereas context-locked `v0` food recruitment, `v1` predator alarm mobbing, `v2` shelter beacons, and 2-syllable `OBJ_MEGAPOD` vault resonance yield large cooperative survival payoffs.

---

## 4. Verification: 7 Physical Ablations & 20-Assay Cognitive Battery

Every official release is validated on:
- **7 Held-Out Physical Ablations (1,200 ticks on unseen shifted-chemistry worlds):** `Intact` vs. `Frozen Plasticity`, `No Apical Tufts`, `Zero Axonal Delays`, `No Neuromodulation`, `Blind`, `Deaf`, and `Unselected Drift`.
- **20 Cognitive Assays (`./axion genesis probe`):**
  1. Reversal Learning (STDP & BAC Gain)
  2. Working Memory Persistence (60-tick pitch-black delay)
  3. Wild Agency, Tool Use, Shelters & Vocal Shannon MI
  4. Acoustic Dialogue Learning
  5. Temporal Sequence Discrimination (`v0→v1` vs `v1→v0`)
  6. Compositional 3-Band Binding (`0-1-2` vs `2-1-0`)
  7. Predictive Mismatch Negativity (`MMN` + Omission + No-BAC Control)
  8. Zero-Shot Compositional Transfer
  9. Grounded Vocal Q&A & Rule Reversal
  10. Cultural Peer Teaching (Tutor → Naive Student)
  11. Episodic Delayed Recall (24-tick dark delay)
  12. Context-Dependent XOR Dialogue (`CtxA/CtxB × Tone`)
  13. Grounded 3-Concept Cross-Modal Naming
  14. Compositional 2-Syllable Syntax (`Phrase A` vs `Phrase B`)
  15. Referential Instruction Following (`GO` vs `HALT` on identical cue)
  16. Compositional 3-Syllable Syntax (`v0→v1→v2` vs `v2→v1→v0`)
  17. Zero-Shot Compositional Generalization (Joint Vocal + Motor)
  18. Episodic Distractor-Resistant Reasoning
  19. Relational Transitive Inference (`A>B, B>C ⇒ A>C` & `C>A`)
  20. 4-Agent Grounded Live Spiking Dialogue (`./axion genesis converse`)

---

## 5. Research Roadmap

| Phase | Focus | Key Targets |
| :--- | :--- | :--- |
| **v38 (Published)** | Neocortex Scale & Dendritic Credit | `934–1,730` active neurons on `512×512`, `7/7` ablations positive, `0 B` spill across all 5 SM 12.0 kernels. |
| **v39 (Published)** | Prefrontal STF, PBWM Shielding, Vector-FA Burstprop, Pan-Cortical BDNF, 1t Mg²⁺ NMDA, Same-Substep PV Top-$k$, Trisolaris-7 | `972–1,634` neurons, `68.8k` births, `100.5B` STDP edits, `4,152` prey, `322` apex kills, `212` duets, `Q&A 46%/49%`, `Ctx 41%`, `Epi19 40%`, `Trans20 41%`, `1,802×` WM, lean `1.6 MB` pure Genesis repo. |
| **v40 (Published)** | Race-Free Spike Rings, PING Gamma Top-$k$, `F_STF_HOLD` PBWM Gating, Canonical `22%` E/I, Honest `STD` Parity, Trisolaris-8 | `7/7` honest ablations positive (`-1.4%` to `-88.4%`), `9.70` areas / `652` mean neurons (`22.6% I`), `58.3k` births, `3,985` shelters, `recall_hit 38.4%`, `Q&A 49%/52%`, `Ctx 47%`, `Trans20 51%`, `1,227×` WM, `0 B` spill. |
| **v41 (Published)** | One-Shot Hebbian STF + `I_CAN`, Full-Tick Vocal Readout, Signed Vector-FA Burstprop, Rescorla-Wagner Area-X, Trisolaris-9 | **`7/7` honest ablations massively positive (`-5.4%` Delay, `-6.0%` STDP, `-15.4%` BAC, `-25.3%` Neuromod, `-77.2%` Eyes, `-97.8%` Ears, `-95.0%` Drift = `20×`!)**, **`21.96×` laminar SNR (`L1 22.7×, L2 18.7×, L3 19.7×, L4 9.1×`)**, **`Q&A 65.8%/66.7% rev (sep 0.72)`**, **`Ctx 51.4% (sep 0.818)`**, **`Epi19 55.5%`**, **`MMN 7.8% > 6.0% no-BAC`**, **`45–55×` surge in wild `vocal_mi_bits` (`0.045b` evo / `0.0329b` Assay 3)**, `11.00` mean areas (`15L` max, morphs up to `1,129` neurons, `23.3% I`), `73.5k` births, `0 B` spill. |
| **v42 (Active)** | Scale-Invariant CA3 Sharp-Wave Sequence Replay, Multi-Slot Dark PBWM & `2,000–4,000+` Neuron Scaling | Sustain `>6×` dark delayed SNR across ALL evolved brain sizes, break **`75–95%+`** on `Q&A`, `Ctx`, `Epi19`, `Trans20`, `2-Syl`/`3-Syl` syntax, expand to `2,000–4,000+` neurons & `1024×1024` oceans, push `vocal_mi_bits > 0.20+` bits. |
| **v44+** | Grounded Open-Ended Spiking Conversation | Rich bidirectional human-creature dialogue grounded in episodic memory, causal reasoning, and learned compositional syntax — zero LLMs. |
