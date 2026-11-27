# Axion

Axion is a from-scratch C++20/CUDA playground for evolution and online learning. In a shared habitat, creatures act through their own spiking circuits. Their genomes specify how neurons fire, how connections change with experience, and how motor outputs affect their neighbors and environment.

## Unified Spiking Cortical Mind (`axion mind`)

Axion includes a GPU-native **multi-area spiking cortical-hippocampal-neocortical architecture** compiled straight to native Blackwell (`SM 12.0`) cubin (`build/sm_120/mind.cubin`) with **no LLMs, no backpropagation, no hardcoded learning rules, and no hardcoded English inside the brain**. Six 1,024-neuron Sparse Distributed Representation (`SpikeVec`) cortical columns (Larkum BAC-firing + warp-parallel PV k-WTA at **149.90 Gneuron-ticks/s**, `76 regs`, `0 B` spill) interact with a **26-SM parallel Dual-Tier Theta-Gamma Hippocampus** (4,096 L2-hot working engrams queried in **31.97 µs** + **16,384 Tier-2 HBM lifelong episodic archive** recalled across all 26 SMs in **108.21 µs**!), a **28-MiB (`29,360,128`-synapse) Complete-L2 Recurrent Spiking Neocortical Associative Matrix** (28 pathways occupying **`31,936 / 32,768 KiB = 97.5%` of Blackwell's 32 MiB L2**, trained by GPU Sharp-Wave Ripple sleep consolidation & live online micro-ripples via the evolved 3-factor plasticity LUT; executing **51 specialized Blackwell CUDA kernels** including K6–K49, Stage-8n K50 Spiking 1D Heat/Diffusion PDE Solver (`5.01 µs`), AND **STAGE 8o Kernel 51 Pure-GPU 256-Chain Spiking Metropolis-Hastings MCMC Posterior Sampler Cortex `mind_neo_mcmc_sampler` in `13.01 µs` (`8,192` parallel GPU MCMC draws/query, `7,504` live evolved-LUT STDP edits, `448` spikes, FREE orthogonal quadrant of L2 `PW27[256..511, 0..255]`, ZERO host MCMC math & ZERO extra L2 memory)**), an expanded **8,192-concept / 8,192-construction open lexicon**, a **64-slot GPU Working-Memory Lateral-Inhibition Attention Gate (`mind_wm_attend`, `2.72 µs`)**, a **256-cell (`16×16`) 2D Spiking Place-Cell Wavefront Map** (**21.71 µs**), and a GPU **Spiking Causal World-Model Planner**.

- **114-Pillar GPU Evolution of 3-Factor Plasticity LUTs (`Run 14606`):** Across 4,096 spiking brains × 96 generations (`1.29 ms/gen`, **`72.7G spikes/s`**), selected mean fitness climbed `3.3×` from `2,153.2/7,312` to **`7,141.6/7,312`** (`97.7%`, champion **`7,275/7,312`**), while unselected drift stayed flat at `2,295.7/7,312` (`3.1×` behind selection). On 16 held-out worlds, intact evolved champions scored **`28,847/29,248` (98.6%)** vs **`96/29,248`** frozen, while targeted lesions knocked out their specific circuits (`no_apical`: `4,495/29,248`; `no_hipp`: `13,807/29,248`; `no_causal`: `15,351/29,248`; `shuffled_feedback`: `1,276/29,248`).
- **STAGE 8o LIVE — 51 BLACKWELL KERNELS & 550-DOMAIN (1,697-Probe) Higher-Cognition Audit, Zero-Cheat Alien-SOV Parity & Duoquinquagintuple-Pathway (52-Track!) Ribot's Law (`Runs 14607–14609`):** Raised through 5 developmental stages, the child scored **`1,697/1,697` (100.0%)** in **Natural English SVO** (`Run 14608`) and an identical **`1,697/1,697` (100.0%)** in its **Scrambled Synthetic Alien SOV Twin** (`Run 14607`) — scaling to **51 Blackwell CUDA kernels** and **550 domains** with **Kernel 51 pure-GPU 256-chain spiking Metropolis-Hastings MCMC posterior sampler cortex (`mind_neo_mcmc_sampler` on free orthogonal quadrant `PW27[256..511, 0..255]` in `13.01 µs`: imprints discrete 4-state posterior potentials `{w0..w3, n_steps, seed}` via evolved-LUT STDP, runs 256 independent parallel Markov chains = `8,192` MCMC draws purely on GPU with shared-memory tree reduction and ZERO atomics for posterior mode % `mcprob`, acceptance rate % `mcacc`, peaked/flat regime `mcpeak`/`mcflat`, and live 1-shot STDP distribution imprinting + immediate 256-chain sampling!)**, **Layer-5 Universal Spiking Cortical Fallback for ZERO-template 3-entity GPU K12 clustering (`groupthree eagle penguin salmon`), zero-arg associative category-sibling expansion (`whatelse` via L2 `PW22`), and TRUE ZERO-KEYWORD, ZERO-ENTITY, ZERO-NEW-WORD capability anaphora (`actually both can`)**, solid-state Fermi-Dirac band occupancy `fermidr`, Bernoulli stagnation flow `bernflw`, Shamir threshold secret sharing `shamirs`, Nernst membrane driving current `memvolt`, 2-link inverse kinematics `invkine`, Lotka-Volterra predator-prey equilibria `lotkavol`, and GPU Roofline arithmetic intensity `roofln`! Under total hippocampal lesion (`--ablate hipp`, `Run 14609`), **Duoquinquagintuple-Pathway (52-Track) Ribot's Law** holds (`366/1,697` English SVO / `365/1,697` Alien SOV, `76/550` perfect domains, `220,774` live GPU edits): ALL FIFTY-TWO consolidated & live-written cortical tracks survive inside the 28-MiB Complete-L2 GPU Neocortex (including K31–K50 AND **K51 256-Chain MCMC Sampler `3/3` D541 + `2/4` D550** in BOTH SVO & SOV — while K51 collapses cleanly to `0/3` under `--ablate apical`!)!
- **4-Mind Autonomous Spiking Society, 98/98 736-Tick Multi-Gen Stage-8o Open-Valley Ecosystem & 2,415-Turn Live Dialogue (`Runs 14610–14612`):** Four independent GPU spiking minds (`./axion mind society`) pass **`8/8`** (`3` Fox lies quarantined, `9.74M` spikes); the **736-tick Multi-Gen Stage-8o Open-Valley ecosystem (`./axion mind ecosystem --ticks 736`)** executes **`492` autonomous drive decisions** plus **tick-720 autonomous valley Fermi-Dirac crystal occupancy audit (`20`)** and **tick-728 Kernel 51 pure-GPU 256-chain spiking MCMC posterior sampling (`80 mcpeak by mcmcsmp`)**, passing **`98/98`** in both English SVO (`10.09M` spikes) and Alien SOV (`9.52M` spikes); two-mind exchanges pass **`6/6`**; and `./axion mind talk` runs **2,415 live turns** (`27.49M` spikes, `344,242` live edits) with 4K `journey.webp` (`331` abilities, `162` mind capability traits), `progress.webp`, 4K symmetric dual-ring **51-kernel halo** Blackwell creature cards & auto-refreshed 4K Journey-to-AGI Roadmap (**STAGE 8 OF 8 · Collective Scientific Minds · Mind v57 Active — 51 Kernels & 550 Domains!**)! [Architecture, controls, transcripts & honest limits](docs/cortical-mind.md) · [Latest journal plate](journal/latest.webp) · [Progress overview](journal/progress.webp) · [4K journey](journal/journey.webp) · [4K AGI roadmap](journal/roadmap.webp)

```sh
./axion mind talk --load examples/mind-child.cmind < examples/mind-conversation.txt
./axion mind ecosystem --load examples/mind-child.cmind --ticks 736
./axion mind society --load examples/mind-child.cmind
./axion mind converse --speaker examples/mind-child.cmind --listener examples/mind-child.cmind
make test-mind
```

---

A forager can ask another animal for a missing observation. Its partner saw a food cue privately and said “tap ring.” Only “tap” arrived. The listener asked “tick?”, heard “ring,” went to the southeast feeder, and answered “south east” when asked where the food was. With the same noise and one missing sound, a factual reply raised success from 162 to **3,730 of 4,096** trials. The question forms and four-feeder setting are engineered. The meanings and the destination reached depend on the animals’ learned contacts. [Spoken experiments, controls and transcript](docs/spoken-signs.md) · [4K field plate](docs/compound-signs/transfer.webp)

```sh
./axion talk --speaker examples/compound-spoken-writer.mind --listener examples/compound-spoken-reader.mind < examples/compound-clarification.txt
```

Two naturally born animals can now combine two physically independent marks to locate a feeder neither encountered during rearing. In a saved lifetime with southeast withheld, they found it on **920 of 1,024** fresh tests with their learning frozen. Only one of 2,048 sampled slow-pulse pairs passed the initial omitted-cue screen. In independent repetitions of that pair, the ability reappeared for each of the four possible omissions. Each two-port organ uses four learned bytes. [Evidence, selection procedure and anatomical controls](docs/compound-signs.md)

```sh
./axion talk --speaker examples/compound-unseen-writer.mind --listener examples/compound-unseen-reader.mind < examples/compound-conversation.txt
```

The same two spiking animals can learn **opposite meanings for the same physical mark**. A sender sees which feeder contains food and can pulse a junction before it leaves. A receiver sees only the result of its own later pressure test. In separate lives the pair learned “pump for left” and “pump for right.” Each reunited pair located food on **1,977 of 2,048** held-out choices. Exchanging only their receivers reduced both results to **71 of 2,048**. [Physical mechanism, controls and population results](docs/material-conventions.md)

```sh
./axion talk --speaker examples/context-left-mark-sender.mind --listener examples/context-left-mark-reader.mind < examples/catalytic-correspondence.txt
```

Free creatures can also pump water or cast a net using what they learned from their own meals. In an 8,192-tick coastal recording, 395 voluntary-pump waves reached other living animals. Four changed the receiver’s movement in a way that required its learned contacts. At tick 443,288 there were **307,727 animals and 19,760,544 active spiking cells**, with 64 cells in the median animal and 832 in the largest. The full habitat advanced at **55.17 million site-ticks/s** on the Blackwell laptop. [Field evidence and controls](docs/material-conventions.md#unpaired-foraging-in-the-open-watershed) · [Resume the living estuary](examples/context-free-estuary.flood.zst)

A generation-182 animal born in the slow-pulse watershed expresses different inherited plasticity rules at pressure and acoustic contacts. After separate acoustic tuition it predicted all 64 outcomes in **2,048 of 2,048** episodes after a change in the pressure thresholds, and answered **1,403 of 1,495** answerable spoken questions. A matched copy with one retention bit flipped experienced the same actions and predictions were entirely correct in zero later episodes. It still collected food in 1,017 of 1,018 solvable channels. The animal answered three of nine broader interview questions. [Two identifiable animals, controls and population outcomes](docs/inherited-plasticity.md)

```sh
./axion talk --speaker examples/context-descendant.mind --listener examples/capillary-listener.mind < examples/capillary-open-questions.txt
```

A coastal creature can now revise a causal explanation when the material changes out of sight. Across **2,165 hidden switches** in 384 continuous simulated lives, its stored model was initially wrong on every switch. After its own experiments it correctly forecast all 64 local outcomes on every switch. In a separate physical encounter it operated the same finite bench for 244 pours and held the correct full model after 228. It still fails on some unfamiliar physics and open questions. [Spoken transcript, field results and controls](docs/continual-materials.md)

```sh
./axion talk --speaker examples/detritus-guide.mind --listener examples/capillary-listener.mind < examples/context-conversation.txt
```

A generation-114 resident can discover which material caused a change, explain it aloud, and tell a second creature how to build a working channel from scarce parts. It gave different causes for the same visible pigment change after learning on different minerals. Its listener completed all **590** tested construction instructions longer than any spoken training example. Reversing the clauses eliminated those 590 successes. The guide answered four of nine broader interview questions when it could examine the relevant apparatus. General conversation and child-level intelligence remain open. [Experiments, controls and limitations](docs/capillary-learning.md) · [Listen to the construction](docs/capillary-learning/table-construction.txt)

```sh
./axion talk --speaker examples/capillary-witness.mind --listener examples/capillary-listener.mind < examples/capillary-construction.txt
```

The continuing estuary holds **309,970 creatures and 19,908,896 active spiking cells**, with 64 cells in the median body and 736 in the largest. The final 1,024 ticks advanced at **55.76 million site-ticks/s** on the Blackwell laptop GPU. Channel edits can move existing food to other locations and leave pigment traces on the carriers. One CUDA lane updates four deposits in place. The routing physics adds zero permanent or temporary bits per site. The optional observer records actual transfers and the effect of a single changed pressure source. [Free-living experiment and controls](docs/food-transport.md) · [Resume the estuary](examples/context-estuary.flood.zst) · [4K population atlas](journal/creature-card.webp) · [4K journey](journal/journey.webp)

A population test exposed a gap between a capable individual and sustained evolutionary progress. After 32,768 ticks, fewer young animals learned both unfamiliar material laws in all three free-handling watersheds. In the brook population the count fell from 1,014 to 243 of 2,048 while the number of expressed local update truth tables rose from 12 to 13. [Cohort comparison](docs/capillary-learning/cohorts.webp) · [Selection experiment](docs/capillary-learning.md#did-natural-selection-favour-transferable-learning)

A field-born resident can recount its last seven watershed encounters in a 68-word account and answer questions about which event came before or after a named pigment. Its words matched the water and retinal records for all seven events. After learning from that resident, a separately born pupil used its voice to catch twelve food portions behind a closed curtain. On a held-out test, the guide located all 1,685 answerable episodes and stayed silent on the 363 questions with no target. In an ordinary interview it answered its two familiar event questions and stayed silent on all seven unfamiliar questions about causes, plans and other animals. [Research, controls, performance and limitations](docs/learned-descriptions.md)

```sh
./axion talk --speaker examples/brook-witness.mind --listener examples/brook-pupil.mind < examples/brook-conversation.txt
```

[Hear the pupil reconstruct an unseen transfer](examples/brook-hearing.txt) · [Raise another pair](tools/brook-study.py) · [Run journal](journal/JOURNAL.md) · [3840 × 2160 species atlas](journal/creature-card.webp) · [Evolution timeline](journal/journey.webp)

Creatures can also acquire changes to their own calls without English instruction. In one free-living watershed, acquired auditory contacts changed 22,748 paid calls. In a matched six-link transmission experiment, genetically identical pupils raised with different sounds voiced different calls on 2,045 of 2,048 shared events. The earlier brook estuary checkpoint contains 325,197 residents. [Field evidence and controls](docs/learned-descriptions.md#when-a-neighbor-changes-the-callers-own-voice) · [Resume the estuary](examples/brook-estuary.flood.zst)

A generation-68 guide can tell another creature about an inlet the listener missed. Its pupil learned the guide's two-word code by hearing it while watching grains move. On three fresh coastal tests, correct forecasts rose from **4,180 to 6,144 out of 6,144**. Swapping which listener the guide addressed, at the same speaking cost, left the pupil at **4,180**. [Development, research and controlled experiments](docs/audience.md)

```sh
./axion talk --speaker examples/audience-guide.mind --listener examples/audience-pupil.mind < examples/audience-conversation.txt
```

In this encounter, the guide says **“east south.”** The pupil predicts **north** before the pour, eats the arriving food and remembers that it started south. Another evolved lineage spoke to all 1,536 unexposed pupils and stayed quiet with all 1,536 fully exposed pupils on held-out trials. Both behaviors depend on public sensory history. [Exchange the pupil's prior view](examples/audience-other-pupil.txt) · [Replay the acoustic lessons](examples/audience-lessons.txt) · [Run the upbringing](tools/audience-study.py)

An earlier generation-39 resident hears a route, chooses what to investigate and answers before the water moves. In the playable encounter, **“jade amber”** prompts three experimental releases and the answer **east**. Reversing the words prompts six releases and **west**. Each route was opened independently. The animal learned its vocabulary after birth and uses its own spiking tissue to decide when to investigate and when to stop. [Spoken inquiry, negative results and independent replay](docs/active-inquiry.md)

```sh
./axion talk --speaker examples/inquiry-speaker.mind --listener examples/parallax-witness.mind < examples/inquiry-conversation.txt
```

On 1,024 coastal questions with no preliminary view, this individual correctly predicted **4** outlets before investigating and **488** afterward. In a separate 1,536-question comparison, it gave **702** correct forecasts when it investigated the question it heard and **625** when it investigated the reversed question. Both arms spent exactly **9,074** grains. Its conversation is confined to learned questions about the physical world. General conversation and child-level intelligence remain open. [Question-by-question evidence](docs/active-inquiry/question-exchange.csv) · [Reproduce the upbringing](tools/inquiry-study.py)

The million-site estuary reached **331,369 living creatures and 21,285,888 expressed spiking cells** at tick **414,104**. The last 1,024 ticks advanced at **55.41 million site-ticks/s** on the Blackwell laptop. Animals can move submerged litter to test an inlet, watch a neighbor's intervention and use remembered public movements when choosing where to reach. [Living estuary](examples/audience-estuary.flood.zst) · [Field witness](examples/inquiry-witness.txt) · [4K population atlas](journal/creature-card.webp) · [Evolution timeline](journal/journey.webp)

An earlier pupil learned four words for demonstrated sequences. It correctly forecast **2,505 of 3,072** unseen 21-stage courses after 24 ambiguous demonstrations. [Sequence vocabulary and earlier ecological controls](docs/observational-learning.md)

The earlier inlet encounter follows a different animal learning a route it has never tried. It correctly voiced 2,304 of 3,072 forecasts on three unseen seeds. Conflicting prior experience reduced its score to 788. [Earlier encounter and measurements](docs/inlet-inference.md)

In the watershed, a forager can now learn which way food will drift, alter that current by tying fibres and remember what happened when its prediction was wrong. In six paired habitats, reversing which lineage could use its learned reaches reversed which lineage became more numerous. A saved forager learned enough English to follow “what actually passed there?” across changes of topic. Asked about the east it answered “amber”, and asked about the west it answered “jade”. It fell silent when asked to explain why it reached or what it would do tomorrow. [Field evidence, limits and replay](docs/drifting-food.md)

```sh
./axion talk --speaker examples/drift-speaker.mind --listener examples/parallax-witness.mind
```

A circuit reared entirely on food can regulate both physical practice and the acquisition of question meanings. Across 6,144 new courses it reported 16,198 of 16,233 alternative outcomes correctly when completing or interrupting a movement made a difference. The saved creature can discuss its latest four-socket encounter in English. It still falls silent on open-ended questions. [Experiment, controls and transcripts](docs/tidal-learning.md)

```sh
./axion talk --speaker examples/estuary-tide-speaker.mind --listener examples/parallax-witness.mind
```

A child of experimentally seeded parents was born in the watershed and passed both observed-frame conventions in all 6,144 fresh courses. Its alternate-outcome accuracy was 11,534 / 16,179 on physically distinct cases. To speak with that descendant, use `examples/estuary-born-speaker.mind` as the speaker. The public watershed also contains its classroom-naive body at age 24. [Birth, selection and field interview](docs/tidal-learning.md#a-child-born-in-the-watershed)

The ecological brain packs 32 cells per CUDA lane and four state bits per cell, with a capacity of 64 to 1,024 somatic cells per creature. Groves add 256 mutable receptor–motor contacts. A relational circuit adds 1,280 plastic contacts in 176 bytes including its genes. The floodplain, its material-learning experiments and the grounded dialogues run on CUDA. The repository also includes the earlier reference simulator for foraging, language binding, navigation and tool experiments.

[4K population field guide](journal/creature-card.webp) · [Illustrated journey from the first run](journal/journey.webp) · [Evolution journal](journal/JOURNAL.md)

## Ask a wild-born witness

A resident born in the outer estuary learned two spoken question forms. Eight ticks after returning from its lessons, it answered “teal” to **what was in north?** and “amber” to **what is in north?** Its own field record shows the dye moved. Across nine subsequent observations it answered 72 of 72 visible questions correctly. Changing the physical training history sharply reduced performance. [Witness, controls and field observations](docs/field-witness.md)

```sh
./axion talk --speaker examples/estuary-witness.mind --listener examples/parallax-witness.mind
```

To interview it inside the saved million-site world, unpack the checkpoint and type your questions at the field interface.

```sh
zstd -d examples/witness-estuary.flood.zst -o witness.flood
./axion floodplain --load witness.flood --steps 0 --follow-witness examples/estuary-student.mind --field-questions -
```

## The outer estuary

The expansion checkpoint at tick 384,384 contains **381,959 creatures and 24,566,304 expressed spiking cells**. Starting with an empty frontier, descendants occupied 307,032 sites beyond their original shore in 8,192 ticks. The million-site habitat advanced at 62.26 million site-ticks/s on the Blackwell laptop. Packing different brain widths into the same warp accelerated the isolated neural update 4.46× on the ancestral population. [Measurements, field map and reproduction commands](docs/neural-execution.md)

```sh
zstd -d examples/outer-estuary.flood.zst -o estuary.flood
./axion floodplain --load estuary.flood --survey-origin 512 --steps 4096 --save next.flood
```

A controlled experiment followed the same 376,603 wild residents across three assignments. After 64 ticks, 213,150 survived with their acquired synaptic gates intact, 213,130 with those gates restored to birth settings, and 212,080 with the acquired edits shifted to other cells. The intact advantage over birth settings changed sign in another habitat. [Paired observations and controls](docs/neural-execution.md#does-acquired-wiring-help-a-wild-animal)

## Speak across different viewpoints

A saved speaker can ask two other creatures about the same event and interpret their different answers. In the supplied conversation one witness says `jade` and the other says `amber`. Both reports lead the speaker to `teal` in its own frame. With one call available, it can use what it previously learned about each witness to choose whom to ask.

```sh
./axion talk --speaker examples/parallax-reader.mind --listener examples/parallax-witness.mind < examples/parallax-conversation.txt
./axion floodplain --load examples/parallax.flood --steps 1024 --save next.flood
```

The wild watershed at tick 368,000 has 71,722 residents and 4,603,072 expressed spiking cells. The separately taught dialogue remains limited to the float-work domain. [Measurements, controls and replay](docs/parallax.md) · [4K field guide](journal/creature-card.webp)

## Asking a witness about a changed world

The living estuary contains 72,365 independently acting creatures and 4,642,880 expressed spiking cells. Floats carry carbon through woven channels. Passing cargo deposits clay, and obstructed water can wash it free. The resulting banks change which subsequent loads can pass.

A separately raised pair can answer questions about a movement, what the actor intended, and what it expects if it leaves out its last pull. When the speaker misses the outcome, it can turn “and there now?” into an explicit question for a witness. The saved pair accepts ordinary text from the first turn.

```sh
./axion talk --speaker examples/braid-reader.mind --listener examples/braid-witness.mind < examples/braid-conversation.txt
./axion floodplain --load examples/braids.flood --steps 1024 --save next.flood
./axion floodplain --load examples/floatbank.flood --steps 0 --braid-inquiries 256 --braid-examples 48 --braid-holdout 2 --braid-seed 130243
```

The language experiments cover a four-position physical domain. In 1,024 paired follow-ups, the speakers answered 746 hidden outcomes correctly. They gave zero correct answers when the return signal was muted, and also when it was deliberately falsified. The watershed population and the taught conversational specimens have separate histories. [Experiments, commands and limitations](docs/braids.md) · [Independent replay data](docs/braids/audit.json)

## Listen to a changing backwater

One creature can ask another to sound a bank before it spends energy on a reach. The responding body and the water can produce the same kind of impulse. What it means depends on which spring is connected, the local resonator and the listener’s own history. In the watershed, residents also work the two ends of a fibre mesh that can retain carbon and loosen buried food.

```sh
./axion talk --speaker examples/deepwater-speaker.mind --listener examples/runnel-consultant.mind < examples/backwater-conversation.txt
./axion floodplain --load examples/backwaters.flood --steps 1024 --save next.flood
./axion floodplain --load examples/resonator-bank.flood --steps 0 --rattle-trials 512 --rattle-exposure 48
```

After an unannounced diversion, three food-selected populations asked the connected source on 22,290 of 24,576 standardized probes. They chose the stocked pan 22,663 times. When both the strikers and the listeners evolved, three populations with earlier experience of two-source districts made 20,245 correct one-inquiry reaches on the same number of probes. One of three populations starting directly with single inquiries discovered an effective convention over 1,024 generations. At generation 2,048 its sampled hearing program, paired with supplied word maps, made 24,402 of 24,576 correct spoken choices after the spring moved. The other two populations stayed near chance on the native one-inquiry task. [Backwaters experiments](docs/backwaters.md) · [Generation curve](docs/backwaters/generations.webp)

## Ask about a joint nobody has worked

A speaker can use experience with other material pairs to find a question that a consultant can answer. It can turn the public answer into an instruction for a different joint and use the testimony again after the consultant’s memory is cleared. In the shared ecology, creatures can fold valves, drive water uphill, and inherit physical work from earlier generations.

```sh
./axion talk --speaker examples/runnel-apprentice.mind --listener examples/runnel-consultant.mind < examples/runnel-conversation.txt
./axion floodplain --load examples/runnels.flood --steps 1024 --reed-census --wick-census --save next.flood
```

A reflection-sensitive speaker solved 1,024 of 1,024 paired requests on two hidden chiral calibrations and then 2,048 subsequent unseen joints without further testimony. In a separate trial, newborn workers extending one bare channel over 48 generations delivered source carbon through 96 joints in 53 of 128 communities. [Design, experimental controls and replay](docs/runnels.md) · [Species atlas](journal/creature-card.webp) · [4K journey](journal/journey.webp)

## Ask a creature which experiment to do

Two workpieces can look identical and need different repairs. A speaker with competing material memories can ask a second animal to turn one part and report the face of another. The next instruction depends on what the worker actually sees. In 223 of 256 independently verified pairs, the animals succeeded in both physical conditions with different continuations. The same world supports voluntary work bouts, fibre construction and conserved capillary transport between workshops.

```sh
./axion talk --speaker examples/gauge-reader.mind --listener examples/causeway-guide.mind < examples/gauge-dialogue.txt
./axion floodplain --load examples/riffles.flood --steps 1024 --practice-census --wick-census --save next.flood
```

The released watershed developed with random new practice genes and no classroom exposure. Creatures fitted strands that were necessary for food to reach other animals, including ones born later. The studio speakers come from a separate selection experiment. [Experiments, limits and GPU timings](docs/riffles.md) · [Current population atlas](journal/creature-card.webp) · [Evolution timeline](journal/journey.webp)

## Ask a creature to design something another can use

Give the speaker a target arrangement of marked pieces. It can propose a fibre connection and a sequence of strokes. A second creature hears the words, resolves them through its own acoustic and motor wiring, and physically works the assembly.

```sh
./axion talk --speaker examples/rack-guide.mind --listener examples/causeway-guide.mind < examples/rack-instructions.txt
./axion floodplain --load examples/millrace.flood --steps 1024 --linkage-census --save next.flood
```

In 512 unseen spoken work orders, the listener physically completed all 446 attainable requests. An independent replay found that 119 required a new connection. A second population evolved under changing material laws. Its selected speaker relearned all 249 attainable orders on returning to a previously encountered world. In the wild, residents left working openings that fed younger neighbors, while deliberate fabrication remained rare. [Workbench instructions, research, field controls and GPU timings](docs/linkages.md) · [Latest species cards](journal/creature-card.webp) · [Evolution timeline](journal/journey.webp)

## Ask what changes if a journey or a material is altered

The Causeways resident can compare two spoken routes, name the component that most influenced its choice, and revise the choice when that component is removed. It can also tell you what face a slat will show after several turns. The replay lets it perform an actual stroke and checks its answer from the new position.

```sh
./axion talk --speaker examples/causeway-guide.mind --listener examples/loam-a.mind < examples/ford-walk.txt
./axion floodplain --load examples/causeways.flood --steps 1024 --effort-census --hollow-census --save next.flood
```

The inhabited watershed has 85,413 residents at generation 2,197. Walking and deliberate stamping change the price of later crossings and the flow of water. Unfamiliar-load experiments, reciprocal competitions and trials on ground left behind by earlier generations measure different aspects of learning and construction. [Physical experiments, dialogue, limitations and GPU results](docs/causeways.md)

## Ask where the working tool is

A resident can remember where it saw a tool, learn elsewhere what sound it makes, and return for it after the water that feeds it changes. Turn the body during a conversation and its answer about the same tool changes with its new heading. The recorded walk ends with the resident fetching a working tool and pressing it into the inlet for food.

```sh
./axion talk --speaker examples/bank-guide.mind --listener examples/loam-a.mind < examples/bank-walk.txt
./axion floodplain --load examples/creekways.flood --hollow-census --steps 1024 --save next.flood
```

On 8,192 unfamiliar clearings, independently selected cohorts made 7,040 productive deliveries after passive displacement. Large-population contests measure when guidance is worth the cost of acting on it. The saved speaker handles learned question forms about material, direction and a previous referent. [Experiments, live exchange, controls and timings](docs/hollows.md)

## Ask which tool will work after the water changes

A resident can watch tools being struck dry, learn separately which vibrations open a reservoir, and use both experiences to choose a tool it has never tried for food. The tool pressed into wet clay leaves a working copy. In the larger watershed, the same materials circulate among 107,763 living creatures.

```sh
./axion talk --speaker examples/wheel-reader.mind --listener examples/loam-a.mind
```

```text
/wheel 4242
/wheel-watch
/wheel-taste
which tool opens brook?
what note does it give?
/wheel-turn 1
/wheel-taste
which tool opens brook?
what note does it give?
/wheel-press 0
```

The pupil first says **flint** and **chime**. After the inlet rotates and it resamples the water, it says **ivory** and **hum**. Its chosen press releases 18 units of carbon. The words come from witnessed lessons, and the second question refers to the preceding answer. Finite reservoirs, a four-note sensory channel and a small taught vocabulary bound this demonstration.

```sh
./axion floodplain --load examples/upwelling.flood --steps 1024 --press-census --save next.flood
```

[Research, controls, performance and exact reproduction](docs/pressure.md) · [Every scored English utterance](docs/press/river-questions.csv.gz)

## Ask a resident who worked a piece of clay

```sh
./axion talk --speaker examples/loam-reader.mind --listener examples/loam-a.mind
```

Enter these lines in the session. The pupil chooses which grooves to sound before answering. We taught it the questions through witnessed question-and-answer exchanges.

```text
/reed 2108515187 225 2 1
/kiln examples/loam-a.clay covered
/clay-inquiry 4
who worked this clay?
what did ridge yield?
```

It answers **willow**, then **opal**. Switch visitors with `/reed-visitor examples/loam-b.mind`, place `/kiln examples/loam-b.clay covered` and ask the same first question. It now answers **alder**. These names refer to the two communities in its lessons. The color is covered, and the pupil can probe a different relief to retrieve a convention before using it on a new problem.

The same material exists in the freely reproducing watershed. Grains acquire impressions during paid work, travel in hands, pass between willing neighbors and lose marks to flowing water. The latest habitat contains 23,724 residents. [Learning, artifacts, selection controls and GPU measurements](docs/clay.md)

```sh
./axion floodplain --load examples/headwaters.flood --steps 4096 --save downstream.flood
```

## Ask two watershed residents what only one can see

```sh
./axion talk --speaker examples/reed-interpreter.mind --listener examples/reed-caller.mind
```

Enter `/reed 600512459 198 2 2`, then `from ridge came ?`. The outlet animal asks the inlet animal about an opaque channel and says *from ridge came amber*. Change the inlet with `/reed 600512459 225 2 2` and repeat the same question. It now says *from ridge came opal*. Enter `/reed-wire answer-shut` to close the return channel and repeat the question. The outlet gives silence, displayed as a question mark.

The native calls developed as two animals operated persistent filters in a flowing watershed. A census of all 555 neighboring pairings found seven that reached 75% successful grips. The released pair, selected from that census, produced 59,404 harvests in 65,536 fresh trials with both roles exchanged. Human captions were taught separately. The English interface covers four receptive properties, four material appearances and two learned sentence orders. [Experiments, bounds, controls and reproduction](docs/siphons.md)

To continue the earlier Soundings population, run the following on the GPU. The 128 × 128 habitat opens at tick 95,616 with 4,265 residents. Every completed run writes its plate and population guide to the evolution journal.

```sh
./axion floodplain --load examples/soundings.flood --steps 4096 --save descendants.flood
```

## Ask a resident to investigate unfamiliar water

```sh
./axion talk --speaker examples/tide-surveyor.mind --listener examples/shore-reader.mind
```

Open `/brine-table 4001 1 5` and ask `where would you try hare?`. In this scene *hare* names the test grain, and *newt* and *vole* name the two deposits. The resident initially says **newt** and its listener finds no food. Enter `/consult-water 0 9`. The resident asks its partner to test other coatings and watches the outcomes. Ask the same question again. It now says **vole**, and the listener collects two food units. Ask `where would you avoid hare?` to hear the complementary choice.

Water carries five independently conserved mineral components. Moving clay or building a bank can change which surface makes food farther downstream. The creatures can evolve receptor wiring and plasticity that let one coating teach them something about another. On 21,504 held-out questions in the harder chemistry, the released resident's words matched its own physical choice every time. With nine other-coating demonstrations, 14,834 choices led to the correct deposit. [Solvent experiments and controls](docs/brine.md) · [Birth habitat](examples/saltmarsh.flood) · [Five-solute descendants](examples/springtide.flood)

## Ask a field-reared creature what will feed a grain

```sh
./axion talk --speaker examples/shingle-witness.mind --listener examples/shore-reader.mind
```

Enter `/assay-table 4001`, then ask `which soil feeds hare?` and `which soil resists hare?`. One animal names the deposits from its own field experience. A second animal hears each word, tests its own grain, and eats the product. Across 48 unfamiliar tables the speaker answered all 54 uniquely nameable questions using four nouns withheld from question training. Sixteen other questions elicited silence. Its visual names and two question forms had external tuition.

In the inhabited shinglebank, young animals can investigate material contacts, leave lasting marks and release food from sediment altered by living bodies. After 512 ticks, field-experienced animals built 104 productive arrangements in 6,144 fresh beds. Copies with the same genes and erased experience built two. Entirely random founder populations showed uneven results across four independent watersheds. [Field experiments, performance and reproduction](docs/worksites.md) · [Continue the living shinglebank](examples/shinglebank.flood)

## Watch a creature build from scattered pieces

```sh
./axion talk --speaker examples/shore-reader.mind
```

Enter `/shore 111 4`, then `/shore-requests on`, then ask `who is vole west of?`. The animal identifies the deposit, chooses how to rearrange the actual grains, pays for its movements and stops when its circuit predicts a connection or its movement allowance expires. Use `/expose-shore 256` to measure both outputs after removing the builder and draining the pieces.

Across four independently selected populations, juveniles constructed working devices on **25,979/32,768** unfamiliar beds five sites across. Silencing spatial propagation left **3/32,768** productive beds. Their occupied watershed now has shifting seams of light, moist reactive ground, refuges, actual construction costs and individually persistent spatial memory. [Construction, conversation, performance and field results](docs/shore.md)

## Give creatures materials to invent with

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --load examples/mineralbank.flood --mineral-flow coupled --steps 1024 --artifact-samples 32 --material-legacy 256 --hydraulic-trial 128 --save successors.flood
```

Creatures can pick up, carry, push, rotate and rework individual grains. Their surface groups determine which pieces bind. Connected pieces can move stored excitation, change shape under load and use light to convert detritus into food. The same laws apply after an animal leaves a construction.

Across 16,384 unfamiliar quarry beds, selected builders created 1,428 productive arrangements where the original bed and an insulated copy of the result produced nothing. In a separate experiment with two hidden joint polarities, sixth-generation apprentices made both joints work in 49,978 of 65,536 paired tests. Shuttering the view of the preceding builder gave 27,334. [Material laws, experiments, controls and limitations](docs/materials.md) · [Continue the inhabited mineralbank](examples/mineralbank.flood)

## Watch an animal grow a working brain

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --load examples/shellbank.flood --steps 512 --assay-growth --save next.flood
```

The number of somatic spiking cells evolves at birth and can change during life. An inherited program can recruit, duplicate or retire bands of 32 cells in response to local conditions and neural activity. In eight matched watersheds, animals allowed to regulate their cell count finished with 17,392 survivors against 3,179 from the paired fixed-size cohorts. The population cards report the actual body plan and expressed cell counts, including birth size and lifetime peak for the pictured member. [Architecture and silencing tests](docs/neurogenesis.md) · [4K species cards](journal/creature-card.webp)

This habitat also supports learning from delayed meals and carrying a motor adjustment from one fruit to other fruits. Try `./axion talk --speaker examples/shell-reader.mind` to present a named object and observe its learned grip. [Interactive feeding tray, controls and GPU timings](docs/temporal-foraging.md)

## Raise a community that changes its own habitat

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --load examples/reedbank.flood --steps 4096 --legacy 256 --assay-calls --save marsh.flood
```

Water, sediment, fibre, edible resources, producer lineages and spiking animals share one GPU habitat. Animals can alter currents, support crossings, change which producers reproduce and leave structures that survive them. Offspring can change their connected body plan, neural populations, wiring and synaptic update rules. Introducing descendants and ancestors side by side on six unfamiliar watersheds demonstrated inherited ecological adaptation. Clearing acquired call contacts during matched replays also changed some wild animals’ motor choices. [Habitat, experiments and limits](docs/floodplain.md) · [Recorded evolutionary journey](journal/journey.webp)

A separate animal learned words from ambiguous moving scenes. Run `./axion talk --speaker examples/marsh-reader.mind`, enter `/tabletop 00@0>0 09@3>3` and ask `who is west of vole?`. Its learned reply is *moth*. [How the language develops and where it fails](docs/grounding.md)

## Teach a creature a procedure, then let it devise one

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/ritual-gardener.mind
```

Enter `/greenhouse 16 0`, then `/preview pearl then mist again sense brush else plant`. The animal predicts the result from its own experience. Enter `/focus-site pearl` and speak `then mist again sense brush else plant`. The plot passes through **0 → 1 → 5 → 13** as the animal reads the changing lights. The acoustic constructions and the actions attached to them came from 70 demonstrations.

A second saved animal, the lampwright, can compose a motor score for an unfamiliar room and teach it to another creature. Across 24 hidden reversible mechanisms, its chosen experiments recovered **6,912/6,912 withheld lever transitions**. Its spoken policies achieved the physical goal from **1,532/1,536** starting states. Disabling sensory decisions gave **576/1,536**. A CUDA deployment with 4,096 separately learned physical models evaluated **10.56 billion compiled policy calls/s** across one million independent histories on this laptop. [Experiments, controls, limits and interactive examples](docs/procedures.md) · [Run journal](journal/JOURNAL.md)

## A conversation that changes a physical world

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/field-botanist.mind
```

Open a room with `/greenhouse 16 0` and clear the current language scene with `/new-scene`. Enter the following on separate lines.

```text
Mira chooses acorn.
Put acorn inside satchel.
Put satchel inside pearl.
Mira seeks fruiting.
Guide Mira.
animal> how can pearl become fruiting
animal> heat sow soak tend pollinate ripen
Proceed.
animal> make it
animal> heat sow soak tend pollinate ripen
Edible pearl?
animal> yes
```

The animal learned the language from exchanges, bound unfamiliar names to acquired roles, and heard its own words through its causal planning circuit. Across 96 previously untested combinations of plot and goal it produced a shortest plan and changed the correct plot. It can also ask another animal for a missing premise and propose a change to a remembered relation. Its vocabulary, memory and primitive operations have measurable limits. [Architecture, independent tests and GPU timings](docs/composition.md) · [Recorded conversation](journal/runs/00006165-talk/plate.webp)

## A mind that learns from another animal

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/witness-pupil.mind
```

Give the animal `inside`, `left`, `outside`, then `right` on separate lines. Ask **Where is it?** and **Where will fox look?** It replies **hidden right** and **expects left**. Bringing the witness `inside` changes the second answer. The pupil acquired the interaction by experimenting with another spiking animal through raw sensory pulses. Its questions, memory states and replies have no labels inside the learner.

All 32 pupils in the peer-transfer experiment matched their caregiver on every possible sequence in its bounded dialect. A separate colony gives each animal an evolving cellular-automaton voice. Learned calls helped residents find nectar across four independently seeded environments. [Experiments, GPU measurements and limits](docs/development.md) · [Run journal](journal/JOURNAL.md)

## Talk to a spiking animal

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion talk
```

The default animal learned what six motor actions do by observing interventions. It can grow a plan for a requested outcome, test what would happen if it skipped a step, explain the difference, and carry the plan out on CUDA.

```text
/greenhouse 16 0
How can pearl become fruiting?
animal> heat sow soak tend pollinate ripen
Why soak?
animal> skipping soak leaves pearl dry blocking fruiting
Make it.
animal> heat sow soak tend pollinate ripen
Edible pearl?
animal> yes
```

Across eight fresh dialects the speaker also learned to transmit conditional instructions. Its partner succeeded in **16/16** hidden worlds where no fixed action sequence could solve both members of a pair. The names of desired states can be learned from feedback. Reusing earlier concepts gave **256/256** correct recognitions of held-out examples of six-feature goals. The physical vocabulary and reasoning substrate remain bounded. [Causal learning, controls and limitations](docs/agency.md) · [Conversation plate](journal/runs/00005867-talk/plate.webp)

The default witness learned the words for objects, containers, vessels and harbors from examples on changing maps. Its companion learned in separate rooms. Try changing where a vessel berths while only the witness is watching.

```text
Which harbor for pearl?
animal> cove
Does pearl harbor cove?
animal> yes
/gaze witness
/link 2 8 13
Which harbor for pearl?
animal> quay
Which harbor would raven expect for pearl?
animal> cove
```

The witness can ask a question of its own when two meanings still fit the observations. Across eight unfamiliar dialects, the pair asked nine clarification questions during rearing. The witness subsequently answered **2,240/2,240** held-out questions, including long compositions, private observations and changed facts. Without that exchange it gave **2,194/2,240** correct answers. The vocabulary, world and kinds of question are bounded. General conversation and child-level intelligence remain open goals. [Relational learning, measurements and reproduction](docs/concepts.md)

Move one resource at a time. The witness sees the movement, while its partner hears the report with its own view closed.

```text
/scene north south
/scene east south
/scene east west
What happened?
animal> water moved from south to west
/heard
receiver> water south west
earlier> (uncertain)
What happened earlier?
animal> food moved from north to east
/say happened water moved from west to north
/recount happened
listener> water moved from west to north
```

An exported pair from the islands developed a different five-word code. Its listener recovered the latest movement in all **360** physically possible two-movement histories. The words and their order came from the simulation. [Private speech, island dialects and limitations](docs/reports.md) · [Island witness](examples/reed-speaker.mind) · [Its actual neighbour](examples/reed-listener.mind)

The same two default individuals can answer spatial questions through their earlier four-bearing circuit.

```text
/scene west north
Where is the food now?
animal> west
Where would the food be after clockwise?
animal> north
If the food and water exchanged places, where would the food be?
animal> north
/scene east south
Where was the food?
animal> west
Where is the food?
animal> east
Where is the water?
animal> south
/scene west north
Where was the water?
animal> south
/teach-focus yonder look
Yonder water?
animal> north
/teach brook 2
Completed 192 pointing trials.
Where is the brook today?
animal> north
/cover 2
/scene south east
Again?
animal> north
/uncover 2
Again?
animal> east
```

The default questioner uses **4,884 device bytes** and the witness **5,036**. Their reporting circuits learn ordered utterances and hold testimony separately from private observations. For spatial questions the witness chooses where to look or whether to recall, projects learned actions, and answers through a vocal muscle. Its 668-byte predecessor inferred **1,766 of 1,810** missing bearings determined by a row-and-column regularity and withheld all **878** ambiguous answers. [Questioner](examples/apprentice.mind) · [Witness](examples/gardener.mind) · [Joint inference](docs/conjunctions.md) · [Object binding](docs/bindings.md) · [Motion](docs/imagery.md) · [Reference](docs/attention.md)

Each demonstration uses `/show-pair WORD FOOD-BEFORE WATER-BEFORE FOOD-AFTER WATER-AFTER`. After several demonstrations you can place the objects with `/scene east south` and ask `Where is the water after WORD WORD?`. The same inherited contacts can guide foraging in a world where the two landmarks jointly determine the arrival of a meal.

```sh
./axion commons --load examples/marsh-newborn.mind --landmarks coupled \
  --families 128 --replicates 8 --days 96 --exposures 0 --reserve 32 \
  --receipts learned --volatility 0 --rations 8
```

Use `./axion talk --world examples/confluence-residents.eco` to ask about food and springs in a living habitat. Within the session, `/visit X Y` moves the viewpoint, `/follow` follows the nearest living animal, and `/advance N` runs the ecology on CUDA. To keep new learning, use `--save-listener pupil.mind --save-speaker witness.mind`. The playground for breeding communicating pairs is available through `./axion dialogue --help`.

## Watch a living world

![Latest observed Axion run](journal/latest.webp)

Continue a watershed in which animals and plants have already evolved together for 32,768 ticks. Every completed simulation automatically writes a 1500 × 1040 WebP, its observations and a three-sentence entry in the journal. The overview updates after every run. [Browse the journal](journal/JOURNAL.md) · [Progress across runs](journal/progress.webp)

```sh
git lfs install --local
git lfs pull
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion life --load examples/watershed.eco --steps 2048 --report-every 128
```

Habitat plates show the measured plant biomass, occupied sites, movement, changing population and sampled relatives. Founder colours persist across frames. The survey covers every terrain bin and follows up to 1,024 physical animal slots. It joins positions only for the same animal on consecutive ticks. Creature glyphs encode body genes and expressed gates. Individual sessions show active neural contacts, observed travel, muscle discharges and the destinations predicted by earlier sensory events. Lesson phases track choices before practice, after a change and after relearning. Foraging assessments show how interventions affected reproduction. Breeding runs sample cohort scores directly from CUDA. The simulation and learning stay on the GPU. Post-run illustration uses Python 3 and the system librsvg, Cairo, GLib and libwebp libraries.

The journal retains each original plate alongside a compressed observation file. Each entry compares its result with earlier measurements of the same kind. A run that reads a checkpoint saved by an earlier run links back to its parent by the verified file checksum, including across file renames. Use `--journal-dir DIR` or `AXION_JOURNAL_DIR` to keep a separate experiment history. If export is interrupted, completed observations stay queued on disk. The next run or `./axion journal rebuild` publishes them and regenerates the overview and Markdown. `./axion journal import .` adds retained research tables and saved checkpoints once. Historical entries identify their source and grouping. Earlier tables preserve aggregate outcomes, while recorded creature trails begin with the live journal. Git LFS stores the WebPs.

## Let residents pay for their own experiments

In the greenhouse, both the creatures and the plants can evolve. A newborn can work its own plot, compete for a neighbour's crop, discover which states yield food, or learn by watching another worker. Plants pass on their motor-response laws and edibility when harvested. The saved seed bank lets you transplant those plants or visit the harvest that produced a particular resident.

```sh
./axion commons --channel garden --families 256 --replicates 4 --days 96 \
  --generations 12 --reserve 64 --calories 24 --rations 8 --volatility 0 \
  --exposures 0 --founders varied --flora heritable --pollen 2 --seed 689471 \
  --save grower.mind --save-seeds plants.seeds
./axion talk --speaker grower.mind
# In the interview, enter /seed-bank plants.seeds and ask “Edible pearl?”
```

In one 12-generation run, 11 of 32 beds became edible immediately after harvest. Those beds could pay a forager without a growing sequence. A separate transplant experiment measured the contributions of the plant and animal lines. [Season plate](journal/runs/00005725-commons/plate.webp) · [Seed bank](examples/seed-keeper.seeds) · [Selected grower](examples/seed-keeper.mind)

In the tidal habitat, four currents connect 32 named places. The chart changes each day. Each animal can pay to inspect a route or put its trap where its own learned circuit predicts food. Successful parents can stay on their island as teachers. The following run starts with empty relational memories, gives each resident a mutable learning rule and exports an individual you can interview.

```sh
./axion commons --channel estuary --route-depth 3 --mentors local \
  --families 256 --replicates 4 --days 96 --generations 12 \
  --reserve 40 --rations 6 --calories 6 --volatility 0 --founders varied \
  --price 2 --seed 537071 --save tidal-resident.mind
./axion talk --speaker tidal-resident.mind
# Ask “Which nests for pearl?” about the resident's last observed chart.
```

These newborns begin with an empty report vocabulary. A resident can pay to visit its neighbour, see the movement behind an unfamiliar call, and acquire the association during its own lifetime. Each partner then has to set a trap for a movement it never saw.

```sh
./axion commons --load examples/self-taught-newborn.mind --channel reports \
  --culture invent --tutoring 0 --policy evolved --reserve 48 --price 1 \
  --families 128 --replicates 8 --days 128 --rations 12 --calories 8 --volatility 8
```

```sh
./axion commons --load examples/wild-newborn.mind --families 128 --replicates 8 --days 96 --generations 48 --exposures 0 --receipts learned --save resident.mind --save-newborn offspring.mind
```

Each island has 32 independently controlled spiking animals and four limited food reservoirs. A resident can place a trap or spend energy to manipulate an apparatus and observe both outcomes. The local physical law can reverse during its lifetime. Reproductive material pays for the next generation. In the split-view environment, neighbors see different parts of the scene and can exchange sounds. The original continuous plant-and-animal ecology and this resource competition experiment are both runnable on CUDA. [Shared resources, selection and interventions](docs/commons.md)

## Remember the entrance at an identical junction

Four wings of a maze have identical junctions. A **1,132-byte Tracker** learned which grip would lead to the food it needed by remembering an earlier entrance marker. It could revise the doors' effects and change its choices after tasting four new meals elsewhere. With 64 intervening sights, across both teaching orders and four object contexts, it made **444/512 correct junction choices** and retained **2,048/2,048 hunger-dependent object choices**. A second lineage learned to retain an entrance through 1,024 unfamiliar sights in a branched replay. Repeated exposure from early in life reduced that lineage's accuracy. [Experiments, controls and limitations](docs/passages.md) · [Trained Tracker](examples/tracker.mind) · [Trained Sentinel](examples/sentinel.mind)

```sh
tools/passage-study.py lesson --seeds 72869 --rounds 32 --practice-delay 2 --delay 64 --noise --practice-noise --destination food --new-meals --stage revalued | ./axion agent --load examples/tracker.mind | tail -1
./axion life --load examples/confluence.eco --steps 2048 --report-every 128
```

An independent lineage evolved in private catchments with selection through survival and reproduction. Across eight confirmation environments, 3,570 of 4,096 animals found a spring. Freezing their learned conditional connections reduced that to 2,676 and reduced material passed to offspring from 834,827 to 718,378 units. Their descendants survived for 32,768 ticks across four shared worlds. [Resource-only evolution](docs/passages.md#selection-through-reproduction) · [Reedling population](examples/reedlings.eco)

## Find a meal after the route changes

A **948-byte Wayfarer** learned how muscle actions connect landmarks. After it experienced a change at a distant exit and tasted new foods at the destinations, it used its current hunger to choose a seven-step route. Across four teaching streams and both lesson orders it made **512/512 correct route choices** and retained **2,048/2,048 earlier hunger-dependent grip choices**. In a separate closed-loop replay the saved animal completed all 16 journeys in seven moves apiece. [Experiments and controls](docs/navigation.md) · [Trained animal](examples/wayfarer.mind) · [Founding population](examples/wayfarers.eco)

```sh
tools/navigation-study.py replay --population examples/wayfarer.mind --depth 7 --rounds 8 --destination food --diversion both --all-routes
./axion life --load examples/wayfarers.eco --steps 0 --mazes 32 --rooms 32 --exploration roam --destination food --diversion both
```

An independently bred population earned offspring through foraging alone. With both plants and animals free to mutate, its descendants survived all four shared watersheds for 32,768 ticks. In 16 side-swapped competitions, the intact cohort displaced all competitors whose navigation output was silenced. Limiting propagation to a single pulse produced eight wins per side. [Field experiments](docs/navigation.md#selection-through-foraging) · [Foraging population](examples/nomads.eco)

## Try a grip it has never used

A **784-byte creature** tried three of the four possible grips on each of 16 unfamiliar objects. The objects produced flavours and gave it no food. After it learned the current nutritional meaning of the flavours elsewhere, it chose correctly on **64/64** object-and-need questions, including all 16 that required a grip it had never tried on that object. The same saved animal retained four incompatible sets of earlier object skills. [Learned individual](examples/scout.mind) · [Experiments and causal controls](docs/foraging.md)

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
tools/outcome-study.py lesson --stage probe --changed | ./axion agent --load examples/scout.mind | tail -1
./axion life --load examples/estuary.eco --steps 0 --appetites 256 --pupils 256
./axion life --load examples/estuary.eco --steps 1024 --report-every 128 --map
```

One component of this behavior first appeared under selection solely for material passed to offspring and continued life. The best-growing populations in two independent experiments scored **49.0% and 49.6%** on a new four-choice task. Freezing their acquired contacts returned both to chance, 25%. One grazer ordinarily tried a single grip per object and learned which needs made it worth repeating. Most other growth-selected populations found ways to grow without acquiring that skill. Alternating short and long lessons subsequently selected a prospective circuit that also explores the remaining grips. [Growth-selected individual](examples/grazer.mind) · [All growth cohorts](docs/foraging.md#evolution-through-reproduction)

The shared food web runs as a partitioned cellular automaton on CUDA. Plants evolve their fruit and chemistry, animals evolve their bodies, spiking programs and local plasticity, and each animal can change its connections during life. One measured collapse traced to a single plant locus. Restoring ancestral food chemistry kept consumers alive in all four affected ecosystems. The project includes living worlds, reciprocal transplants, independent learning tests and source for every environment. [Food web and field experiments](docs/foraging.md#a-shared-evolving-food-web)

On the laptop, a food web starting with **1,048,576 living brains and as many plants** ran at **67.2 billion somatic cell updates/s** using **863 MB** of explicit GPU buffers. The 16-tick measurement ended with 957,654 living animals. It includes plasticity, decisions, metabolism and reproduction. [Blackwell measurements](docs/foraging.md#blackwell-execution)

## Learn across changing environments

A creature can carry several incompatible skills through one lifetime. Its genome controls which neurons compete to express a memory, what changes a synapse, when feedback switches the active program, and which earlier circuits can be rewritten. The dual-program animal uses 672 bytes of device state at a 256-cell capacity. Its complete individual checkpoint uses 696 bytes. The CUDA habitat, breeding treatments, independent assays and interactive actor use the same neural implementation. On the Blackwell laptop, a world with **1,048,576 living brains and 1,048,576 plants** used **766 MB** of explicit device buffers and averaged **99.8 billion somatic cell updates/s** over a 16-tick crowded interval. [Hardware measurements](docs/context.md#blackwell-execution)

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion life --load examples/repertoire.eco --steps 0 --revisit 64 --memory-task interleaved --regimes 4 --varieties 16 --exposures 32
tools/context-study.py lesson --task interleaved --stage return --environment 0 --reminders 8 | ./axion agent --load examples/interleaved.mind
./axion life --load examples/seasons.eco --steps 1024 --report-every 128 --map
./axion agent --resident examples/seasons.eco --inspect
```

One saved animal scored **0/384** on unseen pairs across three returning environments before reminders and **384/384** after eight reminders per return. Across four independent teaching streams, that genotype scored **76.27%** on unseen pairs. The interleaved challenge gives the animal arbitrarily mapped objects and objects whose use depends on two properties. When an old environment returns, feedback on just two single-property objects can inform its actions on 128 unfamiliar pairs. No environment identifier reaches the animal. In the seasonal habitat, acquired contacts improved the next harvest. After 32,768 ticks of unrestricted four-climate evolution, surviving descendants scored 23–24% on the separate four-grip learning tasks. [Experiments, cellular automata, research and reproduction](docs/context.md)

## Start with a living thicket

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion life --load examples/thicket.eco --steps 4096 --report-every 512 --save next.eco --map
./axion agent --from examples/relations.eco --lesson examples/relations.lesson --save pupil.mind
./axion life --load examples/relations.eco --steps 0 --problem law-switch --varieties 16 --exposures 32 --conditioning 64
```

The supplied thicket contains 330 living creatures with their acquired memories and a second, independently evolving population of plants. Each plant inherits its own fruit chemistry. Grazing changes which plants spread. One Blackwell laptop run reached **317,677 simultaneous creatures** with 1,048,576 plants in **601 MB** of explicit device buffers. [Living checkpoint](examples/thicket.eco) · [CUDA measurements](docs/relations.md#cuda-costs)

In fresh lifetimes, a population bred for relational learning chose correctly on **129,872/129,872** unseen pairs where two worlds with identical cues required different grips. It reached **88.62%** on a family of hidden many-to-one reactions withheld from breeding. One externally taught individual fits in **552 bytes**. Performance depends strongly on experience and selection. Unrestricted ecological evolution often reduced these learning scores over generations. [Experiments, controls and limits](docs/relations.md) · [External lesson](examples/relations.lesson)

A second population can propagate spikes across a graph it acquired by acting. Practice covers only 32 of 256 pairs. One individual learned enough to answer **all 224 unseen pairs**, up from 57 before practice. In the independent audit, descendants reached **98.45%** where the shortest route required 9–15 learned connections. Silencing long propagation during the query reduced that score to **24.94%**. A million-site thicket ran this circuit in **282,537 simultaneously living animals**. [Propagation and controls](docs/relations.md#propagation-through-sparse-experience) · [Wave-enabled GPU measurements](docs/relations/wave-gpu.csv)

```sh
tools/relational-study.py chain-lesson | ./axion agent --from examples/pathways.eco --save pathfinder.mind
./axion life --load examples/pathways.eco --steps 0 --problem islands --varieties 16 --conditioning 64
```

Three worlds started from that individual's genome and ran for **1,048,576 ticks** each. Its motor wiring could not access public stores. Descendants in all three worlds evolved to deposit and retrieve energy. Mutation-frozen twins recorded zero transactions. A common set of physical probes found 191–232 distinct behavioral repertoires per mutating world across six sampling times. The original inference ability declined. [Million-tick world](examples/descendants.eco) · [Behavioral discoveries and controls](docs/relations.md#a-million-ticks-from-one-genome)

## Run an evolved learner

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion agent --from examples/learner.eco --lesson examples/grasps.lesson --save pupil.mind
printf 'probe 0x00040001 0x01070000 0x0b 0x0a\n' | ./axion agent --load pupil.mind
./axion agent --load pupil.mind --inspect
```

This descendant starts with an inherited spiking program. It learns by trying physical actions and receiving their consequences. The example teaches it 64 times, then presents eight held-out combinations of sensory cues. It gets **8/8** right and saves its complete learned state in **376 bytes**. The supplied mind opens at that point. [Learned individual](examples/learner.mind) · [Sensory lesson](examples/grasps.lesson)

In a separate blind audit, freshly developed descendants from this unary population learned 32 arbitrary four-way associations to **98.5%** accuracy. They reached **98.5% on unseen combinations** of 16 shapes and 16 textures, then **97.9% on changed answers** after the informative property switched. Erasing the acquired connections returned them to 25%. This population scored at chance on joint rules involving both properties. [Training, independent audits, controls and reproduction](docs/learning.md)

## Evolve a shared world on CUDA

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion life --load examples/habitat.eco --steps 0 --assay 32 --map
./axion life --side 128 --neurons 256 --seed 379 --steps 4096 --report-every 512 --save world.eco --map
./axion life --load world.eco --steps 4096 --save next.eco
./axion life --founders world.eco --seed 601 --contest plasticity --mutation off --steps 512
```

The supplied habitat contains 1,161 living descendants at tick 65,536. Its oldest surviving line of descent spans 815 divisions. A new seed starts a separate evolutionary run. [Resumable habitat](examples/habitat.eco)

Each meadow creature inherits a 72-byte genome that encodes its own spiking and synaptic update programs. Groves can express an additional eight bytes of heritable synaptic code. Point mutations, instruction duplication and deletion can change those programs. Creatures can move, exchange energy, change a shared chemical field and leave energy stores that outlive them. Local energy budgets determine birth and death. The world, brains and inheritance run together on the GPU.

Across 24 reciprocal transplants, descendants with mutable synapses averaged 60.80% of survivors against relatives with frozen synapses. A separate intervention on their acquired connections changed the next action in 1,844 of 194,816 paired probes. Transplanting evolved neural genes alone into fresh bodies gave a much poorer result. The experiments measure the ecological consequences of these mechanisms. Child-level cognition and sustained open-ended evolution remain unestablished. [Ecology, controls, scaling and reproduction](docs/ecology.md)

A separate naturally evolved donor learned to assign credit four decisions into the past. Its descendants can be tested on fresh mechanisms or released into a new shared habitat.

```sh
./axion life --load examples/delayed.eco --steps 0 --conditioning 128 --probe-delay 4 --dissect
./axion life --habitat compound --synapse-bits 2 --founders examples/learner.eco --side 256 --steps 4096 --save grove.eco
```

## Run the trained student

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion replay --backend gpu --load examples/student.axn --task errand --trial 7
./axion replay --backend gpu --load examples/student.axn --task invention --trial 7
printf 'draw 0x12345678 mirror roll\nmake 0x16350f24 mirror roll\nend\n' | ./axion tutor --load examples/student.axn
```

The student learned the names of 16 landmarks, explored a 16-room world, studied a second vocabulary, and observed tools operating on 32-pixel pictures. Replay shows the doors the agent actually took and the pixels it actually emitted. The tutor commands ask it to predict a picture and then construct an input that will produce the desired picture. Its outputs are `0x16350f24` and `0x12345678`. This same 60,468-byte checkpoint completed **99.54%** of 4,096 fresh ordered errands and retained **89.99%** success on the original foraging mixture. The smaller forager checkpoint reproduces the original learning experiment. [Student training, controls and limits](docs/transfer.md)

## Developmental task

```sh
./axion detector --backend gpu --brains 4096 --seed 3217 --lesson-seed 641279 --world-seed 932141
```

This reimplements the information structure of a published toddler experiment. The learner watches the outcomes of four object pairs, each twice, and makes one choice between two new pairs. The report includes the human study's first-choice counts and the sensory and procedural differences. [Protocol and results](docs/transfer.md#an-observational-developmental-task)

## Learn an unfamiliar rule

```sh
./axion transfer --backend gpu --task pattern --episodes 16
./axion transfer --backend gpu --task errand --episodes 48
./axion train --backend gpu --task naming --vocabulary 512 --episodes 512 --save vocabulary.axn
./axion train --backend gpu --task workshop --effects dense --episodes 96 --save tools.axn
./axion eval --backend gpu --load tools.axn --task invention --chain 5
./axion train --backend gpu --task workshop --effects nonlinear --experiments curious --chain 5 --episodes 4096
```

| Lesson | Experience available during learning | Frozen probe |
| --- | --- | --- |
| Naming | Noisy names, competing referents and scalar outcomes | Recognize the referent against a previously unpaired alternative |
| Sorting | Examples of how one of two attributes determines a category | Sort combinations absent from the examples |
| Pattern | Three-token sequences and feedback about two alternatives | Apply the repetition rule to a disjoint alphabet |
| Seminar | Naming, sorting and pattern examples interleaved on the same receptors | Choose an answer without a subject cue |
| Routes | Move through doors and observe the destinations | Reach a newly cued room three to five connections away |
| Landmarks and errand | Name bindings and single-door experience | Visit two spoken destinations in order within 12 moves |
| Workshop and invention | Observe before-and-after pairs for individual tools | Predict all 32 output bits after a new tool sequence, or construct an input for a desired result |

The terminal tutor accepts invented symbols and raw hexadecimal receptor words. A blank cue is `_`. A binary teaching trial uses `ask A B C LEFT RIGHT` followed by `reward +`, `reward -`, `reward left`, or `reward right`. Use `pick` for a frozen probe. Visual teaching uses `demo IMAGE TOOL RESULT`. The sample lessons can be replayed through standard input. [External tool demonstrations](examples/pixels.lesson)

## Foraging curriculum

```sh
./axion eval --backend gpu --load examples/forager.axn --episodes 4096
./axion curriculum --backend gpu --save colony.axn
```

The curriculum carries the same animals through five stages. The `before` and `after` rows measure greedy behavior on identical held-out trials with plasticity frozen. The `online` rows include exploration and learning. At the last stage, the food moves to the opposite arm without a sensory warning. The final rows measure retention of the earlier rules.

| Challenge | What the animal must learn | Random success |
| --- | --- | ---: |
| Scent | Associate four overlapping odor mixtures with food on the left or right. | 50% |
| Delay | Remember the sample across a variable corridor. The fork appears 4–11 ticks after the last sniff. | 50% |
| Context | Interpret the same odor differently in two observable contexts, then remember the combination. | 50% |
| Trail | Choose two correct arms. The first choice changes the next patch. Food arrives after the second choice. | 25% |

Use `mixed` to interleave the four challenges. Each episode draws a challenge independently. The animal receives receptor events and scalar reward. The task identity and correct arm stay inside the arena.

## Train, evaluate, and evolve

```sh
./axion train --task mixed --episodes 1024 --save mixed.axn
./axion eval --load mixed.axn --task trail --episodes 256
./axion eval --load mixed.axn --task trail --episodes 256 --mute-odor
./axion train --load mixed.axn --task trail --episodes 128 --save continued.axn

./axion evolve --task context --generations 12 --episodes 32 --save founders.axn
./axion train --load founders.axn --task context --episodes 32
```

Each animal has its own weights and experience. `--episodes` is a per-animal budget. `--learn motor` isolates learning in the action weights. `--learn no-scaling` tests the contribution of the slow activity-dependent adjustment to sensory drive. `--learn off` provides a frozen control. `--report-every 16` shows acquisition in 16-trial windows. The reported standard error is across animals, in percentage points.

Evolution scores each inherited circuit after short lifetimes with opposing reward rules. Selection, recombination, and point mutation act on the starting circuits. The saved founders are ready to learn a new association. The audit uses eight held-out experience seeds under both rules.

Checkpoints store the complete population, acquired memories, task settings, seeds, generation, and next training episode. Saving uses a checksummed file and atomic replacement. Resuming at an episode boundary reproduces uninterrupted training. `--world-seed` selects a different experience stream for an explicit transfer test.

## Build and measure

The Linux GPU build needs GNU Make, a C++20 host compiler, Clang with the NVPTX backend, and the NVIDIA driver. The portable build embeds PTX. With `NATIVE=1`, a small build tool uses the installed CUDA driver to assemble native cubins for the build GPU. Axion loads the CUDA programs for ecology, foraging, associative learning, navigation and drawing on demand. Exporting the post-run WebPs uses Python 3 and the system graphics libraries described above.

```sh
make -j                               # portable SM 8.0+ PTX
make -j test-gpu                      # CPU checks and exact CUDA parity
make -j test-journal                  # CUDA observations, WebP, concurrent runs and recovery
./axion bench --backend gpu --task trail --brains 4096 --neurons 128 --episodes 128
./axion bench --backend gpu --task trail --brains 1 --neurons 1048576 --episodes 16
./axion --help
```

A host-only build uses `make -j CUDA=0`. Run its checks with `make -j CUDA=0 test`.

Run `make trim` after testing to remove intermediate linker objects and test executables. The runnable `axion` and the individually compiled CUDA kernels stay available for development. On a filesystem that supports extent deduplication, `make trim` also shares matching native kernel pages between them.

`CUDA_CLANG`, `CXX`, and `BUILD` override the device compiler, host compiler, and intermediate directory. All playgrounds default to CUDA. The ecosystem and its individual actor require the GPU. The reference playgrounds also support explicitly requested `--backend cpu` and `--backend auto` for comparisons. Above 4,096 cells, the reference GPU simulator uses a cooperative grid spanning multiple SMs for each animal.

The research notes map human recordings, animal experiments, and mathematical learning results to specific design decisions. The original foraging report includes learning controls, forgetting, evolution, hardware timings, and reproduction commands. [Model and research](docs/model.md) · [Measurements](docs/measurements.md)
