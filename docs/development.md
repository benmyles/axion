# Learning a hidden world from its consequences

A newborn starts with one recurrent state. It can move, sense a 64-bit spike pattern and, in some environments, return to a familiar starting point. Two visits can look identical and still have different consequences after the same motor action. Axion can recruit separate cells for those histories and carry the distinction forward indefinitely.

The saved acoustic pupil learned from another animal's emitted pulses. It distinguishes an object's current position from the position available to a witness who went outside. The following exchange works with the public executable.

```sh
./axion talk --speaker examples/witness-pupil.mind
```

```text
/home
inside
left
outside
right
Where is it?
animal> hidden right
Where will fox look?
animal> expects left
inside
Where will fox look?
animal> expects right
```

The first four statements place the witness inside, move the object left, send the witness outside and move the object right. The last statement brings the witness back into view. Spelling is a reversible physical port encoding. The learner sees spikes, including silence. The host splits words and gives the animal listening ticks to finish a reply. The learned recurrent contacts determine which sounds it emits. The caregiver's 12 available gestures and five possible sounds delimit this experiment. A person supplied the caregiver's physical and linguistic conventions. The result establishes mastery of that interaction and leaves general language understanding and theory of mind unproven. [Live exchange](../journal/runs/00005998-talk/plate.webp)

## Research and design

Pan and colleagues found that people used learned relational structure to infer new relationships. The study also found that perturbing hippocampal activity impaired structure-based inference. Schiereck and colleagues tracked belief updating in rat orbitofrontal cortex during a task with hidden states. Axion takes two testable requirements from these results. Past observations must be able to change the interpretation of the same present stimulus, and a circuit must be able to infer consequences it never experienced in that particular sequence. [Pan et al., November 2026](https://www.nature.com/articles/s41562-026-02627-w) · [Schiereck et al., Neuron](https://pubmed.ncbi.nlm.nih.gov/41253149/)

A July 2026 preprint showed that a learned perceptual encoder and a clone-structured model could jointly recover maps in visually ambiguous worlds. Work on early speech learning also showed that predictive learning can induce categories from unlabelled input. Axion tests a much smaller input surface. It clusters 64-channel pulse patterns, then recruits recurrent cells only when future observations can distinguish them. Natural audio segmentation and perception from camera images remain open. [Nikzad et al., 2026 preprint](https://arxiv.org/abs/2607.12382) · [Simulating Early Phonetic and Word Learning Without Linguistic Categories](https://pubmed.ncbi.nlm.nih.gov/39763096/)

Active experiments draw on classical finite-automaton inference, including the use of counterexamples to find a distinction inside a sequence. Axion generates its own candidate counterexamples by moving in the environment. The verifier has access to the physical law only after learning. All CUDA kernels, the external port and the ecology were written for this project. [Rivest and Schapire](https://people.csail.mit.edu/rivest/pubs/RS93a.pdf)

Two recent artificial-life studies shaped the ecology tests. Swarm-robotics work showed that agents can develop grounded lexicons through interaction. Experiments with neural cellular automata separately measured diversity in inherited weights and their expressed behaviors. Here a neighbor must hear actual pulses, and we count two vocal organs as behaviorally equivalent when every possible sequence of gestures elicits the same waveform sequence. [Grounded lexicons in robot swarms](https://doi.org/10.1016/j.asoc.2026.115879) · [Phenotypic and genotypic diversity in neural cellular automata](https://pmc.ncbi.nlm.nih.gov/articles/PMC13705881/)

## What develops

* A sensory prototype stores two bit planes for 64 receptors. An inherited table maps the previous two-bit state and the incoming spike into a new state. Genes control recruitment distance, confidence weighting and initialization. Masked receptors abstain from updates.
* In active rearing, an animal executes a motor sequence from home, then compares the predicted and experienced waves. A discrepancy prompts shorter physical experiments. Reproducible differences can recruit a cell and redirect the incoming contacts whose continuations changed. Some probes reuse the animal's earlier distinguishing experiences. Every trip and every motor action counts toward the reported cost.
* During uninterrupted life, a 392-byte circular memory retains up to 256 motor and sensory events. A CUDA warp replays them into a temporary graph. It can test whether a recurring sound localizes the animal, then test whether two histories can share a cell. A proposed fusion forces all overlapping motor contacts to agree recursively. A contradictory observation rolls back the whole fusion. The inherited program controls fusion evidence, search breadth and which auditory landmarks to test.
* During action, a 64-bit spike population represents possible current states. Observed consequences filter the population. The creature can seek a waveform by tracing acquired contacts and agreeing on a first gesture across its viable states. `/lost` removes position knowledge, `/seek ECHO` exposes a proposed gesture, `/show GESTURE ECHO` admits new observations and `/sleep` consolidates them. `/raw` exposes each physical response without automatically advancing a listening tick.
* A 96-bit germline encodes sensory plasticity and structural decisions. The developmental nursery selects genomes on waveform accuracy in new walks through small physical worlds. Crossover and mutation operate on those bits. The primitive search operations, capacity limits, physical channel and reproduction mechanism are engineering choices. This supplies measurable evolutionary freedom within a designed substrate.

This is a discrete recurrent spiking circuit with a state-induction algorithm. A successful finite-state reconstruction can imply perfect behavior on an infinite set of input histories while using very few cells. Neither that result nor a fluent answer in this small dialect measures human-level intelligence.

## Independent evidence

The founder's rules evolved for 96 generations of 128 candidates, using two small worlds per candidate per generation. Each new animal started development with empty learned contacts. The caregiver, the social-transmission experiment and the independently written seven-state apparatus were excluded from selection.

| Experiment | Result |
| --- | --- |
| Fresh eight-state worlds with four gestures and four sensory waves | 127/128 learned circuits had exact raw outputs for every finite history. One failed on a one-action witness. |
| Physical caregiver | 31/32 animals exactly reconstructed the 24 distinguishable states. The verifier found a six-action failure in the remaining animal. |
| Transmission from the first certified caregiver | 32/32 pupils reconstructed all 24 states. Pinning their recurrent activity to the home state reduced exact circuits to 0/32 on the same learned contacts and training budget. |
| External apparatus with an independently generated law and unfamiliar words | The pupil chose 346 trips comprising 3,452 moves including returns. An independent verifier matched every one of the 21 reachable state–motor contacts. |
| Delayed conversation | Both the compiled CUDA animal and an independent test agreed after 65,536 irrelevant inputs. The pupil produced different replies for physical and witnessed location and changed its reply after the witness returned. |

The public verifier explores the product of the learned and physical states from home and compares the complete 64-bit output on every reachable edge. A missing transition fails. A discrepancy yields a shortest counterexample for that circuit. Independent partition refinement counts the minimum number of distinguishable physical states. The learner's stopping criterion is empirical, so neither a quiet period nor a good sampled score constitutes a proof. [Founder](../journal/runs/00005995-develop/plate.webp) · [Caregiver](../journal/runs/00005996-develop/plate.webp) · [Peer transfer](../journal/runs/00005997-develop/plate.webp) · [External evidence](../journal/runs/00005999-develop/plate.webp)

A second campaign used 128 new worlds per condition. Zero-noise active rearing recovered 128/128 four-state rings, 128/128 four-state networks, 128/128 sixteen-state networks and 127/128 thirty-two-state rings. With one uninterrupted 256-movement lifetime, the four-state ring and network scores were 108/128 and 61/128. Both longer challenges fell to 0/128. All 128 actively reared animals in the physical 32-state latch task recovered its four behaviorally distinguishable states. Flipping two randomly selected receptor bits at each training contact reduced the sixteen-state active score to 91/128 under the strict clean-waveform test. Active rearing used more movements, sometimes by two orders of magnitude. [Costs and results for all 20 cohorts](development/discovery.csv)

## Living voices

Thirty-two creatures share each island. Every creature has a predictive brain and a separate recurrent vocal organ. Its voice comes from three to six locally connected spiking cells arranged in a ring. Heritable truth tables select how the neighboring spikes and the incoming gesture drive each cell. Mutable receptors and output projections determine which gestures affect the ring and which sounds it emits. Both members of a pair call and respond.

Each juvenile first hears its partner through an uninterrupted sensorimotor stream. In adulthood a scent marks a flowering resource for eight decisions. A call that elicits its matching waveform can open nectar. Callers pay one energy unit per attempt. A successful meal returns six. Partners compete with their neighbors for a limited number of meals. Energy weights reproduction within each island and exhausted islands can go extinct. Newborns inherit mutable learning rules, voice physiology and action preferences. Their auditory contacts develop again.

Across four preselected seeds, 512 residents and eight seasons per seed, the learned policy harvested more food than wandering or scrambled hearing in all four paired comparisons. Fitness selection yielded fewer total meals than reproductive drift over this horizon. Selection increased the fraction of predictable responses. Its value for long-term ecological productivity is still unresolved.

| Inheritance and behavior | Nectar meals | Motor calls | Correct next-wave forecasts |
| --- | ---: | ---: | ---: |
| Energy-selected, intact | 191,350 | 1,001,372 | 669,166 |
| Energy-selected, wandering | 171,036 | 1,005,906 | 547,203 |
| Energy-selected, scrambled hearing | 177,869 | 1,013,100 | 40,441 |
| Reproductive drift, intact | 193,338 | 1,010,278 | 599,508 |

A larger run began with 4,096 residents. After 12 seasons it had harvested 561,814 meals. The final adult season held 3,968 creatures, 327 founding lineages and 2,569 distinct vocal genotypes. Exhaustive minimization found **1,878 behaviorally distinct voices** among those 2,569 genotypes. For 3,436 residents there existed histories after which the same gesture elicited different replies. The largest voice required 64 distinguishable states. The founding population had 3,588 distinct voices, so this run documents continued diversity and a decline in the total number of behaviors. Open-ended growth in complexity remains unproven. The saved brood resumes at season 12 with newborns on 123 surviving islands. [Season data](development/chorus.csv) · [Genotypes and observable behaviors by season](development/voices.csv) · [Large colony](../journal/runs/00006037-develop/plate.webp) · [Eggs](../examples/tidal.eggs)

## Blackwell inference

A plastic developmental organ takes 3,144 bytes. The acoustic pupil's full individual payload takes 3,496 bytes and its checksummed file takes 3,520 bytes. Freezing a complete 24-state, 12-motor, five-wave circuit takes 288 bytes of packed transitions and 136 bytes of raw motor and sensory waveforms. Incomplete graphs get an additional known-contact bitmap. The payload width also shrinks for smaller alphabets.

The laptop's RTX PRO 2000 Blackwell ran 1,048,576 independent histories for 128 gestures each, using 32 independently reared graphs. The clock covers action generation, graph lookup, recurrent state advance and a rolling digest. Each layout had a warm-up and five CUDA event timings. Every packed contact and every replica's final digest were checked against the original. Sensory clustering, development, reproduction, physical simulation and host transport are outside these timings.

| Frozen layout | Resident graph storage | Median GPU time | Billion transitions/s |
| --- | ---: | ---: | ---: |
| Native 16-bit contacts with fixed row capacity, 32 shared graphs | 65,664 bytes | 0.736 ms | 182.27 |
| Packed contacts, 32 shared graphs | 9,216 bytes | 0.849 ms | 158.13 |
| Packed contacts, one private copy per history | 288 MiB | 5.900 ms | 22.75 |

The native table was fastest on this workload. Packing the shared graphs reduced storage by 86%. The private layout cyclically initializes its million graphs from the 32 teachers. These timings do not establish a global performance optimum or a million independently trained brains. [Timing data](development/throughput.csv) · [Benchmark record](../journal/runs/00006036-develop/observations.json.gz)

Learning costs differ substantially from frozen inference. In one CUDA event measurement covering birth, movement, sensory updates and structural learning, 4,096 animals investigated eight-state worlds in 9.45 ms and produced 4,090 exact circuits. They made 5,699,797 moves including returns. Uninterrupted four-state rings took 17.51 ms for 1,048,576 movements and warp replay, with 3,320 exact circuits. These measurements exclude physical world generation, genetic selection, certification and journaling. [Acquisition timings and denominators](development/acquisition.csv)

## Reproduce

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./tools/development-study.py interview --delay 65536
./tools/development-study.py external --save /var/tmp/axion-stranger.mind
./tools/development-study.py campaign
./axion develop --load examples/echo-founder.mind --origin inherited --generations 0 \
    --peer examples/witness-carer.mind --worlds 32 --seed 913 --span 40 \
    --budget 30000 --patience 2048 --replicas 1048576 --ticks 128
./axion develop --resume-colony examples/tidal.eggs --seasons 4 --save-colony next.eggs
make NATIVE=1 CUDA_ARCH=sm_120 test-development
```

The external program receives one `experiment>` line at a time. It returns to its physical home state, executes the requested gestures in order, and replies with one raw waveform per gesture. The allowed encodings are a short alphabetic word, `_` for silence, and `@` followed by up to sixteen hexadecimal digits. An opaque hexadecimal pulse can represent any simultaneous configuration of the 64 receptors.
