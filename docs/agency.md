# Learning which intervention will work

The default witness can be asked to bring about a state it has never produced in a demonstration. It learns the effects of actuators from before-and-after observations. It can imagine a sequence, explain a necessary step, issue a conditional instruction to another animal, and check what actually happened when it acts. Its causal tissue occupies 2,048 bytes. The complete witness occupies 5,036 device bytes and its companion 4,884. [Witness](../examples/gardener.mind) · [Companion](../examples/apprentice.mind) · [Recorded conversation](../journal/runs/00005867-talk/plate.webp)

```text
/greenhouse 16 0
How can pearl become fruiting?
animal> heat sow soak tend pollinate ripen
Suppose pearl sow tend fruiting?
animal> no
Why soak?
animal> skipping soak leaves pearl dry blocking fruiting
Make it.
animal> heat sow soak tend pollinate ripen
Edible pearl?
animal> yes
```

The order has physical consequences. Heating evaporates moisture. Tending a damp, sown plot makes it green. Pollination needs warmth and green growth. The apparatus presents retinal changes and motor events. Its physical gates are held in a different device buffer from the animal's acquired model. The explanatory phrase was learned by imitation across different objects, actions and causes. The particular omission and repair are computed for the current plan.

## Why these experiments

A July 2026 modelling study showed how goal-directed imagination can sample trajectories from an acquired cognitive map. A primate recording study found that animals could recombine action elements in new sequences. Research on shared neural subspaces identified reusable sensory and motor representations across tasks. These findings motivated separate tests of acquiring a transition, composing it toward a new goal, and communicating a reusable action. Axion's bit representation and backward propagation are engineering choices. [Goal-directed imagination, Nature Machine Intelligence](https://www.nature.com/articles/s42256-026-01254-4) · [Action symbols, Nature](https://www.nature.com/articles/s41586-026-10297-x) · [Shared task subspaces, Nature](https://www.nature.com/articles/s41586-025-09805-2)

The November 2026 human hippocampal study tested inference after relational structure remained useful across changes in the environment. Earlier work on causal judgment compared observed events with simulated interventions. Accordingly, the scorer changes the physical mechanism, removes a step from a proposed plan, and tries possible repairs. A plausible sentence earns credit only when its claimed intervention works in the independent physical model. [Human relational inference, Nature Human Behaviour](https://www.nature.com/articles/s41562-026-02627-w) · [Counterfactual simulation and causal judgment](https://cicl.stanford.edu/papers/gerstenberg2021csm.pdf)

## A local model and a desired outcome

Each interaction can reveal six binary properties before and after a motor fires. A field contains 15 dendrites, one per pair of input receptors. Each of the pair's four activity patterns has two mutable synaptic bits. The inherited program controls how a sighting changes those bits, which evidence activates an output, when competing inputs are inhibited, and whether a contradiction permits relearning. An unseen receptor supplies no evidence to its dendrite. A settled field compiles to a receptor pair and a four-bit truth table. The 36 fields, their genes and their visited-state masks occupy 784 bytes. [Layout](../include/axion/causal.hpp) · [Learning and compilation](../src/causal.cuh)

A warp represents 64 possible local configurations. Each lane owns two. Goal configurations fire first. Earlier configurations can join the wave when their learned motor outcomes reach active cells. The genome can express unanimous, permissive, majority or deterministic-only recruitment, a propagation horizon, and motor preference. The released line uses unanimity. For that line a guaranteed plan must work for every sensory outcome still compatible with its model. An optimistic pass distinguishes a provably unreachable request from one that lacks sufficient evidence. [Warp implementation](../src/causal_warp.cuh)

The animal can receive a conjunction of named properties, an acquired descriptive goal, or the appetitive state inferred from its own tastes. Earlier relations can select the subject of an action. In the public regression, the witness privately saw a ship change its berth. A request about the harbor of pearl warmed the new harbor. A request about the harbor its companion expected moistened the old one. [Dialogue circuit](../src/agency_dialogue.cuh) · [Public regression](../tests/journal.py)

For an explanation the witness holds the rest of its plan fixed. It evaluates omission and then tests subsets of the properties changed at that step. It identifies a smallest sufficient repair when the omitted route certainly fails and restoring those properties guarantees success. The justification concerns that plan. Other plans can reach the same goal by different means.

## Independent interviews

In each of eight language experiments, two individuals were reared separately with a new assignment of nonsense words and an acquired phrase order. An external program installed three types of physics, including feedback and latched effects. It presented 36 configurations per motor, then asked questions about new states, available motors and desired changes. During the tests its partner could attempt the instructions while the witness's view was covered. The scorer implemented physical action, breadth-first search and omission tests separately in Python. [Experimental apparatus](../tools/agency-study.py) · [Every interview response](agency/interviews.csv)

| Public test | Result |
| --- | ---: |
| Plan, arrival or justified refusal | 765 / 768 |
| Shortest plan, arrival or justified refusal | 761 / 768 |
| Forecast a sequence's effect | 763 / 768 |
| Explain omission in a successful nonempty plan | 466 / 472 |
| Partner reproduce the expected final state from speech | 472 / 472 |

In the nursery, four separately selected genotypes reached 245, 246, 246 and 217 goals in the same 256 held-out worlds. Four matched genetic-drift controls reached zero. For the released genotype, 245 omission explanations identified a minimum repair in the physical mechanism. The nursery selects on sensory prediction, transfer, revision, consolidation and a short control task. The living habitat selects parents using energy deposited for offspring. [Breeding and interventions](agency/breeding.csv) · [Released lineage](../journal/runs/00005598-cultivate/plate.webp)

### An instruction that must branch

The witness observed the outcome of opening a device only when two indicator bits agreed. The two possible physical laws produced exactly the same teaching history. At test the bits disagreed. Opening the device revealed which handle to use next. Trying the wrong handle sealed it. The independent scorer exhausted the reachable states of the paired physical systems, which proves that no finite fixed sequence of motors could succeed in both. Each individual device had a solution.

Across eight randomly rewired pairs and dialects the witness gave the same appropriate conditional utterance before either outcome was known. The recipient had learned the motor names and a conditional construction. Its transition contacts were unexposed at the start of each test. It heard the actual utterance, carried out the common first action, inspected the resulting feature, and chose a handle. Sixteen of sixteen trials succeeded with intact transmission. Muting the message, reversing it, or covering the recipient's view after the shared first action each gave zero of sixteen. The witness also succeeded autonomously on all sixteen. These conditions used fresh copies of the same individuals. [Twin-world proof and transcripts](agency/forks.csv)

### New words can name new goals

Positive and negative examples can give an unfamiliar word an operational meaning. In the public interface a sighted animal can receive `/approve cured 0 1` or hear a grounded three-word assertion such as `pearl cured yes.` The spelling carries no information about the physical property. The goal can subsequently appear in a forecast or a request for action. Across eight vocabularies, three successive definitions of the same word yielded 768/768 correct recognition answers and 768/768 correct minimum-action decisions on held-out configurations. Alternate definitions were presented as spoken assertions. [Acquisition and reassignment](agency/categories.csv)

Each of eight descriptive fields can also recruit earlier settled descriptions. Two 64-bit evidence planes retain examples. The circuit projects them against pairs of primitive or previously acquired inputs and caches the certainly and possibly positive states. A change to an earlier concept invalidates dependent projections. In the hierarchy experiment a teacher presented 32 of 64 configurations per word. The five successive words named signed parity over two through six randomly selected properties. The teacher supplied labels and examples. The decomposition was withheld.

| Number of relevant properties | Recognition with reuse | Recognition with reuse cut | Shortest goal decisions with reuse | Shortest goal decisions with reuse cut |
| ---: | ---: | ---: | ---: | ---: |
| 2 | 256 / 256 | 256 / 256 | 256 / 256 | 256 / 256 |
| 3 | 256 / 256 | 37 / 256 | 256 / 256 | 28 / 256 |
| 4 | 256 / 256 | 37 / 256 | 256 / 256 | 16 / 256 |
| 5 | 256 / 256 | 18 / 256 | 256 / 256 | 12 / 256 |
| 6 | 256 / 256 | 6 / 256 | 256 / 256 | 11 / 256 |

Goal probes also required two familiar properties to hold. The scorer searched the physical state graph to check the answer. The need for useful precursor concepts is a substantial learning bias. [Hierarchy trials and ablation](agency/hierarchies.csv)

### Every goal and motor menu

Each physical mechanism admits 729 conjunctions of positive, negative or unconstrained properties, 64 initial configurations and 64 subsets of enabled motors. The CUDA circuit received only the learned contacts. An independent host scorer computed shortest paths from the supplied physical truth tables and then actually followed every proposed policy. [Independent scorer](../src/causal_certificate.hpp)

With 36 examples per motor across 24 tasks, **71,462,688 of 71,663,616** first decisions matched a shortest physical route, an already satisfied goal, or refusal of an unreachable goal. The model withheld a move on 111,798 reachable requests and made 89,130 nonoptimal first moves. **All 20,822,434 policies that initiated an action reached their physical goals**, including the detours. With all 64 configurations observed, every first decision matched the reference. These are exhaustive queries within each of the 24 physical tasks. [Sparse experience](agency/census.csv) · [Complete experience](agency/census-full.csv)

## When the food evolves too

Thirty-two residents share a bed of 32 plots. Each plant line carries 36 mutable physical truth tables, an edibility rule and a post-harvest configuration. A resident can manipulate a plot, sample food and learn from a visible neighbour. Only one contender can work a given plot on the same day. Each season accounts for replenished food, consumption, expenditure and reproductive deposits. Optional weather copies a property between adjacent plots in a synchronous cellular-automaton update. New seed lines inherit from nearby beds in proportion to actual harvest. A bed persists when there is no local seed supply. Mutation can change a receptor, a truth-table entry, or the post-harvest configuration. [Plant and animal dynamics](../src/orchard.cuh)

One twelve-generation population began with 32 founder beds, all inedible immediately after harvest. By the final season, 11 beds were edible in their reset configuration. Of 32,974 meals, 17,860 were collected from a plot in that configuration. Five founder lines remained and their descendants carried 28 distinct genomes. This change allowed reproduction without a long growing sequence on those plots. The selected resident is available with its physical seed bank. Asked for a way to make one particular plot edible, it replied “unsure.” [Season plate](../journal/runs/00005725-commons/plate.webp) · [Resident](../examples/seed-keeper.mind) · [Actual seed bank](../examples/seed-keeper.seeds) · [Resident interview](../journal/runs/00005870-talk/plate.webp)

Across eight populations the intact final seasons deposited 620,784 offspring-energy units. Disabling prospective action selection gave 385,836. Freezing learning gave 463,260, including 19,463 meals from reset-state plots out of 22,869 meals. Fixing the original plant lines gave 34,752 offspring-energy units. The interaction between changing food and changing animals makes total offspring energy a poor stand-alone measure of cognition. [Season-by-season ecology and controls](agency/ecology.csv)

A reciprocal transplant used the winning animal family's newborn genotype and the archived plant lines from generation eight. Every comparison used fresh plots and genetically identical animal founders within that arm. Birth erased acquired action and taste contacts. Seed evolution was suspended during each evaluation season. Pooled results from eight founding seeds follow. [Individual transplant results](agency/transplants.csv)

| Animal genotype | Plant lines | Online learning | Offspring energy |
| --- | --- | --- | ---: |
| Original | Original | Enabled | 0 |
| Selected | Original | Enabled | 44,376 |
| Original | Descendant | Enabled | 0 |
| Selected | Descendant | Enabled | 699,552 |
| Selected | Original | Frozen | 13,560 |
| Selected | Descendant | Frozen | 313,248 |

## GPU cost

A frozen policy has eight possible codes at each of 64 configurations, comprising six motors, arrival and abstention. Three bit planes use exactly the 192 bits needed to store an unrestricted mapping of that shape. In the living world, each resident also carries eight bytes for cache validity and rebuild counts. Acquired evidence can invalidate the policy within a lifetime. Across nine 96-day seasons of 1,024 residents, caching reduced the median simulation time from 8.033 to 5.933 ms. All ecological counts outside the cache counters and all exported individual bytes matched. [Cache comparison](agency/cache.csv)

On the RTX PRO 2000 Blackwell laptop GPU, **1,048,576 independently learning residents** ran for 32 days in **1.324 seconds at the median of three seeds**. The complete simulation requested 5,466,358,632 device bytes. CUDA event times cover simulation days. Initialization, breeding, census, journaling and file operations occur outside that interval. [All scale measurements](agency/scaling.csv)

## Reproduce

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion cultivate --origin random --generations 100 --candidates 192 --worlds 6 \
  --cycles 3 --lessons 36 --probe-worlds 256 --seed 190753 --probe-seed 764321 --save pupil.mind
python3 tools/agency-study.py rear --speaker pupil.mind --peer examples/lookout.mind \
  --save-speaker witness.mind --save-peer companion.mind
python3 tools/agency-study.py interview --lessons 36 --trials 32
python3 tools/agency-study.py forks
python3 tools/agency-study.py categories
python3 tools/agency-study.py hierarchies --examples 32
python3 tools/agency-study.py certify --lessons 36
python3 tools/agency-study.py certify --lessons 64
python3 tools/agency-study.py breeding --seeds 190753 792211 1427251 2933521
python3 tools/garden-study.py ecology
python3 tools/garden-study.py transplants
python3 tools/garden-study.py cache --seeds 217217 541197 811945
python3 tools/garden-study.py scale --seeds 217217 541197 811945 --max-residents 1048576
```

The simulator supplies six local sensory properties, six motor ports, a bounded goal-wave circuit and a limited set of speech roles. The evolved genome governs local plasticity, competition, recruitment and aspects of motor choice. The curricula supply objects, words and demonstrations. Successful plant genotypes can make the habitat easier. The witness remained silent about why a boat floats and answered “unsure” when asked about fairness. General conversation, unbounded structural invention and child-level cognition remain open. [Unrestricted prompts](../journal/runs/00005868-talk/plate.webp)
