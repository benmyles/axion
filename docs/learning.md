# Evolving a rule for learning from consequences

These experiments characterize the original receptor–motor learner. A subsequent relational circuit addresses the joint-property failure measured here and has separate population and ecological controls. [Relational learning in thickets](relations.md)

Axion can breed a spiking circuit that learns previously unknown motor contingencies during its own lifetime. The strongest released population learned **32 unfamiliar four-way associations** with 98.52% accuracy and reacquired 97.13% of the changed associations after reversal. It also learned which property of an object mattered and transferred the resulting behavior to new combinations of familiar features. The supplied checkpoints represent two different experimental treatments.

| File | Origin | What it contains |
| --- | --- | --- |
| [learner.eco](../examples/learner.eco) | Founders selected for within-lifetime learning | A fresh 16 × 16 habitat of untrained descendants carrying inherited learning programs |
| [delayed.eco](../examples/delayed.eco) | Local survival and reproduction with delayed digestion | Untrained descendants transplanted from a naturally evolved population |
| [learner.mind](../examples/learner.mind) | One descendant from the first habitat, trained on a user-level lesson | A 376-byte resumable individual, including learned connections, recurrent activity, pending feedback and clock |

## Teach the individual

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion agent --from examples/learner.eco --lesson examples/grasps.lesson --save pupil.mind
printf 'probe 0x00040001 0x01070000 0x0b 0x0a\n' | ./axion agent --load pupil.mind
./axion agent --load pupil.mind --inspect
```

The example delivered 64 trial outcomes and probed eight previously unseen shape–texture combinations. The first living donor's descendant matched **54/64** training outcomes and **8/8** probes. Each of the 32 receptor channels can be driven independently. These input patterns are supplied directly to the same CUDA neural interpreter and mutable synapses used in the habitat. The actor needs 352 bytes of device-resident brain state at this capacity. The file adds a header and checksum. Driver, compiled code and input/output buffers have additional costs.

| Event | Effect |
| --- | --- |
| `trial RAW FIELD MASK VALUE` | The creature acts on the two input words. The environment then tests `(effector & MASK) == VALUE` and schedules a positive or negative metabolic signal. |
| `probe RAW FIELD [MASK VALUE]` | Report what the creature would do next. A copy can process pending feedback. The original state, clock and queue are preserved. |
| `step RAW FIELD SIGNAL` | Supply an external positive, negative or absent signal for this tick, replacing any scheduled signal due now. The new action is returned to the caller. Signals are `+`, `-` and `0`. |

`RAW` is the binary receptor population. `FIELD` supplies other inputs to the spiking cortex. File and pipe input runs in batches of up to 256 events per CUDA launch. A terminal flushes each decision for closed-loop interaction. A saved agent resumes across batches and processes. `--from` starts a new lifetime with an occupied site's genome. It verifies the source habitat with a bounded read buffer. `--load` restores an individual's acquired experience. [Example sensory stream](../examples/grasps.lesson)

## What evolution can change

A grove creature has eight motor populations and 32 receptor channels. Connections from receptors to the motor populations add 256 plastic contacts. Each motor population stores all its one-bit contacts in a single 32-bit word. A motor command produces a delayed internal copy of which population acted. The preceding receptor spikes, that motor discharge and the recent change in bodily energy are available at each contact.

For one-bit contacts the genotype specifies the entire Boolean transition

```text
next weight = T[old weight + 2·earlier receptor + 4·matching motor + 8·positive outcome]
```

The metabolic and motor bits select one four-entry slice of the table for a whole motor population. CUDA evaluates this slice on 32 weights at once. Developmental receptor wiring, initial state and the gain of this population in the motor vote are also heritable. De novo founders start with table `0xaaaa`, which copies the old weight for every possible input. Point mutation can change every one of its 16 entries.

With `--synapse-bits 2`, the extra state selects a second table for the primary weight. A further 32-entry table writes the extra state. Both outputs read the same old state. A heritable readout can express different levels of influence. The two-bit variant costs eight extra genome bytes and 32 extra lifetime bytes per site. Both states initially copy themselves. The fixed parts include the instruction alphabet, the peripheral inputs, one to two bits per contact, the available actuators and the physical bookkeeping. The sequence of synaptic changes is inherited as an executable rule.

The supplied individual's primary tables are `0xa22e` and `0x2800` and its second-state table is `0xbfff00c0`. From its developmental state, three of the four possible contact states are reachable. State 1 contributes to the motor vote. States 0 and 2 are silent. When a receptor and its motor are both active and the outcome is negative, all three states enter state 2. A different motor's failure with the same receptor turns state 0 on but leaves state 2 silent. A subsequent success of the original motor can release state 2 back to state 0. The circuit has thus evolved two behaviorally distinct ways for a connection to be silent. The complete table can be inspected in the executable.

## Shared ecological selection

In a grove, attempting to handle a pod exposes the next variety. Four physical grip orientations can open pods. The relation between surface and grip depends on the territory and season. Each group of four varieties has a hidden permutation of the grips. In a compound grove each object also has a second visible attribute. The property controlling the opening can change with the season. The animals see receptor activity and bodily consequences. They can also move, use caches, exchange energy, emit chemicals and divide under the same conditions as in the meadow.

With `--digestion N`, an optional eight-tick sensorimotor history becomes available. An independent allele selects a conduction delay from one through eight. Food stays in a bodily reservoir before assimilation. Bitter outcomes also arrive at the physical delay. Unassimilated food in a dying body returns to vegetation up to its capacity, with overflow counted as expenditure. Every census includes outstanding calories in energy conservation.

```sh
./axion life --habitat compound --side 64 --seed 27191 --steps 16384 --save grove.eco
./axion life --load grove.eco --steps 0 --problem dimensions --conditioning 256 --dissect
./axion life --habitat grove --digestion 4 --side 64 --seed 8461 --steps 32768 --save slow.eco
./axion life --load slow.eco --steps 0 --probe-delay 4 --conditioning 128 --dissect
```

Eight independent populations per treatment evolved through local births and deaths. The ordinary and compound populations ran for 16,384 ticks. The delayed populations ran for 32,768. Assays started each tested lifetime from its inherited program in a new hidden world. The following percentages give the population means across training seeds. The first two rows tested four values. The third used four intervening decision intervals from action to observed outcome. [Population-level results, controls and allele interventions](learning/field.csv)

| Ecology and test | Naive | After acquisition | After erasing acquired contacts | Changed answers after retraining |
| --- | ---: | ---: | ---: | ---: |
| One visible property | 18.6% | 45.1% | 18.5% | 39.4% |
| Two visible properties, held-out combinations | 23.4% | 52.1% | 23.4% | 49.3% |
| Delayed outcome, one property | 19.8% | 40.7% | 20.0% | 32.0% |

Some physical phenotypes lost one or more feeding actuators. The `reachable` count measures the maximum allowed by each donor's anatomy. For the delayed donor population with training seed 8461, a fresh panel reached 72.0% after practice. Forcing its neural conduction time to one tick reduced it to 20.7%. A different delayed population, seed 2191, stayed near 21% at every imposed conduction time. Selection produced a useful four-tick mechanism in some lineages and failed in others.

Four of eight independent compound habitats had the same most frequent primary allele, `0x822e`. The `--dissect` assay restores individual truth-table entries to their ancestral value and reruns identical lifetimes. In reciprocal field competitions with further mutation disabled, descendants with mutable receptor–motor contacts averaged **99.83%** of survivors in ordinary groves and **99.93%** in compound groves against relatives with frozen contacts. Each comparison used eight donor populations, three new habitats per donor and both assignments of the founder locations. Other somatic plasticity remained active in both cohorts. [Initial and final counts for all 96 competitions](learning/contests.csv)

## How the blind test works

- **Grips.** Every test fruit receives a private four-way assignment. Four otherwise identical lifetimes cycle all possible orientations, so an uninformed initial choice can succeed at most 25% of the time. After acquisition, half the fruit types silently change to one of their three other grips. The rest retain their old effects.
- **Dimensions.** The objects combine two distinct sensory channels, one per property. Each property has up to 16 possible values. Practice uses half the diagonals of the Cartesian product. The other half is held out. Every value occurs in both partitions. Initially only one property determines the correct actuator. The other becomes relevant after a hidden switch. Each audited set contains two quartets with identical donor, sensory stream, action tie-breaks and practice order. The two quartets differ in which property has consequences. The agent receives no change cue.
- **Independent probes.** Every query starts from a copy of the same acquired state after three uninformative neural intervals. Probe feedback never becomes a training example. `changed_items` counts queries whose correct physical response changed. `stable_items` counts those whose answer stayed the same. `after_one` follows one more *encounter per variety*.
- **Controls.** Yoking delivers another independent lifetime's sequence of positive and negative signals, with exactly the same population total. Scrambling the motor discharge disrupts assignment to the action actually taken. Freezing contacts keeps the spiking cortex active. Erasure replaces the acquired contact states at test while preserving the rest of the brain. A two-bit circuit can also be tested with only its second state frozen from birth.

## Selecting for transferable learning

The optional nursery is deliberate artificial selection before a habitat starts. In each generation, each candidate's newly developed clones face eight fresh lifetimes on eight individual cues and eight on four values per property. The number of encounters per variety varies from four to eleven. Somatic and synaptic changes stay inside each lifetime. Tournaments of eight parents select inherited programs using acquisition and post-change accuracy on both tasks. Seven eighths of tournaments prioritize the worst of the four scores, with total score breaking ties. The rest prioritize one of the scores. One eighth of offspring copy their parent exactly, and the others use the ordinary mutation machinery. Drift controls sample parents independently of their performance.

Twelve candidate populations used six independent seeds at each synaptic capacity, 1,024 genomes and 384 generations. Validation used seed 923543. We selected the population with the best minimum across four validation scores covering acquisition and recovery on 32 grips and on new combinations of 16 values per property. Two-bit seed 1277 won. The checkpoint contains fresh descendants sampled into a new habitat at seed 48611. The four final audit seeds were held back from selection. The paired-property audit also tests both possible initial relevant dimensions. [All candidate and drift-control results](learning/breeding.csv)

```sh
./axion life --habitat compound --synapse-bits 2 --side 32 --seed 1277 --nursery 384 --steps 0 --save founders.eco
./axion life --habitat compound --synapse-bits 2 --side 16 --density 100 --seed 48611 --founders founders.eco --steps 0 --save learner.eco
./axion life --load learner.eco --steps 0 --problem dimensions --varieties 16 --exposures 16 --conditioning 512 --probe-seed 115889
```

**Selected population, independent audit.** Grips used four times the number of varieties encountered during nursery selection. The second test contained 256 possible objects. The following results are percentages across four audit seeds with equal numbers of rule sets. Sixteen encounters per value means 512 training choices per phase for grips, and 256 for the two-property test. Recovery is scored on changed answers only. [Raw acquisition curves and controls](learning/audit.csv)

| Encounters per value | 32 grips, acquired | 32 grips, recovered | Novel pairs, acquired | Novel pairs, recovered |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 69.9% | 59.1% | 48.5% | 49.1% |
| 4 | 95.0% | 93.1% | 78.9% | 69.8% |
| 8 | 98.4% | 96.9% | 96.6% | 93.5% |
| 16 | **98.5%** | **97.1%** | **98.5%** | **97.9%** |
| 32 | 98.6% | 97.2% | 98.5% | 98.1% |

At 16 encounters per value, the mirrored feature-switch audit recorded 2,065,059 correct choices on 2,097,152 unseen-pair probes. It recovered 1,539,579 of 1,573,136 changed answers. Erasing acquired contacts gave 25% on both tasks. Yoked feedback also gave about 25%. Freezing only the second synaptic state reduced acquisition on held-out pairs to about 52%. The strongest individual population used two-bit contacts. Across independent seeds, increasing synaptic capacity gave mixed results. Several training runs plateaued far below the released population. When its descendants subsequently evolved for 16,384 ticks in four ordinary compound habitats, their mean performance on fresh 16-value held-out pairs fell to about 59%. Selection in these habitats values reproduction under their local conditions. [Post-release measurements](learning/limits.csv)

## A relationship it still fails to learn

In `--problem composition`, both properties have consequences. Each property value contributes a hidden quantity, and their sum modulo four sets the grip. The same disjoint train/test partition is used. If the exact physical answers for the training pairs were known, the connected training graph would determine the two sets of contributions up to one arbitrary offset. The animal has to discover the consequences by acting.

The released population achieved **about 25%** on new pairs even after 64 encounters per property value. Six further two-bit nurseries selected explicitly for this problem also averaged about 25% on unseen 16-value combinations. The feature-switch result and the joint-effect failure delimit the kind of transfer demonstrated here. The experiments supply discrete sensory attributes, four grips and controlled outcome timing. Language, natural perception and a developmental comparison with children remain untested. [Joint-effect audit](learning/audit.csv) · [Explicit selection for the failed task](learning/limits.csv)

## GPU cost and reproduction

The spiking tissue, contact tables, environment and breeding kernels are written here in freestanding CUDA C++. The CUDA driver executes the kernels directly. One-bit groves add 36 bytes of dendritic state and one pod byte per site, plus a per-neighborhood harvest ledger. Delayed credit adds 24 bytes per site. Four-state synapses add 40 bytes per site including their genes. Graph batches can share 32 ticks of deterministic chemistry across each 16 × 16 territory at a cost of one byte per site for habitats at least 16 cells wide.

With `--schedule occupied`, one warp ballots 32 adjacent habitat sites and uses PTX `fns` to assign only the occupied ones to its tissue subgroups. This costs zero global bytes for a work queue. In seven alternating Blackwell runs on a 256 × 256 delayed habitat, 128 ticks took 21.426 ms with dense scheduling and 15.830 ms with occupied scheduling, with identical reported state. [Paired hardware measurements](learning/gpu.csv) At 64 × 64, dense scheduling won. At the maximum tissue width, dense scheduling also won on a nearly full 256 × 256 habitat. Automatic selection uses the compact mapping for larger groves with smaller tissue, and both overrides are available. [NVIDIA PTX instruction semantics](https://docs.nvidia.com/cuda/parallel-thread-execution/#integer-arithmetic-instructions-fns)

A separate 1,024 × 1,024 compound habitat seeded from naturally evolved stock reached **394,966 living creatures at tick 64** and had 332,938 at tick 256. It executed 110,600,893,952 active somatic cell updates through tick 256 using **354,549,988 bytes of explicit device buffers**. Each animal had a capacity of 256 soma cells and 256 plastic receptor–motor contacts. The count of somatic updates excludes extra dendritic operations, which are included in the runtime. Driver and module overhead are additional.

All raw tables can be regenerated from the repository root. The Python script launches the native CUDA executable and formats its measurements. The checks include conservation with delayed reservoirs, graph and direct equivalence, both scheduling layouts, read-only probes, checkpoint continuation, yoking, delayed credit, novel pairs and external actor save/resume across the batch boundary.

```sh
for study in field contests breeding audit limits; do
    ./tools/conditioning-study.py "$study" > "$study.csv"
done
make -j NATIVE=1 CUDA_ARCH=sm_120 test-gpu
```

## Research that informed the tests

- A November 2026 rat experiment manipulated previously learned action–outcome contingencies by delivering an outcome independently of the action. This motivated checking whether a learner needs contingent feedback even when the overall outcome count is maintained. [Original experiment](https://pubmed.ncbi.nlm.nih.gov/43014315/)
- A 2026 macaque study distinguished exploratory and exploitative attention when the reward-relevant visual feature changed. It motivated measuring acquisition, interference and switching on novel objects separately. Axion receives encoded receptor events and has no gaze apparatus. [Wen et al., iScience](https://pubmed.ncbi.nlm.nih.gov/41847109/)
- A timing experiment showed a limited window in which dopamine influenced structural plasticity following synaptic stimulation. The programmable conduction path asks whether selection can identify an appropriate credit horizon from delayed consequences. [Yagishita et al., Science](https://pubmed.ncbi.nlm.nih.gov/25258080/)
- Research on discovering reinforcement-learning rules across many agent lifetimes motivated comparing rule discovery under deliberate cross-task selection with ordinary ecological survival. The two Axion treatments are measured separately. [Discovering state-of-the-art reinforcement learning algorithms, Nature](https://www.nature.com/articles/s41586-025-09761-x)
