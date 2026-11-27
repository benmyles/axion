# A shared world for evolving spiking circuits

`axion life` puts independent creatures in a single persistent habitat. A creature can improve its chances of reproducing by changing the conditions its neighbors will encounter. The simulator supplies energy, space and a set of physical interactions. Organisms divide whenever local resources permit. Competition takes place across overlapping generations, and a population can go extinct.

The measurements on this page describe the original meadow. Groves extend the same cellular brain with evolvable receptor–motor connections and an optional digestive delay. [Lifetime learning and feature transfer](learning.md)

## The inherited program

Each meadow organism has a **72-byte genome**. It encodes a neural instruction tape, recurrent wiring, tissue development, sensory projections, motor readout, peripheral effectors and reproduction. Five instructions are expressed in a founder. Descendants can express one to eight. Point mutation can change any instruction's operands, truth table, destination or cell-type mask. Duplication and deletion change the tape. The genome also influences the probability of point and structural edits.

A neural cell has four bits of persistent state. These represent a spike, a memory register and two synaptic gates. Each gate selects one of two recurrent afferents. An instruction reads three sources, evaluates its heritable eight-entry truth table and writes a register. Sources include local registers, routed spikes, sensed conditions and a developmental cell-type pattern. Different cells can express different instructions from the same genome. The program that changes a synapse can itself mutate. It can learn a dependence on experience, oscillate, lock a connection, or ignore its sensory input. A birth transmits the genome and initializes new neural tissue.

The sensory interface exposes local food, neighboring occupancy and energy, chemical gradients, cache availability, heading, a coarse phase signal and recent change in the organism's energy. An inherited projection maps those signals onto the neural sheet. After four neural updates, eight motor populations vote. Each population addresses an independently mutable peripheral instruction. A mutation can couple motion, secretion and resource exchange to the same output or silence a port. The genome sets the extent of active tissue within the configured capacity, from 64 through 1,024 cells.

## Where the selection pressure comes from

The world is a torus. Local sunlight replenishes edible material at rates varying in space and time. Basal metabolism, more expensive combinations of actuators, movement and the upkeep of larger tissue draw on the same energy budget. A creature can consume plants, transfer energy to or from another creature, secrete a diffusing chemical or draw from or add to a local cache. A cache persists after its maker leaves or dies. Part of its contents gradually returns to vegetation.

A creature with enough energy and a vacant adjacent site can divide. Its threshold and division ratio are inherited. Transfers and division conserve energy. The program audits initial energy plus sunlight against the sum of living reserves, vegetation, caches and expenditure. It also checks that births and deaths account for every change in population.

A tick reads one world snapshot, resolves local actions, and publishes the next. Disjoint two-by-two neighborhoods share a changing origin that visits all four offsets. A proposed movement or contact is possible when the target is in that neighborhood. The order of contested interactions is drawn independently for each neighborhood and tick. The physics therefore has an explicit spatial and temporal scale. The simulator fixes the available energy carriers, ports, instruction alphabet and maximum tissue size. Organisms can diversify within those constraints. Sustained innovation or child-level cognition would require separate evidence.

## What survived selection

Three 64 × 64 habitats ran for 4,096 ticks with independent genomes, random seeds and open reproduction. Every founding organism initially had a distinct ancestry label. Gene copies and mutations can coexist within one surviving founder lineage.

| Habitat seed | Living at tick 4,096 | Births | Deaths | Deepest living generation | Surviving founder lineages | Structural tape edits |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 379 | 1,010 | 90,304 | 90,775 | 99 | 1 | 3,177 |
| 2191 | 1,019 | 115,216 | 115,678 | 85 | 1 | 4,076 |
| 8461 | 1,015 | 101,415 | 101,823 | 102 | 1 | 3,467 |

All three underwent a founder sweep. The same populations continued through tick **65,536**. Respectively, 1,047, 1,161 and 1,087 creatures remained alive, with deepest extant generations 487, 815 and 640. They accumulated 26,687, 43,894 and 28,530 structural tape edits by that tick. We included the second world as a checkpoint because it had the deepest surviving line of descent among the three. [Evolved habitat](../examples/habitat.eco) Ancestry labels stayed fixed after the sweeps. Edit counts alone leave behavioral novelty unresolved. In seed 379, bodies put 134,505 energy units into external caches and recovered 46,611. These are energy fluxes. They leave the question of purposeful caching or cooperation open.

### Separate the body, the brain and its synapses

Each donor population then seeded four previously unseen habitats. Two cohorts competed in the same world. In the reciprocal replicate, the same founding sites received the opposite treatment. Starting share averaged 50%. All edits were disabled during these 512-tick tests so changes in cohort frequency measured differential survival and reproduction of the inherited variants. Each table row averages final treatment shares over 24 worlds. The three donor columns average eight worlds each. The experimental units are the habitats and donor populations. [Initial and final counts from all 120 contests](common-garden.csv)

| Treatment against its co-resident control | Seed 379 | Seed 2191 | Seed 8461 | Mean final treatment share |
| --- | ---: | ---: | ---: | ---: |
| Whole evolved genomes against fresh founders | 100.00% | 99.46% | 100.00% | **99.82%** |
| Evolved neural genes in fresh bodies against fresh founders | 32.36% | 30.26% | 2.39% | **21.67%** |
| Evolved peripheral genes with fresh brains against fresh founders | 100.00% | 99.28% | 100.00% | **99.76%** |
| Neural motor output against random output, both from the same evolved stock | 100.00% | 100.00% | 100.00% | **100.00%** |
| Mutable against frozen synaptic gates, both from the same evolved stock | 54.70% | 75.61% | 52.09% | **60.80%** |

The body transplant replaces physiology, reproductive traits and the motor-to-effector wiring. The brain transplant replaces the remaining genes, including the sensory projection, neural tissue and motor readout. A fresh animal receives a newly generated genome. The neural-control and plasticity contests start both cohorts from the same donor distribution. Random control replaces the neural motor choice each tick. Frozen synapses hold both gate planes fixed from birth while the other neural registers continue to change.

Peripheral adaptation carried most of the advantage against fresh founders. The neural controllers were effective with their evolved bodies and usually transferred poorly to fresh ones. Plastic lineages finished ahead in 17 of 24 contests. The range was 41.72% to 83.17%. Disabling all synaptic changes disrupts both experience-dependent updates and internally generated gate dynamics. This experiment establishes a fitness effect of mutable connections here. The content of any acquired knowledge remains an open question.

```sh
./axion life --side 64 --seed 379 --steps 4096 --save donor.eco
./axion life --side 64 --seed 601 --founders donor.eco --contest plasticity --mutation off --steps 512
./axion life --side 64 --seed 601 --founders donor.eco --contest plasticity --mutation off --steps 512 --swap-cohorts
./tools/common-garden.sh > gardens.csv
```

### Does sensory history change the next action through a synapse?

The `--assay` option starts counterfactual copies of every living brain on the GPU. Two copies start with identical tissue and receive four different sensory observations each, sampled from occupied sites in their current habitat. At the probe, both receive the first copy's spike and memory registers. They retain the two separately acquired synaptic gate planes. They then run the same neural program on an identical new observation with identical tie-breaking. A difference in the motor output can therefore be traced to prior sensory input through the gate planes. An identical-history control and a control that freezes both gates during induction repeat the entire procedure. The cloned tissue is discarded.

| Donor at tick 4,096 | Paired probes | Different acquired gates | Different final spikes | Different motor choices |
| --- | ---: | ---: | ---: | ---: |
| 379 | 64,640 | 31,789 | 1,921 | 343 |
| 2191 | 65,216 | 38,807 | 8,880 | 1,435 |
| 8461 | 64,960 | 11,851 | 299 | 66 |

Across 194,816 paired probes, different histories changed the acquired connections 82,447 times. They changed the next physical effector instruction **1,844 times (0.95%)**. Each control produced zero differences in connections, final spikes, motor choices and effectors. A checkpoint saved after the assay was byte-identical to the input checkpoint.

Fresh transplants tested whether ecological selection had increased this particular form of sensitivity. Before any creature acted in each of the four common gardens, the assay delivered 32 probes per living brain. Fresh genomes changed motor choice on **9.63%** of probes on average. Evolved genomes from donors 379, 2191 and 8461 changed it on **0.64%, 3.33% and 0.15%**, respectively. The exposure protocol and starting habitat were matched within each garden. The fitness advantage of mutable synapses coexisted with substantially *less* sensitivity to these sampled sensory histories after selection. These probes measure a causal effect on behavior. The environment supplies no correct answer for choosing a different actuator.

```sh
./axion life --load donor.eco --steps 0 --assay 64
./axion life --seed 601 --steps 0 --assay 32
./axion life --seed 601 --founders donor.eco --steps 0 --assay 32
```

## Blackwell execution and scale

One CUDA lane advances 32 neurons together. The four neural planes occupy **half a byte per neuron**. A subgroup of two to 32 lanes owns a creature. Its genome supplies the afferent offsets and local rotations, so connection indices need no per-neuron storage. Program instructions are distributed across lanes and broadcast with PTX shuffles. Biological activity, online neural changes, interaction resolution, births, mutations and population measurements run on the GPU. The CPU submits work, displays measurements and performs checkpoint I/O.

Moving bodies exchange handles. Their neural tissue stays in place. A vacancy carries its own reusable handle, so reproduction needs no global allocator. Habitat neighborhoods keep their dynamically indexed occupants in shared memory. A CUDA graph submits 32 complete ticks in one call. Direct dispatch is available for comparison. On this GPU both hot kernels report zero compiler-allocated local bytes per thread at the 256-cell capacity. Distributing the neural tape reduced its register count from 187 to 47 in the release kernel. Both versions produced byte-identical 4,096-tick checkpoints.

These are medians of seven native runs on the RTX PRO 2000 Blackwell laptop GPU. Each run restored the same tick-128 checkpoint, advanced 128 ticks and included the whole ecology in CUDA event time. The active tissue and number of living creatures change during each run. A cell update means one expressed neuron's transition for one microtick. Comparisons with conventional neuron models or FLOP/s require a separate workload.

| Habitat | Living at start → finish | Explicit device buffers | Graph time | Direct time | Billion active cell updates/s, graph |
| --- | ---: | ---: | ---: | ---: | ---: |
| 64 × 64 | 2,734 → 2,456 | 1.21 MB | 2.882 ms | 2.912 ms | 99.28 |
| 256 × 256 | 44,118 → 41,292 | 19.41 MB | 23.198 ms | 23.246 ms | 180.17 |
| 512 × 512 | 174,062 → 162,583 | 77.63 MB | 114.030 ms | 114.673 ms | 144.19 |

A separate run at **1,024 × 1,024 sites and a 1,024-cell capacity** reached **521,256 simultaneously living creatures** at tick 96. It completed 47,350,120,704 active cell updates using **713,162,956 bytes** of explicit device buffers. Genome-dependent tissue sizes determine the actual number of updates. Those bytes include reserved empty slots, both world snapshots and reporting buffers. CUDA context, module and graph overhead are additional. The graph changes submission cost; event throughput was close to direct dispatch on the larger compute-bound runs. Power consumption was unmeasured.

```sh
./axion life --side 256 --neurons 256 --seed 379 --steps 128 --save warm.eco
./axion life --load warm.eco --steps 128 --dispatch graph
./axion life --load warm.eco --steps 128 --dispatch direct
./axion life --side 1024 --neurons 1024 --seed 983 --steps 96 --report-every 32
```

## Verification on the laptop

Native SM 12.0 and portable SM 8.0 PTX builds passed the GPU suite. Tests cover all five tissue capacities, controls, graph and direct execution, sparse donor transplantation, population accounting, conservation, checkpoint corruption, exact resumption and read-only synaptic assays. The host code passed UndefinedBehaviorSanitizer. Host-only tests passed AddressSanitizer with leak detection. GPU-backed AddressSanitizer tests used `protect_shadow_gap=0` and a leak suppression scoped to `libcuda.so.1`. The 6,509 bytes in 135 reported driver allocations also appeared in an independent executable whose only operation was `cuInit(0)`.

## Research behind the design

- A 2025 Lenia study used diversity search to discover sensorimotor behaviors in a cellular automaton. CARL, reported at ALife 2026, learned to create and steer patterns by locally changing a Lenia substrate. Axion uses cellular computation inside each individual's nervous system and in its shared habitat. [Sensorimotor agency in cellular automata](https://pmc.ncbi.nlm.nih.gov/articles/PMC12577699/) · [CARL, ALife 2026](https://developmentalsystems.org/carl/)
- JaxLife studied embodied agents coevolving in a common programmable environment. It motivated giving the actions of one animal durable consequences for other animals. Axion's kernels, genomes and world physics were implemented here. [JaxLife, original paper](https://arxiv.org/abs/2409.00853)
- Work on evolving synapse-specific Hebbian coefficients showed that evolution can shape within-lifetime plasticity. Axion makes the update program itself heritable. A truth table can implement coincidence, inhibition, copying or another Boolean response. The ecological controls above test what that freedom actually yields. [Najarro and Risi, original paper](https://arxiv.org/abs/2007.02686)
- A November 2026 world-model study measured adaptation to previously unseen mechanics by counting exposures and checking the cells affected by the mechanic. It informed our decision to treat ecological fitness and evidence for specific learned knowledge as separate experimental questions. [Onoda et al., November 2026 preprint](https://arxiv.org/abs/2611.12324)
