# Life on a floodplain

A bank can outlive every animal that made it. The water diverted by the bank changes which plants grow, which bodies can reach food and which descendants can reproduce. Axion runs these interactions in a shared CUDA habitat. No animal receives points for building, signalling, surviving a test or growing a larger brain.

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --load examples/reedbank.flood --steps 4096 --legacy 256 --assay-calls --save river.flood
./axion floodplain --load river.flood --frontier 128 --steps 16384 --save river.flood
```

The reedbank starts at tick 16,384 with 559 expressed body plans, 896 expressed spiking programs and 454 producer genotypes. Its deepest living lineage is 221 divisions from a founder. The older clay-and-fibre community at `examples/alluvial.flood` continues with its original producer and hearing modes. Enabling `--producers evolving` on a legacy bed introduces local rooted programs and records the introduction.

The field guide covers the eight most common expressed body forms, ranked over every living resident. At tick 577,536, 7,554 of 17,834 animals belonged to those eight forms out of 958 recorded. All eight had 15 segments. Some differed by one receptor site or one feeding pore. The atlas marks those differences directly on the actual shape. The corresponding 256 × 256 world can be resumed from `examples/delta.flood`. [Population field guide](../journal/creature-card.webp)

## What can evolve

- An animal grows a connected arrangement of limbs, eyes and pores from up to 16 developmental instructions. Their number, order, branch points, expression, direction and differentiation can change. Body shape affects drag, metabolism, reach, harvesting and carrying. Some genomes change the adult shape in response to energy or wetness.
- A 72-byte neural genome specifies recurrent inputs, a variable instruction tape, thresholds for movement, developmental patterning and the operations that change synaptic gates. One CUDA lane advances 32 cells using four state bits per cell. A second heritable program can duplicate and differentiate up to four neural populations, their afferents, their update gates and their allocation to tissue. Founders express one population. A census distinguishes expressed differences from silent copies.
- The nervous system controls eight overlapping motor populations. Mutable peripheral contacts connect them to rotation, movement, feeding, gathering, release, contact, secretion and sound. An animal can join clay and fibre already placed by another. Fibre can catch litter, reinforce a clay face, fray under flow and support a crossing. Physical support is counted only when the same animal's force would have been insufficient without it.
- Eight call types can acquire local visual associations. Two-bit synapses change under a heritable finite-state table. Receptors can project the identities of remembered sensory contacts onto the nervous system. Food, water, neighbours, sound and secretion are available locally. The world supplies no words, alignment labels, teaching signal or prescribed cooperation.
- A rooted producer has a moisture phenotype and three heritable eight-entry tables. Local conditions determine allocation to fruit, processing of detritus and investment of actual fruit in dispersal. Grazing can create space for another producer. Offspring can change their tables and traits. The animals therefore face a changing resource community even on a fixed patch of soil.

A genome occupies 172 bytes. At 128 somatic cells, the main resident arrays consume 457 bytes per site, including its terrain, vacancy slot, germline, body, nervous tissue, ear and motor output. Census buffers, budgets and CUDA driver allocations add overhead. Each tick advances disjoint pairs for transport, the neural populations for sensing and action, then disjoint pairs for encounters and reproduction. A compiled CUDA Graph replays blocks of 16 ticks. Counter sharding helps larger grids. On a 256 × 256 comparison with identical final counters, the three-run median went from 166.69 to 196.12 million site-ticks/s. Small basins use a shared ledger. These timings include scheduled censuses and precede the rooted producer extension. [Timing samples](floodplain/counter-timing.csv)

## Experiments with an environmental legacy

Four independent 64 × 64 basins were raised for 8,192 ticks. In each terrain-only test, two copies lost all animals. One retained its banks and the other returned clay and fibre to their loose stores. Both received identical subsequent weather. With both construction materials available, 2,391–3,789 of 4,096 sites had different water levels after 256 ticks. Food differed at 477–2,543 sites. When material could only be deposited loose, the same intervention changed zero sites in all four worlds. [Matched habitats and controls](floodplain/material-controls.csv)

Fitness effects varied across watersheds. With the original animals present, removing the structures left 2,640, 2,938, 2,850 and 2,717 residents after another 2,048 ticks. Their matched intact branches had 2,644, 2,957, 2,549 and 2,531. The experiment also exchanges soil between independently developed branches while preserving every animal, acquired contact and nervous system in the recipient. The transplant starts a new material ledger. [Material interventions](floodplain/material-controls.csv) · [Earlier clay-only basins](floodplain/clay-controls.csv)

## Selection on borrowed ground

A common-garden experiment samples germlines from living adults and develops new juveniles. Two populations hatch in neighbouring pairs with equal counts and equal starting energy. A neutral ancestry marker passes to offspring. Each of six new watersheds repeats the introduction with the two groups' spatial placements exchanged. The neutral control gives the two tags identical paired germlines. Across both placements it produced exactly 18,761 residents of each tag at tick 2,048.

Descendants of one population raised for 8,192 ticks displaced the ancestral cohort in all 12 introductions, leaving 36,897 descendants in aggregate. Swapping the neural tapes, routing, population definitions and acoustic learning genes while retaining the body, reproduction and effector traits still gave the descendant-body cohort all 32,425 surviving places. The result identifies inherited ecological adaptation. Peripheral and reproductive traits remained sufficient for the advantage after the neural swap. The six gardens test one evolved source population. [Common-garden census](floodplain/offspring-competition.csv)

## How much learned hearing affected action

Across four evolving-producer habitats, 16 censuses per habitat sampled 963 encounters in which the listener had previously heard the current call. For each encounter the GPU replayed the next neural decision on the same cells and sensory scene. Clearing the acquired auditory contacts changed 334 receptor inputs and 28 winning motor choices. Identical replay changed zero. Scrambled communication also yielded some dependence on acoustic memory. Incidental associations can also change a motor choice. Whether the wild population uses shared meanings remains unresolved. [Auditory interventions and producer controls](floodplain/producer-succession.csv)

The occupied habitats recorded 146,700 new producer placements and 4,778 mutations. Matched empty habitats recorded 39,110 placements. Freezing producer reproduction left the original producer population in place. A genotype count can go down as a successful lineage spreads. The illustrated journal also measures persistent animal innovations, counting a changed inherited phenotype when at least two carriers remain after a full season. [Producer succession](floodplain/producer-succession.csv)

## The long passage

The reedbank expanded from 64 × 64 to 128 × 128. Eight successive checkpoint continuations completed **8,589,541,376 site-ticks**. At tick 540,672 the lineage reached 2,929 divisions, with 24,810,230 births and one surviving founding line. The first expanded census had 7,879 living animals and 683 body plans. The last had 1,907 animals and 174 plans. Mixed-material faces rose from 1,029 to 12,100. After the animals were removed, remobilizing those structures changed 14,777 water levels and food at 15,298 of the 16,384 sites. Each census, including the losses, is recorded. Resume the terminal state with `--load examples/longwater.flood`. [Passage record](floodplain/long-passage.csv) · [Branches from the terminal state](floodplain/late-interventions.csv)

Releasing the inherited structures at tick 540,672 was consequential for the remaining population. After 8,192 more ticks, the intact branch had **1,813 residents** and the remobilized branch had **6,237**. Releasing the banks and closing further construction left 7,171. Transplanting the same residents onto fresh soil left 10,371. This is a controlled comparison of descendants in one late habitat. Earlier experiments show that removing structures can also reduce the population. [Terminal-state interventions](floodplain/late-interventions.csv)

The same population also established a new frontier without remobilization. At tick 540,768, 1,973 residents gained access to a 256 × 256 watershed. After another 32,672 ticks there were 18,470 residents, 918 body plans and 3,297 spiking programs. Advancing the 32,768-tick observation took 8.139 seconds for 2,142,765,056 site-ticks (263.3 million/s) on the laptop Blackwell. The observation loop including censuses took 8.404 seconds. Each time the frontier opens, the simulator explicitly accounts for imported water, soil and primary stocks.

## Run another experiment

```sh
./axion floodplain --side 32 --density 22 --seed 1877 --frontier 128 --season 64 --steps 8192 --save colony.flood
./axion floodplain --load colony.flood --steps 2048 --remobilize --assay-calls
./axion floodplain --load colony.flood --steps 2048 --forget-calls --wire relabeled
./axion floodplain --load examples/reedbank.flood --steps 0 --observer --export-mind visitor.mind
./axion floodplain --resident visitor.mind --density 0 --side 32 --steps 256 --save new-marsh.flood
python3 tools/floodplain-competition.py --output competition.csv
python3 tools/producer-study.py --output succession.csv
python3 tools/floodplain-passage.py --load examples/reedbank.flood --save passage.flood --output passages.csv
```

Use `--senses-from examples/marsh-reader.mind` to graft unambiguous learned visual detectors onto otherwise diverse founders. A full wild `.mind` carries the animal's developmental and neural genomes, body, native call contacts and somatic tissue. Rehousing retains age, energy, experience and tissue and starts a new local ancestry record. `--founder` hatches genetically matching juveniles. `--observer` selects the living animal with the most call encounters for export. The default selects the highest-energy animal. `--slot N` selects an individual explicitly.

Each checkpoint carries its physical clock and a checksum. Water and clay are conserved. Total food, fibre and animal energy plus recorded heat equal the initial stock plus captured sunlight. The population ledger independently checks births and deaths. A frontier can double when a resident with offspring reaches its margin. Added geography and matter are recorded, and the user sets the maximum extent. Earlier food, banks, residents and learned tissue survive expansion. The five-by-five body, 64–1,024-cell nervous system, chemical stores, sensory interface and actuator repertoire bound every experiment. Long-run ecological novelty and changes in ancestry alone cannot demonstrate open-ended intelligence.

## Research behind the design

A 2026 community-assembly study examined how competitors, consumers and mutualists jointly change evolutionary outcomes. A separate 2026 theoretical preprint showed how a population that depletes a regenerating resource can change the selection gradient experienced by its successors. Axion makes the corresponding feedback experimentally accessible by preserving physical stocks, colonists and terrain independently. These studies motivated the experiments. The measured outcomes above come from Axion. [Community assembly](https://www.nature.com/articles/s41467-026-70117-8) · [Self-propelled evolution on regenerating landscapes](https://softmath.seas.harvard.edu/wp-content/uploads/2026/07/2606.15530v1.pdf)

Theoretical work on costly niche construction shows why the reproductive cost of changing an environment matters. The floodplain charges energy for work and leaves its consequences accessible to unrelated residents. A cellular-automaton approach supplies local, parallel updates and persistent spatial state. The neural instruction tables, mutable bodies and ecological interactions have their own representations. [Costly niche construction](https://academic.oup.com/evolut/article/79/12/2667/8134642) · [2026 self-replicating neural cellular automata study](https://www.mdpi.com/2313-433X/12/10/496)
