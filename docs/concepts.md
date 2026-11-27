# Learning a relation and using it again

The released witness learned the meanings of eight relation words from examples. One names an eight-crossing journey that the newborn could not express using its two primitive contacts alone. On a new map it can follow the journey from a previously untested object, answer a yes-or-no question about the destination, or say what its partner would expect after seeing a different change. The two minds occupy 2,988 and 2,836 device bytes. Both retain their earlier event reporting and spatial abilities. [Witness](../examples/cartographer.mind) · [Companion](../examples/lookout.mind) · [Recorded conversation](../journal/runs/00005049-talk/plate.webp)

## Research behind the experiment

Pan and colleagues reported on 16 November 2026 that participants reused relational structure as the preferences of characters in a game changed. Perturbing the hippocampus disrupted inference from that structure. This motivated a falsifiable design choice for Axion. A word can retain an acquired operation while every object-to-object connection in the scene changes. The test then gives the animal an entirely new map. [Pan et al., Nature Human Behaviour](https://www.nature.com/articles/s41562-026-02627-w)

Work on plastic recurrent networks showed how meta-learned plasticity can support relational inference and the reassembly of previously learned lists. A 2026 computational study also derived order representations from learned sequence statistics. Here selection can change the transition table for synaptic evidence and determine when an earlier relation becomes available to a new one. A matched intervention removes those extra inputs. The bit planes, primitive operations and GPU scheduling are engineering choices. [Plastic relational learning, Nature Neuroscience](https://www.nature.com/articles/s41593-024-01852-8) · [Learning order relations, Nature Communications](https://www.nature.com/articles/s41467-026-76102-5)

A developmental experiment found that 18-month-old children could use spoken information when another person's belief made a reference ambiguous. That finding suggested a stricter social assay. Two animals receive independent lessons, the companion actually emits an answer from its own contacts, and the witness learns a sound that predicts which perspective explains that answer. One experiment then changes visibility and another changes who heard a statement. The resulting score measures this particular perspective-tracking task. [Verbal information and belief updating, Frontiers in Developmental Psychology](https://www.frontiersin.org/journals/developmental-psychology/articles/10.3389/fdpys.2026.1764377/full)

## What changes in one lifetime

An activation is a 32-bit set of object cells and a flag for an unresolved branch. Each of four observed relation fields holds five target bit planes, a visibility mask and a connectivity mask. The animal can propagate a pulse forward or excite every predecessor. Missing observations, observed empty connections and multiple predecessors produce different states. Two private views each cost 112 bytes. Shared observation, a partner looking alone, and testimony present different sensory contexts to a heritable write gate. The apparatus never supplies a concealed destination.

A spoken form recruits a field of two-bit contacts. There are 256 physical slots and an initial repertoire of eight directed primitives. Candidate connections can use an earlier relation once its competing interpretations agree on the same operation. The genome supplies the 16-entry transition table relating prior synaptic strength and new evidence to the next strength. It also governs competition, question choice, vocabulary replacement and the recruitment of newly settled prerequisites. Paths of up to 21 crossings fit in a register. A question can present a longer sequence of words. Sources with incompatible predictions remain jointly active.

One field holds up to eight relation words. A heritable replacement policy can reclaim a slot, taking account of use and surviving dependencies. A pending question protects the vocabulary while awaiting a reply. Its 184-byte record holds the observed graph, starting object and earlier paths that were available when the question was asked. The animal can look at a different world, be saved to disk, and still assign the reply to the original observation. Its acquired noun and verb contacts also accept a three-word spoken example with two grounded names. [State layout](../include/axion/concept.hpp) · [Learning and readout](../src/concept.cuh) · [CUDA sensory ports](../src/dialogue.cu)

## Interviews and controls

Breeding began from random values of the 96 relational gene bits. The selected line had 160 generations of 192 candidates, six worlds per candidate and three cycles per world. In the final independent audit no individual reached the maximum aggregate score. The selected genotype was reproduced from the original seed. [Breeding run](../journal/runs/00004936-infer/plate.webp) · [Intervention counts](concepts/nursery.csv)

| Held-out test | Intact | Matched intervention |
| --- | ---: | ---: |
| Transfer a newly taught two-crossing word | 749 / 768 | 0 / 768 without demonstrations |
| Reuse it to learn a four-crossing word | 715 / 768 | 90 / 768 with reuse cut |
| Select the informative object among four | 762 / 768 | 207 / 768 with a random choice |
| Predict the companion's perceptual access | 6,144 / 6,144 | 3,072 / 6,144 from the witness's own view |

In the public interview, eight pairs grew up with independently shuffled names, nonsense verbs and directed physical relations. Demonstrations used only the even-numbered source objects. The evaluator asked about odd-numbered objects on new maps. It also compared affirmative and negative claims, twelve-word compositions, private testimony, and all four combinations of visual access. A separately implemented Python traversal supplied the expected answers. [Initial interviews](concepts/interviews.csv) · [Rearing and scoring](../tools/concept-study.py)

| Teaching condition | Correct answers | Further examples requested by the learners |
| --- | ---: | ---: |
| Paired pointing examples | 2,194 / 2,240 | 0 |
| Same examples and an opportunity to ask | 2,240 / 2,240 | 9 |
| Three-word spoken examples and an opportunity to ask | 2,240 / 2,240 | 9 |

All 46 initial errors were “unsure” in one dialect. Two four-crossing interpretations still fit the witness's recent examples. Its eight-crossing word had consequently developed without that prerequisite. Additional answers distinguished the alternatives and the later field could recruit the newly available contacts. The test questions and their answers were withheld from the additional lessons. [Clarified interviews](concepts/clarified.csv) · [Questions the animals actually asked](concepts/questions.csv) · [Spoken teaching](concepts/spoken.csv)

In 16 separate confounded rooms the witness spontaneously asked about an informative object 16 times. Giving it four explicit choices also yielded 16 informative questions. Random choice yielded three. The display changed before every reply. The three arms subsequently gave 256/256, 256/256 and 66/256 correct destination names. All 48 split-session checkpoints reproduced the uninterrupted individual's bytes and utterances. [Delayed inquiry and controls](concepts/inquiry.csv)

For the released eight-crossing word, an exhaustive text interview covered all 1,296 reversible three-object maps and all three possible sources. It answered 3,888/3,888. The best of all 72 fixed one- and two-crossing alternatives answered 2,592/3,888. A separate GPU census applied the acquired word to all 531,441 four-field functions on three objects, including many-to-one links. Every one of its 1,594,323 destination sets and gap flags agreed with an independent reference traversal. The inverse word also matched on every query, including 472,392 empty and 413,343 ambiguous outcomes. [Public interview and all competing routes](concepts/finite.csv) · [Complete graph censuses](concepts/census.csv)

## Food and learning across generations

Thirty-two residents share an island with 32 locations, four currents and four replenishing food reserves. A tide changes the connectivity every day. In the default habitat, harvesting also changes the next tide. An individual can pay for a physical demonstration or trap a particular destination. A correct trap can lose to another resident when food runs out. Energy deposited in clutches determines parentage. Birth clears acquired words, learned pathways and personal observations. Varying circuits for exploration, contact updates and motor gating survive or disappear through reproduction.

In the mentoring condition the parent that deposited the most energy on an island can continue as a teacher. It observes the day's chart and answers through its own acquired contacts. A receptive newborn hears a place name. The elder can be silent or wrong. The first generation has no elders. The displacement control brings an elder from a different island to see the same chart. [Physical currents and learning](../src/estuary.cuh) · [Settlement and energy conservation](../src/commons.cuh)

The following totals pool the final season of eight seeded populations of 256 newborns each. The clock-driven tide held map sequences matched between interventions. Every season's energy budget, food budget and count of living decisions were checked on the normal execution path. [Every measured season](concepts/seasons.csv)

| Condition | Last generation reached | Right destination / trap placements | Social lessons | Energy deposited |
| --- | ---: | ---: | ---: | ---: |
| Local mentors | 8 | 115,750 / 117,989 | 5,569 | 225,252 |
| Silent | 8 | 114,667 / 116,333 | 0 | 224,256 |
| Displaced mentors | 8 | 114,971 / 115,356 | 393 | 224,244 |
| Reuse cut | 1 | 497 / 16,412 | 0 | 0 |
| Learning frozen | 1 | 481 / 15,976 | 0 | 0 |
| Parentage by drift | 8 | 49,562 / 56,779 | 6,361 | 164,304 |
| Local mentors and within-season reversals | 8 | 40,398 / 74,851 | 4,619 | 70,056 |

Local mentors had a small reproductive advantage over silence in seven of eight seeds. Reuse and learning had much larger effects. Voluntary inspection alone supported reproduction. A separate 12-generation run with harvest-dependent tides produced an exportable resident. Asked about the last observed chart, it said “delta” for the nesting destination of pearl and “swift” for its feeding destination. [Resident](../examples/tidepool.mind) · [Season plate](../journal/runs/00004887-commons/plate.webp) · [Resident interview](../journal/runs/00005081-talk/plate.webp)

## Blackwell execution

One CUDA warp updates a learner's two-bit candidate weights using two ballot instructions per block of 32 connections. Another warp constructs a whole island's chart. The direct and cooperative layouts produced identical exported minds and every ecological count in three three-generation experiments. Their median times per 64-day season of 1,024 residents were 5.180 and 2.513 ms, respectively. On the RTX PRO 2000 Blackwell laptop GPU, 1,048,576 independently acting newborns occupied 3,278,766,736 bytes of requested simulation buffers. A 32-day season took 523.940, 544.864 and 552.810 ms across three seeds. These CUDA event times cover simulation days. Startup, natal initialization, census, disk writes and image rendering happen outside that interval. [Layout equivalence and timings](concepts/layouts.csv) · [Scale measurements](concepts/scaling.csv)

A frozen single-word query on a fixed chart admits a smaller representation. A warp asks the full circuit about every possible source, then packs five label planes and a grounded-answer mask into 24 bytes. The mask preserves abstention. On 1,048,576 independently generated charts, all 33,554,432 possible endpoint queries matched the full readout in each seed. Across nine timed passes of 512 queries per chart, median throughput was **224.60 billion endpoint lookups/s**, compared with **1.60 billion/s** for direct traversal. Projection took a median of 11.19 ms. Each timed pass reuses the fields in registers and computes a per-chart digest. These figures describe motor endpoint lookup on unchanged maps. The living estuary continues to use the full plastic circuit. [Per-pass timings](concepts/throughput.csv) · [GPU projection and verification](../src/concept_batch.cuh)

## Reproduce

```sh
make -j4 CUDA_ARCH=sm_120 NATIVE=1
./axion infer --origin random --generations 160 --candidates 192 --worlds 6 \
  --cycles 3 --exposures 10 --probe-worlds 256 --seed 41609 --probe-seed 1184269 \
  --save pupil.mind --save-listener peer.mind
python3 tools/concept-study.py rear --speaker pupil.mind --peer peer.mind \
  --save-speaker witness.mind --save-peer companion.mind
python3 tools/concept-study.py interview --spoken --clarify 3
python3 tools/concept-study.py inquiry --seeds 1216019 1542853 1834021 2198659 --trials 4
python3 tools/concept-study.py enumerate
./axion infer --load examples/cartographer.mind --census circuit --law 01230123
./axion infer --load examples/cartographer.mind --census unloads --law 6
python3 tools/estuary-study.py ecology
python3 tools/estuary-study.py layouts --seeds 213097 437531 658741
python3 tools/estuary-study.py scale --seeds 213097 437531 658741 --max-residents 1048576
python3 tools/concept-study.py speed --seeds 213097 437531 658741
```

The body supplies a finite repertoire of directed relations, competition, two perspectives and a constrained acoustic interface. Word meanings, candidate strengths, reading gates and questions can change. The simulation supplies the primitive operations, action costs and world physics. The witness was silent when asked why ice floats or what to do if it rains tomorrow. Sustained open-ended invention, general dialogue and human-level cognition remain unestablished. [Unrestricted prompts](../journal/runs/00005080-talk/plate.webp)
