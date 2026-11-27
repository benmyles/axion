# Choosing for an outcome several steps away

A 948-byte Wayfarer learned locally experienced transitions before receiving any food. It could subsequently follow a bodily need through seven decisions, revise its first action after learning about a distant change, and retain earlier object skills. Its ecological, teaching and deployment circuits share the same CUDA implementation. The experiments below distinguish explicit selection for those abilities from selection solely through foraging and reproduction.

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
tools/navigation-study.py lesson --stage revised --depth 7 --rounds 8 --destination food --diversion both |
  ./axion agent --load examples/wayfarer.mind | tail -1
tools/navigation-study.py replay --population examples/wayfarer.mind --depth 7 --rounds 8 --destination food --diversion both --all-routes
./axion life --load examples/wayfarers.eco --steps 0 --mazes 32 --rooms 32 --exploration roam --destination food --diversion both
```

The first stream poses 16 questions without feedback. The second places a fresh copy of that same individual at each entrance for each need and follows its actual muscular choices. It prints the landmarks visited, the effector discharges and the length of each journey. The last command hatches animals and tests them on private cyclic maps. [Individual](../examples/wayfarer.mind) · [Population](../examples/wayfarers.eco)

## Why represent transitions?

A 2026 human study found that linking formerly separate sequences in virtual environments changed hippocampal representations and enabled new multistep predictions. A second study measured human brain activity during navigation among abstract images and found that the time required to plan depended on distance during novel routes. A November 2026 intervention also supplied causal evidence for hippocampal involvement in inference using a learned relational structure. These observations motivated tests in which changing a distant transition can alter a decision at a place the animal has never revisited. Neither the experimental tasks nor their measurements establish a neural implementation for Axion. [Sequence integration](https://pubmed.ncbi.nlm.nih.gov/42421578/) · [Human sequential planning](https://doi.org/10.1073/pnas.2513547123) · [Causal intervention on relational inference](https://www.nature.com/articles/s41562-026-02627-w)

Earlier Axion creatures could recall useful grips from food consequences. Scout received 960 neutral doorway encounters across four five-step environments through its original public sensory interface. It answered 16/64 initial route questions before practice and 16/64 after. The earlier route playground has an engineered graph-search procedure. The experiment here starts with ordinary spiking animals whose transition plasticity initially copies its old state. [Scout control](navigation/baseline.csv) · [Earlier route playground](transfer.md)

## An evolvable circuit on the GPU

A creature has 32 sensory ensembles with four efference-tagged contacts per ensemble. A contact occupies one byte. Five bits can encode another sensory ensemble. The remaining bits can respond to an observed event, displacement or lack of displacement. Each bit updates according to a heritable Boolean table with inputs from its own prior state, the observed sensory bit, the previously active ensemble and the efference copy. A second set of 128 contacts can associate places with bodily outcomes. An organism can learn a transition, ignore it, preserve it despite contradictory experience or overwrite unrelated contacts. Which physical effector bits carry a motor class and which learned bits permit a signal to pass are inherited. [Circuit](../src/wayfinding.cuh)

When a desired sensation or a learned nutritional association fires, transient spikes can propagate over these contacts. Each ensemble applies an inherited three-input Boolean function to its prior activity, its incoming excitation and its input from the goal. An inherited temporal readout integrates up to 16 pulses. Mutations can alter transmission, sensory and motor wiring, the update functions, the wave horizon, the handling of simultaneous alternatives and the currents delivered to muscles. A creature can also gate competing dendritic or somatic motor currents according to incoming activity. The circuit contains no distance table. The experimental environment independently computes optimal distances for scoring after the animal acts. [Private corridors](../src/crossings.cuh) · [Cyclic environment and distance oracle](../src/terrain.cuh)

The complete extension uses **164 bytes per animal**. These comprise 16 bytes of germline, 128 bytes of action-conditioned sensory contacts, 16 bytes of nutrient associations and four bytes of pending sensorimotor history. No adjacency lists, trajectory histories or priority queues are allocated for a decision. An animal with 256-cell capacity, contextual memory, the prospective food organ and this circuit uses 924 bytes of device state and saves in 948 bytes. The 32-ensemble capacity bounds alias-free sensory representation. Four motor classes bound the number of physical alternatives distinguished at one place.

## What reaches the animal

In a corridor world, four entrances lead to four distinct passages. The exit assignment is independently permuted at every entrance. At subsequent places one private muscle choice advances and the other three encounter the local wall. During practice the animal chooses actual actions at each presented place and receives the resulting sensation. It receives no reward for a successful motion. One test supplies a desired sensory pattern. Another supplies only a bodily need after four passive meals at the terminal places. An altered distal junction can be explored without revisiting any entrance. Alternatively, four further meals can change which place meets a need without another doorway trial. The public actor can experience both changes.

Cyclic worlds contain eight, sixteen or 32 places, bidirectional ring edges, directed shortcuts and walls. Every room has its own private permutation of effector labels. The graph permits cycles and multiple ways to reach a goal. In local practice each room is presented repeatedly. In roaming practice the learner chooses every move during one uninterrupted outing. The correction phase takes it only to altered junctions or food sites. The scorer restricts the changed-choice comparison to untouched starting places with a unique best first move that actually differs between the old and new problem. An independent endpoint then executes the animal's policy from start to finish and measures the actual path length. [GPU tests on cyclic graphs](../src/labyrinth.cuh) · [Independent sensory stream and executable replay](../tools/navigation-study.py)

The public commands `travel RAW GOAL EXITS [NEEDS]` and `pace RAW GOAL EXITS [NEEDS]` accept a private environment description with four destination indices in four bytes. The policy cannot inspect that word. The environment uses it after an effector has fired. In an external simulation, `stride RAW GOAL [NEEDS FOOD]` accepts consecutive raw observations and links the result to the preceding discharge without supplying any transition table at all. `savour RAW FOOD` pairs a location with an absorbed nutrient without an action. `orient` and `face` are read-only choices. A zero goal with a nonzero need leaves the animal to provide its own target through acquired contacts.

## How the populations were bred

Six independently initialized cohorts of 256 candidates ran for 384 generations in a mixture of landmark- and need-guided corridor lifetimes. Local synaptic tables initially copied their input state. All animal genes could mutate. Selection used acquisition and revision choices on one-step problems and corridors up to three steps. The separate population ranking also included four-step problems. Seed 1277 was best on this validation and supplied the published initial population. Other founders often failed on at least one task. Depths five through seven, cyclic graphs and the joint public histories below were outside breeding and cohort selection. [Every founding cohort](navigation/young.csv) · [From-scratch population](../examples/landmarks.eco)

We transplanted just the 16-byte inherited program from that population onto Scout and ran six further cohorts of 1,024 animals for 768 generations. Only the 16-byte program could change in this stage. The selected population handled the new routes on its own. When a fully trained map was followed by the old object lessons, the 164 map bytes remained identical in all four public histories while its choices deteriorated. This located a failure at expression of existing information. When object lessons came first it also acquired a somewhat different map. [Integrated cohorts](navigation/integrated.csv) · [Concurrent skills before further breeding](navigation/joint-before.csv)

The final six cohorts included earlier motor experiences in some lifetimes and allowed the transition circuit to gate competing activity. Across four independent external teaching streams, two lesson orders and four returning contexts, the selected individual answered **512/512 route questions**, **2,048/2,048 hunger-dependent grip questions**, and **2,811/4,096 previously unseen property pairs**. In the route questions, both the far-end doors and the nutritional identities of the destinations had changed. For 416/512 questions the correct first action differed from its original one. These totals include repeated probes under four contexts within each independent history. The saved example answered all 128 new pairs in each of its four context branches. The matched treatment allowing extra prior experience with the earlier readout also reached 512/512 route questions. Learned shunting is one permitted mechanism. [Final cohorts](navigation/reared.csv) · [Matched earlier readout](navigation/reared-tonic.csv) · [Fresh-lifetime competition](navigation/fresh-competitive.csv) · [Joint individual results by history](navigation/joint.csv)

## Reproduce the integrated animal

```sh
tools/navigation-study.py breed --wild --navigation exploratory --destination mixed --depth 3 --rounds 6 --side 16 --generations 384 --save landmarks.eco
tools/navigation-study.py breed --population examples/scouts.eco --donor landmarks.eco --navigation exploratory --destination mixed --depth 3 --rounds 8 --side 32 --generations 768 --save integrated.eco
tools/navigation-study.py breed --population examples/scouts.eco --donor integrated.eco --navigation competitive --history mixed --destination mixed --depth 3 --rounds 8 --side 32 --generations 512 --save wayfarers.eco
tools/navigation-study.py combined --population wayfarers.eco --rounds 8 --save wayfarer.mind
tools/navigation-study.py audit --population wayfarers.eco --destination food --diversion both
tools/navigation-study.py mazes --population wayfarers.eco --destination food --diversion both
```

## Transfer to unseen maps

Wayfarer had been selected on corridors up to four steps long, including cohort selection. Four independent audit seeds supplied 32-room cyclic worlds. The animal chose its own movements throughout exploration. Four meals grounded its destinations in bodily needs. After both the shortcuts and their foods changed, the experiment restarted it at independent locations and ran the complete resulting policy.

| Encounters per room on average | Goals reached | Shortest journeys | Changed first decisions |
| --- | ---: | ---: | ---: |
| 4 | 1,916 / 2,048 | 1,387 / 2,048 | 9,679 / 14,584 |
| 12 | 2,008 / 2,048 | 1,939 / 2,048 | 13,711 / 14,584 |
| 24 | 2,017 / 2,048 | 1,992 / 2,048 | 14,285 / 14,584 |
| 24, fixed local contacts | 708 / 2,048 | 118 / 2,048 | 3,646 / 14,584 |
| 24, a single propagation pulse | 268 / 2,048 | 249 / 2,048 | 2,111 / 14,584 |
| 24, nutrient associations silenced | 1,051 / 2,048 | 185 / 2,048 | 4,841 / 14,584 |

The action readout, number of sensory ensembles, graph family and range of goals constrain this test. In the independent seven-step corridor audit, the intact population chose correctly after revision on 16,164/16,384 trials. Freezing local plasticity reduced that to 4,096. The saved individual completed all 16 public seven-move journeys in the expected number of steps. [Cyclic maps and interventions](navigation/maps-food-both.csv) · [Corridor controls](navigation/reared-food-both.csv) · [Every public journey](navigation/journeys.txt)

## Selection through foraging

Private habitats provide four kinds of mineral, carbon, a limited pouch for each mineral and finite body energy. The animal can walk, open fruit, oxidize the material it has acquired and pass material to offspring. Each candidate experiences private catchments with different motor labels and seasonal foods. Breeding considers material transferred to offspring, reserves and time alive. A separate growth evaluation ranks whole founding cohorts using offspring mass in two catchment sizes. Cognitive tests run after selection. [Metabolism and breeding](../src/migration.cuh) · [Resource-selected cohorts](navigation/growth.csv)

A population bred with grip-mediated movement transferred **255,940 units** to offspring across 2,048 long lifetimes on held-out 32-room maps. Freezing transition plasticity reduced this to 79,203 units, silencing the output to 8,344 and silencing learned nutrient associations to 122,552. On independent, read-only probes of the same experienced animals, the intact animals chose a shortest move toward the needed spring on 4,951/16,036 trials. Erasing their acquired map reduced that to 4,072. Their corridor scores remained close to chance. Transplanting these animals into a world where walking required leg discharges often led to extinction. [Resource and choice interventions](navigation/migrants-audit.csv) · [Shared-habitat transfers](navigation/migrants-habitats.csv)

A second population experienced a catchment in which locomotion required a leg discharge. It transferred 14,303 units in the same long-lifetime audit. Freezing its contacts or silencing their output reduced this to zero. Its descendants also established populations in the shared cellular-automaton food web, where animals compete for the same moving resources and can modify their environment. Four aquifers supply season-dependent minerals. In four independently seeded watersheds, the animals and plants could mutate throughout 32,768 ticks. The endpoint counts were 362, 203, 90 and 235 living animals. The deepest surviving lineages had undergone 132, 261, 214 and 197 divisions, respectively. The saved watershed is the first of those four worlds. [Founding population](../examples/nomads.eco) · [Foot-based foraging](navigation/nomads-audit.csv) · [All field treatments](navigation/nomad-field.csv) · [Living watershed](../examples/watershed.eco)

Paired competitions tested whether the new contacts mattered in the shared ecology. Eight world seeds each ran twice with the cohort assignments exchanged. Animal mutation was disabled. At tick 2,048 intact animals outnumbered competitors with silenced navigation output in all 16 worlds, with 6,773 intact animals and zero silenced competitors across the endpoints. Against animals with frozen navigation contacts, the intact animals won 15 of 16, with a pooled count of 5,251 to 1,116. Against animals limited to one pulse, they won 8 of 16, with 3,435 to 3,522. Learned contact changes aided competition here. Long-range propagation has a clearer effect in the controlled navigation tasks. [Side-swapped competitions](navigation/nomad-duels.csv)

```sh
tools/navigation-study.py migration --population examples/spring-foragers.eco --locomotion hand
tools/navigation-study.py migration --population examples/nomads.eco --locomotion foot
tools/navigation-study.py habitat --population examples/nomads.eco --navigation exploratory --side 32 --season 128 --spring-flow 16 --seep 2
tools/navigation-study.py duels --population examples/nomads.eco --navigation exploratory --side 32 --season 128 --spring-flow 16 --seep 2
./axion life --load examples/watershed.eco --steps 2048 --report-every 128
```

## Blackwell execution

On the NVIDIA RTX PRO 2000 Blackwell Generation Laptop GPU, a complete shared world starting with **1,048,576 Wayfarer brains and 1,048,576 plants** used **1,035,338,324 bytes** of explicit resident simulation buffers. After 16 ticks it had 941,237 surviving animals. The median throughput over four runs was **76.42 billion expressed somatic cell updates/s**. Every tick also advanced the navigation organ, food prediction, mutable connections, plants, body metabolism, reproduction and competing animals. Driver memory, machine code, temporary observation buffers and final graphics generation are outside that buffer count. [Measured world sizes and fingerprints](navigation/field-scale.csv)

For a separate frozen-inference experiment, 65,536 separately allocated individuals each answered 64 route questions. Each individual retained its full 924-byte state, and the decision kernel read the same inherited circuits and memory used in the deployed actor. The 60,555,264 bytes of replicated state exclude inputs, outputs and the driver. Median throughput across 12 timed launches was **180.85 million complete choices/s**, with all 4,194,304 choices correct in every timed launch. Prior to moving already-due synaptic feedback outside the frozen sequence, the same work ran at 162.54 million choices/s and used 88 bytes of thread-local memory per thread. The final kernel reports 246 registers and zero local bytes per thread. The benchmark cycles 16 scored questions per brain, uses CUDA event times and excludes transfers. [Paired frozen-inference measurements](navigation/inference.csv)

The field kernel packs eight saturating motor currents into one 32-bit word. Each warp subgroup computes the four channel currents in parallel, shares them through register shuffles, and skips the remaining wave iterations when the active cells reach a fixed point. An experiment that tried to prove the winning muscle from current bounds was slower on all three tested batch sizes. The released inference kernel retains the faster evaluation. [Component comparisons](navigation/speed.csv) · [Current-bound experiment](navigation/current-bound.csv)

```sh
tools/navigation-study.py lesson --stage revised --depth 7 --rounds 8 --destination food --diversion both |
  ./axion agent --load examples/wayfarer.mind --batch-bench 65536 --decisions 64
./axion life --habitat thicket --metabolism food-fuel --seep 2 --leach 3 --fruit inherited --species 8 --climates 4 --season 16 --stand-size 8 --synapse-bits 2 --relations contexts --programs 2 --polarity evolvable --navigation competitive --founders examples/wayfarers.eco --side 1024 --density 100 --seed 1847 --steps 16
```

The same snapshots were run before and after optimization at all five neural widths and four relation settings. Complete ecological checkpoints and experienced individuals matched byte for byte in all 20 configurations. Another 83,620 batched decisions produced identical scores and motor fingerprints. Separate native and portable PTX tests cover learning, inheritance, deterministic state restoration and read-only probes. [Per-configuration checks](navigation/equivalence.csv)
