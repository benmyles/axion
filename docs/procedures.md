# Learning, inventing and teaching motor procedures

A creature can hear an unfamiliar score, choose movements according to a changing scene, compose a score for a new goal, keep it in memory and speak it to another creature. In the reversible lantern cabinet, a five-move score devised by one animal lit the requested lamp from every one of 64 starting states. A second animal learned the speaker’s motor language by watching demonstrations and could carry out the spoken score. The wire and physical states were recorded. [Teacher’s forecast](../journal/runs/00006473-imitate/plate.webp) · [Two-creature conversation](../journal/runs/00006475-talk/plate.webp)

## Try the saved animals

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/ritual-gardener.mind
```

Enter the following on separate lines. The plot starts at zero. The preview predicts state 13 while the plot stays at zero. When the animal carries out the score, its actual path is 0 → 1 → 5 → 13. The two invocations of the sensory clause select different actions because the first changes the plot.

```text
/greenhouse 16 0
/plot 0 0
/preview pearl then mist again sense brush else plant
/focus-site pearl
then mist again sense brush else plant
/read-plot 0
/focus-site off
```

The gardener also has the field botanist’s acquired constructions and earlier planning circuit. After the example above it can hear “Mira chooses acorn.”, “Put acorn inside satchel.”, “Put satchel inside pearl.” and “Mira seeks fruiting.” Asking “Guide Mira.” elicits a plan for the currently observed plot. “Proceed.” carries it out, and “Edible pearl?” asks about the result. [Earlier language and goal experiments](composition.md)

The lampwright has a different acquired model and a score remembered from its own planning. The following experiment rears a separate listener by sound and action, then opens their common room.

```sh
./axion imitate --load examples/field-botanist.mind --mentor examples/lampwright.mind \
  --origin permissive --generations 0 --worlds 1 --drills 0 --save pupil.mind
./axion talk --speaker examples/lampwright.mind --listener pupil.mind
```

```text
/lantern 16 509
/plot 0 0
/pass-score pearl muted
/pass-score pearl reversed
/pass-score pearl open
/plot 0 0
/focus-site pearl
Proceed.
/read-plot 0
```

With the open wire, the pupil’s five physical movements took the plot through **0 → 4 → 6 → 9 → 58 → 19**. Its own previously learned greenhouse model had predicted 6. The teacher predicted 19 and reached 19 when it subsequently heard its own learned action request. Both the muted and reversed transmissions left the plot at zero. The receiver gets only the speaker’s acoustic events and its own observations. [Recorded exchange](../journal/runs/00006475-talk/observations.json.gz)

## Research behind the experiments

Children have inferred new procedural patterns from sparse examples, including patterns that extrapolate beyond observed sequence lengths. That finding motivated separate tests for memorizing examples, inferring their generating procedure and executing that procedure under new conditions. Axion’s teachers use generated cultures and external acoustic demonstrations. Axion has undergone no developmental comparison with children. [Children use algorithm induction to discover patterns in data](https://pmc.ncbi.nlm.nih.gov/articles/PMC13392036/)

Recent human experiments found that people reused learned relationships after a change of context. A perturbation study found that interfering with hippocampal activity affected structure-based inference. A separate study of human multidimensional maps examined how people select the dimension relevant to a goal. These results motivated measuring transfer after changing the physical mechanism and changing the goal independently. The particular induction operators, finite-state representation and GPU compiler are engineering hypotheses. [Relational structure learning and inference](https://pubmed.ncbi.nlm.nih.gov/43040044/) · [Cognitive control and multidimensional maps](https://pmc.ncbi.nlm.nih.gov/articles/PMC13640497/)

Inference from demonstrations has a mathematical precedent in program induction and machine learning experiments on composition. Axion uses that precedent to define independent test oracles, conflicting evidence, variable binding and controls for blind repetition. Its sensory input is a stream of 64-bit receptor events. The implementation, dialect generator, mutable contacts, habitats, exercises and evaluation code live in this repository. [Learning compositional rules through program synthesis](https://cims.nyu.edu/~brenden/papers/2003.05562.pdf) · [Human few-shot instruction generalization](https://www.cs.princeton.edu/~bl8144/papers/LakeEtAl2019CogSci.pdf)

## What develops during a lifetime

- **Hearing.** A temporal envelope, optionally sensitive to changes during a crest, converts varying-duration samples into receptor events. Two confidence bits per receptor can integrate a burst. The matching radius, updates and release behavior depend on inherited bits. A second learned receptor bank matches events to prototypes. The readable port turns text into carriers and removes whitespace. A raw port accepts arbitrary 64-bit waveforms. Adjacent identical sounds with no physical boundary are indistinguishable to this ear.
- **Induction.** The creature retains whole paired sensory and motor episodes. Shorter experienced cues and differences between utterances suggest possible boundaries. Candidate productions bind up to two spans. A motor response can call a bound procedure repeatedly, reorder the calls, emit literals or echo a raw span. Support, variation, conflicts and compression feed a heritable acceptance table. An assimilating genotype can revise contradictory episodes and replace a superseded production. During conversation, four heritable bits control whether a repeated, novel, differently observed or contradicted demonstration triggers immediate reconsolidation. Consolidation is transactional at the circuit’s capacity.
- **Recursion and questions.** A separate inference step fits an observed base case and learns where to split a terminal’s motor fragment around a recursive invocation. Several legal splits can survive short demonstrations. A learner can request a demonstration on which those hypotheses predict different movements. Explicit data structures make it possible to test whether the acquired transitions agree with an independently generated teacher. A forward interpretation can emit a slice of a long recursive response. A bounded inverse search can describe an observed motor sequence and checks its answer by replay.
- **Sensing during action.** When similar cues have different demonstrated motor responses, the learner can fit two alternatives and retinal evidence for choosing between them. It retains the agreement of candidate dendrites over eight observed light channels. A call selects its branch when invoked and can suspend while a needed observation is missing. Re-entering the call after a movement reads the new view. The active score and its cursor survive saving and loading a brain.
- **Planning and speech.** Each creature’s learned acoustic forms connect to its own learned actuator names. Its intervention model predicts the possible next plot under each motor contact. A grounded score can be executed against that model, executed live against the room, or spoken on the wire. Replacing the physical room allows those three measurements to disagree.

A procedural organ occupies **15,368 bytes** in the saved animal, including bounded episode memory, motor calibration, a resumable stack and unused capacity. The greenhouse language occupied **168 bytes** of production cells and their wires. The conditional gate, auditory receptors, history and the animal’s earlier faculties occupy additional space. [Data layout](../include/axion/procedure.hpp) · [External teaching episodes](../examples/greenhouse-motions.lesson)

## What the animals learned

| Experiment | Result |
|---|---|
| 128 freshly reared dialects, nested commands | 4,096/4,096 exact probe responses. All 128 learned the generated grammar. |
| Return trips, four informative questions per learner | 4,096/4,096 exact return plans. Random questions with each learner’s matched budget gave 137/4,096. |
| Decisions after sensory changes | 131,072/131,072 exact paths. Holding the initial view gave 67,179/131,072. On 57,668 paths requiring a changed branch, the held-view control solved zero. |
| Inverse body control | 392,858/393,216 correct actuator choices. 123/128 animals were correct for every tested visual condition and desired movement. |
| Culture transmitted by an independently running teacher | All 32 pupils recovered the greenhouse teacher’s 11 productions, including agreement with its conditional behavior over all 256 retinal settings. |

The nested-command check compares total length and a sequence digest. It additionally checks every symbol for short outputs and sampled positions for long outputs. A separate structural comparison checks each generated grammar and recursive return state against the teacher. The sampled depth, 64-bit length, grammar class and stack bounds define the scope of these results. The inert cohort recovered zero of 64 generated grammars. [Newborn transfer](../journal/runs/00006397-imitate/observations.json.gz) · [Directed questions](../journal/runs/00006195-imitate/observations.json.gz) · [Matched random questions](../journal/runs/00006196-imitate/observations.json.gz)

## Acquiring a physical model and choosing an experiment

Each lantern room has six levers and six binary lamps. Its hidden wiring permutes the lights and applies reversible Boolean couplings. Every lever is a bijection on the 64 states. Consequently, any fixed sequence maps a uniform population back to a uniform population. A fixed sequence can satisfy a one-lamp goal from exactly 32 of 64 starts and a specified two-positive-lamp goal from exactly 16. A sensory procedure can make a different decision after seeing what a preceding move changed.

For the 24-room study, the fixtures and goal dimensions were drawn from a fixed seed. Half of the goals required one positive lamp and half required two. The teacher presented the same number of contacts per lever within each treatment. The active learner selected the initial lamp pattern for each experimental contact. Its genome assigns a four-bit priority to each possible number of unresolved next-state bits. Selection rewards accurate predictions of withheld physical transitions as well as the acoustic and motor tasks. The search for a goal uses only the learned model. An independent device-side apparatus then runs the spoken policy from all 64 starts.

| 24 new rooms | Correct withheld transitions | Physically achieved goals | Starts the model could guarantee |
|---|---:|---:|---:|
| 16 supplied contacts per lever | 5,256 / 6,912 | 1,470 / 1,536 | 1,468 |
| 16 contacts chosen with the original question priorities | 1,220 / 6,912 | 718 / 1,536 | 463 |
| 16 contacts chosen with selected question priorities | **6,912 / 6,912** | **1,532 / 1,536** | **1,532** |
| 32 supplied contacts per lever | 4,360 / 4,608 | 1,528 / 1,536 | 1,526 |

For the selected genotype at 16 contacts per lever, the blind action control reached **576/1,536**. The live controller reached every goal in 21 of 24 rooms. The gap between guaranteed and actual outcomes in other treatments includes successful actions for which the learner lacked complete evidence. The physical starts, actions, forecasts, goals and articulated solutions are retained in the data. [Selected questions](procedures/selected16.csv) · [Original priorities](procedures/inherited16.csv) · [Short sweep](procedures/sweep16.csv) · [Long sweep](procedures/sweep32.csv)

A second experiment bred four populations independently for 24 generations with 64 candidates. Each retained genotype received 12 contacts per lever in 256 new rooms per seed. Selection led to **317,839/319,488** correct withheld transitions and 951 completely settled, correct room models. Reproductive drift gave 169,520 correct, the original priorities gave 39,552 and the supplied sweep gave 199,484. [All selection seeds and controls](procedures/selection.csv)

## Exact composition and bounded search

For a learned score \(P\), a table gives its destination and motor count from each possible observed state. Calling \(P\) and then \(Q\) composes the destination functions and adds the counts along that path. A learned sensory operator selects its alternative at entry to the call. Successive calls can therefore make different decisions. CUDA threads evaluate all possible origins together in postorder. Unknown contacts propagate only along affected paths. An unrepresentable 64-bit count also makes that path indeterminate. A 42-fold doubling of a three-action score was checked against independent repeated squaring.

To invent a score, the learner extracts independently grounded action words, branching constructions and a sequencing construction from its acquired circuit. The search works backwards from a goal set. For each learned gesture and candidate suffix it computes the set of starting states that reach that suffix’s acceptable origins. Two warp ballots encode that entire set. Each retained set carries an acoustic witness, so the resulting procedure can be spoken and understood by another learner. Collisions in the bounded search retain a high-coverage witness and set the pruning flag. An exhaustive alternative checks short horizons. A goal already satisfied at all specified origins requires zero movements. [Compiler](../src/procedure_policy.cuh) · [CUDA search](../src/procedure_grounds.cuh)

```sh
./axion imitate --load examples/ritual-gardener.mind --lantern 509 --contacts 32 \
  --goal-states aaaaaaaaaaaaaaaa --horizon 4 --search exhaustive --verify-lantern 509
./axion imitate --load examples/lampwright.mind --forecast 'then mist again sense brush else plant' \
  --families 4096 --replicas 1048576 --episodes 64 --seed 725511
```

## Blackwell performance

The RTX PRO 2000 Blackwell Laptop GPU processed **67,108,864** three-action calls per timed pass. Each of 1,048,576 independent histories received 64 disturbances and carried its previous state into the next call. All final states, movement counts and trajectory digests matched the motor interpreter. The device exposed 32- and 72-byte compiled policies for the greenhouse and lantern examples. A warp can hold two transitions per lane, then shuffle the response for each resident’s current state.

| Deployment | Explicit interpreter | Dense projection | Compact projection |
|---|---:|---:|---:|
| Shared greenhouse score and model | 0.266 billion calls/s | 225.8 billion calls/s | 214.2 billion calls/s, warp, 32 bytes |
| Shared lantern score and model | 0.258 billion calls/s | 219.0 billion calls/s | 233.7 billion calls/s, warp, 72 bytes |
| 4,096 independently fitted lantern models, randomized per history | 0.222 billion calls/s | 11.49 billion calls/s | **10.56 billion calls/s, 311,264 bytes total including offsets** |

Each independent family in the throughput benchmark received all 384 lever-state contacts. Before timing, the physical oracle replayed the entire score from all **262,144 family–origin pairs**. Timings are five-pass CUDA-event medians after warm-up. They cover inference with a shared parsed command and independently changing histories. Training, parsing, compilation, physical audit, host transfers and rendering fall outside the timed interval. The explicit interpreter reads the model and emits each pulse. The compiled kernel evaluates its closed-loop consequences. External disturbances within a call require live interpretation or a new observation at the boundary. [Raw timings](procedures/throughput.csv)

## Life at the roost

Islands hold 32 separately reared animals. A scout can physically return from a trip and emit an acoustic description of its proprioceptive trajectory. Its neighbour hears the report, reconstructs a route and tries to fly it. A different, tide-dependent muscle mapping can produce an entirely different flight from the same desired motions. A spatial cellular automaton regrows fruit. Residents compete for pods, pay for movement and signaling, and reproduce in proportion to retained energy. A surviving adult can demonstrate its actual repertoire to the next generation. Lineages can invent a short call for a repeatedly experienced route.

Across 12 seasons of 256 residents, the open-wire population took **1,139,156 movements**, won **12,295 meals**, introduced **2,268** calls and learned **586** short calls from contemporaries. Muting the wire gave 10,911 meals and zero contemporary call acquisitions. Isolating newborns after the founder generation gave 4,658 meals. The fixed-gait treatment still won 12,261 meals in this climate. These single-seed interventions do not establish an ecological benefit for every available learning mechanism. The restored brood reproduces the subsequent population, terrain and culture byte for byte. [All seasons and interventions](procedures/roost.csv) · [Roost plate](../journal/runs/00006188-imitate/plate.webp) · [Actual sampled flights](../journal/runs/00006399-imitate/plate.webp)

```sh
./axion imitate --resume-roost examples/windward.roost --seasons 2 --save-roost descendants.roost
python3 tools/procedure-study.py --mind examples/lampwright.mind --contacts 16 --contact-policy active --output rooms.csv
python3 tools/procedure-study.py --evolution --rooms 256 --contacts 12 --output selection.csv
make NATIVE=1 CUDA_ARCH=sm_120 test-procedures
```

## Revising a body model after a storm

An unannounced wind change can alter both the actuator law and which lights predict it. New roost cohorts can replay recent sensory, motor and proprioceptive events, fit competing visual contacts and try a proposed model on subsequent movements. Both predictors see each new outcome after committing their predictions. The animal adopts a sufficiently better challenger under its inherited threshold. The competing 2-bit contact bank reuses 144 bytes of the juvenile practice tape. The animals pay for replays.

Four matched founder populations each supplied 256 residents per treatment. Before the storm, prospective admission achieved 98.7% of intended movements and immediate replacement achieved 79.9%. Across the last four days, prospective admission achieved 60.7%, fixed sensory attention with writable motor contacts achieved 31.9%, and completely frozen gait achieved 9.9%. The open groups won 10,143 meals. They still regularly missed their intended movements after the change. [Every storm and control window](procedures/storms.csv)

## Limits of the evidence

The ear receives synthetic carriers, supplied whole-turn boundaries and occasional external demonstrations. The inducer has a fixed repertoire of span and recursion hypotheses. The planner has an engineered search algorithm, a finite sensory state and a bounded beam. The GPU compiler, physical laws, memory capacities and reproduction machinery are supplied mechanisms. Evolution changes learning tables, evidence thresholds, connectivity, body control, signaling and life-history traits within those mechanisms. A creature can learn a procedure, invent a score, ask informative questions and transfer a skill in these worlds. General conversation, unrestricted open-ended evolution and human-level intelligence remain unestablished.
