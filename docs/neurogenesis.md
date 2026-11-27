# Evolving the number of spiking cells

A floodplain creature inherits a birth size and a rule for recruiting or retiring cells during its own life. A child can start with a different number of cells from its parent. Two siblings with identical genes can end up with different counts after different experiences. New habitats reserve room for 128 somatic cells per animal by default. Across supported arena sizes, a resident can express 64 to 1,024 cells in bands of 32.

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --side 32 --neurons 512 --seed 7933 --steps 512 --assay-growth
./axion floodplain --load examples/shellbank.flood --neurons 1024 --steps 2048 --assay-growth --save larger.flood
```

The 512-tick experiment ended with 834 living residents. Of the 120 living residents whose brains exceeded their birth size, silencing the added cells changed 25 next actions, 693 spikes in the original bands and 1,174 gate bits in the original bands. The probe runs both copies through the ordinary neural and motor kernels from the same instant and leaves the habitat intact.

The saved shellbank contains an individual that hatched with 64 cells and now has 416. You can extract this exact animal and inspect or rehouse it.

```sh
./axion floodplain --load examples/shellbank.flood --steps 0 --slot 1933 --export-mind grown.mind
./axion agent --load grown.mind --inspect
./axion floodplain --resident grown.mind --density 0 --side 8 --steps 64 --assay-growth
```

## From chromosome to living circuit

- The pre-existing developmental chromosome sets the size at hatching. Point mutations can change it. Holding the arena fixed makes changes from parent to newborn directly comparable.
- Eight inherited bytes encode a developmental truth table, a clock and sensory sampling. Its four input wires can select such signals as body reserves, water, a received call, age or activity in an existing neural band. The selected entry can leave the tissue alone, recruit a new band, duplicate an active band or retire the last band. Mutation changes the detector and all table entries. Offspring inherit the rule and hatch with fresh tissue.
- Four more bytes record present, natal, maximum and minimum lifetime extent. Each group contains 32 genuine somatic cells with individual spiking, memory and gate bits. The new cells contribute to recurrent input, population votes and the cost of living. Recruiting a band spends two units of body energy. Pruning clears its tissue.
- The GPU reserves a rectangular arena for coalesced addressing. Dormant bands hold zero state and contribute zero drive. The warp still carries the shared instruction broadcasts. Each band uses 16 bytes for its four bit planes. Increasing the arena on a saved world preserves each individual's active count and cell state. A larger allocation provides room for subsequent development.
- New floodplains develop by default. Older worlds preserve their recorded behavior when resumed. Add `--neurogenesis inherited` to start development in an old habitat. `--neurogenesis fixed` holds lifetime size fixed while offspring retain genetic variation in birth size. `--fixed-size-cohort b` applies the lifetime intervention to one neutral lineage tag. The portable mind carries its acquired size and program.

The census checks a cell balance independently of the ordinary energy ledger. Starting cells plus cells born in offspring plus cells recruited during life must equal living cells plus cells lost through death and pruning. The species cards give the actual range of current cell counts for the form and the oldest representative’s own birth size, current count and lifetime peak. Capacity is reported separately.

## Selection in a shared watershed

Identical paired germlines from the reedbank hatched with equal starting energy in four new watersheds. Each introduction ran twice with placement exchanged. A separate neutral experiment allowed development in both tags.

| Watershed | Developing survivors | Fixed-at-birth survivors |
| --- | ---: | ---: |
| 3931 | 4,334 | 750 |
| 1787 | 4,109 | 826 |
| 6947 | 4,748 | 761 |
| 8233 | 4,201 | 842 |
| All eight introductions | 17,392 | 3,179 |

Allowing both groups to develop produced 11,202 survivors for each tag across the paired placements. The developmental treatment permitted growth **and** pruning. In these finite environments, reducing a costly circuit could also improve survival. A claim about improved reasoning would require a separate behavioral test that varied cell count while controlling for energy costs. [All 16 censuses and silencing replays](growth/common-gardens.csv) · [Reproduction script](../tools/cortical-study.py)

The `--silence-new-cohort b` intervention let both cohorts grow and prune under the same energy prices and silenced the postnatal bands of one cohort. The group with conducting postnatal tissue finished ahead in three of eight introductions, with 8,877 survivors against 11,267 across all eight. In this ecology, regulating tissue size had a more consistent benefit than transmitting spikes from every extra band. These counts also reflect competition between neighbors and changes in their shared habitat. [Conduction intervention](growth/silent-cohort.csv)

The earlier 256 × 256 delta was also given room for 1,024 cells per resident. After 512 ticks, the developing world held 7,336 animals and 3,083,168 active somatic cells. Silencing postnatal tissue changed 294 of 772 growing residents’ next actions. Advancing the GPU habitat took 0.122 seconds for 33,554,432 site-ticks, or 275.8 million site-ticks/s. The same starting checkpoint with lifetime size fixed ended with 10,490 animals. The short whole-world comparison illustrates a real tradeoff in population and resource use. [Developing delta](../journal/runs/00006995-floodplain/plate.webp) · [Fixed-size delta](../journal/runs/00006994-floodplain/plate.webp)

Recent adult human hippocampal studies identified proliferating progenitors and molecular stages associated with maturation. They motivated asking whether cell recruitment could be a useful computational variable. Axion's truth tables, costs and band sizes are engineering choices. Human cognitive ability has yet to be demonstrated in Axion. [Dumitru et al., Science, 2025](https://pubmed.ncbi.nlm.nih.gov/40608919/) · [Human hippocampal neurogenesis across adulthood, Nature, 2026](https://www.nature.com/articles/s41586-026-10169-4)
