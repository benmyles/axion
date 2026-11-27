# Food follows a channel a creature can change

In the brook, resident 12,236 changed a junction during beat 5,131. Two grains entered the same southeast deposit from different sides. Replaying that tile with the resident’s pressure change removed left only one incoming grain. The deposits and their exposed pigments are recorded before and after the beat. The animal received no reward for moving a grain. [Observed work by the brook population](food-transport/colour-bank-grains.tsv.gz) · [Individual material interventions](food-transport/events.csv)

The free-running, million-site estuary now has **304,530 residents and 19,560,736 active spiking cells** at tick **428,952**. The median brain has 64 spiking cells and the largest has 544. The most recent 1,024 ticks ran at **55.73 million site-ticks/s** on the Blackwell laptop. Its resume file contains the population, genomes, cortical state, moved food and pigments. [Continuing estuary](../examples/detritus-estuary.flood.zst) · [Complete comparison](food-transport/comparison.csv)

## How the food moves

A pressure beat divides the torus into disjoint two-by-two blocks. The block origin shifts each beat so a site interacts across all four neighbouring block placements. A wet donor with edible litter and an active pulse can send one existing portion along an interior edge. Water depth, clay height and fibre determine which exit offers a positive pressure drop. The northwest donor can choose east or south. Both middle donors can reach the southeast deposit. If only one portion fits, a deterministic local tie-break admits one donor and leaves the other grain in place. Incoming material becomes ordinary edible litter and is available to subsequent animals and other world processes.

One CUDA lane owns all four deposits for the beat. It reads the existing soil and pressure planes, computes the accepted edges from the initial stocks, and writes the resulting stocks in place. The physical routing step requires **zero new persistent or temporary bits per site**, no global material atomics and no second grid of food. On this GPU the ordinary kernel uses 38 registers per thread and zero local-memory bytes at 128 threads per block. The observed kernel uses 68 registers and zero local-memory bytes. Its optional instruments add a single bit per site and a bounded set of receipts. [Routing law](../include/axion/detritus.hpp) · [CUDA partition update](../src/flood_detritus.cuh) · [Driver-reported kernel attributes](food-transport/device.csv)

With pigment exchange enabled, the moving portions exchange the two-bit surface films already present on the visible carriers. The films return to source seats as the grains move downstream. Donor surfaces are transferred before accepting upstream grains during the same beat. The downstream tie-break fixes the order when two films touch the same deposit. Each tile conserves all four pigment counts. A footprint remains visible to existing optical circuitry until subsequent material processes disturb it. Intentional marking and learned use of these new footprints remain untested.

The design was prompted by the earlier finding that plentiful opportunities to handle materials did not sustain broad causal learning in the simulated population. El Mahmoudi and colleagues measured how two freely foraging marmosets changed their choices with repeated experience in an environment with depleting resources. That result motivated keeping resource location and depletion consequential across Axion’s generations and measuring the response. The artificial transport law is an engineering choice. [Earlier common-garden exam](capillary-learning.md#did-natural-selection-favour-transferable-learning) · [Primary foraging study, September 2026](https://doi.org/10.1016/j.isci.2026.117294)

## What changed in the populations

Three small watersheds each ran for 32,768 more ticks, and the million-site coast ran for 1,024. All animals could move, reproduce, die and forage through their existing nervous systems. Four habitat treatments controlled the physical food movement. The displaced treatment samples the same pressure field half a world away in both coordinates, preserving the number of firing sites on a given map while changing their relationship to the local terrain. The populations subsequently diverge, so the quantity transported also differs. No classroom term enters reproduction or survival.

| Starting population | Stationary food in the pressure pathway | Local transport | Displaced transport | Local transport with surface dye |
|---|---:|---:|---:|---:|
| Brook | 243 | 537 | 511 | 589 |
| Older watershed | 418 | 393 | 510 | 276 |
| Slow-pulse nursery | 586 | 429 | 619 | 400 |
| Coast | 0 | 4 | 2 | 1 |

Each entry counts how many of 2,048 sampled young survivors learned both full material laws across all 24 paired tests. The exam uses the same seed, fresh pressure contacts and scarce-stock workpieces throughout. The largest positive change occurred in the brook. In two other small habitats, adding surface dye produced fewer complete learners than their stationary-food controls. No consistent advantage across environments emerged. On the coastal dye branch, the cohort collectively earned 65 portions in 98,304 examination releases. This population result is much weaker than the specially selected guide’s performance in conversation and teaching. [All examination, biomass and timing results](food-transport/comparison.csv) · [Wild-born coastal individuals](food-transport/colour-coast-residents.tsv.gz)

In the coloured brook, the simulation registered **6,305,292 food transfers and 8,367,626 exposed site-colour changes**. Controlled one-beat replay found 392 directed transfers that depended on the pressure from a resident's material action. The replay holds the resulting physical bed and stock fixed and restores only that source's alternate pulse. A material change can affect geometry as well, so this measures the pressure-mediated effect. Summing over separately perturbed sources can count overlapping effects. The observations include uniform physical tiles and the most recent consequential receipt from each of a fixed set of watched tiles. Their frequencies have different sampling denominators. [Field receipts and selection effects](food-transport/transport.csv) · [Independent verification of the raw samples](food-transport/verified-samples.csv)

## A learner from the current estuary

Slot 681,449 was the sole coastal animal in the 2,048-member stained-food sample to learn both full laws on every exam pair. It was born in the estuary at tick 428,844 in generation 3,435. It had 64 active spiking cells, a five-input binding capacity and the same local OR update truth table as the brook guide. Their full inherited words differed. On a new seed it repaired all 510 physically solvable scarce-stock channels in 512 releases. It learned a prescribed acoustic vocabulary and gave 4,580 correct responses on 4,580 answerable causal prompts out of 6,144, including 362 single-cause explanations. As a guide it gave instructions that the older watershed's listener executed correctly in all 311 cases requiring at least three actions. The population checkpoint retains the selected animal's natural, pre-tuition state. [Original individual](../examples/detritus-young.mind) · [Schooled individual](../examples/detritus-guide.mind) · [Individual receipts](food-transport/coastal-repairs.tsv.gz)

A continuous interview exposed a retention problem. Both guides handled six successive samples, with the effective material changing without a cue. The brook guide kept attributing changes to the wick. The coastal guide correctly attributed the first change and subsequently withheld single-cause explanations. When the examiner explicitly cleared their material contacts before each sample, both correctly identified the material in all six. The batch language results use a fresh material investigation for each case. They establish transfer of the learned vocabulary to a new encounter. The continuous results measure what happens when previous physical learning is carried forward. [Continuous and cleared transcripts](food-transport/material-switches.csv) · [Replay the changes with a living guide](../examples/detritus-changing-materials.txt)

```sh
./axion talk --speaker examples/detritus-guide.mind --listener examples/capillary-listener.mind < examples/capillary-construction.txt
./axion talk --speaker examples/detritus-guide.mind --listener examples/capillary-listener.mind < examples/detritus-changing-materials.txt
```

## Throughput and integrity

| Blackwell benchmark | Result |
|---|---|
| Six alternating 512-tick million-site runs | 1,065,645 physical transfers in each run |
| Full CUDA dispatch | 512 transport launches for 64 pressure beats, median 55.60 million site-ticks/s |
| Scheduled CUDA dispatch | 64 transport launches for 64 pressure beats, median 55.65 million site-ticks/s |
| Saturation, dry donors, directional priority and simultaneous claims | 65,536 cases, 29,664 transfers, 10,272 denied offers |
| Mass and pigment accounting | Checked independently for every published sampled tile |

The timing difference is within the spread of the measurements. The reduction in launches is exact. The 16-tick CUDA graph omits frames that cannot contain a pressure beat. Direct dispatch aligns irregular resumes, report intervals and diagnostic windows. A device-clock guard covers periods longer than the captured graph. The tests compare full checkpoints and sampled physical receipts between schedules at periods of 1, 8, 64 and 256 ticks. [Timing repetitions](food-transport/schedule-timing.csv) · [Exhaustive local receipts](food-transport/boundary-cases.tsv.gz) · [Regression test](../tests/detritus.py)

```sh
make -j3 NATIVE=1 CUDA_ARCH=sm_120
make test-detritus NATIVE=1 CUDA_ARCH=sm_120
zstd -dc examples/detritus-estuary.flood.zst > /var/tmp/axion-food-estuary.flood
./axion floodplain --load /var/tmp/axion-food-estuary.flood --steps 1024 --report-every 256 --capillary-cargo-events /var/tmp/axion-food-events.tsv
python3 tools/detritus_audit.py /var/tmp/axion-food-events.tsv
python3 tools/detritus_audit.py --boundaries docs/food-transport/boundary-cases.tsv.gz
python3 tools/detritus-study.py /var/tmp/axion-food-comparison --habitat bank
```

The teaching guide remains available through the ordinary conversation interface. It speaks from its own acquired connections and can direct another animal at a stocked bench. Answering open-ended questions about this estuary or devising technologies autonomously over generations remains an open problem. [Talk with the guide](capillary-learning.md#try-the-two-creatures)
