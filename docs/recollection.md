# Remembering what moved out of sight

Place food to the west, let the witness see it, cover its view, and move the food. The direction it reports now depends on its own history. A newly hatched witness stays silent if its first inspection is covered. The location stored in its contacts survives a save and reload. [Witness checkpoint](../examples/witness.mind)

```text
/scene west north
Where is the food?
animal> west
/cover 1
/scene east north
Where is the food?
animal> west
/uncover 1
Where is the food?
animal> east
```

## The circuit

Four proprioceptive assemblies and four retinal cells meet at sixteen two-bit synapses. Four acquired bytes store all the synaptic strengths. Twelve inherited bytes specify two Boolean transition tables and the expression of sensory wiring, thresholds, ambiguous firing and turnover when the retina goes dark. Each synapse observes its own state, the local presynaptic and postsynaptic spikes, and one physical bit of illumination. The CUDA kernel updates all sixteen contacts together in two bit planes. Nothing writes an object’s world coordinates to these synapses. A covered look can reinstate a pattern of firing that enters the same spiking motor circuit used for visible locations. [Contact dynamics](../src/recollection.cuh) · [Spiking sensory pathway](../src/dialogue.cu)

In a human navigation experiment, 7-T fMRI associated subicular responses with binding events to locations and times. An independent human EEG experiment distinguished shifts of attention to an external stimulus from shifts to a remembered item. These findings motivated separating the external scene, the retinal event and the internally reinstated activity in this experiment. The small Boolean circuit is an engineering hypothesis. [Shin, Bingman and Lee, 2026](https://doi.org/10.1038/s41467-026-79267-1) · [Dissociating external and internal attentional selection](https://pmc.ncbi.nlm.nih.gov/articles/PMC12005331/)

## Evolution and tests

Six independent populations started with transition tables containing only zero bits. Each ran 256 generations of 256 candidates across eight environments per generation. Recombination and mutation could change both tables and the expressed wiring. Selection used the weakest of five behaviors together with their sum. In six matched drift populations, parents were sampled independently of fitness. Each reported champion was the strongest measured animal in its final population. The renderer journals every run. [CUDA environment and evolution](../src/recollection.cu) · [Study driver](../tools/memory-study.py)

| Covered probe | Selected cohorts / 36,864 | Drift cohorts / 36,864 | Fixed-interval witness / 98,304 |
| --- | ---: | ---: | ---: |
| After one clear sighting | 36,864 | 31,023 | 98,304 |
| After intervening events in other views | 36,864 | 9,272 | 98,304 |
| After one conflicting dim sighting | 36,864 | 9,939 | 98,304 |
| After a clear relocation | 36,864 | 27,069 | 98,304 |
| After four consistent dim sightings | 36,864 | 19,576 | 98,304 |

The fixed-interval witness came from the first tied cohort. Its tests used eight new seeds, 512 worlds per seed and 64 intervening observations. It also stayed silent on all 16,384 probes of previously unseen items. Across all six selected cohorts the corresponding figure was 5,632/6,144. Freezing the newborn contacts eliminated all five learned behaviors in the fixed-interval animal. Inverting the brightness signal or always repeating the last sighting in each view gave zero correct reports after a conflicting dim sighting. [Selection](recollection/cohorts-select.csv) · [Drift](recollection/cohorts-drift.csv) · [Held-out interventions and baselines](recollection/held-out.csv)

A separate interview test of that first witness changed the physical scene and sent actual questions through the two saved individuals. Across eight fresh streams the witness reported the former bearing on 64/64 hidden relocations. It retained it on 64/64 after 32 intervening questions about the other resource and on 64/64 after a conflicting dim observation. A clear relocation and four consistent dim observations each changed the next covered answer on 64/64 trials. In two matched streams with plasticity frozen the witness answered all 512 intervening visible questions correctly and was silent on all 16 post-delay probes. These measurements establish source-sensitive retention for a small set of cues. Autobiographical memory and general language understanding remain untested. [Spoken trials](recollection/spoken.csv) · [Frozen contacts](recollection/spoken-fixed.csv) · [Crossed cues](recollection/spoken-crossed.csv) · [Inverted clarity](recollection/spoken-source.csv) · [Erased recall](recollection/spoken-erased.csv)

## From a fixed delay to a verified memory

A four-turn interview exposed a failure hidden by the original test. The witness remembered food behind a shutter, answered one question about water, then stayed silent when asked about food again. On 24,576 tests with randomized intervals it retained the clear sighting only 12,147 times. Breeding with intervals of zero to fifteen observations and additional shuttered inspections produced five perfect sampled champions in six cohorts. The first of these scored 98,304/98,304 on all five larger tests, including intervals up to 127. Exhaustive exploration still found violations after three events. [Original animal](../examples/fixed-interval.mind) · [Variable intervals](recollection/fixed-on-varied.csv) · [Second animal](../examples/variable-interval.mind) · [Variable-interval cohorts](recollection/varied-selection.csv) · [Held-out tests](recollection/varied-holdout.csv)

Each of the four contacts in a cue row has two state bits. A CUDA block can therefore explore the entire 256-state transition graph for one cue. The other rows cannot feed back into that row. The checker includes signals that share its inherited sensory address and signals at different addresses. For every newborn-reachable state, it considers every bearing, both illumination levels and covered inspections. It closes each obligation under arbitrarily many unrelated looks and covered inspections. A failure reports its shortest event depth, expected activity, actual activity and contact state. The guarantee applies to this visual circuit and these four claims. Hearing and speaking are tested separately. [GPU model checker](../src/memory_proof.cuh)

| Obligation | Checked state–claim combinations for the released animal | Violations |
| --- | ---: | ---: |
| Stay silent before a direct sighting | 1 | 0 |
| Retain a clear observation until new direct evidence | 4 | 0 |
| Retain it through one conflicting dim observation | 24 | 0 |
| Adopt four consistent dim observations | 12 | 0 |

Two of six populations bred from zero transition tables passed all four checks. Each population contained 384 candidates and ran for 640 generations. Selection used the proportion of reachable obligations that held, with extra weight on the weakest condition. It evolved the synaptic program anew. The released animal came from seed 61487 and has 61 reachable configurations per cue row. One of four populations started from the variable-interval elite also reached zero violations. [Independent populations](recollection/invariant-proofs.csv) · [Transfer populations](recollection/invariant-transfer-proofs.csv) · [Release certificate](recollection/released-proof.csv)

On eight unseen seeds the released animal answered every one of 24,576 questions in each of five conditions, with up to 1,024 intervening observations. In the public interview it retained the location on 96/96 hidden relocations, 96/96 variable delays and 96/96 conflicting dim sightings while answering all 5,809 intervening questions about the other view. Freezing the contacts preserved 1,592/1,592 visible answers and eliminated all 24 post-delay answers in two matched streams. [Held-out worlds](recollection/released-holdout.csv) · [Spoken reports](recollection/released-spoken.csv) · [Frozen contacts](recollection/released-fixed.csv)

```sh
./axion recollect --load examples/discourse-speaker.mind --origin inert --objective invariant --generations 640 --candidates 384 --seed 61487 --save my-witness.mind
./axion recollect --load my-witness.mind --generations 0 --probe-worlds 256 --cycles 12 --delay 1024
./axion talk --speaker my-witness.mind
python3 tools/memory-study.py interview --speaker my-witness.mind --cycles 12 --delay 127
```
