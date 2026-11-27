# A learning circuit that travelled from food to speech

A population of digital foragers evolved under selection for swallowed food. One descendant's two-word cortical program could also regulate the acquisition of meanings and a physical forward model. Across 6,144 independently taught copies of one complete creature’s hand, somatic and speech circuits, it correctly answered 16,198 of 16,233 questions where an alternative outcome actually differed from what the creature saw. This experiment used four sockets, four pigments, four motors and a finite family of physical mechanisms. The creature can discuss those events and still falls silent on open-ended questions. [Raw trials and measurements](tidal-learning/language.csv)

## Talk to the speaker

```sh
./axion talk --speaker examples/estuary-tide-speaker.mind --listener examples/parallax-witness.mind
```

The checkpoint remembers a new, three-pull encounter that took place after teaching. These are its replies about the east socket. The question strings pass through acquired sound and referent contacts on CUDA. [Saved question set](../examples/estuary-tide-conversation.txt) · [Recorded answers](tidal-learning/conversation.txt)

| Question | Creature's answer |
| --- | --- |
| What was in east? | teal |
| What is in east? | plum |
| What did you expect in east? | amber |
| What was in east before the last pull? | plum |

In this encounter the teaching apparatus obstructed the last pull. The independent mechanical readings were 78 before the movement, 201 afterwards, 177 with all attachments open and 201 at the end of the prefix. Each number packs four two-bit pigments. The pupil's predicted unobstructed board was 177. Only the resident's private episode and the public question enter its reply. The fixture supplies the externally prompted movement sequence. The speaker earned its forward model through earlier sensory practice. [Teaching transcript and mechanical receipt](tidal-learning/english.txt)

To start with the classroom-naive experimental body and reproduce the teaching and a new encounter, run the following commands. The file also serves as a short example of the interactive controls. A station specifies its mechanism, four starting pigments and the four-bit attachment mask on the last pull. Each motor action costs the operator a unit from its purse.

```sh
./axion talk --speaker examples/estuary-tide-student.mind --listener examples/parallax-witness.mind \
  --save-speaker tidal.mind < examples/estuary-tide-lessons.txt
./axion talk --speaker tidal.mind --listener examples/parallax-witness.mind < examples/estuary-tide-conversation.txt
```

## Where the program came from

A funded nursery starts from cortical gene pairs sampled from living residents. One bead enters a visible inlet. Hidden physical routing takes it to one of four outlets. An animal commits to an outlet before tasting the result. After 32 reaches the current shifts without warning and after 32 more it shifts back. Swallowed carbon funds local reproduction at eight beads per offspring. A food-weighted parent lottery and a uniform-parent control run separately with the same local budgets, climates and founders. The nursery represents the relevant inherited loci in 16 bytes per animal. Full bodies live in the watershed.

Three independent populations of 1,024 were reared for 96 generations. Each final animal was weighed in 32 common, previously unpractised three-season climates. Each row below has 3,145,728 opportunities per population. The first experiment held the ancestral contact law fixed and let the two module words mutate. In the second, the local update law and mutation rate could also change. The normal rearing command uses the latter setting. [All generations and material accounts](tidal-learning/food.csv)

| Nursery seed | Food-selected, module free | Uniform, module free | Food-selected, all loci free | Uniform, all loci free |
| --- | ---: | ---: | ---: | ---: |
| 14327 | 2,279,126 | 905,143 | 2,225,367 | 776,178 |
| 34781 | 2,408,387 | 886,048 | 2,201,506 | 896,584 |
| 91513 | 2,275,731 | 912,055 | 2,139,427 | 899,966 |

The distribution in every food-weighted population improved. Transfer to speech varied sharply. Each population's candidate was selected by material intake before testing language. Two food-selected candidates passed both conventions in only 4 of 512 courses each. The third passed 467. A uniform-parent candidate also reached 467. The public donor comes from food-selected seed 91513. Its strong transfer motivated the larger tests below. The numerator from that retrospective comparison measures the discovered program, while the independent breeding runs measure the effect of food selection on food. [Donor record](../examples/estuary-regulator.txt) · [Final population and founder scores](tidal-learning/paid-91513.tsv.gz)

## What the evolved circuit computes

An expressed somatic band contains 32 binary cells. Its inherited routing and gate words specify two banks of 16 parallel three-input truth tables. Inputs can select local contact state, sensory incompatibility, the native proposed update, recurrent spikes, public feedback or other somatic state. The second bank releases or inhibits each of 16 proposed writes. Its spikes and the first bank's spikes return to the same underlying somatic band. Reproduction already mutates these words and their assignment to expressed bands. Direct field operation adds zero permanent bytes per resident.

The food-selected donor routed old contact state, local conflict and agreement into its first bank. Its second bank read those spikes, conflict and whether a prediction had been attempted. Exhaustive enumeration gives this truth table for a single contact. A one means that interpretation has been excluded. Each column contains the next state. [Independent decoder and material replay](../tools/apprentice-audit.py)

| Input to one contact | Old 0, consistent | Old 0, conflict | Old 1, consistent | Old 1, conflict |
| --- | ---: | ---: | ---: | ---: |
| No prediction emitted | 0 | 1 | 1 | 1 |
| Prediction confirmed | 0 | 1 | 1 | 1 |
| Prediction contradicted | 1 | 1 | 0 | 1 |

On an uncommitted observation or a confirmation, incompatibilities accumulate. A contradicted commitment reopens previously excluded, currently compatible possibilities and closes currently open ones. Subsequent observations can then narrow the alternatives again. Replaying this table independently reproduced all 2,361 swallowed beads of the selected animal's 3,072-opportunity validation harvest. This small rule is a post-hoc description of one digital lineage. The implementation executes the mutable circuit words.

```sh
python3 tools/apprentice-audit.py donor examples/estuary-regulator.txt
```

## What transferred

First, the reared regulator was connected to the question contacts of an experimental copy of the estuary witness. Each course started from empty language memory, used new names, new forms and a new physical teaching history, then reversed the meanings of the two question forms. Across three fresh seeds, 5,561 of 6,144 copies passed all eight diagnostic questions both before and after the switch. When the feedback inputs were quiet, zero did. Playing another pupil's contact-release schedule yielded two. The full-feedback schedule and playback released exactly the same total number of bits within each cohort. Erasing the private field event elicited zero replies. [Example two-form trace](tidal-learning/fresh-417811.tsv.gz)

Next, the same donor law was routed into both the hand and the voice of a classroom-naive experimental copy of a field body. This routing is an explicit intervention. The original wild animal carried different genes. The hand received 12 practice rounds of four movements. It had to form its own forward model from starting pigments, its attempted movement and the measured result. The voice heard up to 64 demonstrations per form in each convention. During a demonstration it could attempt an answer before hearing the instructor's sound. The external instructor chose the order of presentations.

In a new test, 6,144 courses varied the physical rig across lifetimes as well as the acoustic convention and history. Each taught four forms, then reversed both pairs. Each course drew one of 128 possible rig settings. The final probe used a previously unseen movement on the practised rig after all teaching. Scoring independently executed its actual, unobstructed and shortened trajectories. For the alternative questions, the pupil had access only to its learned motor projections. The traces record unfamiliar layouts tested using the words acquired during each course. [First seed](tidal-learning/transfer-four-57889.tsv.gz) · [Second seed](tidal-learning/transfer-four-476887.tsv.gz) · [Third seed](tidal-learning/transfer-four-885539.tsv.gz)

| Meaning after the switch | Correct / asked | Correct when different from actual |
| --- | ---: | ---: |
| Before the action | 24,576 / 24,576 | 17,791 / 17,791 |
| Actual outcome | 24,576 / 24,576 | — |
| If all pulls were completed unobstructed | 24,517 / 24,576 | 8,884 / 8,907 |
| With the final pull omitted | 24,535 / 24,576 | 7,314 / 7,326 |

All 6,144 also passed the separate eight-question observed-frame diagnostic under both conventions. Six passed that diagnostic with another pupil's release schedule and zero with feedback muted. The controls share the same motor training. In a matched 1,024-course routing experiment, donor law in both organs with native hand learning passed both conventions in 137 courses. Adding cortical gating during hand practice brought this to 1,024. The four-frame outcomes are scored against physical executions, including alternatives that disagree with what was seen. [Factorial controls](tidal-learning/hand-both-native.tsv.gz) · [Cortically regulated hand](tidal-learning/hand-both-cortical.tsv.gz)

```sh
python3 tools/apprentice-audit.py classroom docs/tidal-learning/transfer-four-57889.tsv.gz
```

## The living population

The shore school operates only on a new, naturally occurring retinal encounter. The instructor points to actual occupied sockets, emits arbitrary sounds and offers examples. All questions about an encounter are scored before the instructor teaches that encounter. Time, mortality, movement and births continue throughout. Pupils are tracked by their physical identities, and a successor in a reused slot cannot inherit the attendance record.

Of 4,096 sighted residents enrolled from the population with connected speech organs, 3,580 attended. They gave 42 correct replies in 435,656 pre-lesson questions, with 16 pupils knowing all eight names. They reported both sides of zero out of 28,148 changing sockets. In the matched examination-only habitat, the enrolled animals gave zero replies. When a single skilled witness was re-exposed to the public lesson, its original regulator damaged a previously stable vocabulary. With the food-reared regulator it retained all 256 correct replies across 32 new encounters. Starting the same reared witness without the classroom vocabulary gave much lower attainment, including incomplete naming. [School and survival measurements](tidal-learning/encounters.csv) · [Individual attendance roll](tidal-learning/shore-teaching.tsv.gz)

The physical port also acts when an ordinary animal pulls on a real float. During one 2,048-tick intervention, 21,298,785 expressed motor contacts proposed 78,208,735 bit changes. The somatic assemblies admitted 16,900,879. The open habitat ran at 58.64 million site-ticks/s, the muted-signal habitat at 58.68 million, and the original pathway at 59.17 million on this laptop. These are single paired throughput observations. The open interval used 114,688 bytes of sharded telemetry over the entire million-site world. Saving and resuming both dense and paged storage reproduces the same physical state. [Physical and performance records](tidal-learning/wild.csv)

In a separate release, 2,048 living bodies received the same inherited program and both contact laws. They continued naturally in the food web for 4,096 ticks. At the end, 702 living animals still expressed the exact two-word program, all born after introduction. Of those, 697 retained both local laws. Another 511 living animals expressed a one-bit neighbour. The matched habitat without an introduction ended with zero exact carriers. The release ran 4,294,967,296 site-ticks in 73.84 measured simulation seconds. Carriage and language proficiency are separate measurements.

## A child born in the watershed

A second introduction targeted parents with connected auditory organs and a name-binding law that passed a separate calibration. At tick 387,736, 512 living parents received the food-reared regulator. Ninety-six ticks of unsupervised ecological life later, 462 residents expressed the exact program and 245 of them were born strictly after the intervention. The observer prospectively enrolled 213 young carriers with eligible hearing and sight. Of these, 207 passed all eight questions under both conventions in at least one of twelve independent courses. Two additional sets of 32 unseen courses tested every eligible child equally. The same 206 of the 213 children passed all 64 courses under both conventions. In total they passed 13,200 of 13,632 paired diagnostics. [First cohort roll](tidal-learning/cohort-122761.tsv.gz) · [Second cohort roll](tidal-learning/cohort-422873.tsv.gz)

In two less selective releases, the later calibration found zero eligible carriers despite widespread inheritance of the two-word module. The inherited architecture around the module matters.

Resident **858336** was born at tick 387,808 in generation 3,208. It had 64 expressed somatic spiking cells, three inherited modules and separate sensory, hand and vocal contacts. Its own unmodified body passed all twelve enrollment courses. Three new seeds then tested it in 6,144 courses with unfamiliar dialects, physical histories and sampled rig settings. It passed the eight observed-frame questions under both conventions in all 6,144. On the separate physical probe it got all 49,152 before/after answers right. When the alternate physical history differed from the observed result it got **11,534 / 16,179** right, below the original experimental body’s 16,198 / 16,233. The tests used this animal’s own hand, body and sensory record. [Offspring trial data](tidal-learning/offspring-class-577109.tsv.gz) · [Full measurements](tidal-learning/language.csv)

The child also faced the same 512 courses with different amounts of motor practice. With zero, two, four and eight rounds, it correctly reported 2, 203, 627 and 956 of the same 1,365 distinct mechanical alternatives. Twelve rounds brought it to 984. It reached 990 at 24 rounds and stayed at 990 at 48 and 64. Extending this child's existing exercise had a measurable ceiling. [Motor practice curve and controls](tidal-learning/practice.csv)

The calibrated release subsequently ran for 8,192 more ticks without a visitor or instructor. Exact carriers rose and fell from 462 to 417, 446, 293, 122, 52, 23, 17 and finally zero. The last census contained 358,741 living creatures and zero expressed one-bit neighbours. The habitat processed 8,589,934,592 site-ticks at 58.22 million site-ticks/s. This measures the disappearance of this particular program in one ecology. The contributions of metabolic cost, genetic linkage, drift and local competition remain unmeasured. [Successive natural-selection censuses](tidal-learning/wild.csv)

The portable speaker carries the newborn's motor and speech contacts after the English course. Asked “what is in west?”, it answered “plum”. Asked “what did you expect in west?”, it answered “teal”. The actual and unobstructed physical boards give those respective pigments. Its east-socket prediction was unresolved, and the two open-ended English questions elicited silence. [Newborn's measured encounter](tidal-learning/born-english.txt) Its source habitat preserves the untrained descendant at age 24. It can also be taught again, returned to the same individual and interviewed as the world advances.

```sh
./axion talk --speaker examples/estuary-born-speaker.mind --listener examples/parallax-witness.mind
zstd -d examples/learning-estuary.flood.zst -o learning.flood
./axion floodplain --load learning.flood --steps 4 \
  --return-practice examples/estuary-born-student.mind examples/estuary-born-speaker.mind \
  --follow-witness examples/estuary-born-student.mind --field-questions -
```

At four ticks after the return, the resident answered the four observed-frame questions from its retained field sighting. At the sixteen-tick visit it was gone. The stored speaker can still be studied from its checkpoint.

The archived watershed expands to SHA-256 `710e62d13325f5af11e98ade6612f32b1bebb94f05ddb6b3f3592969628e50bf`. A bystander can report an observed arrangement and can lack a private intention or shortened motor plan. The resident's field reports reflect its role in that particular event. [Real visits and acquisition](tidal-learning/encounters.csv)

## Reproduce the experiment

```sh
zstd -d examples/witness-estuary.flood.zst -o witness.flood
./axion floodplain --load witness.flood --slot 890939 --steps 0 \
  --feedback-generations 96 --feedback-cohort 1024 --feedback-loci module \
  --class-seed 91513 --feedback-donor reared.txt
./axion floodplain --load witness.flood --slot 890939 --steps 0 \
  --receive-donor reared.txt --receive-ports both --feedback-class 2048 \
  --feedback-motor cortical --feedback-rig varied --feedback-frames 4 \
  --feedback-convention reversed --class-practice 12 --class-seed 57889 --feedback-trace trial.tsv
python3 tools/apprentice-audit.py classroom trial.tsv
./axion floodplain --load witness.flood --receive-donor reared.txt --receive-ports both \
  --receive-count 2048 --trace-donor reared.txt --class-seed 58963 --steps 4096 \
  --report-every 512 --feedback-ports open --save descendants.flood
```

The published four-frame trials began at tick 387,728, before classroom teaching. For an exact source checkpoint, decompress `examples/outer-estuary.flood.zst` and advance it 3,344 ticks with `--steps 3344 --save discovery.flood`. Use that habitat for the transfer command. For speaking in the field, `--return-practice ORIGINAL TAUGHT` returns the changed speech and hand contacts to the same living individual after verifying its identity and the original contact state. Its current retinal event comes from the field. `--return-lesson` transfers speech alone.

## Execution checks

The closed-path release reproduced the preceding implementation’s complete million-site checkpoint byte for byte after 2,048 ticks. A separate test ran the changed CUDA modules as native Blackwell machine code and as portable `sm_80` PTX on the same device. The two executions produced byte-identical million-site habitats after a live four-tick lesson return, portable minds, cohort qualification traces and four-frame acoustic responses. Dense and paged cortical storage, fixed and packed dispatch, a split checkpoint restart, and an examination-only observer also agreed in the regression suite. [Build targets, device and replay checksums](tidal-learning/validation.json) · [Executable regression](../tests/apprentice.py) · [Independent acoustic and physical auditor](../tools/apprentice-audit.py)

## Research behind the test

Work with mice and recurrent models found that the structure of early experience affected how flexibly later strategies transferred. This motivated varying the physical curriculum independently of the words and testing on a new mechanism configuration in each lifetime. A 2026 model of sensory mismatch proposed distinct roles for prediction and plasticity in local circuits. Axion exposes an emitted prediction and the subsequent sensory result as candidate inputs to inherited logic. The substrate’s correspondence to human learning remains an open empirical question. [Structured experience and strategy learning, Nature Neuroscience, 2026](https://www.nature.com/articles/s41593-026-02409-7) · [Mismatch calculation and plasticity, Nature Communications, 2026](https://www.nature.com/articles/s41467-026-70354-x)

Studies in children provide a higher benchmark. Children can choose practice tasks according to their expectations and can adjust their challenge choices after performance feedback. Axion's current test demonstrates adaptation to a supplied curriculum. Independently choosing a practice goal, explaining a novel mechanism and sustaining an unscripted conversation remain open tests. [Strategic practice in children, Child Development, 2025](https://academic.oup.com/chidev/article/96/5/1619/8341639) · [Challenge choice and metacognitive monitoring, Child Development, 2026](https://pubmed.ncbi.nlm.nih.gov/41636421/)
