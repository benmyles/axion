# Learning to feed in a changing current

A watershed-born forager reached for food, missed and later answered questions about the mistake. Its recorded expectation, in its own optical frame, put jade in the east. The current actually took amber there. Acquired speech contacts let it discuss the difference and carry a reference from one turn to the next. [Recorded conversation](drifting-food/conversation.txt)

| Question in sequence | Spoken reply |
| --- | --- |
| What actually passed there? | ? |
| What did you expect in east? | jade |
| What actually passed there? | amber |
| And before? | amber |
| What did you expect in west? | teal |
| What actually passed there? | jade |
| And before? | teal |
| Where did amber start? | east |
| Where did teal start? | west |
| Why did you reach there? | silence |
| What will you do tomorrow? | silence |

Run the first command to talk to that individual. Redirect the example questions into its input to reproduce the full transcript. A second checkpoint contains a freely evolved, untaught watershed at tick 16,384. It has 5,852 living creatures descended from 16,384 founders, with 724 distinct combinations of expressed cortical program and local contact law. Its most common program occurred in 867 residents. [Conversation input](../examples/drift-conversation.txt) · [Population censuses](drifting-food/free-populations.csv)

```sh
./axion talk --speaker examples/drift-speaker.mind --listener examples/parallax-witness.mind
zstd -d examples/drift-foundry.flood.zst -o foundry.flood
./axion floodplain --load foundry.flood --steps 4096 --report-every 1024 --save downstream.flood
```

## How an animal experiences the river

Each pulse entrains existing litter or fruit from a wet site. Four neighbours share a finite load. Regional tide, water level, clay and the fibres across each seam determine which horizontal exchanges open, then which vertical exchanges open. Overlapping neighbourhoods shift as the clock advances. Grains can travel across many neighbourhoods on successive pulses. Creatures can add a strand from body stock or local litter, or lift one back into their bodies. Nearby animals then face different currents. In the optional raft environment a pulse also drags existing material carriers if its impulse can move their combined cargo. In a million-site habitat over 1,024 ticks, the water drove 97,776,906 carrier exchanges and 1,230,848 loads resisted the current. [Mass, uptake and material records](drifting-food/material-inheritance.csv)

A reach requires a limb, a pore, a motor impulse and a free unit of metabolic energy. The animal commits to a port from its own inlet perception, learned connections and motor state. Its draw is fixed before the outcome enters any sensory or learning port. Earlier hands get first access to the catch in a rotating priority order. A correct choice can leave an animal hungry if a neighbour has already eaten. Swallowed carbon becomes energy, a reach releases heat and uneaten matter settles into the substrate.

Each creature has 16 mutable contacts between perceived inlets and possible reaches. The circuit uses the high half of an existing 32-bit hand signal. Nonzero inherited receptor masks can carry the contact state into the general brain. Two banks of 16 spiking cells in an expressed somatic module process local sensory conflict, the previous state, a proposed write, previous spikes and feedback. The genome determines which signals each bank receives and the truth tables it executes. The second bank controls which contacts can change. A further eight heritable bits encode any Boolean correction to the proposed write as a function of the old bit, proposal and first-bank spike. Those corrections can reopen a connection. Reproduction resets personal reach memory, transmits the genes and permits further mutation. Somatic brain width, allocation of modules, receptors, appendages and body shape continue to evolve. The four-port sensory and motor interface, the mutation operators and the upper neural allocation are engineering choices. The current habitat supports 64 to 1,024 somatic cells per resident. [CUDA field kernel](../src/flood_drift.cuh) · [Heritable circuit](../include/axion/regulation.hpp) · [Neural development](neurogenesis.md)

For one encounter let `C` denote the 16 contact bits, `P` the inherited hand law's proposed state, `S` the first bank of spikes, `R` the second bank of spikes and `M` the mask of four contacts innervated by this inlet. A set bit in `C` excludes a reach. The inherited eight-entry table `T` acts independently on each contact. The state transition uses bitwise operations.

```text
Q      = P xor T(C, P, S)
C_next = C xor ((C xor Q) and R and M)
```

The two spiking banks can therefore preserve, eliminate or reinstate an option under the same sensory event. Selection acts on calories and births in the full field. Changing the genome changes what counts as a useful experience for this circuit. [State transition](../include/axion/drift.hpp)

## Does personal experience pay for itself?

Starting from the million-site learning estuary, a 256-tick run released **52,107,574** units of food. Animals caught **5,018,100** and the remaining **47,089,474** settled. On exactly the same pulses, mouths, draws and priorities, replacing acquired reaches with blank ones would have caught **5,012,931**. The local net gain was 5,169 units. This replay measures immediate access to food. To measure reproduction over time we ran the populations below. [Physical controls](drifting-food/watersheds.csv) · [Raw sampled reaches](drifting-food/pilot-live.tsv.gz)

Six independent 128 × 128 landscapes began with the same full body, brain and motor germline, placed under two heritable labels. Each label began with 8,192 residents. Both groups could learn, pay for reaches, build, metabolize, mutate and reproduce. Only one group could read acquired inlet-to-reach contacts at the moment of commitment. The swapped release changed which label had access. A third release gave both groups access. No reproduction score named the desired behaviour.

| Seed | Live A, unread B | Unread A, live B | Both live |
| --- | ---: | ---: | ---: |
| 3,913, residents after 4,096 ticks | 9,079 / 1,010 | 22 / 9,115 | 4,625 / 4,028 |

The lineage with access finished ahead in both reciprocal releases on all six landscapes. The experimental pairs share their founders and terrain, giving six independent landscapes. Label abundances also varied in the controls. On seed 3,913, the first live group captured 11,744,477 units. Restoring its original empty contacts for those individual reaches, with earlier competitors' claims held fixed, gave 6,407,385. The unread group captured 385,011 under both calculations. The ledger checks beginning population plus births against deaths plus survivors for every lineage. [All releases, including the equal-access controls](drifting-food/competition.csv)

In the diverse million-site habitat, the previously introduced two-word food circuit had **462** exact carriers at release. After 4,096 ticks there were **20** with access to reach memory and **zero** in the unread world. The experiment with that same source and ordinary food, reported earlier, retained 122 at the corresponding tick. A specific useful learning circuit can dwindle while hundreds of thousands of other creatures thrive. [Carrier census over time](drifting-food/carrier-survival.csv) · [Earlier estuary release](tidal-learning.md)

## What evolves without an introduced teacher?

Four independently generated watersheds began with empty cortical routing and gate words. Each ran for 16,384 ticks without a school, donor, target allele or language-based selection. We then grew a common body with newborn contacts on a fresh landscape. For each source watershed, one lineage received the cortical and hand-law loci sampled from its living population, the other received the founding pool. We swapped the labels and repeated with memory access silenced for both groups. Pool sampling preserves abundance. The new habitat's reproduction and mutation dynamics remain active. [Four founding and final censuses](drifting-food/free-populations.csv) · [Reciprocal genetic transfers](drifting-food/genetic-transfer.csv)

Seed 27,361 produced a positive transfer. With access to personal memory, the late pool finished ahead in both assignments, **4,803 to 4,490** and **5,933 to 3,661**. Silencing access reversed these outcomes, **24 to 11,084** and **733 to 10,655**. Two other source populations lost both live assignments and the fourth split. These populations found different ways to survive in the same class of world.

We also screened every distinct living program from each source on 64 new three-season feeding lives. Each assay clears both contacts and spike state, changes the physical current after 32 reaches and restores it after another 32. Selection ranks the worst season first, then total food, with prevalence breaking ties. A disjoint set of 64 further lives measures the same preselected program. Seeds 27,361 and 89,003 both yielded programs that caught **4,910 of 6,144** on the second panel. Matched random reaches caught **1,570**. Those programs were rare in their source populations, with seven and two copies. Transferred into a standard speech body, each passed a changed spoken convention in only **2 of 1,024** courses before and after the change. The modal program from seed 27,361 passed all 1,024 courses with a stable convention, learned the first convention in all 1,024 reversal courses and passed zero after it changed. [Held-out food tests](drifting-food/food-panels.csv) · [Individual feeding traces](drifting-food/screen-27361-old.tsv.gz) · [Language controls and failures](drifting-food/language-transfer.csv)

For a matched anatomical experiment, each of the four source populations continued another 32,768 ticks under a tide that changed every 16 ticks. One world in each pair enabled the additional heritable contact-reopening rule. The best confirmed programs caught **3,922 vs 4,395**, **4,697 vs 4,402**, **3,146 vs 2,233** and **4,402 vs 4,910** meals with the rule enabled versus gated, in seed order 7,919, 27,361, 51,971 and 89,003. Additional structural freedom changed what emerged. It improved performance on this test in two source populations. [Matched gated population](drifting-food/screen-27361-matched.tsv.gz) · [Structurally plastic population](drifting-food/screen-27361-remodel.tsv.gz)

## Giving development more room

We also released each population at tick 16,384 into two otherwise identical habitats. One retained its 256-cell ceiling and the other made room for up to 1,024 somatic cells per body. Opening the arena preserves every existing cell, synaptic state and tissue count. New cells must still arise through birth or paid development. After 2,048 ticks, the open populations contained **7, 106, 8 and 20** living animals above the former ceiling. Their largest brains had **480, 512, 480 and 704** cells, respectively. The medians remained **64, 256, 64 and 64**. All four closed populations returned zero over 256. [Width histograms, newborns, growth energy and food](drifting-food/neural-widths.csv)

The population from seed 27,361 recorded **1,714** offspring with more cells at birth than their parents had at birth in the wider world, compared with zero in its matched closed world. Its residents caught 14,032,784 food units, versus 13,905,627 in the closed world. Food totals increased in two open habitats and decreased in two. The measurements cover whole populations. The effect of brain size on an individual’s learning remains unmeasured. The example below reproduces the seed 27,361 expansion.

```sh
./axion floodplain --load foundry.flood --neurons 1024 --steps 2048 --save wider.flood
```

## Material that outlives its maker

In an estuary run lasting 1,536 ticks, we observed 131,072 of 2,097,152 physical seams chosen in advance. Tying a particular strand records its maker's slot and birth clock. Any later change in that seam's thickness invalidates the record. At a later encounter the GPU removes just that one unit in a simultaneous counterfactual, leaving all other strands, bed geometry, priorities and reaches fixed. The sampler considers one marked seam per pulse. It reports consumption only when the missing unit would have changed the outlet.

The cumulative ledger counts all qualifying encounters on the monitored seams. The saved raw examples keep one observation per sampled edge and favour a verified delivery to another animal, especially one born after the builder died. Their rows can be independently checked against the physical model. Population totals come from the cumulative ledger.

Animals tied **72,660** tracked units. Later currents revisited one of these 7,388,183 times and changed outlet in **515,947** one-strand comparisons. Consumers actually took 90,730 food units in those comparisons. They would have taken 90,002 through the alternate outlets, a net gain of **728**. Of the real food, **53,718** reached animals born after construction and after the builder had died. The longest construction-to-feeding gap for another animal was **1,505 ticks**. Birth-clock identity establishes that these were different individuals. Kinship and any intent on the part of the builder remain unknown. Turning off the physical conductance of fibres gave zero one-strand diversions even though animals kept tying fibres. [Material ledger and bypass control](drifting-food/material-inheritance.csv) · [3,233 independently replayable receipts](drifting-food/infrastructure.tsv.gz)

A second million-site release began in the earlier estuary before the food-circuit transplant. It combined mobile cargo, private sight and heritable contact reopening. After 2,048 ticks it held **345,898 animals and 22,250,496 expressed spiking cells**. Builder 927,198 tied a strand at tick 387,911. It was dead when resident 858,608, born at tick 389,622, caught two food units because that strand changed the outlet. The elapsed interval was **1,791 ticks**. Across the sample, 81,598 units reached later-born creatures after the builder died. Whole-sample intake at redirected outlets was 129,218 units versus 129,408 with the individual strand removed. Across all reaches in the world, the real intake of 45,786,081 fell 695,520 below the blank-contact replay. The environment can sustain an enduring construction while selection is still contending with costly learned mistakes. [Raw material and body records](drifting-food/closing-legacy.tsv.gz) · [Sampled physical and neural transitions](drifting-food/closing-flow.tsv.gz)

The closing habitat is saved in Git LFS. It resumes with its current material, surviving bodies, acquired contacts, developmental tissue, private scenes and the same feeding physics. The run plate depicts the recorded builder and receiver. The population card ranks the eight most frequent body plans in the final census. [Closing run](../journal/runs/00010619-floodplain/plate.webp) · [Closing field guide](../journal/runs/00010619-floodplain/creature-card.webp)

```sh
zstd -d examples/drift-estuary.flood.zst -o drift-estuary.flood
./axion floodplain --load drift-estuary.flood --steps 1024 --save next-estuary.flood
```

## Following a spoken reference after a mistake

We prospectively selected the first forecast error with a changed visible scene among residents carrying the previously introduced food circuit. These witnesses therefore come from an experimentally influenced population. Each received eight sound-to-referent names and nine question constructions through the simulator's physical demonstration interface. We returned the learned acoustic contacts to the same living body. Its own retinal frame supplied what to say. The experimenter supplied the English questions and activated the interviews. [Teaching sequence](../examples/drift-lessons.txt) · [Disjoint interview prompts](../examples/drift-questions.txt)

We followed the same two identities for a further 512 ticks each. For each of their 64 real encounters, we replayed four anchored multi-turn interviews on CUDA. The question after every anchor was identical. The circuit could use the preceding spoken topic to interpret “there” and use a previous answer to interpret “that”. Each replay also received the same questions with the topic cleared or the private scene withheld.

| Witness | Subsequent events | Forecast, actual and before all correct | Both sides of a visible error | Correct later locations after a unique earlier colour |
| --- | ---: | ---: | ---: | ---: |
| Loose pigment, resident 913,493 | 24 | 64 / 96 | 7 / 7 | 20 / 36 |
| Loaded floats, resident 919,720 | 40 | 108 / 160 | 29 / 29 | 30 / 53 |

Across **256** anchored conversations, **172** three-answer sequences were completely correct. The witnesses correctly reported both sides of all **36** discriminating socket forecasts. Clearing the prior topic produced zero answers to the shared follow-up. Withholding the encounter produced zero audible scene answers. The same two animals with their field-acquired contacts but without English instruction completed zero sequences. The food memories, school language, controlled replay and ordinary interactive transcript are recorded separately. These tests cover a small learned vocabulary about visible four-port events. General conversation, causal explanation, long-horizon plans and human-level intelligence remain unachieved. [Full results](drifting-food/field-speech.csv) · [First body](drifting-food/final-river-taught.tsv.gz) · [Second body](drifting-food/final-raft-taught.tsv.gz)

## Reproduce the field and controls

The following commands work from the repository root with the CUDA executable and Git LFS assets. The large checkpoint is optional for trying the speaker or running the compact foundry. The published historic releases explicitly select the earlier `gated` proposal. The default for a newly enabled current allows the additional inherited contact rule. A loaded checkpoint resumes its saved physical and plasticity settings.

```sh
zstd -d examples/learning-estuary.flood.zst -o learning.flood
./axion floodplain --load learning.flood --drift tidal --contact-remodel gated \
  --drift-stride 1 --drift-load 32 --drift-sight own \
  --watch-drift surprise --drift-witness-sample carriers --drift-look 262144 \
  --trace-donor examples/estuary-regulator.txt --steps 256 \
  --save seen.flood --export-mind pupil.mind
./axion talk --speaker pupil.mind --listener examples/parallax-witness.mind \
  --save-speaker taught.mind < examples/drift-lessons.txt
./axion floodplain --load seen.flood --return-lesson pupil.mind taught.mind \
  --follow-witness pupil.mind --steps 0 --field-questions -
./axion floodplain --load seen.flood --return-lesson pupil.mind taught.mind \
  --follow-witness pupil.mind --steps 512 --drift-diary encounters.tsv \
  --drift-questions examples/drift-questions.txt
python3 tools/drift-audit.py --diary encounters.tsv
```

To repeat a reciprocal selection study, start all arms from the saved release. Omit `--drift-stock` and the contest arguments to start a free population. `--drift-germ FILE` exports its most prevalent expressed program. `--drift-probe 64 --drift-food-germ FILE` screens the actual local repertoire and exports the food-selected individual.

```sh
./axion floodplain --side 128 --density 100 --seed 3913 --neurons 256 \
  --neurogenesis inherited --drift tidal --drift-load 32 --drift-stride 1 \
  --contact-remodel gated --drift-stock examples/estuary-born-student.mind \
  --drift-contest sham --drift-label 7019 --steps 0 --save parents.flood
for arm in reciprocal swapped sham
do
  ./axion floodplain --load parents.flood --drift-contest "$arm" --steps 4096
done
python3 tools/drift-audit.py docs/drifting-food/pilot-live.tsv.gz
python3 tools/drift-audit.py --legacy docs/drifting-food/infrastructure.tsv.gz
```

## GPU execution

One CUDA lane advances a four-site physical encounter with four possible contenders. The core uses register selects for indexed corners, warp reductions for shared ledgers and sharded counters for sparse lineage records. Its compact current contacts add **zero bytes per creature** to an existing float-work animal. The simulation allocates 448 KiB of aggregate current counters across a million-site world. Opting into private sight on a habitat that lacks an episode store adds 12 bytes per site. Tracking individual seams, recording encounters and profiling have their own optional measurement buffers. An animal's developmental allocation remains part of the wider neural arena.

Three paired 1,024-tick runs in the million-site watershed recorded **55.84–55.98 million site-ticks/s** for advancing the entire habitat. Each pair started from the same state. Elapsed process time is recorded separately from simulation time and isolated device events. The throughput is hardware-, population- and observer-dependent. [Per-run performance](drifting-food/performance.csv)

On the RTX PRO 2000 Blackwell laptop, the current's core kernel used **96 registers and zero bytes of local memory per lane**. The earlier indexed version used 95 registers and 32 local bytes. Eighteen profiled runs alternated three accounting implementations and two measurement configurations. With aggregate physical counts, the medians of the three per-run device medians for atomic, hybrid and warp accounting were **512.8, 508.6 and 508.5 microseconds** per 262,144 candidate tiles. With genotype-specific harvest attribution they were **518.1, 512.7 and 522.2 microseconds**. The default uses hybrid accounting. Device images with sight or tracing have additional resource requirements. The full mobile-cargo and provenance release above advanced at **54.10 million site-ticks/s** with the heavy trace enabled. [Timing samples](drifting-food/performance.csv) · [Habitat intervals](drifting-food/watersheds.csv)

Native Blackwell code and driver-compiled portable CUDA produced identical habitat, current and material-trace bytes in a 256-tick replay. The original feature-off executable matched the new executable on 256 ticks of the million-site estuary. The released wild habitat also reached the same checkpoint after one 96-tick advance and after a save and restart at tick 32. Repeating the physical encounter, acoustic lessons and return to the living witness reproduced both published mind images byte for byte. [Builds, regression targets, raw audits and SHA-256 hashes](drifting-food/validation.json)

## Research that shaped the experiment

Schaffhauser and colleagues let mice forage on a physical course where the rewarding movement could reverse. Repeated reversals improved later flexibility. This motivated a real cost for reaching and a comparison of habits across hidden current changes. Scholey and colleagues separately tested stochastic choice models in human and rat patch foraging. Holding the random draw fixed in Axion lets us ask whether a learned state actually changed a particular reach or meal. [Tower Foraging Park](https://doi.org/10.1016/j.isci.2026.116498) · [Stochastic patch choices](https://doi.org/10.1038/s44271-026-00465-0)

Mice also gave up water to resolve uncertainty in an orbitofrontal study. At present the watershed records predictions and their sensory consequences. Whether its animals would incur an independent cost exclusively for information is an open experiment. Grubmüller and colleagues measured how material changes made by burying beetle parents altered the subsequent environment and success of their young. That work motivated following a specific physical construction past its maker's lifetime and measuring its effect on a later animal. [Information preference in mice](https://doi.org/10.1038/s41593-026-02377-y) · [Parental niche construction](https://doi.org/10.1073/pnas.2617749123)

Recent human single-neuron recordings distinguish uncertainty before a choice from error after its outcome. That distinction matters here because a four-way unresolved forecast and a confident wrong reach are different observations. A November 2026 human hippocampal study also found that a targeted intervention impaired inference from learned relational structure. It suggests a more demanding benchmark for the foragers. After training on several connected parts of a watercourse, can an animal use their relationship to predict a port it has never personally sampled? [Human uncertainty and error signals](https://doi.org/10.1038/s41593-026-02475-x) · [Human relational structure and inference](https://doi.org/10.1038/s41562-026-02627-w)
