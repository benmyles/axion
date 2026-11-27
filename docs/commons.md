# Food, exploration and communication in a shared commons

A resident can use its current knowledge to place a trap or spend part of its energy on an experiment of its own choosing. Thirty-two residents share four finite food reservoirs on each island. Neither a correct prediction nor a useful sound earns fitness points. A resident must obtain food, cover its metabolic and investigative expenses, and deposit surplus energy into reproduction. In the default population process, a family's probability of contributing a descendant is proportional to that deposited material. A generation with no deposits ends the run.

```sh
./axion commons --load examples/wild-newborn.mind --families 128 --replicates 8 --days 128 --exposures 0 --receipts learned
./axion agent --load examples/wild-explorer.mind --inspect
```

## What happens on a day

Every resident runs its own detached spiking body on CUDA. Its somatic motor output, the result of its previous action, and uncertainty in its local prediction reach an inherited action-selection table. A trap intercepts a packet if it is at the correct bearing and wins access to the shared reservoir. Rotation of allocation priority prevents a permanent advantage for low-numbered residents. An experiment lets the resident arrange two objects at any of sixteen positions, observe their actual departures and arrivals, and change its own synapses. It uses inherited response strengths to compare disagreements among its predictions. It receives no information bonus.

The environment can reverse which object controls each output without announcing the change. Aliased landmarks can have the same effect despite different appearances. Distinct landmarks have four different physical consequences. The optional receipt pathway exposes one observed arrival after a meal and only the attempted location after an empty trap. An inherited gate can suppress either update. A resident that spends its energy stops foraging. Every reported cohort balances both its food packets and its energy. Twelve energy units fund a unit of reproductive investment recorded as 12 units of material in the tables.

The nursery recruits the next cohort from that material, samples mutations of the somatic genome and expressed plasticity programs, and clears the newborns' acquired contacts. The fixed number of places per generation makes this an experimental common garden. The original spatial ecology supports continuous births, deaths, movement and evolving plants. `--save` exports an experienced individual from the most productive family. `--save-newborn` hatches the same family's inherited program for an independent developmental test when the population has funded offspring. [CUDA environment](../src/commons.cuh) · [Population schedule](../src/commons_host.hpp) · [Experiments](../tools/commons-study.py)

## Does investigation pay?

Six populations per treatment ran 96 generations of 128 families with eight residents per family for 96 days. Founders inherited the visual, motor and source-learning programs developed earlier, with variation in their investigation policies. In the first series each newborn had eight confounded observations. In the second series it had **zero natal observations** and could acquire evidence only during its own activity. During the latter series the optional trap-feedback pathways could evolve. The number of objects, sensory cells, actuators and reservoir mechanics are part of the environment and architecture.

| Final generations across six populations | Reproductive material, mass-weighted recruitment | Reproductive material, random parents |
| --- | ---: | ---: |
| Eight natal observations | 366,228 | 104,820 |
| Zero natal observations | 249,816 | 14,412 |

The released family was selected on final cohort deposits before testing on fresh islands. Eight fresh 128-day runs allocated 8,192 independent newborns. With no natal observations the selected family conducted 49,733 experiments and deposited **550,248** units. The drift family deposited 50,004. Forcing the selected family's animals to harvest every turn yielded 95,232, and forcing them to investigate every turn yielded zero. Choosing experiments randomly yielded 467,844. Freezing learned source support yielded zero.

This selected family made zero trap-contact updates even when the receipt pathway was available. Cutting somatic contributions to the decision left 549,204 units. Its main advantage in the solitary task came from the plastic object associations and the policy for using them. In the primed, food-scarce treatment, random experiment choice actually beat the selected choice policy on deposited material. The outcome depends on the environment. [Primed selection](commons/mass.csv) · [Primed drift](commons/drift.csv) · [No-demonstration selection](commons/free-play-mass.csv) · [No-demonstration drift](commons/free-play-drift.csv) · [Fresh free-play worlds and controls](commons/free-play-confirmation.csv) · [Drift confirmation](commons/free-play-drift-confirmation.csv) · [Scarcity](commons/scarce.csv)

## Can a call tell a neighbor what it saw?

In the split-view world each pair encounters the same initial arrangement. Each resident can see only one object's initial bearing. Its simulated spiking body can emit a pitch or remain silent. Its partner hears that physical emission on the next neural beat. Paying for an experiment opens the concealed view and lets auditory contacts learn an association between the sound already heard and a physical sighting. No table tells either animal what a pitch means. A shuffled call comes from the corresponding role on another island. Kin clustering puts relatives together during reproduction.

With somatic, visceral and visual inputs mixed during the first beat, sounds from the right partner and from a different island decoded at approximately 25% on the four-way task. Suppressing sound reduced income, but displacing a call had little effect. This first result gave little evidence for information about the concealed bearing. We then made the first vocal beat use the same focused retinal transducer as an individual inspecting a view in the interview. After six new 128-generation cohorts, the selected family decoded 416,689/877,954 correct bearings on fresh islands. A different island's calls gave 202,065/811,006. Its reproductive material was 448,260 with its partner, 408,792 with displaced calls, and 327,048 with incoming sound suppressed. [Initial channel](commons/social-confirmation.csv) · [Focused-eye selection](commons/focused-mass.csv) · [Fresh focused-eye interventions](commons/focused-confirmation.csv)

The tandem apparatus requires both residents to set their respective traps correctly before either can obtain a packet. We used four distinct outcomes, 24 starting energy units, seven energy units per packet, and zero teaching observations. Six populations ran for 256 generations under selection on reproductive material. Every animal supplied its own actual optical observations, experimental history, cortical discharges and effort. Eight new seeds gave the following results for the selected newborn family.

| Tandem treatment | Reproductive material | Correct decoded bearings | Decoded bearings |
| --- | ---: | ---: | ---: |
| Partner's voice | 1,134,252 | 474,105 | 853,453 |
| Another island's voice | 279,660 | 86,638 | 345,772 |
| Incoming sound suppressed | 71,028 | 0 | 0 |
| Hearing contacts frozen at birth | 72,396 | 0 | 0 |
| Somatic motor output disconnected | 119,412 | 0 | 0 |

Income changes survival and consequently changes the number of occasions on which a signal can be decoded. A separate read-only audit used the same resident, neural contacts, scene and time for three alternative sounds. It retained trials only when all three would elicit a unique reading. Of **727,192 matched readings**, 407,483 were correct for the partner's current sound, 184,902 for a preceding sound from the same partner, and 184,587 for another island's current sound. The alternative readings never affected behavior or learning. The released family carried about 0.83 bits per emission about the observed bearing in the original confirmation. This is mutual information calculated separately within each island and visible role, including silence, then weighted by the number of vocal opportunities. [Tandem selection](commons/tandem-mass.csv) · [Random recruitment](commons/tandem-drift.csv) · [Independent intervention streams](commons/tandem-confirmation.csv) · [Matched readings](commons/matched-calls.csv) · [Experienced animal](../examples/tandem-forager.mind) · [Newborn](../examples/tandem-newborn.mind)

To test dependence on the visual addressing, we sampled four afferents without replacement from 32 cells independently for each role and island. These addresses change across generations. Four populations trained for another 512 generations. In unfamiliar wiring and fresh encounters, the selected family deposited 853,164 units with the partner and 253,320 with displaced incoming calls. On 523,289 matched readings the actual sound gave 253,382 correct bearings, compared with 132,668 at a delay and 132,617 when displaced. [Unreared optical transfer](commons/shuffled-transfer.csv) · [Breeding on changing optics](commons/shuffled-mass.csv) · [New optical worlds](commons/shuffled-confirmation.csv) · [Matched shuffled-optics readings](commons/matched-shuffled.csv)

```sh
./axion commons --load examples/tandem-newborn.mind --families 128 --replicates 8 --days 128 --channel open --neighbors kin --retina focused --landmarks distinct --pods tandem --reserve 24 --calories 7 --exposures 0 --receipts learned --audit-calls on
python3 tools/commons-study.py audit --source examples/wild-newborn.mind --days 128 --exposures 0 --receipts learned --arms intact harvest random fixed
```

The independent garden comparisons can be rerun directly with the saved random-parent families. [Primed drift](../examples/controls/primed-drift.mind) · [Zero-demonstration drift](../examples/controls/free-play-drift.mind) · [Split-view drift](../examples/controls/social-drift.mind)

## A million full spiking residents

The RTX PRO 2000 Blackwell laptop ran **1,048,576 independent 256-cell residents** across 32,768 islands. Each of three seeds ran 32 days. The solitary environment used **655,978,768 bytes** of simulation buffers and took a median **133.035 ms**, or 219.0 million living resident-days/s. The tandem environment used **699,756,816 bytes** and took **321.160 ms**, or 99.7 million living resident-days/s. A tandem day includes two separate somatic discharges, vocal transmission, hearing, investment or trapping, competition, and the resulting online updates. Both measurements include graph dispatch and exclude hatching, census downloads, rendering and driver-owned storage.

The kernel stores each island's stable physical transformation once, computes the four exploratory readouts once per search, and allocates private auditory state only in environments with sound. The previous layout took 145.298 ms solitary and 334.757 ms tandem on the same streams. All recorded demographic and behavioral totals matched across the 18 before-and-after runs. The solitary layout saves 43,646,976 bytes at the largest size. [Original layout](commons/scale-reference.csv) · [Current layout](commons/scale-packed.csv) · [Shuffled-optics individual](../examples/shuffled-forager.mind) · [Shuffled-optics newborn](../examples/shuffled-newborn.mind)

## What transfers to a human interview?

We also put actual adult foragers through the earlier 1,024-round joint-play course, pitch practice and movement demonstrations, with no new generations. The wild explorer went from 4/32 to 14/32 on current-location questions and from 7/32 to 15/32 on exchanges. The islander went from 10/32 to 9/32 on current locations and from 13/32 to 7/32 on exchanges. Ecological success, a useful call, and robust performance in the human interview are distinct observed capabilities. The default interview individual remains the separately tested naturalist. [Wild adult](commons/reared-wild.csv) · [Islander](commons/reared-islander.csv) · [Joint play omitted](commons/reared-without-joint-play.csv) · [Rearing experiment](../tools/rearing-study.py)
