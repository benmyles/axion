# Transfer, planning and sensory prediction

Axion's strongest result here is composition of separately learned skills. An animal can learn names from consequences, discover local transitions by moving through a world, and later follow an ordered pair of those names. A separate generative circuit can learn tool effects by observation and produce a complete intervention for a new goal.

The implementation is a hybrid. The simulator supplies event boundaries and the interaction protocol. Coincidence detection, an ordered goal queue, a graph search rule and candidate classes for visual transformations are architectural priors. Synaptic weights, associations, graph edges, pixel mappings and tool-specific transition bases are acquired during experience. The benchmarks measure those abilities in procedurally generated domains. A comparison to one published toddler task is included below. A developmental age for the system as a whole remains undetermined.

## Experimental basis

- In an infant speech study, familiarization with repetition patterns supported discrimination on new syllables. The pattern arena borrows the disjoint-alphabet test. Its receptor words, reward signal and binary choices are specific to Axion. [Marcus et al., original experiment](https://wmit-pages-prod.s3.amazonaws.com/wp-content/uploads/sites/316/2025/05/13123214/MarcusVijayanBandiRaoVishton1999science.pdf)
- Research with toddlers measured how unfamiliar words become associated with referents. A separate experiment tested toddlers' transfer of observed causal relations to novel objects. These studies motivated separating what the learner observes during practice from what it must do at test. [Novel word–referent mapping, 2026](https://doi.org/10.1111/desc.70272) · [Causal relational problem solving in toddlers, 2024](https://pubmed.ncbi.nlm.nih.gov/39340872/)
- A November 2026 human experiment found causal evidence for hippocampal involvement in learning relational structure and using it to infer unexperienced relationships. Axion preserves action-conditioned transitions and tests whether it can connect them into routes it was never taught. [Causal role of human hippocampus in relational structure learning and inference](https://www.nature.com/articles/s41562-026-02627-w)
- Relational abstraction models have shown how restricting a channel to relationships can support generalization across object identities. Research on human and machine instruction learning has tested novel combinations of familiar primitives. These results motivated the content-only ablation and the single-tool training split. [Relational abstraction, ICLR 2024](https://proceedings.iclr.cc/paper_files/paper/2024/file/6b070264f20d25fbd1488de8ea51575f-Paper-Conference.pdf) · [Lake and Baroni, 2023](https://www.nature.com/articles/s41586-023-06668-3)

## An observational developmental task

Walker and Gopnik's Experiment 2 tested 38 children aged 18–30 months. A toy activated for matching blocks in one condition and dissimilar blocks in the other. Each child saw four distinct demonstration pairs, each twice, before choosing one of two novel pairs. Fifteen of 19 children in each condition made the correct first choice. [Walker and Gopnik, primary report](https://escholarship.org/uc/item/68z4n01t)

The from-scratch digital protocol preserves the number of distinct demonstrations, repetitions, activation outcomes, novel test identities, randomized side, and first-choice scoring. The agent receives no motor reward during observation or the probe. It stores the percept it witnessed, including an explicit event for a completed silent interval. It evaluates each new tray by its predicted effect. The two counterbalanced conditions use copies of the same initial digital population. Each copy makes exactly one frozen test choice. The existing coincidence channel and attention rule were taken from the preceding transfer experiments.

| Toy condition | Published children | Axion, recurrent readout | Axion, identity memory | Axion, full |
| --- | ---: | ---: | ---: | ---: |
| Matching pair activates | 15/19 | 52.56% | 49.15% | 99.63% |
| Dissimilar pair activates | 15/19 | 47.58% | 49.54% | 99.68% |

Each digital condition has 4,096 learners. The physical study involved speech, actual blocks, a musical toy and simultaneous presentation on trays. The digital study uses segmented receptor words and scores two trays sequentially with the same reset state. Its goal cue and relation-comparison channel are built in. The percentages describe performance on those respective protocols. They do not establish an age-equivalent intelligence score.

```sh
./axion detector --backend gpu --brains 4096 --seed 3217 --lesson-seed 641279 --world-seed 932141
```

## Few-shot experiment

Each animal gets its own wiring and lesson-specific codebook. Three brief receptor volleys present each item. A majority circuit rejects transient receptor noise. The resulting 64-bit assembly contains the 32 relay bits and 32 threshold responses to a fixed projection. The projection reuses the local recurrent weight words, which stay stable within a lifetime.

An association stores three context assemblies, two option assemblies and three pairwise coincidence scores. Following scalar feedback, the two options are recorded in preference order. With two exhaustive actions and deterministic reward, negative feedback identifies the preferred alternative. Four attention weights track the predictive value of the individual context positions and the joint relationship pattern. Similarity uses two XORs and two population counts. A centered vote over the 32 most recent experiences contributes to action value alongside the recurrent readout. None of the heads receives the hidden mapping or the expected choice during inference.

The table gives percent success on 131,072 frozen probes per cell. Each condition used 256 independently initialized animals, eight different lessons per animal, and 64 probe trials per lesson. Conditions shared wiring, worlds and nuisance draws. Standard errors in the full condition range from 0.04 to 0.11 percentage points. The cortical control runs the original readout and plasticity on the same presented stream. Content enables identity recall and one-step transitions. Full adds the relation channel and multistep propagation.

| Probe | Rewarded examples or moves | Cortical | Content | Full | Full with shuffled feedback |
| --- | ---: | ---: | ---: | ---: | ---: |
| Eight arbitrary names, new foils | 8 | 49.81 | 98.48 | 98.28 | 50.14 |
| Sorting, held-out attribute pairs | 8 | 49.90 | 99.89 | 99.89 | 49.95 |
| Repetition, disjoint alphabet | 8 | 49.86 | 49.10 | 98.84 | 49.21 |
| Distant goal, 16-room map | 32 | 25.32 | 45.51 | 99.39 | 99.37 |
| Ordered two-name errand | 48 | 7.25 | 25.72 | 98.30 | 36.68 |

The naming lesson presents each name once per balanced block. Sorting uses eight of the sixteen attribute pairs for support and the complementary eight for probes. The sequence task assigns the four shapes ABC, AAB, ABA and ABB to two categories according to the lesson. In seminar, the generator interleaves naming, sorting and sequence examples on the same five input frames. The learner receives no subject label. After 48 interleaved training examples, a development run scored 88.81% overall, with 91.26% on naming, 95.05% on sorting and 80.98% on pattern probes.

```sh
./axion transfer --backend gpu --seed 83 --lesson-seed 931155 --world-seed 234116 \
    --brains 256 --lessons 8 --episodes 48 --eval-episodes 64 --task errand
```

Select the other subjects with `--task`. Repeat with `--recall cortex`, `--recall content`, `--feedback shuffled`, or `--task errand --swap-words` to reproduce the controls. `transfer` restores the same starting circuit for each new lesson and probes after increasing experience budgets. `train` and checkpoint continuation preserve experience between runs.

An evolutionary control evaluated whether selection on six-example seminar adaptation improved fresh lineages. After 24 generations with eight lesson draws per selection round, success on a separate 32-lesson audit was 63.30%, against 62.96% for the founders. A single lesson draw per selection round gave 63.20%. This small difference limits the evidence for evolution on this particular mixed task. Most of the measured transfer comes from lifetime experience and the representation.

## Exploration and command following

Sixteen distinct landmarks label an unfamiliar directed graph. Each room offers two actions. A hidden cycle guarantees reachability and an additional door supplies a shortcut. The learner sees a landmark through noisy receptors. Observing a move writes a directed, motor-gated connection between two recognized assemblies. The bank can recruit 32 places. Its learned topology costs 516 bytes per animal. Once a goal has been cued, a backward wave through the learned edges identifies actions leading toward it. A CUDA warp expands every known predecessor with one ballot.

The standard route lessons offer one door choice at a time, with balanced initial rooms. Probes start three to five edges from the target and permit six moves. The environment computes this distance to generate trials. The planner sees its own learned graph. Shuffling scalar rewards preserves route learning because destinations remain observable.

A second protocol allows continuous exploration. The animal chooses an untried door when possible and can use its learned map to seek another room with an unexplored exit. After **one uninterrupted 32-move outing**, goal-reaching success was **94.67%**. Uniformly random movement under the same budget gave **77.70%**. At 64 moves, the scores were 99.65% and 92.42%. Each outing starts in one room and runs for its full move budget.

```sh
./axion transfer --backend gpu --task routes --survey 32 --explore curious --episodes 1 \
    --brains 256 --lessons 8 --eval-episodes 64 --seed 83 --lesson-seed 931155 --world-seed 234116
```

Errand practice interleaves 16 individual name bindings and 32 single-door experiences. A probe presents two names once, then a live starting landmark. Content-addressed recall supplies destination assemblies. The controller tracks progress by matching the observed place against its pending goal. The simulator scores the physical visits separately. Swapping the two input names while leaving the requested scoring order fixed reduced success from **98.30% to 15.24%**. Visiting a destination incidentally during a longer trip can still yield a win under this control.

## Retaining old experience

The short-term bank occupies 1,432 bytes per animal. Optional longer-term storage consolidates evicted cue–outcome pairs. Each lookup examines at most eight records in two four-way sets. Distinct masked keys provide partial tolerance to noise. A record costs 12 bytes and stores a cue, an outcome and a bounded support count. The table scales with a configured capacity. On open-ended cued recall, old and recent memories compete by match quality.

| Naming vocabulary | Examples per name | Extra archive memory | Frozen success | Recent bank alone |
| ---: | ---: | ---: | ---: | ---: |
| 512 | 1 | 24 KiB | 94.57% | 53.32% |
| 4,096 | 1 | 192 KiB | 90.02% | 50.42% |

These were two-choice recognition probes with new foils, using 128 animals and 1,024 probes per animal. Each synthetic name is an arbitrary receptor pattern. The animal receives feedback once per name. The archive stores several candidate cue associations per exposure because the informative temporal position is hidden.

```sh
./axion train --backend gpu --task naming --seed 143 --lesson-seed 3931 --vocabulary 4096 \
    --archive 16384 --brains 128 --episodes 4096 --eval-episodes 1024
```

The two-timescale update also matters for retention across domains. Training the shipped forager on 16 landmark examples and one 64-move survey at the original shared-synapse learning rate reduced its mixed-foraging score from 91.89% to 44.48%. Dividing shared updates by 64 when the fast pathway was active preserved 92.46% foraging and 99.95% errand success. Both scores used 4,096 frozen trials. `--consolidation 1` reproduces the single-rate control. The original foraging learning rule still operates at its original rate. A second experiment started from 128 other animals (seed 23) trained on 4,096 mixed trials each. Their original score was 93.15%. After the same cross-domain experience, foraging and errand success were 56.03% and 97.77% at the single rate, and 93.27% and 97.91% at the slower shared rate. Each probe had 32,768 outcomes.

## Generating and constructing pictures

The workshop adds a 32-neuron binary output population. Each tool has an observed cue. During practice the learner sees an input picture and the result of a single application. During a probe the machine must emit all 32 pixels after a novel sequence. A nonlinear partition of the 32-bit input space makes the generated practice and probe inputs disjoint.

Two complementary learners share this population. In each local dendritic branch, two-bit mismatch counters select among signed afferents. The branches support individual input bits and two fixed neighbor interactions. The counts are bit-sliced so a machine word updates 32 synapses together. A second learner accumulates independent input–output differences as a reduced binary basis. A previously spanned input with a contradictory response restarts the basis. Two corroborating observations with inputs already in the learned span are required before the basis can drive output. Four tool circuits and their data occupy 7,240 bytes.

For an inverse request, the only observations are the target and tool cues. Confirmed full-rank input bases allow the machine to compose the learned affine maps and solve for an intervention by binary elimination. Singular output maps can have several valid inputs. The original nonlinear planner searched for a better input using its own forward hypotheses. Each warp evaluates all 32 single-pixel interventions together. A five-shuffle transpose yields one predicted picture per lane. Three starts and a bounded search budget limit the cost. The environment executes the proposed input to score it.

| Hidden tool family | Exact five-step prediction | Exact inverse goal construction |
| --- | ---: | ---: |
| Pixel permutations, inversion, sampled local AND or XOR | 100.00% | 72.48% |
| Dense affine dependencies across pixels | 100.00% | 100.00% |

In these runs, 512 new animals each saw 96 demonstrations, 48 per primitive tool. Forward success counted exact 32-bit pictures. Inverse success counted exact outcomes after the proposed setting went through the real five-tool sequence. Each inverse row used 65,536 probes with frozen state. Dense effects with shuffled observations gave 0.00% exact forward matches and approximately 50% pixel agreement. With ordinary passive observation, the separate `nonlinear` family combines nonlocal quadratic and cubic interactions outside the supplied branches and the affine model. It yielded 0.00% exact forward matches with 55.59% of pixels correct, and 0.00% exact inverse outcomes with 57.75% of pixels correct. This measures a limit of the passive predictors. The controlled-experiment result below uses an additional learned representation. The tool algebra, search operator and event boundaries are supplied by the architecture.

```sh
./axion train --backend gpu --task workshop --effects dense --seed 139 --lesson-seed 529489 \
    --world-seed 151153 --chain 5 --brains 512 --episodes 96 --eval-episodes 128 --save tools.axn
./axion eval --backend gpu --load tools.axn --task invention --episodes 128
```

## Learning a mechanism by intervention

The optional experimental memory learns which input spikes can affect each output. A pair of observations in which only one receptor changed can establish that receptor as a parent of any output that changed. Each output with at most eight discovered parents learns a table of responses indexed only by those parents. Discovering a new parent clears the old table for that output. Predictions require earlier observations of the local configuration and corroboration on repeated configurations. The optional state occupies 8,880 bytes per animal across four tool slots.

`--experiments random` and `paired` enable the identical learner. The former presents independent pictures. The latter holds the tool and all but one input fixed across consecutive observations and sweeps across the input dimensions. `curious` additionally lets the agent choose among eight admissible backgrounds on the basis of the number of local outcomes it has yet to observe. Candidate selection has no access to the physical pixel functions. A loaded agent can continue using the causal tables it acquired earlier. Both members of every experiment belong to the practice partition. The query partition is disjoint and shared by all three methods.

The challenge was the previously unsolved nonlocal nonlinear family. With 8,192 individual observations per animal and identical observation budgets, independent pictures gave **0/65,536** exact five-tool predictions, with 55.67% pixel agreement. Controlled changes gave **65,536/65,536** exact pictures. CPU tests separately verify the discovered afferents against the actual seven contributing receptors for each output of the nonlinear tool. These results are for deterministic mechanisms with reliable observations.

| Individual practice observations | Controlled changes, random background | Controlled changes, uncertainty-selected background |
| ---: | ---: | ---: |
| 2,048 | 5.88% | 22.78% |
| 4,096 | 93.99% | 99.23% |
| 8,192 | 100.00% | 100.00% |

Each table cell is exact-image accuracy over 32,768 frozen five-tool probes from 128 animals and four new lessons per animal. A separate 16,384-probe inverse test asked the animal to produce an input that would yield an exact goal. The original local search succeeded on **0.41%** after 8,192 controlled observations, with 84.52% pixel agreement.

### Planning with learned intermediate constraints

The CUDA planner turns fully specified learned responses into local constraints. A warp carries on/off evidence at every intermediate picture. Each output cell filters the rows of its learned table that match the evidence and votes on deductions. Contradictions retract implications of a previous assignment. A decision stack is distributed across warp lanes. This search sees the desired picture, tool cues and learned memory. The world executes and scores the proposed input. The design draws on compact-table constraint propagation. [Demeulenaere et al., 2016](https://arxiv.org/abs/1604.06641)

| Search nodes per goal | Original 256 worlds | Independently trained 256 worlds |
| ---: | ---: | ---: |
| 0 (original local search) | 0.41% | 0.32% |
| 1,024 | 84.13% | 83.81% |
| 4,096 | 98.53% | 98.52% |
| 16,384 | 99.98% | 99.98% |

Each column contains 16,384 frozen five-step goals. All rows within a column use identical learned weights, test goals and physical scoring. The second column of worlds used wiring seed 91843, lesson seed 714701 and world seed 427123. The runtime option `--plan-nodes` defaults to 4,096. An incomplete gate or exhausted budget falls back to local search. Affine composition retains its direct algebraic solver. The earlier local-branch results above can be reproduced with `--plan-nodes 0`.

```sh
./axion train --backend gpu --task workshop --effects nonlinear --experiments paired \
    --seed 379 --lesson-seed 17401 --world-seed 71231 --chain 5 --brains 256 \
    --episodes 8192 --eval-episodes 256 --save investigated.axn
./axion eval --backend gpu --load investigated.axn --task invention --episodes 64
./axion transfer --backend gpu --task workshop --effects nonlinear --experiments curious \
    --seed 379 --lesson-seed 17401 --world-seed 71231 --chain 5 --brains 128 \
    --lessons 4 --episodes 8192 --eval-episodes 64
```

## The supplied student

This is one brain selected by its original seed, with the same checkpoint carried through every stage. No task-specific checkpoints are loaded between probes. The external lesson teaches two named pixel operations via ordinary standard input. The complete file is **60,468 bytes**.

```sh
./axion train --load examples/forager.axn --task landmarks --episodes 16 --save student.axn
./axion train --load student.axn --task routes --survey 64 --episodes 1 --save student.axn
./axion train --load student.axn --task naming --vocabulary 512 --archive 4096 --episodes 512 --save student.axn
./axion train --load student.axn --task landmarks --episodes 16 --save student.axn
./axion train --load student.axn --task workshop --effects dense --chain 5 --episodes 96 --save student.axn
./axion tutor --load student.axn --save student.axn < examples/pixels.lesson
```

| Frozen probe from this checkpoint | Success on 4,096 trials |
| --- | ---: |
| Original mixed foraging | 89.99% |
| 512-name recognition | 94.92% |
| Landmark-name recognition | 99.88% |
| Ordered errands | 99.54% |
| Five-tool forward prediction | 100.00% |
| Five-tool goal construction | 100.00% |

The 512 new names interfered with the previous bindings. Before the 16 landmark review trials, errand success was 87.35%. The transfer measurements above quantify across-lesson variation separately from this particular animal.

## Limits of the comparison

The core receives 32 receptor bits. Tokens and events in these experiments are segmented. A “spoken” instruction consists of coded pulses. There is no acoustic model. The world graph has 16 rooms and deterministic doors. The controller is given the requirement to follow a sequence of goals. The setting does not test whether an agent invents that procedure, acquires grammar from continuous speech, develops social understanding, or learns open-ended physics. Distinct assay protocols select the relevant subsystem. Seminar specifically tests task selection within a common stream. More independent behavioral evidence would be needed to establish a developmental age equivalence.

## Blackwell execution and verification

Native build mode assembles each PTX family with the installed NVIDIA driver during the build. At runtime the process loads only the GPU programs it uses. The generative population maps one 32-pixel canvas to a warp regardless of the width of the recurrent circuit. Its output events are counted separately from recurrent neuron-ticks. Ordinary foraging allocates none of the new memory banks. Each animal acquires an episodic bank, a place bank, a tool bank, or a larger association table as the experience requires.

Measurements below used the RTX PRO 2000 Blackwell laptop GPU, driver 615.71.09 and native SM 12.0 code. The timer covers the resident simulation, decisions, procedural environment and counters. Wall time includes synchronization and copying counters. Initial model transfers, compilation and benchmark restores occur outside the timed interval. The laptop managed its own clocks.

| Workload | Frozen inference | Online learning |
| --- | ---: | ---: |
| Trail, 4,096 animals × 128 neurons, 128 episodes | 123.67 billion neuron-ticks/s | 80.45 billion neuron-ticks/s |
| Trail, one 1,048,576-neuron animal, 16 episodes | 40.76 billion neuron-ticks/s | 27.00 billion neuron-ticks/s |
| Pattern, 4,096 animals × 128 neurons, 64 episodes, full recent memory | 60.56 million trials/s | 43.85 million trials/s |
| Dense tools, 512 animals, 128 episodes | 61.66 million five-tool inverse probes/s | 213.98 million single-tool demonstrations/s |

The trail and pattern results are medians of nine runs. The drawing result is the median of seven. The two drawing columns execute different workloads. Five-tool inverse inference took 1.063 ms per run on the device and 1.169 ms wall time. Sharing instruction cues by warp shuffle, packing the search hypotheses and computing each physical pixel in its own lane reduced this kernel from 216 to 64 local bytes per thread. A single pattern probe updates the complete 128-neuron recurrent circuit for 15 ticks. Keeping context slots in registers reduced local storage in its inference kernel from 208 to 80 bytes per thread.

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120 test-gpu
./axion bench --backend gpu --task trail --brains 4096 --neurons 128 --episodes 128 --repeats 9
./axion bench --backend gpu --task trail --brains 1 --neurons 1048576 --episodes 16 --repeats 9
./axion train --backend gpu --task pattern --brains 4096 --episodes 32 --save patterns.axn
./axion bench --backend gpu --load patterns.axn --episodes 64 --repeats 9
```

Tests compare CPU and GPU integer counters, chosen outcomes, generated images, every recurrent weight and the complete learned stores. They cover all supported recurrent widths, old and new checkpoint loading, frozen probes, archive resizing, independent training of components, and external teacher input. The supplied student's SHA-256 is `7a4a42f3d9bb74deebe31b27858f3b7fc1ffa22a8174eaf8fc9fe7627859f76f`. Replaying the training commands above reproduced it byte for byte.
