# Model and research basis

This document describes the recurrent foraging circuit. The subsequent memory, world-model, and generative extensions have separate equations, experiments and controls. [Transfer and planning](transfer.md)

Axion tests a computational hypothesis. A recurrent threshold circuit can learn from delayed outcomes using a small set of state variables per cell, a value estimate for each action, and a sampled record of recent coincidence at plastic synapses. The challenge is to preserve the information needed for credit assignment while keeping both memory and work per tick linear in the number of neurons.

## Evidence that shaped the design

The literature review for this implementation covers primary work available on 19 November 2026. Each design choice below is an engineering inference.

| Primary finding | Choice in Axion |
| --- | --- |
| Human single-unit recordings during rapid visual learning showed changes in occipital and medial temporal responses. Learned degraded images required extra processing time in occipital cortex. The authors interpret the timing as consistent with recurrent processing. | Allow the same sensory evidence to act over several recurrent ticks. Measure performance after the stimulus has disappeared. [Armendariz et al., 4 November 2026](https://www.nature.com/articles/s41593-026-02496-6) |
| During human avoidance learning, subsets of claustrum and anterior cingulate neurons tracked model-derived uncertainty and prediction error at distinct times. | Give the agent action-dependent predictions, and use the discrepancy at an outcome to drive learning. Axion implements the prediction-error part of this observation. [Human claustrum recordings, 6 October 2026](https://www.nature.com/articles/s41593-026-02475-x) |
| Mouse neurofeedback experiments found cell-specific instructive signals in cortical dendrites. | A common reward error has different effects on different cells. A cell's influence on the selected action sets its feedback sign. This sign is a computational approximation of targeted feedback. [Francioni et al., Nature, 2026](https://www.nature.com/articles/s41586-026-10190-7) |
| A theoretical treatment of behavioral-timescale synaptic plasticity showed how local traces and instructive events could support rapid changes and attractor memories in model networks. | Preserve transient coincidence, latch its relevance at a choice, and allow reward to arrive several ticks later. Axion measures time in simulation ticks. [Cone, Clopath and Costa, 14 November 2026](https://doi.org/10.1038/s41467-026-79000-y) |
| E-prop derived a decomposition into local eligibility and neuron-specific learning signals. More recently, forward-only pre/post trace approximations demonstrated linear auxiliary memory for recurrent spiking models. | Keep two activity timescales and bounded forward traces. Sample one candidate afferent per cell per trial. The sampled update is an original heuristic with bias and variance. The convergence and accuracy results of those papers apply to their respective algorithms. [Bellec et al., 2020](https://doi.org/10.1038/s41467-020-17236-y) · [Wang et al., 2026](https://pubmed.ncbi.nlm.nih.gov/41554722/) |
| In human navigation experiments, subicular fMRI responses tracked the binding of spatial locations across an ordered sequence. | Include an action-dependent route with an ordered pair of decisions. This uses a behavioral demand from that work while leaving hippocampal anatomy outside the model. [Shin, Bingman and Lee, 18 November 2026](https://doi.org/10.1038/s41467-026-79267-1) |
| Cultured cortical neurons adjusted their excitatory input strength in response to sustained changes in activity. | Use a rare compensatory sensory-weight adjustment to keep inactive cells recruitable over long training runs. The sampling, timescale, and discrete weight update are engineering choices. [Turrigiano et al., 1998](https://pubmed.ncbi.nlm.nih.gov/9495341/) |
| A computational evolution experiment showed how selection can favor inherited circuits through what they learn during a lifetime. | Measure success after a brief adaptation period, restore the inherited weights for each lifetime, and select on fresh experiences. [Hinton and Nowlan, 1987](https://www.cs.toronto.edu/~hinton/absps/evolution.htm) |

## One recurrent tick

Cells are arranged in tiles of 32. Each receives a 32-bit receptor word, the previous spikes of its own tile, and the previous spikes of another tile. For tile index `g`, cell index `i`, and power-of-two tile count `G`, the remote index is `g XOR (1 + 2 * (i AND (G/2 - 1)))`. This connects disjoint source tiles and lets lanes sample different remote populations as the brain grows.

One weight word stores the signs of 32 connections. For active input bits `b` and weight signs `w`, the signed dot product is

```text
D(b,w) = 2 popcount(b AND w) - popcount(b)
```

Let `s` be sensory bits and `p,h` the two recurrent spike words. The cell combines three dots with fast population inhibition. `v` is membrane state, `a` is threshold adaptation, `z` is a binary spike and `x` is the filtered activity available to the motor system.

```text
I = 3 D(s,Ws) + D(p,Wp) + D(h,Wh) - floor((popcount(p) + popcount(h))/4)
u = trunc(3v/4) + I
θ = 7 + (i mod 4) + floor(a/4)
z = 1 if u ≥ θ, else 0
v ← clip(u - θ z, -16, 24)
a ← a - ceil(a/8) + 6 z
x ← clip(x - ceil(x/τ) + 24 z, 0, 255)
```

Each tile has cells with `τ = 4` and `τ = 16`. A spike raises the cell's short-term firing threshold. Slow activity traces can preserve evidence long after a sample vanishes. Every neuron reads the preceding recurrent tick. All spikes for the new tick become visible together.

## Choice, credit and plasticity

For motor weight `U[i,a]`, action `a` receives the value `Q[a] = trunc(sum(U[i,a] x[i]) / max(1, sum(x[i])))`. The policy acts greedily in evaluation, breaking ties with a seeded random draw. During learning it chooses a random arm with probability 1/8.

At the first fork, the learner latches its chosen value and accumulates motor eligibility. At a later fork it evaluates both arms, selects one, and updates the preceding decision using the on-policy temporal-difference error. At the end of the route it uses the observed reward. Only the arena knows which sequence leads to food.

```text
δ at next choice   = trunc(7 Q[new action]/8) - previous prediction
δ at outcome       = reward - previous prediction           reward ∈ {-256, +256}
e[i,a] at choice   ← floor(3 e[i,a]/4) + x[i] if a was chosen
                     floor(3 e[i,a]/4)         otherwise
ΔU[i,a]            = trunc(clip(δ,-512,512) e[i,a]/128)
U[i,a]             ← clip(U[i,a] + ΔU[i,a], -2048, 2048)
```

Motor eligibility decays per decision and persists during passive travel. The update at a later fork uses the old eligibility before latching the new one. Activity, eligibility, and the action prediction reset on entry to each trial. Synapses persist across trials.

For hidden plasticity, each cell samples one of 64 candidate sensory or remote recurrent synapses at trial entry. An active presynaptic bit marks that synapse for 12 ticks if the postsynaptic drive lies in `[θ-4, θ+8]`. At a decision the cell latches `sign(U[i,chosen] - Q[chosen])` if marked. This measures whether increasing its relative contribution would raise or lower the chosen value. A later error times that feedback sign requests potentiation or depression. The magnitude must exceed a seeded uniform integer gate in `[0,2047]` to set or clear the sampled weight bit. The local recurrent bank provides a stable scaffold during a lifetime.

A second, slower rule regulates sensory drive. On 1/256 of sensory probes, the outcome update checks the sum of the two existing motor eligibilities. Zero activity sets the sampled sensory sign positive. A sum of 32 or more sets it negative. This compensatory update uses local activity and operates even when the reward error is zero. The `no-scaling` ablation keeps the reward-dependent sensory and recurrent updates. The long-run comparison motivated the addition of this rule.

This assignment keeps the current decision's hidden tag and decayed motor eligibility for earlier choices. The design trades information about individual historical synapses for bounded state. Fixed-point clipping and the stochastic binary switch also make it an approximate learning rule. Each of these choices can be tested against task returns and a motor-only control.

## Execution and memory

| Mapping | Width per brain | Where recurrence runs |
| --- | ---: | --- |
| Warp | 64, 128 | Two or four cells per lane. Warp ballots produce complete spike words. PTX selects between registers for remote inputs. Integer warp reductions produce the two action values. |
| Block | 64–1,024 | One cell per thread. Two shared spike buffers alternate with one block barrier per recurrent tick. |
| Striped block | 2,048, 4,096 | Each of 512 threads owns four or eight cells. For 4,096-cell training, shared memory holds motor weights and eligibility between decisions, and packed state controls register pressure. |
| Cooperative grid | 64–1,048,576, automatic above 4,096 | Multiple resident blocks integrate one animal in parallel. Global spike buffers handle recurrence across SMs. The same grid advances the world, makes decisions, and learns. |

The supported widths are powers of two. The compact kernels fuse up to 32 episodes per launch. Independent warps or blocks own separate brains. The cooperative path fuses up to four episodes to bound launch duration. The driver supplies the occupancy limit for the exact kernel. Every launched block can reside concurrently. Block leaders use device-scope acquire/release operations at tick boundaries. The grid uses 64-bit sums for action values, including at maximum width. [CUDA cooperative launch contract](https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXEC.html) · [PTX instruction set](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html)

| Storage | Cost |
| --- | ---: |
| Persistent weights | Three 32-bit sign words and two 16-bit motor weights, **16 B/cell** |
| Global transient state of one cooperative brain during inference | **4.25 B/cell** for packed voltage, adaptation, activity, tag age and two one-bit spike buffers |
| Additional cooperative state during learning | **4 B/cell** for both motor eligibilities and signed hidden feedback. The candidate synapse is regenerated from the trial key. |
| Cooperative synchronization and reductions | 8 bytes and 24 bytes per allocated partial-sum slot, with one slot per resident block in the largest selected grid |
| Statistics returned to the host | 40-byte record in the current build, including outcome, tick, spike, pixel-match and last-image fields |

At maximum width, explicit device state is 16 MiB of weights plus 4.25 MiB for the simulation, or 8.25 MiB for the simulation during learning, plus the small control allocations above. Independent large animals share the scratch area and run sequentially. Compact kernels use registers, shared memory, and any local memory reported by the driver. The CPU reference uses a wider representation for inspection. Driver allocations, code, and host copies are additional. The work per tick scales with neuron count at fixed fan-in and includes updating silent cells.

## Scope of the evidence

The implemented sensorium has 32 binary receptors and two actions. The playground tests rapid association, short-term memory, conditional decisions, delayed reward, transfer, and adaptation to reversal. Holding out a seed changes layouts, nuisance events, and receptor dropout within the same task family. Generalization to new senses, long naturalistic histories, language, and open-ended behavior remains unmeasured. Human-level sufficiency and a globally optimal GPU implementation are open questions.
