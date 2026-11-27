# The unopened inlet

A forager can make a prediction about an inlet it has never touched, say that prediction aloud, and answer a follow-up after someone diverts the water. In a three-seed test, a single watershed-born pupil correctly voiced **2,304 of 3,072** first-reach predictions from its own saved neural state. It gave no forecast in the other 768 encounters. When it had practiced all four inlets before the watercourse changed, the same pupil gave only **788 correct forecasts in 3,072**. This is a small, measurable form of inference, with a reproducible failure under interference. [Interview results](inlet-inference/speech.csv)

## Try the pupil

```sh
./axion talk --speaker examples/inlet-speaker.mind --listener examples/parallax-witness.mind < examples/inlet-conversation.txt
```

At the table the animal took 24 paid reaches at three inlet addresses. The fourth was still unopened. This exchange comes from its acoustic output.

| Moment | What the person asked or did | What the animal said |
|---|---|---|
| Before release | Where will amber go? | north |
| Before release | Where did it go? | where did amber end? |
| After turning the outlet and releasing the marker | Where did it go? | east |
| Same event | Where did amber start? | east |
| Same event | Why did you choose that outlet? | Silence |
| Same event | What will the world look like tomorrow? | Silence |

With identical initial contacts, motor draws, conversation and practice, leaving the coupling in place produced **north** as the outcome answer. In the diverted encounter the animal spent one unit to commit, missed the food and retained its prediction long enough to talk about the discrepancy. The ordinary text interface and the paired GPU examination both use the pupil's acquired acoustic contacts. The human-readable words are drawn from the small vocabulary it learned. There is no language model in the exchange. [Both physical transcripts](inlet-inference/conversation.txt) · [Playable commands](../examples/inlet-conversation.txt)

## What the animal inherits and learns

Four sensory addresses each have four motor contacts. A reach provides one bit of tactile evidence from the chosen mouth. Two spiking cell populations, represented by two 16-bit masks, gate changes under inherited Boolean programs. The underlying somatic cortex can grow or shrink in 32-cell bands. Development, receptors, module allocation, the hand and its plasticity still mutate during births.

The extension gives a motor cell access to the corresponding contact in the three other sensory rows. The circuit can use any Boolean function of those three contacts, any two-input function to change the local opening, and a five-bit inherited table that can invert the distal response at each population count from zero to four. All 17 bits already fit in the animal's existing germline. An empty readout falls back to the local contact row. Choosing which inlets drain to the same mouth is a property of water, fibre and clay. The readout receives contact states. Holding the afferent columns fixed while permuting the hidden pipes changes the correct reach.

The architecture supplies four addresses, aligned candidate contacts, finite gene encodings and a finite family of question meanings. Which contacts close or reopen, whether the other rows matter, which somatic band controls a hand or receives speech, and which sound refers to an object are variable. The code still supplies the laws of the world and the representational bounds. [Device equations](../include/axion/drift.hpp) · [Withheld-inlet protocol](../include/axion/inlet.hpp) · [Somatic development](neurogenesis.md)

## What selected the behavior

A sample of 32,768 living residents of the million-site estuary contained eight copies of a circuit that could use aligned contacts immediately. This stock predates the new connection. Across three subsequent held-out panels the circuit made **6,144/6,144** correct first reaches at the untried inlet of a reversible watercourse. Reading only the local row yielded **1,536/6,144**. Rotating the distal contact columns yielded **0/6,144**. On irregular, partly one-way seepage the original circuit scored **1,418/6,144**. Some hidden outlets genuinely admit several explanations after the available touches. The independent evaluator enumerates all 256 four-inlet maps to identify such ambiguity in unrestricted beds. [Paired panels](inlet-inference/panels.csv) · [Reversible layouts](inlet-inference/twin-exchange.tsv.gz) · [Irregular seepage](inlet-inference/twin-seep.tsv.gz)

In a separate nursery, each animal ate only what its actual reach intercepted. Within neighborhoods of 32, eight units of swallowed carbon paid for each child. Random parentage used exactly the same food pools, brood limits, environment draws and mutation machinery. It changed only which local adults furnished the genes. No test score or English answer funded reproduction. A seed used to select a circuit was held apart from its confirmation seed. Four outlet rotations share a layout and two physical interventions share a history. The layout is the unit of replication for paired observations. [Carbon ledger](inlet-inference/birth-carbon.csv) · [Generation histories](inlet-inference/broods.csv)

| Condition | Generations | Correct held-out first reaches for the three food-selected circuits | Uniform-parent controls |
|---|---:|---|---|
| Mixed channels and common drains, calm initial tissue | 384 | 512, 512, 512 of 512 | 140, 374, 136 of 512 |
| Same task, initial spiking state sampled from living animals | 512 | 512, 510, 512 of 512 | 130, 148, 136 of 512 |
| Real prior contacts, all four old routes trained, three new routes available | 768 | 606, 644, 824 of 1,024 | 278, 292, 356 of 1,024 |

The third material circuit recovered the correct new outlet on **530 of 636** trials where the held row's former contacts actively excluded it. With the same tissue and trials, the local motor draw was correct in 256 of 1,024. An evaluator reconstructs the old layout, new layout, chosen mouths, taste pulses and the two banks of spiking cells from the receipt. [Interference replay](inlet-inference/earlier-four.tsv.gz) · [Scalar audit](../tools/drift-audit.py)

## The speaking body

The public pupil was born at slot 10,969, tick 514, in a 128 × 128 experimental colony. Its parents had received a motor program from a 512-generation food-selected population. A controlled developmental duplication preserved a resident acoustic program on another already-expressed band. Both assignments then passed through ordinary ecological reproduction. The pupil had 64 expressed somatic cells and four inherited module genes. A prospective observer chose it at age four because three other inlet reaches changed its prediction. On that qualifying encounter, the food emerged at a different outlet than the animal predicted. The lesson provided eight spoken names and physical examples of eleven question forms, including cases where the intermediate crossing, committed reach and observed result differed. Returning the acquired language to the same living body preserved its original field contacts. The supplied genome and curriculum are experimental interventions. [Cell populations](inlet-inference/anatomy.csv) · [Witness measurements](inlet-inference/witnesses.csv) · [Teaching session](../examples/inlet-lessons.txt)

| Same 3,072 encounters unless indicated | Correct motor reaches | Forecasts spoken | Correct forecasts | Both subsequent outlets named |
|---|---:|---:|---:|---:|
| Taught pupil, own hand and spiking state | 2,304 | 2,304 | 2,304 | 2,304 |
| Same pupil with fresh motor contacts and spikes | 3,072 | 3,072 | 3,072 | 3,072 |
| Same birth, training through a shared motor and vocal band | 2,304 | 0 | 0 | 0 |
| Same birth before English lessons | 2,304 | 0 | 0 | 0 |
| Taught pupil, all four previous routes practiced | 788 | 3,064 | 788 | 3,064 |
| Another birth from the interference nursery, all four previous routes practiced | 1,972 | 928 | 894 | 928 |

The last pupil, slot 7,451, was tested on its own three held-out seeds. It named the original location on all 6,144 releases. A second pupil from the interference nursery produced zero forecasts on the same tests. All four pupils in the original transfer study scored above chance on the mixed material task when the distinct vocal band and inlet curriculum were available. Their carried-state forecast accuracies ranged from 1,048 to 1,536 of 2,048. The simpler float lessons gave the public pupil zero forecasts. Simply placing a strong learning program in another body is an unreliable way to transfer two skills. [All controls and all tested births](inlet-inference/speech.csv) · [Second pupil](../examples/inlet-relearning.mind)

A separate CUDA launch seals the acoustic reply before the code permits the withheld outlet to open. Each sealed state is then continued with the natural release and with the sleeve physically rotated. The public pupil's original 2,048-course test gave 1,536 correct forecasts, zero premature outcome answers and zero blank-scene replies. It named both subsequent outlets in 1,536 cases. Erasing the conversational topic suppressed every outcome follow-up. Keeping only its local contacts preserved 4,096 correct source names while eliminating all forecast answers. Misaligning the inherited distal readout left two correct spoken forecasts. [Sealed speech and both releases](inlet-inference/separate-bands.tsv.gz) · [Matched shared-band pupil](inlet-inference/shared-band.tsv.gz)

## In the watershed

The physical update uses two stages of disjoint neighbor exchanges on a shifting square lattice. Water head, clay and bound fibre can create a one-way seam. A raised bank can retain existing food until the combination of local water, stored stock and tidal head overtops it. An animal can tie or remove fibre with its existing limb, using real material and energy. Shallow water can carry a trickle. Closing a bank preserves its contents for later pulses. These local laws act on all animals and can survive the maker's death.

In 1,536 ticks in the million-site estuary, water overtopped a bank on 10,711,878 pulses, released 46,332,430 units of food in those events and supplied 1,285,587 swallowed units. Holding rock, water, clay and stock fixed, removing bound fibre would have opened 18,642,462 of the observed held pools at the same site. These are repeated visits. That experiment establishes a physical effect of construction. Intentional dam engineering remains an open behavioral question. [Mass balance and controls](inlet-inference/pools.csv) · [Hydraulic receipts](inlet-inference/wild-sills.tsv.gz) · [Inheritance of altered seams](inlet-inference/wild-works.tsv.gz)

Unassisted use was much weaker than nursery performance. In an ordinary full estuary, 21,490 of 86,658 lifetime first reaches after three other inlets were correct. Local memory and the identical draw gave 21,432. Requiring the animal to have stayed on the same unchanged bed gave 13,802/55,539 against 13,798/55,539. The initially present adults were excluded from these birth-history counts. In genetically seeded reciprocal competitions the distal-access lineage finished ahead in 16 of 24 assignments. Both labels could use their local memories, and the winning label depended on the seed and physical setting. [First touches from birth](inlet-inference/first-contacts.csv) · [Both competing lineages](inlet-inference/lineages.csv)

Three populations founded from random germlines then lived without experimental donors for 65,536 ticks. Each first evolved to tick 8,192, when the complete habitat was forked with its existing contacts intact. One arm kept distal motor access. The other arm could still learn locally. At ticks 32,768 and 65,536, both populations were given the same distal pathway for the examination, the same held-out physical layouts and initial states sampled from their own residents. At the last examination their abundance-weighted first-reach accuracies were **25.225%, 25.003% and 25.069%** with distal access during life, against **25.007%, 25.000% and 24.924%** for their locally reared counterparts. One locally reared population briefly contained a selected circuit that scored 152/256 at tick 32,768 even as its population average remained 25.07%. Ecological reproduction has maintained considerable body diversity while leaving reliable inference rare. [Natural population histories](inlet-inference/wild-growth.csv) · [All shared-environment examinations](inlet-inference/panels.csv)

The released million-site estuary is at tick **396,952**, with **357,579** living animals, **22,981,280** expressed spiking cells and **22,550** body forms distinguished by anatomy. The eight most common forms account for 9.2% of living residents. Each 4K card uses the group's real limbs, eyes, feeding pores and cell-count range. Its sixteen-square inset is the oldest member's actual inlet-contact mask. At this census all eight representatives had an empty mask. These portraits measure current individual tissue, while the journey records demonstrations across the broader project. [Current species atlas](../journal/creature-card.webp) · [Landscape journey](../journal/journey.webp) · [Closing field plate](../journal/runs/00011094-floodplain/plate.webp)

## GPU cost and reproducibility

The normal ecology advances entirely on CUDA. The extension needs **zero extra persistent bytes per resident**. The optional first-reach recorder uses 24 bytes per sampled site plus sharded counters. The bulk nursery stores 16 bytes per germline. Its isolated 4,096-resident case-table kernel assigns a warp to a distinct genotype and reduces across paired courses. Surveying live tissue gathers at most 2,048 actual motor bands directly on the device, including on newly founded worlds.

On the RTX PRO 2000 Blackwell laptop, the released storage kernel uses **104 registers and zero local bytes per lane** without optional visual and ancestry recorders. A packed maximum stores the rim height and the rock-dependent tie break in one register. Byte permutation gathers the four inherited truth-table outputs on Blackwell. Exhausting all 1,048,576 combinations of an eight-bit table and three four-bit rows returned identical bytes on both the native path and the portable CUDA path. Four alternated paired benchmarks showed a small difference between logic and byte permutation. The two-register saving and the absence of local memory were more consistent than any end-to-end throughput advantage. Three final 192-tick samples gave median device intervals of 439.52–440.35 µs per 262,144 candidate tiles and **54.57–54.87 million site-ticks/s** for the full ecosystem. A site-tick advances one habitat site through one simulation tick. [Hardware and workload figures](inlet-inference/performance.csv) · [Replay and regression record](inlet-inference/validation.json)

```sh
zstd -d examples/inlet-colony.flood.zst -o pupil.flood
./axion floodplain --load pupil.flood --steps 0 --slot 10969 \
  --inlet-trials 1024 --inlet-landscape mixed --inlet-state carried \
  --inlet-prior 24 --inlet-practice 24 --class-seed 73013 \
  --inlet-questions examples/inlet-questions.txt --inlet-voice-trace voices.tsv
python3 tools/drift-audit.py --inlet-voice voices.tsv
python3 tools/drift-audit.py --inlets docs/inlet-inference/earlier-four.tsv.gz

zstd -d examples/inlet-estuary.flood.zst -o estuary.flood
./axion floodplain --load estuary.flood --steps 2048 --report-every 512 \
  --brain-file compact --save next.flood
```

### Repeat the birth and teaching

The food-selected germline can be regenerated from the older wild estuary. Changing the parent selection to `uniform` runs its matched reproductive control. For the earlier-four-inlet challenge add `--inlet-old-inlets all --inlet-history living`, set 768 generations and use the seeds in the generation table. For a specific talking body, `--inlet-state carried` sets the initial private contacts and spiking state in the prospective interview. `--inlet-background` and `--inlet-history` govern the population assays.

```sh
zstd -d examples/drift-estuary.flood.zst -o source.flood
./axion floodplain --load source.flood --steps 0 --drift-lateral contextual \
  --inlet-landscape mixed --inlet-trials 256 --inlet-practice 24 --inlet-prior 24 \
  --inlet-generations 512 --inlet-cohort 4096 --inlet-residents 32768 \
  --inlet-selection food --inlet-background living --inlet-bolus 96 \
  --class-seed 953713 --inlet-germ germ.txt
./axion floodplain --side 128 --density 100 --neurons 256 --neurogenesis inherited \
  --seed 34159 --drift tidal --drift-stores sills --drift-crossing seep \
  --drift-lateral contextual --drift-load 64 --drift-stride 1 --drift-sight own \
  --drift-stock examples/drift-student.mind --steps 0 --save coast.flood
./axion floodplain --load coast.flood --receive-donor germ.txt --receive-count 16384 \
  --receive-ports hand --receive-architecture branch --steps 512 --save parents.flood
./axion floodplain --load parents.flood --steps 256 --drift-first 16384 --drift-sight own \
  --watch-drift catchment --drift-witness-sample carriers --trace-donor germ.txt \
  --drift-look 4096 --save natal.flood --export-mind child.mind
./axion talk --speaker child.mind --listener examples/parallax-witness.mind \
  --save-speaker taught.mind < examples/inlet-lessons.txt
./axion floodplain --load natal.flood --steps 0 --follow-witness child.mind \
  --return-lesson child.mind taught.mind --export-mind speaker.mind
```

## Research basis

Pan and colleagues found that people used relational knowledge to infer an unexperienced transition after a change of context. Perturbing hippocampal activity affected that behavior. This motivated the held-out-address intervention, the matched distal disconnection and the test with obsolete old routes. Axion implements a candidate computation with mutable Boolean tissue. The proposed computational link remains a hypothesis. [Nature Human Behaviour, November 16, 2026](https://doi.org/10.1038/s41562-026-02627-w)

Recent mouse work measured dimensions of hippocampal activity that generalize across conditions, while human single-neuron recordings distinguished populations carrying content and context. Those observations motivated separate controls for outlet labels, physical layout and cortical routing. Testing for corresponding representations in the simulated pupil remains an open experiment. [Nature Neuroscience, October 27, 2026](https://doi.org/10.1038/s41593-026-02491-x) · [Nature, distinct neuronal populations for content and context](https://doi.org/10.1038/s41586-025-09910-2)
