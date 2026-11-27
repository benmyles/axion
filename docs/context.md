# Learning when an old world returns

An animal eats by choosing a physical grip. The same object can require a different grip when local conditions change. An old skill becomes valuable again when those conditions return. Axion tests whether feedback about one object can recover appropriate actions on other objects in the same lifetime.

## Why build this circuit?

Human motor-learning experiments found that a reminder can change which memory people express and change what they learn from the next trial. Recordings during human rule switching also distinguish neural activity associated with a persistent task context from responses to individual trials. These observations motivated testing memory retrieval before an animal has a chance to practice the test items again. [Heald, Lengyel and Wolpert, Nature](https://pubmed.ncbi.nlm.nih.gov/34819674/) · [Neural dynamics underlying minute-timescale persistent behaviour in the human brain, Nature Human Behaviour, 2026](https://www.nature.com/articles/s41562-026-02537-x)

In macaques, sensory and motor representations recur across tasks that share parts of their structure. That suggested a second experiment. An animal that already recognizes an object's functional class might learn a new motor convention from a few members of that class. Our computational interpretation uses one shared spiking circuit whose activity can teach other contacts. The actual routing and local updates are heritable. [Building compositional tasks with shared neural subspaces, Nature](https://pubmed.ncbi.nlm.nih.gov/41299181/)

Cellular automata supply both local interactions and a cheap parallel schedule. Other researchers have observed sensorimotor capabilities emerging in Lenia under diversity search. In Axion a partitioned cellular automaton runs the habitat. Plants, bodies and neural contacts carry information across updates. Measuring animals after they move, breed and die gives a separate ecological test of what the learning circuit provides. [Sensorimotor agency in cellular automata](https://pmc.ncbi.nlm.nih.gov/articles/PMC12577699/) · [Axion's ecological substrate](ecology.md)

## What an animal inherits

Four competing groups can bind physical receptors, spikes returning from the animal's learned recurrent circuit, motor discharges and ambient cues. A group's membrane charge integrates evidence from the consequences of past choices. A winning group contributes current to the motor decision. Genes can redirect plasticity to a different group, change the evidence computation, reduce the number of groups expressed, gate writes to the shared cortex or re-activate other receptors that previously drove the same motor response. Each group's internal motor contacts can also feed activity back toward earlier circuits. The organism receives a raw receptor pattern and subsequent metabolic feedback. It never receives the identity of the hidden world during ordinary learning.

A group carries four 32-bit sensory rows, a 16-bit internal association matrix and 16 ambient contacts. Four groups therefore provide 640 binary contacts in 80 bytes. The rest of the 112-byte layer holds five inherited control words, membrane and recruitment state, and delayed spikes. A second 32-byte layer optionally expresses two update programs using the same contacts. Its inherited gate samples three of eight physical or internal predicates and looks up an expression bit. The animal remembers the chosen program until the outcome of that action arrives. In the optional evolvable-polarity substrate, alleles can change how a program interprets a departure from the resting state of a contact. Both programs still read and write the same physical bit.

The local weight transition is a heritable Boolean table indexed by contact state, incoming activity, efferent activity and feedback. Separate genes control how evidence changes a group's activity and which signals affect motor output. Founders can duplicate portions of prior inherited update programs. Newborns develop fresh contact states. The engineering constraints include four available groups, finite neural tissue and contacts, finite history, local update primitives, a physical sensor and motor interface, and the habitat's update and energy laws. [Context circuit](../src/assemblies.cuh) · [Spiking tissue and plasticity](../src/life_brain.cuh) · [World and inheritance](../src/life.cu)

For a one-bit contact, let $w$ be its current state, $x$ its earlier afferent spike, $e$ the matching efferent discharge and $f$ the feedback bit. A 16-bit germline table $T$ supplies the next state

$$w' = (T \mathbin{\gg} (w + 2x + 4e + 8f)) \mathbin{\&} 1.$$

The CUDA implementation selects the appropriate four-bit subtable once per motor row, then applies the two-input Boolean operation to 32 contacts in parallel. Each context group also updates a signed five-bit charge. Its leak operator can clear, halve, retain three quarters of, or retain the previous charge. An inherited eight-entry table adds signed evidence from the eventual feedback and two properties of the preferred grips. The properties indicate whether the executed grip was preferred and whether an alternative was also preferred. The group clips the result to $[-16,15]$. Those primitives let evolution alter the sign of feedback, ignore it, integrate inconsistent observations over time, or route it to a different memory. [Bit-parallel transitions](../src/life_brain.cuh) · [Evidence accumulation and competition](../src/assemblies.cuh)

## A test that separates learning from retrieval

The GPU creates a private set of two to four mutually incompatible worlds. A single animal experiences each in succession. For a given object the correct physical action differs in every world. The animal's genotype, life history, final neural state, elapsed time, probe objects, input patterns and motor tie breakers are identical at the start of each return branch. After zero to sixteen reminders, it makes read-only choices on other objects. A reminder is an actual attempted action followed by success or failure. Subjects can receive several failures before they have enough information to identify where they are.

| Task | Practice in the first world | Practice in later worlds | Scored when a world returns |
| --- | --- | --- | --- |
| Arbitrary | All 16 objects have private grips | All 16 receive another private mapping | 14 objects absent from reminders |
| Frames | All 16 objects form four private functional classes | Four distinct objects repeatedly attempted, one per class | Other 12 objects under the returning motor convention |
| Paired | Objects combine 16 shapes and 16 textures. Practice samples half the 256 pairs | Four distinct pairs repeatedly attempted | The 128 pairs excluded from all practice |
| Exceptions | A shared class mapping | Four exemplars plus four practiced item-specific exceptions | The four exceptions and eight unpracticed class members, counted separately |
| Interleaved | Single-property objects and paired objects in one lifetime | All 16 single-property objects mixed with four recurring pairs | 14 other single-property objects and 128 never-practiced pairs, after reminders exclusively on two single-property objects |

The acquired-state control freezes contact writes while the animal processes return feedback. A common reminder with identical consequences in every world preserves ambiguity. Other interventions erase acquired contacts, silence group output only for the probe, remove the cross-group reinstatement pathway, pool the groups, suppress feedback evidence, or clamp program expression. In the interleaved assay a matched branch can receive paired reminders instead. A separate oracle-addressed intervention sends the true group address to the circuit for comparison. The breeder and the intact animal never receive that address.

For hidden worlds the assay also counts how many world identities remain compatible with the actions actually taken and the outcomes actually delivered. If three identities are still indistinguishable, even a decoder that already knows every private mapping has expected accuracy at most one third on that branch. The reported bound is summed in twelfths for exact integer arithmetic. Visible landmarks supply additional information outside this bound. [CUDA experiment](../src/reinstatement.cuh)

## Breeding and independent testing

The foundation was one previously released animal with an evolved graph-learning cortex. First, six populations of 256 juveniles per challenge evolved the additional context programs for 384 generations. Independent populations started from six seeds. The strongest specialists on a separate validation seed scored 100.0% on arbitrary lookup, 87.9% on class transfer, and 89.0% on paired transfer after eight reminders in four hidden worlds. These percentages describe three different populations. Their performance on the other tasks varied sharply. [All specialist cohorts](context/breeding.csv)

A joint nursery tested each candidate on 12 acquisition and retention measures, including the three recurring-world tasks. Each cohort evaluated 18,874,368 simulated lifetimes over 384 generations. The core remained fixed during these targeted breeding experiments. All heritable loci can mutate again when descendants reproduce under ordinary ecological selection. The initial repertoire nurseries and their validation criterion excluded exceptions and interleaved lifetimes. A subsequent integrated nursery explicitly selected for interleaving. Exceptions remained withheld throughout. Four additional seeds supply the independent audits. Every independent set includes four physical effector rotations. The many correlated probe choices in a lifetime measure accuracy and do not constitute independent replicates. [Reproduction scripts](../tools/context-study.py) · [Single-program audit](context/audit.csv.gz) · [Two-program donor mixing](context/expression.csv)

## What happened when the same object types shared a lifetime

Evolving each program separately revealed a problem with how two programs share physical contacts. One specialist developed with its sensory contacts initially set, while another used the opposite resting polarity. Expressing the second program on the first one's initial tissue changed its behavior. In a matched experiment, the same two donor populations supplied juvenile programs. Selection ran for 16 generations with mutation disabled. Permitting inherited polarity differences raised the four-world paired score from 53.9% to 82.1%. Performance on the two single-property tasks reached 98.0% and 98.4%. In a mixed lifetime the same population scored 25.1% overall. Changing the representation solved a specific compatibility problem. Success on three separate task families was a poor predictor of whether an individual could learn them together. [Matched experiment](context/polarity.csv)

The integrated nursery gives juveniles both object types in the same hidden environment. Its return reminders come from two single-property objects. Selection scores the two domains separately so the much larger set of paired probes cannot hide failure on single-property objects. The intact animal has the same physical pins, contacts and capacity as before. One breeding treatment evolves only the two inherited context programs and their switches. The other permits mutations across the genome, including the spiking tissue, body and motor interface.

Across six integrated cohorts, the population with the highest minimum validation score came from seed 1601 with context-only mutations during breeding. After a return to one of the three earlier contexts, it scored 58.9% on mixed probes. Counting all four branches, including the most recent world, gave the results below. The independent audit used 2,048 fresh lifetimes per condition across four seeds. Each later world used four practiced pair exemplars.

| After eight reminders | Two hidden worlds | Four hidden worlds | Four worlds with ambiguous reminders | Four worlds with contact writes frozen during reminders |
| --- | ---: | ---: | ---: | ---: |
| Arbitrary grips | 93.85% | 87.73% | 25.00% | 87.93% |
| Shared classes | 86.69% | 74.04% | 25.00% | 62.45% |
| Unseen shape–texture pairs | 98.00% | 79.16% | 25.00% | 51.15% |
| Class remappings with exceptions | 91.88% | 81.10% | 25.00% | 55.63% |
| Mixed lifetime, other single-property objects | 79.61% | 61.08% | 25.00% | 61.79% |
| Mixed lifetime, unseen pairs after single-property reminders | 79.70% | 61.07% | 25.00% | 60.03% |

The mixed animal chose correctly on **640,370 of 1,048,576** never-practiced pairs after feedback exclusively on two single-property objects. The two domains had independently assigned motor mappings within each world. Their shared history linked them. Reminders on pairs themselves raised pair accuracy to 81.44%. The outcomes of the ordinary reminders allowed an ideal decoder that already knew every mapping to reach 97.5%. Both memory representation and readout still leave substantial room for improvement. Freezing writes preserved most of the mixed result. In the isolated paired task, the much larger drop under freezing shows that learning during the reminders also contributed. [Integrated breeding](context/integrated.csv) · [Independent task and circuit audits](context/repertoire-audit.csv.gz) · [Mixed-lifetime measurements](context/interleaved.csv.gz)

The separately bred linked population reached 85.75%, 91.57% and 85.29% on arbitrary, class and paired tasks in four hidden worlds after eight reminders. On mixed-lifetime probes it reached just 29.52% on single-property objects and 27.00% on unseen pairs. Selection in separate lifetimes preserved individually strong programs while leaving systematic interference between them. [Linked population](../examples/linked.eco) · [All linked cohorts](context/linked.csv) · [Independent audit](context/linked-audit.csv.gz) · [Withheld mixed lifetimes](context/linked-interleaved.csv.gz)

## One individual through a public interface

The supplied mixed-life animal experienced 3,119 external trials and matched 2,889 of them. It was the first occupied genotype in the selected habitat, using the first of four separately seeded teaching streams. Each return reloads the very same 696-byte file. No feedback is delivered on the test objects.

| Actual returning environment | Correct before reminders, out of 142 | Other single-property objects after eight reminders | Unseen pairs after eight reminders |
| --- | ---: | ---: | ---: |
| First | 0 | 12/14 | 128/128 |
| Second | 0 | 13/14 | 128/128 |
| Third | 0 | 14/14 | 128/128 |
| Fourth, the most recent | 142 | 14/14 | 128/128 |

Eight ambiguous reminders left the first three rows at zero. Across all four independent teaching streams, that genotype reached 190/224 on the single-property objects and 1,562/2,048 on the pairs. A separate lifetime trained exclusively on pairs had a different failure pattern. The supplied paired-life animal recovered all 128 probes in the first and fourth worlds but none in the second and third. The two files share an inherited nervous system. Their different acquired states determine which skills a user can recover. [Mixed-life animal](../examples/interleaved.mind) · [Paired-life animal](../examples/returning.mind) · [Every external branch](context/external.csv)

## The changing habitat

The living thicket runs two interacting populations. Each producer now can carry one, two or four reaction tables at a cost of four bytes for each additional chemistry. Seasonal changes are local. Each territory has its own period and order of recurrence. A propagating plant passes its chemistries to its offspring, and all can mutate. One treatment makes local light bands correlate with the current climate. In the hidden treatment, an animal can still experience fruit, hunger and the consequences of its actions. Its neural circuits receive no numerical climate address. A small GPU ledger counts actual opening attempts during the first four ticks following a change and during the rest of the season.

Four mutating worlds started with randomly generated, unschooled nervous systems. At tick 131,072 three worlds had 252, 304 and 401 living animals. Their oldest surviving lines had undergone 453 to 1,160 divisions. The fourth population went extinct. Fresh descendants of the three surviving worlds scored 15.3%, 16.1% and 16.6% on arbitrary, class and paired four-grip tests respectively, pooled across surviving worlds. Motor genes were free to lose access to grips that the test requires. These runs demonstrate ecological persistence alongside a measurable failure of spontaneous transfer. [All mutating and mutation-frozen worlds](context/wild.csv)

A causal field intervention takes a snapshot after 2,048 ticks and branches it. One branch applies `--reset-contacts` once. Each resident's acquired sensory and associative contact bits return to that resident's own developmental values. Both branches carry forward the same neurons, instantaneous firing, recent actions, membrane charges, bodies, producers, resources, clock and future random draws. They can acquire new contacts immediately. Comparing the two branches measures how previously acquired contacts affect actual harvesting and survival over the next 64 ticks.

The public actor can also carry a living resident out of a saved ecology. `--resident` preserves its spiking state, contacts, program history, age and pending sensory credit. Its new stream starts with the metabolic signal the animal had already received. Subsequent consequences come from the external environment. The `--from` command hatches a fresh individual of the same genotype.

In eight four-climate thickets, intact residents opened 1,758 of 5,496 attempts on the very next tick. The contact-reset branches opened 1,409 of 5,511. The benefit faded. Over 64 ticks the intact branches opened 120,216 of 363,876 attempts, compared with 122,263 of 368,150 after the reset. In 16 reciprocal mutation-frozen contests, animals with the full context circuit left zero of the 11,664 living descendants when competing with single-group relatives. Against siblings with match-based evidence suppressed, intact animals left 182 of 11,993 survivors. Sensory success, energy flow and competitive reproductive success measure different things. [Contact intervention at four horizons](context/pulse.csv) · [Reciprocal ecological contests](context/field.csv)

The saved seasonal habitat contains 220 animals and 1,024 plants after 2,048 ticks. Animals had produced 9,790 offspring and plants had dispersed 116,126 times. The longest surviving animal lineage spans 112 divisions. In four separate 32,768-tick releases with mutation and four hidden climates, three populations survived. Fresh descendants of those survivors earned 23.82%, 23.46% and 24.47% on the three separate memory tasks. The corresponding founder populations started at 87.82%, 75.18% and 79.66%. Mutation-frozen survivors retained much more of the founding ability. These results bound the claim that a breeding success will carry over into unattended evolution. [Living seasonal habitat](../examples/seasons.eco) · [Multigenerational releases and controls](context/descendants.csv)

## Blackwell execution

The ecological hot path packs 32 somatic cells per CUDA lane. Eight motor channels share two packed integer reductions which are reused by the cortical and peripheral decisions. Struct-address selection in the expression circuit originally forced the compiler to place an entire working brain into thread-local memory. Selecting its five scalar words independently eliminated the local allocation across all five supported widths. In the 256-cell dense kernel this traded 144 registers and 224 bytes of local memory per thread for 168 registers and zero local bytes. The occupied-site kernel uses 203 registers with zero local bytes. An alternating three-repeat comparison on the same 192-tick histories raised throughput by 13.3% at side 256 and 12.0% at side 512. The full final snapshots matched byte for byte in every pair. The earlier build used the struct assignment `program = alternate ? expression.alternative : primary` in the expression switch. [Register measurements](context/registers.csv) · [Matched Blackwell runs](context/compare.csv)

| Population and timing window | Living at start | Living at end | Schedule | Median GPU time | Billion somatic cell updates/s |
| --- | ---: | ---: | --- | ---: | ---: |
| 1,024² sites, 96 ticks after 96 ticks of ordinary life | 43,981 | 92,412 | Dense | 289.72 ms | 20.03 |
| Same world | 43,981 | 92,412 | Occupied | 263.67 ms | 22.01 |
| 1,024² sites, 16 ticks after four ticks at maximum starting density | 1,048,576 | 851,278 | Dense | 166.30 ms | 99.81 |
| Same crowded world | 1,048,576 | 851,278 | Occupied | 208.44 ms | 79.63 |

These timings include sensing, spiking, plasticity, ecological interactions, births, deaths, mutation and plant dispersal on an NVIDIA RTX PRO 2000 Blackwell Generation Laptop GPU. The numerator counts actual somatic cell updates. Both million-site runs also contain 1,048,576 producers with four locally recurring chemistries. The simulator reserves **766,116,172 bytes** of explicit device buffers, about 731 bytes per possible animal-and-plant site. It reserves space for empty sites as well. Driver allocations, loaded CUDA code and host snapshots incur separate costs. Each scheduler produced the same final fingerprint on every repeat. [Ordinary densities](context/scale.csv) · [Crowded populations](context/crowd.csv)

Two independent calibration seeds put the crossover between 33% and 46% initial occupancy on the measured 256-cell context circuit. The automatic scheduler chooses dense execution at and above 40% and occupied-site execution below it for these circuits on grids of side 256 or larger. It reuses the existing population census at reports. During longer intervals it samples 4,096 stratified sites on CUDA every 256 ticks. Both schedules cache their graph executables. A user can pin either schedule for a hardware comparison. Across a 640-tick history beginning at full density, median times were 139.53 ms with automatic switching, 142.26 ms with occupied-site execution throughout and 151.78 ms with dense execution throughout. The sparse-start medians were 159.40 ms automatic and 159.32 ms occupied. The two starting conditions each produced identical final fingerprints across all three schedules. [Density sweep](context/scheduling.csv) · [Automatic schedule measurements](context/adaptive.csv)

## Verification

The full suite passed on this laptop with native SM 12.0 machine code and with portable SM 8.0 PTX JIT-compiled on the Blackwell GPU. The host side also passed AddressSanitizer and UndefinedBehaviorSanitizer while exercising the GPU. Leak detection used the driver-only suppression described in the ecology experiments. Tests cover all five neural widths, graph and direct execution, changes of scheduler during a trajectory, exact world resumption, contact-reset idempotence, resident extraction, delayed credit, batched public streams, frozen probes and counterfactual information bounds. Across 28,920 published audit rows, all per-environment subtotals reconcile and every hidden-condition arm with ordinary sensory access respects its information bound. [Tests](../tests/tests.cpp) · [Driver allocation control](ecology.md#verification-on-the-laptop)

## Reproduce the experiments

Build the native binary on the Blackwell laptop. Change the GPU architecture for another supported device. The PTX build can also use the installed driver's JIT.

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
make -j NATIVE=1 CUDA_ARCH=sm_120 test-gpu
./axion life --load examples/repertoire.eco --steps 0 --revisit 128 --regimes 4 \
  --varieties 16 --exposures 32 --memory-task paired --probe-seed 176497
./axion life --load examples/repertoire.eco --steps 0 --revisit 128 --regimes 4 \
  --varieties 16 --exposures 32 --memory-task interleaved --probe-seed 176497
```

The public actor accepts physical receptor masks and bodily outcomes on standard input. The independent lesson generator supplies a different randomization from the CUDA assay. A training encounter asks the animal to act and evaluates its own effector pins. A probe reads an action from a copy of the present state. Reloading the same file for each branch preserves the common starting history.

```sh
tools/context-study.py lesson --task interleaved --stage return --environment 0 --reminders 8 | \
  ./axion agent --load examples/interleaved.mind
tools/context-study.py lesson --stage training --task paired | \
  ./axion agent --from examples/repertoire.eco --save pupil.mind
tools/context-study.py lesson --stage return --environment 0 --reminders 8 | \
  ./axion agent --load pupil.mind
tools/context-study.py lesson --stage ambiguous --environment 0 --reminders 8 | \
  ./axion agent --load pupil.mind
./axion agent --load pupil.mind --inspect
```

The breeding and experiment drivers use only the Python standard library. Brains, encounters, ecological updates and selection execute on CUDA. The independent public lesson driver supplies stimuli and tabulates outcomes. The following stages write population checkpoints that can also be used as founders in the simulator.

```sh
study=$(mktemp -d /var/tmp/axion-study.XXXXXXXX)
export TMPDIR="$study"
tools/context-study.py breed --specialists "$study/donors" --save "$study/one.eco" > "$study/breeding.csv"
tools/context-study.py combine --specialists "$study/donors" --save "$study/mixed.eco" > "$study/mixed.csv"
tools/context-study.py polarity --specialists "$study/donors" > "$study/polarity.csv"
tools/context-study.py cross --specialists "$study/donors" --save "$study/crossed.eco" > "$study/crossed.csv"
tools/context-study.py link --specialists "$study/donors" --save "$study/linked.eco" > "$study/linked.csv"
tools/context-study.py integrate --population "$study/mixed.eco" --save "$study/integrated.eco" > "$study/integrated.csv"
tools/context-study.py audit --population examples/repertoire.eco > "$study/audit.csv"
tools/context-study.py interleaved --population examples/repertoire.eco > "$study/interleaved.csv"
tools/context-study.py external --population examples/repertoire.eco --save "$study/returning.mind" > "$study/external.csv"
tools/context-study.py field --population examples/repertoire.eco > "$study/field.csv"
tools/context-study.py pulse --population examples/repertoire.eco > "$study/pulse.csv"
tools/context-study.py descendants --population examples/repertoire.eco > "$study/descendants.csv"
tools/context-study.py wild > "$study/wild.csv"
tools/context-study.py scale --population examples/repertoire.eco > "$study/scale.csv"
tools/context-study.py crowd --population examples/repertoire.eco > "$study/crowd.csv"
tools/context-study.py scheduling --population examples/repertoire.eco > "$study/scheduling.csv"
tools/context-study.py adaptive --population examples/repertoire.eco > "$study/adaptive.csv"
```
