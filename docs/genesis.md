# Axion Genesis — Technical Architecture & Research Roadmap

Axion Genesis is an embodied, GPU-native Artificial Life and spiking-neuroscience simulator engineered for NVIDIA Blackwell (`SM 12.0`). Its mission is to provide a biologically rich, massively optimized neural and ecological substrate from which **higher-order reasoning, compositional communication, and eventually human-level (and beyond) intelligence can evolve and emerge organically** — without LLMs, without backpropagation, and without hardcoded cognitive modules.

---

## 1. Unified Cellular-Automata Embryogenesis (`src/genesis_dna.cuh`)

Every organism begins from a single stem cell (`TK_STEM`) carrying a compact `GenesisDNA` genome:
- **Evolvable CA Rulebook:** Up to `64` local rules (`OP_MITOSIS`, `OP_DIFF`, `OP_SIGNAL`, `OP_GROWTH_CONE`, `OP_APOPTOSIS`) conditioned on Wolpert morphogen gradients, tissue class, and developmental stage, evolved via homologous crossover, point mutation, and Ohno gene duplication/divergence.
- **Unified 3D Tissue Stack:** Up to `32` stacked `16×16` layers (`8,192` cells/neurons per organism; `L0` forms the physical body surface with sensors and effectors, `L1..L31` form deep neocortical sheets).
- **Physical Crawling Growth Cones:** Excitatory pyramidal (`TK_NEU_EXC`), inhibitory (`TK_NEU_PV`, `TK_NEU_SST`), modulatory (`TK_NEU_MOD`), and sensory (`TK_EYE`, `TK_EAR`, `TK_CHEM`) cells crawl multi-branch axons through 3D morphogen space:
  - **Sperry (1963) Labeled-Line Columns:** 4 interleaved minicolumn identities separate sensory modalities and cortical assemblies cleanly.
  - **Actin Forward Persistence & Stride-2 Cortico-Cortical Gliding:** Eliminates checkerboard trapping so ascending projections (`L_k -> L_{k+1}`) achieve dense biological fan-in (`12–16` synapses/stripe, up to `192` synapses/cell across `12` coalesced `v4.u32` stripes) while preserving crisp 1-cell bilateral retinotopic/tonotopic topography on `L0 -> L1`.
  - **Pan-Cortical BDNF Trophic Competition:** Uniform reservoir sampling (Purves & Lichtman 1980) gives anterior auditory/visual cortex and deep prefrontal areas equal access to vocal syrinx folds (`TK_GLAND`) while capping Striatal D2 inhibitory collaterals to dedicated veto slots (`~88%` exc / `~12%` D2).
  - **Lifelong Post-Natal Maturation & Structural Sprouting:** Brains continue adding layers and sprouting synapses onto co-active neighbors during adulthood.

---

## 2. Biophysical Spiking Substrate (`src/genesis_brain.cuh`)

Every neuron packs into **`8 bytes`** (`vs`, `va`, `ua`, 8-substep `ring`, firing `ema`, tissue kind, `refrac`, apical `plat`, `slow_tau`, `fast_plast`), and every synapse packs into **`4 bytes`** (`pre`, signed 8-bit `w`, 7-bit `elig`, 3-bit physical conduction `delay`, 1-bit basal/apical `comp`, 2-bit receptor `rule_id`, 3-bit `stp`).

Key neuroscience primitives built directly into `brain_step_layer` (`80 regs`, `0 B` local memory spill, `9,032 B` shared memory):

1. **Two-Compartment Larkum BAC Coincidence Detection:** Basal inputs (`comp=0`) drive somatic integration (`vs`), while descending predictions and cross-column conjunctions (`comp=1`) drive sustained apical Ca²⁺ plateaus (`va`, 2–4 tick window). Apical plateaus act strictly multiplicatively on active basal inputs (`i_basal > 80..180 pA`), triggering high-frequency sodium bursts (`refrac=0`) without runaway autonomous reverberation.
2. **Active 4-Synapse NMDA Dendritic Subunits:** Coincident excitatory spikes within each 4-synapse `SynQuad` trigger nonlinear `+75%` local dendritic spikes with zero extra VRAM.
3. **Three-Interneuron Cortical Microcircuit (PV+ / SST+ / VIP+):**
   - **PV+ Basket Cells:** Pouille-Scanziani feedforward Top-$k$ sparse competition (`~15%` active) with somatic GABA_A reset and wide `64 pA` margin bins for deterministic feature selection.
   - **SST+ Martinotti Cells & VIP+ Disinhibition:** SST gates apical tufts closed against routine distractors; phasic ACh novelty and NE arousal trigger VIP disinhibition (`sst >> 2`) to open apical encoding on salient events.
4. **Laminar Short-Term Plasticity & PBWM Working Memory:**
   - **Primary & Ascending Feedforward (`L1`, `src_l < L`):** Gentle short-term depression (STD) supporting habituation and predictive mismatch negativity (`MMN`).
   - **Deep Prefrontal Local Recurrence (`L2+` basal exc):** Mongillo-Barak-Tsodyks (2008) calcium facilitation (`STF`, `stp: 1 → 5`) carrying activity-silent working memory across delays and distractors while remaining strictly subcritical in darkness.
   - **PBWM Shielding (`L4..L_top`):** Occupied prefrontal slots (tracked via existing 1-bit `has_d0` flag, 0 extra VRAM) damp routine bottom-up distractors (`0.75×`) until phasic ACh novelty or VTA dopamine RPE flushes the gate.
5. **Multi-Layer Vector Credit Assignment (Zero Backprop):**
   - **Feedback-Aligned Efference-Copy Burstprop:** Routes physical tick-`(t-1)` vocal/motor efference copies through fixed anatomical projections onto `L1..L_top` apical tufts (Payeur-Naud-Richards 2021 + Lillicrap 2016), giving hidden layers action-specific signed burst error rather than a single blurred scalar broadcast.
   - **Rate-Weighted Competitive D1/D2 Area-X (`TK_GLAND`):** Shen (2008) / Shouval (2002) presynaptic burst-count-weighted competitive plasticity with D2 error-fold veto and songbird LMAN exploratory switching (0 pA during crystallized testing).
   - **Vogels-Sprekeler Homeostatic iSTDP & Chronic-Hub Homeostasis:** Prevents epileptic seizure while keeping selective feature detectors drift-free across trials.

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
| **v39 (Published)** | Prefrontal STF, PBWM Shielding, Vector-FA Burstprop, Pan-Cortical BDNF, 1t Mg²⁺ NMDA, Same-Substep PV Top-$k$, Trisolaris-7 | `972–1,634` neurons, `68.8k` births, `100.5B` STDP edits, `4,152` prey, `322` apex kills, `212` duets, `Q&A 46%/49%`, `Ctx 41%`, `Epi19 40%`, `Trans20 41%`, `1,802×` WM, `intact > zero_delay`, lean `1.6 MB` pure Genesis repo. |
| **v40 (Active)** | Sparse Laminar Rate Control, Deep SNR (`>6×`) & Hippocampal Sequence Replay | Calm `1,000+` neuron firing (`167M → 35–45M` spikes) to restore `7/7` strongly positive ablations, push `cos(A, B) > 0.60` / `cos(A, A') < 0.08` across `L1..L_top`, break `65–85%+` on `Q&A`, `Ctx`, `Epi19`, `Trans20`, and `2-Syl`/`3-Syl` syntax. |
| **v42–v43** | `2,000–4,000+` Neuron Cortices & `1024×1024` Multi-Biome Worlds | Sustained cortical expansion via modular areas, emergent multi-word dialects (`vocal_mi_bits > 0.25` bits), multi-step tool/shelter planning. |
| **v44+** | Grounded Open-Ended Spiking Conversation | Rich bidirectional human-creature dialogue grounded in episodic memory, causal reasoning, and learned compositional syntax — zero LLMs. |
