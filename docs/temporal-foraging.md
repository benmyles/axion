# Learning from a meal that arrives later

A creature sees a pod, chooses one of four grasps and can change the surface that another animal will encounter. Usable energy or husk returns after a variable digestive delay. Several earlier meals can mature together. At the moment of absorption the circuit receives a nutritional signal. Its own recent activity is the only record linking that signal to past choices.

```sh
./axion floodplain --load examples/shellbank.flood --steps 512 --pod-assay 384 --pod-remap 768 --pod-bench 4194304
./axion talk --speaker examples/shell-reader.mind
```

In the interview enter the following on separate lines. The shell-reader learned eight fruit names from ambiguous scenes. Here the names depend on three light channels, and the appropriate grip depends on three different channels. Swapping locations or changing the low bits of a fruit changes what the animal has to do.

```text
/tidalpool 2971
/ripen 768
/tabletop 00@0>0 09@3>3
/grip-word plum
/turn 1
/handle acorn 64
/grip-word plum
```

The three-bit fields of `09` identify the named class *plum* and mechanical class 1. The text interface passes the heard acoustic pattern to the animal's learned visual circuit. A unique referent supplies its actual surface spectrum to the motor circuit. The output is a muscle index. The tray withholds a choice when no unique referent is available. `/grip` observes without feeding, and `/offer` or `/handle` can change the animal. Saving the speaker retains its acquired tissue and pending meals.

## Design and research

A human experiment with 142 participants found that the information available during feedback changed how well alternative models of temporal credit predicted later choices. We used delayed unlabeled outcomes, intervening grasps, an independent-feedback control and a separate replay of successful action histories to distinguish dependence on outcomes from simple repetition. A November 2026 computational prism-adaptation study compared reusing motor policies with gradually learning a new one. In Axion four motor frames share evidence across different surfaces. A trial can transplant the selected frame into the very same animal's earlier tissue. These are design hypotheses, with empirical controls below. [Maulhardt, Solway and Charpentier, 2026](https://journals.plos.org/ploscompbiol/article?id=10.1371/journal.pcbi.1013879) · [Transferring active inference to newly encountered prism-shifted environments, 2026](https://www.nature.com/articles/s42003-026-11152-5)

The 176-byte feeding organ contains 28 inherited bytes, 256 two-bit temporal contacts, two populations of lag-support integrators, eight efference copies, a four-frame motor circuit, a 32-byte digestive reservoir and a motor emission. Genes choose three sensory wires, synaptic transitions, receptive timing, exploration, muscle routing, consolidation and a motor gate that can space out bites. Some founder routes follow the four-muscle ring. Point mutation and duplication can leave that geometry. The engineered `--pod-origin reference` is available as a positive control. The studies below began with random feeding genes.

The fast nursery places 32 animals on each finite feeding raft. Unconsumed food drifts, sunlight restores stock and each embryo costs its parent 64 energy units. The fostered control exchanges the germline attached to a provisioned egg. Day-by-day grasping, digestion, growth of the feeding contacts, reproductive sampling, mutation and dispersal execute in CUDA. The rafts evolve the feeding germlines. The receiving floodplain also evolves bodies, somatic spiking networks, producers and physical habitat.

## Temporal credit and ecological selection

Four independent populations evolved for 1,024 seasons of 512 feeding opportunities with 16,384 available juvenile places each. For each population, 1,024 new juveniles faced independently assigned opening rules. They practiced on one half of the nuisance-light patterns and the tests used the other half. Every second phase changed the physical opening rules.

| Seed | After practice | After reversal | With another animal's feedback | After erasing contacts |
| --- | ---: | ---: | ---: | ---: |
| 1877 | 58,992 | 50,234 | 16,183 | 16,410 |
| 5569 | 61,892 | 61,016 | 16,590 | 16,498 |
| 8123 | 61,383 | 59,834 | 16,783 | 16,305 |
| 31139 | 54,715 | 54,472 | 16,313 | 16,338 |

Each column has 65,536 probes per seed. The four fostered populations scored between 15,965 and 16,311 after practice. Imposed random motor practice, varying displacement of the neural credit signal and giving a shadow learner the intact animal’s successful motor history supply additional controls. In sixteen direct ecological competitions using reciprocally placed pairs of identical germlines, every frozen-learning cohort died out by tick 2,048. Under neutral labeling, swapping the otherwise identical cohorts exchanged the final counts exactly. [Counts and controls](pods/raft-controls.csv) · [Neutral placements](pods/neutral-gardens.csv) · [Reproduce the nursery and release](../tools/pod-study.py)

## Inferring a shared physical change

Each juvenile initially encounters all eight fruit types. Then a tide rotates the appropriate muscle for all fruits by a hidden angle. During its next 64 opportunities the animal can handle only one fruit class. We test the seven untouched classes with new high-bit appearances. A foil world gives identical evidence on the observed fruit while independently changing the untouched fruits. This is a structured transfer problem with a four-choice chance rate of 25%.

| Rearing seed | Steady ripening | Brief mass fruiting | Steady feedback from a neighbor | Transplant of the steady animal’s chosen frame |
| --- | ---: | ---: | ---: | ---: |
| 1877 | 59.9% | 17.7% | 20.4% | 59.7% |
| 5569 | 40.6% | 66.2% | 14.3% | 40.5% |
| 8123 | 11.4% | 43.9% | 6.7% | 11.4% |
| 31139 | 63.9% | 55.7% | 19.0% | 63.8% |

Each score covers 57,344 untouched-fruit choices. Each cohort first evolved temporal learning for 1,024 seasons, then faced 2,048 shared-tide seasons with variable, heritable motor frames. In the flush ecology one pioneer fruits first and the other seven briefly deliver a richer harvest. Changing the ecology improved some lineages and harmed others. The independently changed fruit foil and the fostered lineages stayed near chance. A preliminary one-stage lineage showed zero self-directed frame switches and scored 5,080/57,344. The ability to earn food in one environment provides limited evidence about responding to a new hidden change. [Steady rearing](pods/frame-controls.csv) · [Flush rearing](pods/flush-controls.csv) · [Transfer study](../tools/frame-study.py)

The retained pilot brood also underwent four new common gardens, each counterbalanced across spatial placements. Disabling only the shared frame left both competitors viable, with 13,256 intact and 12,529 temporal-only survivors pooled across the eight gardens. Freezing acquisition throughout the feeding circuit gave 25,877 intact survivors and zero frozen survivors. Neutral competition yielded 12,818 on each side. This distinction matters when interpreting what supports life in the watershed. [Reciprocal ecological interventions](pods/frame-gardens.csv)

The pilot brood used seed 5569 for temporal acquisition and seed 1877 after duplication into motor frames. These commands reconstruct its rearing schedule and release.

```sh
./axion floodplain --neurogenesis fixed --seed 5569 --pods sealed --pod-frames single --pod-regimes independent --pod-pulse enteric --raft-seasons 1024 --raft-capacity 16384 --raft-days 512 --steps 0 --save parents.flood
./axion floodplain --neurogenesis fixed --seed 1877 --colonists parents.flood --pods sealed --pod-period 48 --pod-regimes shared --raft-frames diverse --raft-seasons 2048 --raft-capacity 16384 --raft-days 768 --steps 0 --save brood.flood
./axion floodplain --load brood.flood --steps 384 --save released.flood --export-mind adult.mind
python3 tools/grip-interview-study.py --adult adult.mind --origin random --generations 48 --speaker pupil.mind --output words.csv
```

## A named fruit in a real animal’s hands

The highest-energy resident of the pilot population at wild tick 384 supplied the retained speaker. Its original lexical genes produced zero resolved probes in a preliminary ambiguous naming lesson. A separate 48-generation visual-word selection and 1,024 ambiguous exposures gave the same resident a functioning receptive vocabulary. The sensory names use channels 3–5. Opening a pod depends on channels 0–2. Each of the eight names can therefore refer to any of the eight mechanical classes. The trial uses the ordinary interactive commands and an independent physical answer key.

Across 24 fresh trays the animal located the intended fruit on all 4,032 named probes and abstained from all 24 unknown words. On the seven untouched mechanical classes it chose correctly in 368/1,344 probes before the tide, 288/1,344 immediately after and 376/1,344 after handling one named class. Word recognition was considerably stronger than the individual's physical transfer. The trained word circuit and evolution of the motor germline occurred in separate rearing stages. Open-ended conversation and child-level learning remain unverified. [Every tray and phase](pods/spoken-grips.csv) · [Original lexical control](pods/inherited-names.csv) · [Generate the naming lessons and repeat the interviews](../tools/grip-interview-study.py)

## GPU deployment

A frozen feeding circuit compiles into 24 bytes covering every receptive class, basal motor input, exploration branch, tie and active crossbar. Online learning still uses its full writable tissue. At wild tick 4,480, 3,067 real residents supplied the deployment. The independent cellular interpreter and compiled output agreed on 2,097,152 combinations from 256 residents. On the RTX PRO 2000 Blackwell laptop, a 4,194,304-choice batch took 0.0780 ms compiled and 1.268 ms with writable circuitry, using interleaved medians of nine timed batches. The corresponding complete floodplain advance measured 60.3 million site-ticks/s at a reserved capacity of 512 cells per animal. Rates for feeding decisions, habitat advances and inference over an entire brain measure different work.

The closing habitat contained 3,067 residents across 637 body forms. Its descendants scored 12,465/57,344 on the untouched-fruit assay. Continued survival and selection in this habitat had eroded the earlier transfer performance. The v8 checkpoint carries somatic development, the acquired fruit circuit and the full material stocks. [Evolving spiking populations](neurogenesis.md) · [Saved habitat](../examples/shellbank.flood)
