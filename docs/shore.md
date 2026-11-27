# Building from scattered stock

A resident can learn how its muscles move unfamiliar material, picture the consequences of moving a piece, and use a series of real movements to connect a source of light to a deposit that produces food. In a five-site transfer test, 25,979 of 32,768 unfamiliar beds produced food after the builders left. Every bed began with zero bonds and zero stored charge. The candidates, muscle impulses, pieces and measured harvest of an individual build can all be replayed independently. [Photographed five-site construction](../journal/runs/00007618-floodplain/plate.webp) · [Per-lineage results](shore/construction.csv) · [Independent verifier](../tests/shoal.py)

## Try the resident

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion talk --speaker examples/shore-reader.mind
```

Enter these lines in order. The question refers to the two visible deposits on the actual table.

```text
/shore 111 4
/shore-requests on
who is vole west of?
/inspect-shore
/expose-shore 256
```

The retained animal answers **moth**. Its learned visual category identifies a deposit. That deposit activates its spatial circuit, which chooses and executes movements until it predicts contact there or exhausts its inherited movement allowance. The exposed copies reveal how much product the constructed geometry can deliver at either site with compatible substrate. Reopen the same bed and ask `who is west of moth?` to direct a construction toward the other deposit. The two replies and outputs were recorded in separate, identically seeded sessions. [First request](../journal/runs/00007616-talk/plate.webp) · [Reversed request](../journal/runs/00007617-talk/plate.webp)

English spatial expressions and visual names were taught separately. The animal can handle a small learned question grammar. Ordinary conversation over arbitrary topics remains open.

`/imagine-shore` reports a proposed movement and a forecast with the actual tray untouched. `/build-shore 12` gives the animal twelve movement opportunities. `/pursue-shore` lets it stop at a predicted outcome. `/light-shore 128` supplies real light to the current tray, and `/eat-shore` transfers its finite product into the body. `/change-deposit` switches which substrate is receptive. `/taste-shore 4` allows fresh tasting without another construction lesson. `/explore-shore 96` supplies practice with loose chips, gradients, pigments and the resident's own actuators. Opening a new table brings in a new stock of material. The animal keeps its acquired contacts. The light and receivers in `/expose-shore` run in detached cold copies. That assay supplies no food to the resident.

## How the animal learns and acts

The shoal is an eight-by-four periodic patch. Each of its two substrate deposits lies three, four or five lattice steps from the light. Light and substrate access belong to positions. Creatures can relocate every grain, including the ones that initially occupy those positions. The stock includes branching, polarized and orientation-dependent pieces. Two immobile floor patches carry visible pigment and distinct odors. A high-energy grain next to a low-energy grain may transmit excitation across a compatible face. Locally compatible substrate, a catalytic surface and excitation still have to meet for food to appear. [Material physics](materials.md)

Each juvenile encounters its own correspondence between visible colors and physical terminals, between muscles and physical effectors, and between odors and productive substrate. During practice it sees whether charge actually propagates, feels the change in palm occupancy, sees rotations and registers actual product. Its two-bit synaptic update table and release thresholds come from its heritable manipulation genome. Newborns begin with empty learned state. Sixteen bytes hold the terminal-pair contacts, motor-effect contacts, odor contacts and the record of what the animal has sampled.

During construction a warp represents thirty-two spatial positions as a firing pattern. A different use of the same warp evaluates thirty-two alternative movements in parallel. Each imagined relocation or rotation changes only the potentially affected local connections. Firing from the light and from the currently desired location spreads across the predicted connections. Inherited gains combine overlap, nearby activity and the spatial extent of those volleys. The animal selects one arrangement, activates its own acquired motor mapping and pays the actual mechanical cost. The next decision uses the changed scene. The physical simulator is called only when material actually moves or receives light.

This supplies strong architectural priors. The system has a spatially organized retina, a palette of candidate movements, weak short-range projections, and a circuit that can favor convergence of activity originating at light and a desired deposit. It chooses its pieces, placements and action sequence. Evolution can change the bodily map, contact transitions, sensory release, projection depth, gains, exploration and stopping allowance. These experiments address construction and flexible goal selection within that architecture. Independent discovery of the architecture, a general theory of physics and cumulative technical culture remain open.

## Construction and incentives

Four populations started from independent random germlines. Each season had 4,096 work sites. The parent could eat the product of its construction, and each set of 24 units of reserve could provision an offspring. The fostered condition supplied the eggs with the same food and assigned a different local parent's germline. After 384 seasons we sampled previously unpracticed juveniles, gave them 96 rounds of exploratory contact, and tested them on four new beds per subject with independently remapped sensors and actuators. The aggregates contain repeated measurements within four evolutionary lineages.

| Source–deposit separation | Provisioned, productive | With multiple necessary relocated grains | Fostered, productive |
| --- | ---: | ---: | ---: |
| 3, rearing distance | 27,981 / 32,768 | 27,362 | 976 / 32,768 |
| 4, held out | 27,380 / 32,768 | 26,754 | 914 / 32,768 |
| 5, held out | 25,979 / 32,768 | 25,852 | 503 / 32,768 |

The builder leaves before assessment. Every grain is drained. The sources illuminate fresh, identical copies for 256 ticks. All unbuilt twins and all insulated twins produced zero. A relocated grain counts as individually necessary when removing it disconnects the active deposit from the source. Other functioning arrangements can contain redundant paths. The independent verifier reconstructs all alternatives in one decision, including placements outside the eventual path, then reproduces the actual movements and chemical output. [Complete construction results](shore/construction.csv)

Keeping the same four seeds, station capacity, number of seasons, opportunities to act and test conditions, shortening the post-build harvest window during rearing from 256 ticks to 64 changed the outcome substantially. The provisioned lineages then completed only **85/32,768** three-site beds. Fostered lineages completed **1,676/32,768**. An early pilot had already exposed the tendency to save effort when machinery had little time to pay for itself. [Short-harvest comparison](shore/short-harvest.csv) · [Rearing and transfer script](../tools/shoal-study.py)

## What experience contributed

On the five-site transfer, preserving the rest of the animal and silencing spatial propagation left **3/32,768** productive beds. Erasing all acquired contacts after practice left **33**. Suppressing terminal learning left **3,323**. The animals anticipated productive output on 24,073 beds, with 23,849 confirmed predictions. Forecasts and true harvests are independently recorded, including missed harvests and false positives. [All nine interventions](shore/construction.csv)

Two copies of each animal saw the same pieces and had the same movement budget. Changing only their prior experience of which substrate yielded food reversed the *exclusive physical output* between the two deposits in **19,430/32,768** five-site beds. With the old preference, 3,123 beds also happened to feed the currently useful substrate. After two tastings of each deposit, and no further experience of moving pieces, the number rose to **16,725**. Each test started from a fresh layout. The result measures redirection of construction toward an acquired goal.

The interactive experiment used sixteen consecutive seeds. The two deposits could always be named separately by the saved resident. A direct name followed by a twelve-movement budget exclusively supplied the requested deposit on **22/32** constructions. Thirty of the thirty-two arrangements of deposits admitted a unique left–right question. In all thirty, the resident's answer named the correct deposit. In **21/30**, the subsequent assembly supplied that deposit exclusively. The resident heard the question without a second instruction naming the answer. When the same questions triggered its own variable stopping rule, **22/30** constructions supplied the requested deposit exclusively. It spent 612 units over those thirty requests, compared with 974 units under the twelve-movement budget. A separate wording test included the unfamiliar surface form “east” and produced fewer resolved requests. [Named and relational requests](shore/requests.csv) · [Alternate wording](shore/directions.csv) · [Spontaneous action and stopping after a spoken request](shore/initiative.csv) · [Protocol](../tools/shore-requests.py)

## An inhabited, changing watershed

```sh
./axion floodplain --load examples/driftbank.flood --steps 1024 --report-every 256 \
  --artifact-samples 32 --material-legacy 128 --save descendants.flood
./axion floodplain --load examples/shore-colony.flood --steps 0 --shoal-span 5 \
  --shoal-trials 4 --work-subjects 2048 --shoal-moves 10 --shoal-ideas 128 --shoal-exposure 256
```

A construction tide translates disjoint eight-by-four regions over the floodplain. Only a living resident with eyes, limbs, sufficient reserve and an open motor gate can work. A neural motor volley and body anatomy determine eligibility and reach. Contact effects, approach and return use the normal material inventory and a real carrying slot. Inspection and handling debit the body's reserves. Deposited material changes ordinary chemistry and hydraulic resistance, can be reused by neighbors and can persist across generations. Two bytes per environmental site hold the visible events from the current physics step. Residents with the requisite senses can learn from these events even while another animal has the work area. The 16-byte predictive tissue is saved per resident, exported with the rest of its body, and reset at birth.

`--construction-tide 16` enables work in a new or existing material field. `--site-replay off` suppresses the extra work bouts while the ordinary hands keep operating. `--field-sight own` limits learning from ambient physics to the occupied site. `--light-regime veined` separates illuminated exposures from damp reactive mineral surfaces. In a veined district, the two exposed bands of every eight can absorb light and support photosynthesis. Reactions require water in a different band. The bands shift after sixteen seasons. Grain transport, stored excitation and newly made connections can bridge the separation. `--light-regime mosaic` places these districts among ordinary growing areas. Both climates conserve the same water and caloric stocks and use the same animal reproduction law.

We introduced the learned hand's *germline* into an older inhabited shellbank. The existing bodies, waters and brain tissue remained in place. After 4,096 ticks the diffuse habitat held 2,688 animals. Descendants of this same population had the following outcomes after a further 2,048 ticks.

| Light regime | Spatial work available | Spatial work suppressed |
| --- | ---: | ---: |
| Diffuse | 2,644 survivors | 2,557 |
| Fully veined | 0 | 8 |
| Mosaic | 1,392 | 1,352 |

These are deterministic branches of one population. The direct comparison does not establish a uniform reproductive benefit from planning. The retained mosaic had **91,008 active spiking cells**, 46,316 accumulated spatial work visits and 1,098,400 alternatives considered since the introduction of the new organ. Residents in the mosaic were weak at the controlled construction task using only their field experience. At four-site separation they completed **1/4,096** beds. After 96 exploratory presentations they completed **591/4,096**. Survival in the ordinary watershed selects for many other behaviors. [Population and mass ledgers](shore/tides.csv) · [Field residents and interventions](shore/residents.csv) · [Climate protocols](../tools/tidal-study.py)

## GPU implementation

The new circuit adds 16 bytes of writable state per resident. A settled version stores its sixteen terminal pair outputs, thirty-two motor outputs and four odor outputs in eight bytes. The morphology and inherited controller remain separate. On the RTX PRO 2000 Blackwell laptop, the 2,688-resident population passed 139,776 exhaustive receptor comparisons. Both representations produced the same answers on 262,144 six-step, 32-site diagrams. The median GPU batches took 0.277 ms writable and 0.255 ms settled. These microbenchmarks time predictions from already represented scenes. Whole habitat advances, held-out construction assays and rendering have separate timing scopes. The diffuse 4,096-tick advance with the new organ ran at 49.1 million site-ticks/s in the recorded run.

```sh
./axion floodplain --load examples/driftbank.flood --steps 0 --shoal-bench 262144
make test-shoal NATIVE=1 CUDA_ARCH=sm_120
```

## Reproduction and research basis

The first command rebuilds the nursery and exports its highest-energy release resident. The second command introduces its hand germline into the previously saved watershed. The third command produces the released mosaic. The language school and physical session produce the portable interactive resident. All three distributed checkpoints were regenerated byte for byte from this sequence on the laptop.

```sh
./axion floodplain --side 32 --seed 1801 --shoal-seasons 384 --work-capacity 4096 --shoal-moves 10 \
  --shoal-ideas 128 --shoal-exposure 256 --steps 0 --save colony.flood --export-mind builder.mind
./axion floodplain --load examples/shellbank.flood --hand-germ builder.mind --construction-tide 16 \
  --steps 4096 --save coast.flood
./axion floodplain --load coast.flood --light-regime mosaic --steps 2048 --save driftbank.flood
./axion ground --load builder.mind --origin random --generations 48 --population 64 --worlds 8 \
  --episodes 1024 --crowd 4 --dialect english --probes 128 --save reader.mind
./axion talk --speaker reader.mind --save-speaker shore-reader.mind <<'END'
/shore 115 4
/explore-shore 128
/imagine-shore
/build-shore 12
/light-shore 128
/eat-shore
/inspect-shore
/quit
END
```

A 2026 human imaging experiment found that learning transitions between previously separate sequences changed multistep predictions and hippocampal representations. A computational study in 2026 explored how sampling and compositional representations could support planning. A 2025 study connected a compositional replay model to rodent recordings. These results motivated a test of reusing learned local transitions in a spatial arrangement the animal had never encountered. Axion's representations, rule tables and CUDA execution scheme are original engineering choices. [Tarder-Stoll, Baldassano and Aly](https://pubmed.ncbi.nlm.nih.gov/42421578/) · [Neural sampling and goal-directed planning](https://www.nature.com/articles/s42256-026-01254-4) · [Constructing future behavior through composition and replay](https://www.nature.com/articles/s41593-025-01908-3)

Cutting and Neilson tested four- to seven-year-old children in paired, goal-directed and open-ended tool tasks. Experiments on New Caledonian crows found that some birds combined previously unassembled parts into tools. Those studies informed the decision to start with isolated pieces and to change the location of the useful output. The artificial tests use abstract appetites, a small retina and remapped actuators. A developmental age for these animals remains unestablished. [Children's tool innovation across contexts](https://pubmed.ncbi.nlm.nih.gov/41941182/) · [Compound tool construction by New Caledonian crows](https://www.nature.com/articles/s41598-018-33458-z)
