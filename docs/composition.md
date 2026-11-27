# Learning a relation that survives a change of names

A creature can hear a name for the first time, bind it to a role, and use it in a new answer. The field botanist goes one step further. It can turn a learned instruction into a goal for its own causal planner and act on the result.

```sh
./axion talk --speaker examples/field-botanist.mind
```

```text
/greenhouse 16 0
/new-scene
Mira chooses acorn.
Put acorn inside satchel.
Put satchel inside pearl.
Mira seeks fruiting.
Guide Mira.
animal> how can pearl become fruiting
animal> heat sow soak tend pollinate ripen
Why soak?
animal> skipping soak leaves pearl dry blocking fruiting
Proceed.
animal> make it
animal> heat sow soak tend pollinate ripen
Edible pearl?
animal> yes
```

The shelter pupil learned how to follow choices and containment. A separate routing lesson introduced a second language. Thirty-five short greenhouse exchanges then taught the field botanist to combine the person’s chosen object, its ultimate location and the person’s desired condition. Its utterance activated the gardener's previously acquired goal and action circuits. The connector passes the emitted waves through the reversible spelling transducer. It contains no dictionary of roles, crops or required motors. The two successive replies after “Guide Mira” are the learned question and the planner’s answer. [Teaching exchanges](../examples/horticulture.lesson) · [Recorded conversation](../journal/runs/00006165-talk/plate.webp)

In an independent test, the creature began each trial with a fresh relational scene and an explicitly initialized greenhouse. The field botanist faced 16 named plots and six possible goals, each initially unsatisfied. In **96/96** trials it executed a shortest action sequence according to an independent breadth-first search, reached the physical condition and left a second plot unchanged. With self-hearing disconnected, it still voiced both learned instructions. None of the 96 physical goals was reached. [All paired interventions and physical readbacks](composition/self-hearing.csv)

## Why factor experience

Recordings from 16 human neurosurgical patients found populations sensitive to image content, context and their combinations. Experiments in mice found separable components for time and position, and a choice representation that generalized between mazes. A human compositional-learning experiment found that signals for familiar elements could predict responses to new combinations in several cortical areas. Axion takes an engineering question from this work. How little state does a learner need to reuse the same operation for new individuals, places and situations? These findings do not establish a unique neural implementation. [Human single-neuron recordings](https://www.nature.com/articles/s41586-025-09910-2) · [Hippocampal generalization, October 2026](https://www.nature.com/articles/s41593-026-02491-x) · [Human compositional generalization](https://pubmed.ncbi.nlm.nih.gov/42330957/)

Fast-weight research motivates keeping slow control circuitry and rapidly writable associations distinct. Research on relational inference in the human hippocampus motivates a behavioral distinction between memorizing the last observed transition and recombining structure in a new context. Axion uses discrete content-addressed synapses, inherited gates, and explicit tests for independently changeable relationships. [Fast weight programmers](https://proceedings.mlr.press/v139/schlag21a) · [Human relational inference, November 2026](https://www.nature.com/articles/s41562-026-02627-w)

## What the learner receives

The port presents up to eight successive 64-channel spike waves in a turn. A teaching exchange can also carry an audible response. In the text interface, whitespace supplies word boundaries and an alphabetic word of up to twelve letters maps reversibly to one wave. Hexadecimal tokens can carry arbitrary 64-bit receptor activity. Raw continuous audio segmentation remains outside this experiment.

During rearing, the pupil recruits auditory prototypes with an inherited distance threshold and a two-plane correction rule. It compares utterances that share some waves in the same positions. Constant positions become context and positions that vary can drive variable-binding contacts. The learner proposes one-key and two-key projections from its own auditory positions. It receives neither the physical fact table nor an annotation naming a subject, observer, location, question or goal.

CUDA blocks replay the teaching stream. For every output position they test candidate connections against entire sensory histories. Candidates can copy a current wave, retrieve a literal, use prior discourse, retrieve the last value at an address, follow two different projections, repeat a projection to its endpoint, or condition a read on the time a different contact changed. A circuit can represent an absent witness by keeping the latest version predating that witness’s loss of sight. Multiple witnesses can have independent cutoffs. The storage engine retains versions which a live cutoff may still require. A cycle in a recurrent retrieval produces an unresolved answer.

These are discrete sparse spiking assemblies with engineered induction and memory primitives. The 96-bit inherited program controls sensory recruitment, local plasticity, which types of connections can grow and which experiments the learner accepts. Selection changes that program, and experience changes the circuit it constructs. Utterance segmentation, the candidate operations, population reproduction and storage limits remain design choices.

The plastic organ takes **12,448 bytes**, including 240 retained exchanges, 255 sensory prototypes, 248 live or historical contacts, and the state of a pending question. It can maintain up to 12 fact patterns, 12 vocal constructions and 48 projected relation types. The field botanist combines it with the earlier cortical, causal and sequence organs in **20,628 bytes** of individual payload. The full-precision sensory and instructional memories support later learning. The smaller deployment measurements below assume a calibrated input inventory.

## Asking for evidence

A first pilot achieved 960/960 answers while some output cells merely echoed the previous utterance. Randomizing the order of care, changing who chose which object, and making both participants independently relevant to a gift exposed the shortcut. The public test uses a separately scheduled world. The comparison also pins or disconnects the acquired contacts on the same animals. [Corrected pilot](../journal/runs/00006045-compose/plate.webp) · [Cohorts and ablations](composition/peer.csv)

A pupil can retain up to eight consistent predictions for each response position during tuition. When they disagree, it can request its mentor’s actual reply and eliminate incompatible projections. In the yoked control, a twin receives exactly the same number of replies at randomly chosen opportunities. All calibration stimuli are distinct from the deployment names, and calibration chains have a maximum depth of three. Across four seeds, the deployment test contained 299,508 questions.

| Feedback policy | Requested mentor replies | Correct whole utterances | Animals correct on every sampled question |
| --- | ---: | ---: | ---: |
| Passive | 0 | 298,196 | 485/512 |
| On disagreement | 1,710 | 299,432 | 511/512 |
| Yoked random opportunities | 1,710 | 298,371 | 488/512 |

The saved courier learned from another trained spiking animal. In its 64-pupil cohort, 209 requested replies resolved a projection ambiguity. All 64 pupils then produced all **37,396** expected utterances, including 2,712 conflicting-view questions and 4,303 questions involving deeper chains than the initial demonstrations. One separate campaign pupil continued to fail after tuition. [Matched active-learning trials](composition/tuition.csv) · [Saved courier](../examples/courier.mind)

The same mechanism supports a question *during* deployment. If a necessary live contact is missing, the animal searches its acquired question forms for a way to ask for that value. It retains the interrupted request, hears the reply and resumes the inference. After an unsuccessful foraging action it can check a previously reported premise with its partner. The pending question, the current discourse and the distinction between observed and reported facts survive saving the mind.

The animal can also work backward from a desired answer. The `/want QUESTION | DESIRED REPLY` port injects an objective into its existing readout. The circuit traces the bindings on which that reply depends, constructs candidate utterances from acquired fact patterns, and simulates each candidate through an isolated contact update. Across 128 independently requested relocations, it voiced an effective fact update, left reality unchanged during imagination, and then produced 1,024 correct readbacks after the proposed updates were actually presented. In 56 cases its proposal used an intermediate object already at the desired endpoint. A request to change what an absent witness already saw yielded an unresolved response. This search currently considers one intervention on a bound address at a time.

```text
/new-scene
Put compass inside pouch.
Put pouch inside loft.
Where should ibis search?
animal> what did ibis choose?
compass
animal> try loft
```

## How far the result transfers

* **Every arrangement of eight objects in four places.** One configuration was presented per CUDA thread for all 65,536 configurations. The pupil answered all 524,288 initial questions. It then passed every one of the 2,097,152 single-object interventions and all 16,777,216 readbacks after those interventions. These arrangements have distinct answer vectors, so a deterministic flat-state controller reproducing them all would require at least 65,536 distinguishable states. This exhausts this particular family of configurations and updates. [Census](composition/census.csv)
* **Independent false-belief scorer.** A Python apparatus generated every ordered pair of 18 small nested scenes. It changed when two different witnesses could observe. The CUDA animal gave **3,888/3,888** exact answers through the public text port, including 324 changes that put the absent witness in conflict with physical reality. Separate tests cover cycles, unrecognized words, and 4,096 overwrites while an old view is pinned. [External scorer](../tools/composition-study.py) · [Regression tests](../tests/composition.py)
* **A different teacher and language.** A separately written speaker taught 159 exchanges using four previously unfamiliar constructions. Twenty-six new names formed a chain 25 links long. All 25 endpoint questions, all 25 immediate-neighbor questions and all 36 ordered two-key ticket queries passed. The same checkpoint retained the original shelter language and the earlier 24-state witness interaction across 65,536 intervening gestures. Appending a lesson uses a separate provisional graph, then merges its settled contacts into existing memory. [Bilingual pupil](../examples/river-courier.mind) · [Previous witness experiment](development.md)
* **Evolvability.** Across four seeds, 16 generations of 64 candidates selected by downstream behavior yielded 27,899/28,095 exact answers in permuted dialects. Reproductive drift yielded 1,745 and unselected random founders 2,619 under the same deployment procedure. All candidates in a generation received the same physical experiences. Held-out names, longer deployment chains and scored histories were excluded from selection. [Four-seed comparison](composition/selection.csv)
* **Sensory noise still hurts.** Across 128 permuted teaching communities, passively reared pupils gave 74,473/74,802 exact clean utterances. Flipping two randomly chosen receptor bits per incoming wave reduced that to 66,032. Four flips reduced the count to 620. This test compares complete clean output waves, so an incorrect bit also invalidates a copied name. [Transfer and sensory noise](composition/transfer.csv)

## Social foraging

Each island contains 16 communicating pairs. Every adult grew a private circuit from childhood experience. Both residents can see the container network. Only one initially observes a given individual's choice of object. Finding food requires binding that private choice to a changing chain of containers. The focal animal pays for motor actions, questions and unsuccessful first attempts. The physical world sees its emitted action, and the partner can answer a question only through its own acquired circuit. Selection samples parents in proportion to the energy remaining after competition for finite nectar.

| Acoustic treatment | Correct choices with a private premise | Opportunities | Nectar meals | Energy for offspring across seasons |
| --- | ---: | ---: | ---: | ---: |
| Open | 188,352 | 196,649 | 190,111 | 871,163 |
| Muted | 0 | 195,904 | 172,294 | 722,774 |
| Shifted reply | 82,279 | 196,614 | 189,330 | 743,140 |
| Solitary | 0 | 196,665 | 172,236 | 910,332 |
| Open, repair disabled | 107,209 | 196,649 | 189,893 | 973,123 |

Each row covers four seeds, 256 residents and six seasons per seed. Private opportunities differ because energy influences participation. Intact communication improved localization and collected more nectar. Solitary and repair-disabled colonies retained more energy for reproduction. The habitat assigns a real cost to asking and checking. Fitness gains and open-ended complexity therefore require separate evidence. [All seasons](composition/society.csv)

In a larger habitat, 2,048 founders collected 507,825 meals and asked 206,566 questions across eight seasons. The last adult season retained 446 founding lines and 795 distinct learning genotypes. The saved nest holds the entire brood and the conditions for the next season. Resuming it on CUDA produced the same offspring bytes as uninterrupted evolution in the checkpoint test. [Large islands](composition/large-islands.csv) · [Saved nest](../examples/shelters.nest) · [Island visualization](../journal/runs/00006103-compose/plate.webp)

## The Blackwell memory layout

A general-purpose contact stores two symbol keys, a relation, a value, a timestamp and a provenance bit. A calibrated finite language can discard the timestamps. The compiler allocates only the relation addresses which its inventory can write, finds the smallest useful value width for each projection and captures only the dated fields when a learned witness gate closes. Each resident gets a packed array of current values and an independent saved view for each possible observer. Earlier contacts can keep changing while that view is sealed.

On this laptop's RTX PRO 2000 Blackwell GPU, 64 independently reared circuits served 1,048,576 independent histories of 128 acoustic events. The timed kernel resets each resident, chooses events from a pretransduced 256-entry catalog, processes statements and questions, and computes a digest of the replies. It excludes raw waveform clustering, training, host transport and WebP rendering. The statistics are medians of five passes after a warm-up.

| Layout | Private memory | Time for 134,217,728 events | Million events/s |
| --- | ---: | ---: | ---: |
| General temporal contacts and discourse | 2,328 B | 224.743 ms | 597.205 |
| Packed relations and discourse | 28 B + 18 B | 125.493 ms | 1,069.53 |

The packed case uses 28 calibrated relation addresses and four possible observer snapshots. Shared anatomy, acoustic inventory, calibration tables, benchmark digests, GPU registers and transient compiler storage are additional. The private memory requirement scales with the number of writable addresses and views. These particular figures do not describe learning a new language or running the whole organism.

The checker compared 2,336,187 individual current and remembered contact values across 65,536 events. It also compared complete emitted history digests for all million residents, including a continuation across kernel launches. A separate public `--catalog FILE --stream FILE` port transduces actual text and rejects writes outside its representable domain. An exported packed session restores into the full plastic organism with the same remembered views. [Benchmark measurements](composition/throughput.csv)

## Reproduce

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/field-botanist.mind
./tools/composition-study.py exhaustive --mind examples/river-courier.mind
./tools/composition-study.py enact --mind examples/field-botanist.mind
./tools/composition-study.py tuition
./axion compose --resume-nest examples/shelters.nest --seasons 4 --save-nest next.nest
make NATIVE=1 CUDA_ARCH=sm_120 test-composition
```

Use `/new-scene` for a new physical episode. `/demonstrate INPUT | REPLY` records an exchange and `/learn` consolidates the pending lesson. `/miss` supplies a failed outcome and `/cancel` dismisses a pending request. An animal with the causal circuit hears its own confident vocal output. `/self-hear off` disconnects that pathway.

The animal can acquire and reuse short language constructions, ask a clarifying question, revise testimony and combine a learned goal with physical planning. General natural language, spontaneous acquisition of these primitives, unbounded evolving morphology and human-child intelligence all remain unverified.
