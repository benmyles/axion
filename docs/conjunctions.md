# Learning what two objects do together

An earlier speaker stayed silent on every one of 512 questions after seeing every arrangement of two interacting objects. The released Weaver answered the same 512 questions correctly. Both encountered unfamiliar action names and independently reassigned bearings. The questioner, visible arrangements, demonstrations and answer decoder were the same. [Earlier answers](bindings/joint-interaction.csv) · [Weaver's answers](conjunctions/joint-interaction.csv) · [Speaker checkpoint](../examples/weaver.mind)

## From a demonstration to a prediction

Human single-neuron recordings during a picture-comparison task found populations responsive to items and to context, with some neurons specific to particular combinations. In a virtual pursuit task, human hippocampal activity represented several agents and gaze through partly distinct spatial response fields. An October 2026 mouse study found that co-activity across cell assemblies separated variables mixed in individual responses. The engineering inference here is modest. Separate object signals make some tasks easy, while a circuit sensitive to coincidences can express relationships neither object specifies alone. The CUDA update equations and its regularity assumptions are design choices. [Bausch et al., Nature](https://pubmed.ncbi.nlm.nih.gov/41501461/) · [Human spatial generalization, Neuron](https://doi.org/10.1016/j.neuron.2026.08.004) · [Cell assemblies in mice, Nature Neuroscience](https://www.nature.com/articles/s41593-026-02491-x)

The animal has four action fields addressed by sounds. Each field has two outputs and sixteen possible arrangements. For each arrangement, four two-bit contacts can acquire the bearing of each output. The plasticity program inherited from the animal's visual memory updates those contacts when it sees the two departures and arrivals. This adds 128 bytes of acquired contacts and 12 bytes of inherited gates. The speaker's total state is 668 bytes. Older individuals retain their original organ sizes. [Visual plasticity](recollection.md) · [Circuit](../src/conjunction.cuh)

When the animal hears the action again, four 16-bit planes carry the possible bearings of one output across all arrangements. The gates can select the older single-source circuit or enable the joint field. Other inherited bits set the minimum evidence, enable row or column inhibition, allow excitation at the last possible site, and set the number of synchronous steps. This is the cellular-automaton idea in the implementation. A whole row or column exchanges signals in parallel using integer masks. Observed spikes anchor the activity. A contradiction discards the attempted completion. Selection can turn the entire pathway off.

During a question, a 36-byte transient field carries possible final scenes for each starting scene. Every action propagates those possibilities to its successor. When both outputs are uncertain, their Cartesian product can include combinations that a richer model of their correlation would rule out. Two paths can disagree in the middle and converge on the same answer later. The animal opens the apertures indicated by the learned dependence, and a covered aperture supplies only previously acquired visual memory. Even a long question never writes an imagined bearing into memory as a physical observation. [Dialogue kernels](../src/dialogue.cu)

## Independent spoken interview

The public interviewer trained two outputs per new word, with four to fourteen randomly located demonstrations. It drew the physical laws from all **576 Latin squares of order four**. A separate enumerator retained every square compatible with what the animal had seen. A bearing counted as forced only when every remaining square agreed. The enumerator runs in the test harness. The animal receives demonstrations, a spoken question, and ordinary retinal input. The held-out seeds differed from those used in development and breeding. [Interviewer and oracle](../tools/binding-study.py) · [All held-out utterances](conjunctions/held-out-interview.csv)

| Demonstrated arrangements | Forced bearings spoken correctly | Ambiguous questions answered with silence |
| ---: | ---: | ---: |
| 4 | 72 / 131 | 637 / 637 |
| 6 | 405 / 415 | 225 / 225 |
| 8 | 461 / 461 | 51 / 51 |
| 10 | 384 / 384 | 0 tested |
| 12 | 256 / 256 | 0 tested |
| 14 | 128 / 128 | 0 tested |

The other 69 forced questions produced silence. On two separately learned, unrestricted transition tables, the same individuals answered **1,920/1,920** questions concerning sequences of 1, 2, 4, 8 or 32 actions. The choices of names, tables, physical starting points and action order came from the external interviewer. The previous single-source intervention also survived, with 236/236 correct answers on fresh arrangements before reversal, 236/236 after reversal, and 64/64 silences while the roles were confounded. [Sequence answers](conjunctions/held-out-interview.csv) · [Single-source controls](conjunctions/single-source-interview.csv)

The distinction between a usable regularity and a certainty matters here. An unrestricted second apparatus can agree with every demonstration and then do something different at the withheld site. The `twin_truth` column gives such a counterexample for each probe. Every one of the 1,706 spoken inferences would be wrong for its corresponding twin. The animal has an effective inductive bias and cannot identify an arbitrary unobserved law from those observations alone.

## Exhaustive check and learning from sparse evidence

The first interview used four or more demonstrations. An exhaustive CUDA audit exposed a problem at two or three. For each of the 576 physical laws, the audit presented every nonempty proper subset of the sixteen demonstrations. All worlds saw a given subset in the same seeded order, so presentation order could reveal nothing about concealed outcomes. That produced **37,747,584 partial histories and 301,980,672 held-out single-output queries**. The scorer computed which of the 576 laws remained possible after each subset. The original Weaver gave 55,296 wrong answers to forced questions and 1,907,712 wrong answers to ambiguous questions. All occurred below four demonstrations. [Results by number of demonstrations](conjunctions/exhaustive.csv)

The next nursery included sparse rows, sparse columns and trials where a familiar single landmark gives a misleading prediction. Selection changed the inherited gates while keeping the same 668-byte body, contact bank, plasticity program and lateral circuit. The resulting speaker answered **228,434,688 of 229,328,640** forced questions correctly and stayed silent on the other 893,952. It stayed silent on **all 72,652,032** ambiguous questions. It gave zero wrong bearings in the complete audit. A matched drift descendant made 353,472 wrong forced reports and 2,331,456 wrong ambiguous reports. [Selected and drift circuits](conjunctions/sparse-census.csv) · [Current speaker](../examples/weaver-sparse.mind) · [Run visualization](../journal/runs/00004241-conjoin/plate.webp)

In a second public interview with sixteen fresh seeds, the new speaker gave 1,766/1,810 forced answers, withheld all 878 ambiguous answers and correctly spoke all 1,920 sequence answers. It also passed all 512 questions on the original joint-interaction benchmark. Its separate single-source transfer survived reversal. [Fresh utterances](conjunctions/sparse-interview.csv) · [Joint interactions](conjunctions/sparse-interaction.csv) · [Single-source transfer](conjunctions/sparse-roles.csv)

The enumerator belongs entirely to the scorer. It groups alternative worlds by the exact cells where each agrees with the actual law. Sixteen passes of bitwise union compute the possible outcomes for every subset of the observations. The animal can read only its acquired contacts. Repeating the complete audit with a second presentation-order seed gave identical counts for the sparse-trained speaker. The audit covers one-output readout under this finite law family and presentation protocol. Public interviews separately exercise the auditory, visual, memory and vocal pathways. Arbitrary dynamics still admit the counterexamples in the interview data. [CUDA audit](../src/conjunction_certificate.cuh) · [Both presentation orders](conjunctions/sparse-census.csv)

## Inheritance and real consequences

The conjunctive program began with all 96 inherited bits zero on top of the previously evolved visual and source circuits. Eight selection populations and eight neutral-drift populations had the same founders, mutation operators and budgets of 256 candidates over 256 generations. On a common held-out population audit, **1,565/2,048** selected individuals and **4/2,048** drift individuals were perfect. The released individual came from the first selection seed, 98413. In its separate 2,048-probe classroom test it inferred every withheld bearing. One lateral step gave 990 correct and disabling recurrence gave zero. [Whole populations](conjunctions/populations.csv) · [All arms and seeds](conjunctions/classroom.csv)

A further audit used probe seed 2845607 and 4,096 trials per condition. The released individual inferred 4,096 withheld bearings, stayed silent on 4,096 ambiguous probes, and successfully revised 4,095 of 4,096 demonstrated changes. A journal plate shows the acquired and inferred directions of one animal in that audit. [Audit](conjunctions/release-audit.csv) · [Journal plate](../journal/runs/00004191-conjoin/plate.webp)

On the coupled islands, two landmark positions jointly determine where food and water arrive. Each island has its own reassignment of bearings. Residents share finite reservoirs and pay energy for hunting, experiments and offspring. Across eight independent gardens, 8,192 creatures saw nine arrangements apiece before foraging for 64 days. Each comparison used the same animals, exposures, resource streams and harvest policy. [Matched gardens](conjunctions/coupled-gardens.csv)

| Readout | Meals obtained | Energy deposited in offspring |
| --- | ---: | ---: |
| Intact | 505,210 | 936,696 |
| Directly observed arrangements only | 358,898 | 507,504 |
| Joint pathway disconnected | 79,272 | 2,556 |

In a second experiment, the creatures hatched with **zero demonstrations** and could choose and pay for their own experiments. A newborn from 64 generations of ecological selection obtained 676,292 meals and deposited 1,104,948 units in offspring across eight 96-day gardens. Disabling recurrence reduced the deposit to 447,612. Disconnecting the joint readout reduced it to zero. The matched drift lineage with its readout intact deposited **1,170,864**, exceeding the selected lineage. This experiment establishes a benefit from lifetime learning in the tested lineage. It supplies no selection advantage over that drift control. [Unprompted gardens](conjunctions/unprompted-gardens.csv) · [Drift control](conjunctions/drift-gardens.csv) · [Selection history](conjunctions/ecological-selection.csv) · [Drift history](conjunctions/ecological-drift.csv) · [Newborn](../examples/marsh-newborn.mind)

## Blackwell measurements

On the RTX PRO 2000 Blackwell laptop GPU, **1,048,576 independently stateful 256-cell residents** experienced 32 days across 32,768 coupled islands in **172.506 ms** of CUDA event time. The simulation allocated 804,339,988 device bytes in its application buffers. Residents carried out 9,378,018 self-chosen experiments, obtained 22,803,969 meals, and deposited 20,954,868 energy units in offspring. The timed interval starts after hatching and ends before host reporting and journal rendering. [Full-world measurement](conjunctions/ecosystem-scale.csv) · [Island journal](../journal/runs/00004194-commons/plate.webp)

The frozen, single-step inference benchmark separately measures the cost of answering queries once observation ends. An action's 52 bytes of mutable joint and single-source contacts can be compiled to a 12-byte record of bearings and valid answers. The compact record supports a fully observed starting arrangement. Online revision and uncertain multi-step replay use the mutable circuit. All million animals had independent learned contacts and shared the inherited program parameters in this benchmark.

| Queries per animal | Direct circuit | Compiled readout | Compilation time for the batch |
| ---: | ---: | ---: | ---: |
| 1 | 1.07 billion/s | 4.58 billion/s | 0.972 ms |
| 64 | 32.83 billion/s | 90.02 billion/s | 0.969 ms |

The kernel builds the direct readout once per animal per pass. The 64-query stream covers all 32 input/output questions twice in permuted order. Timings are medians of three CUDA-event measurements after warm-up. They include reading the private state and writing verification outputs. Both paths produced identical per-individual answers, silences, counts and rolling signatures. Compilation costs more than it saves for a single query in this run. [Timing samples](conjunctions/scale.csv) · [Frozen inference kernel](../src/conjunction_batch.cuh)

## Reproduce

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120 all test-gpu
./axion conjoin --origin inert --generations 256 --candidates 256 --worlds 8 \
  --cycles 4 --seed 98413 --probe-seed 2405329 --probe-worlds 512 --save weaver.mind
python3 tools/binding-study.py interaction --speaker examples/weaver.mind
python3 tools/binding-study.py completion --speaker examples/weaver.mind --cycles 12 \
  --seeds 100267 201289 305093 409891 521009 622957 732829 846137 \
          959597 1066013 1175051 1284481 1393207 1502389 1611851 1721323
python3 tools/commons-study.py audit --source examples/weaver.mind --landmarks coupled \
  --exposures 9 --policy harvest --volatility 0 --rations 8 --days 64 \
  --arms intact observed-only no-joint
./axion conjoin --load examples/weaver.mind --generations 0 --probe-worlds 8 \
  --cycles 2 --replicas 1048576 --decisions 64
./axion conjoin --load examples/weaver.mind --lessons sparse --generations 192 \
  --candidates 256 --worlds 8 --cycles 4 --seed 457121 --probe-seed 2949179 \
  --probe-worlds 512 --enumerate all --save weaver-sparse.mind
```

The animal still speaks through four directional vocal outputs. The number of object slots, available senses, topology of the lateral circuit and environmental physics bound what evolution can discover. General conversation, the ability to introduce new kinds of objects, and child-level cognition remain unverified.
