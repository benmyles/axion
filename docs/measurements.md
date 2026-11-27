# Measured behavior and throughput

These are the original foraging measurements. The expanded learner has a separate report with multi-skill retention, generated images, and native Blackwell measurements. [Transfer experiments](transfer.md)

These runs used the NVIDIA RTX PRO 2000 Blackwell Generation Laptop GPU, compute capability 12.0, driver 615.71.09, Clang 23.1.4, and GCC 16.2.1 on Fedora 45. The host was an Intel Core Ultra 7 356H with 16 hardware threads. Commands below run from the repository root. Build with `make -j CUDA_ARCH=sm_120` for the native measurements.

## What the animals learned

The curriculum used 128 independently initialized animals of 128 neurons each. Every stage provided 256 trials per animal. Each probe then ran 128 frozen trials per animal, or 16,384 outcomes per row. Before and after probes shared the same held-out layouts and nuisance draws. The learner received a signed scalar outcome after each completed trip.

| Stage | Before practice | During practice, with exploration | After practice | After-probe SE, percentage points |
| --- | ---: | ---: | ---: | ---: |
| Scent association | 49.29% | 92.69% | 100.00% | 0.00 |
| Delayed fork | 99.67% | 93.67% | 99.99% | 0.01 |
| Context-dependent fork | 49.51% | 64.84% | 83.66% | 0.99 |
| Two-junction trail | 61.38% | 77.56% | 91.10% | 0.77 |
| Unannounced reversal of scent reward | 0.35% | 81.08% | 99.79% | 0.20 |

Scent training transferred to the delayed fork before the first reward there. Adding a context that changes the meaning of the odor required substantial additional learning. After reversal, the score under the old scent rule was 0.21%. The observable cue was identical under the two opposing rules. Old-rule delay, context and trail scores were 0.90%, 52.34% and 0.04%. This measures a limit of the learned policy when the contingency changes without a distinguishing cue.

```sh
./axion curriculum --backend gpu --seed 1 --brains 128 --neurons 128 --episodes 256 --eval-episodes 128
```

Training on 1,024 interleaved trials per animal gave **89.16%** on a mixed probe. From the same frozen checkpoint, individual probes scored 99.90% on scent, 95.46% on delay, 68.95% on context and 91.45% on trail. A random binary policy averages 43.75% over this mixture. Conditional interpretation was the hardest skill to retain alongside the others.

```sh
./axion train --backend gpu --task mixed --episodes 1024 --eval-episodes 256 --save mixed.axn
for task in scent delay context trail; do
    ./axion eval --backend gpu --load mixed.axn --task "$task" --episodes 256
done
```

## Controls and long-run plasticity

The context experiment started each group from a fresh population. Within a seed, each condition received the same initial wiring and training worlds. Each animal received 256 training and 256 held-out trials. The odorless intervention used the fully trained checkpoint and preserved the context cue, fork signal, delay distribution and nuisance channels.

| Wiring seed | Frozen throughout | Motor plasticity only | Full plasticity | Full, odor removed at evaluation |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 49.97% | 89.50% | 92.72% | 49.95% |
| 17 | 50.13% | 89.28% | 91.62% | 50.27% |
| 77 | 50.52% | 90.47% | 91.36% | 49.89% |

The motor-only result shows that a fixed recurrent representation accounts for much of the short-budget performance. The full rule added 0.89–3.22 percentage points in these comparisons. Removing the odor reduced success to chance.

```sh
for seed in 1 17 77; do
    for learning in full motor off; do
        ./axion train --backend gpu --seed "$seed" --task context --episodes 256 \
            --eval-episodes 256 --learn "$learning" --save "$learning.axn"
    done
    ./axion eval --backend gpu --load full.axn --episodes 256 --mute-odor
done
```

Long runs exposed a separate failure mode. Reward-driven changes progressively silenced the circuit. A rare local correction to sensory drive uses the existing eligibility values to recruit silent cells and restrain highly active ones. The correction rate was developed on seed 1. Seed 17 repeated the comparison with the same settings.

| Mixed training trials per animal | Seed | Sensory scaling | Held-out success | Neurons firing per tick |
| ---: | ---: | --- | ---: | ---: |
| 65,536 | 1 | Off | 82.91% | 0.25% |
| 65,536 | 1 | On | 92.45% | 4.79% |
| 65,536 | 17 | Off | 83.01% | 0.25% |
| 65,536 | 17 | On | 92.35% | 4.78% |
| 262,144 | 1 | On | 92.50% | 4.80% |

After 262,144 mixed trials per animal, switching to the opposite scent rule drove success to 0.05%. Another 512 training trials per animal brought the frozen probe to 100.00%. Across the 128 animals, the long run comprised 33,554,432 online training episodes before this reversal.

```sh
./axion train --backend gpu --seed 17 --task mixed --episodes 65536 --eval-episodes 256 --learn no-scaling
./axion train --backend gpu --seed 17 --task mixed --episodes 65536 --eval-episodes 256
./axion train --backend gpu --task mixed --episodes 262144 --eval-episodes 256 --save long.axn
./axion train --backend gpu --load long.axn --task scent --rule flipped --episodes 512 --eval-episodes 256
```

The shipped `examples/forager.axn` is animal zero from the seed-1 setup, regenerated as a single-animal run with 262,144 mixed training trials. It scored 91.89% on 4,096 held-out trials on both the GPU and CPU. Its 2,104-byte checkpoint can be regenerated with `./axion train --task mixed --brains 1 --neurons 128 --seed 1 --episodes 262144 --save forager.axn`.

## Evolution of fast learning

Each of 128 genomes experienced two separate 32-trial lifetimes per selection round. One lifetime used the normal context rule and one used its opposite. Each started from the inherited circuit. Fitness counted wins on 128 frozen trials per rule. The best eighth of the population supplied the parents for the next generation. Starting weights were recombined cell by cell, then a subset of the binary connections was mutated.

The independent audit used eight additional experience seeds with both rules for each seed. Mean post-learning success on that audit was **59.24% at generation zero and 78.89% after 12 generations**. The audit measures faster adaptation within this task distribution. This is one evolutionary run.

```sh
./axion evolve --backend gpu --task context --brains 128 --episodes 32 --eval-episodes 128 --generations 12
```

## GPU throughput

One neuron-tick means one complete update of one neuron's recurrent state. The workload includes world generation, sensing, decisions, reward and statistics. Full learning also includes eligibility and weight updates. CUDA events bracket the launch sequence. Wall time includes clearing counters, dispatch, synchronization and reading results. Setup, JIT compilation, initial weight transfer and benchmark restores are outside the timed interval. Each mode warms up first and each measured repetition starts from identical weights and experiences. Clocks and power limits were managed by the laptop.

| Animals × neurons | Layout | Episodes per animal | Repetitions | Inference, billion neuron-ticks/s | Full learning, billion neuron-ticks/s |
| --- | --- | ---: | ---: | ---: | ---: |
| 4,096 × 128 | Warp | 128 | 9 | **139.51** | **91.59** |
| 4,096 × 128 | Block | 128 | 9 | 81.66 | 65.41 |
| 128 × 4,096 | Striped block | 32 | 9 | 120.23 | 66.99 |
| 1 × 1,048,576 | Cooperative grid | 16 | 9 | **53.27** | **30.56** |

The warp run completed 524,288 two-junction episodes in a median 6.014 ms for inference and 9.160 ms for learning, with wall times of 6.136 and 9.291 ms. The warp mapping was 1.71× as fast for inference and 1.40× for learning as the block mapping on this workload. Selecting remote spike words from registers and removing dynamic weight-address selection let the JIT keep this kernel at zero local bytes per thread.

The single large brain had 100,663,296 one-bit afferents plus 2,097,152 motor weights. It used 312 cooperative blocks and 96 shared bytes per block. Its median inference and learning times were 4.134 and 7.206 ms for 16 episodes. At 4,096 cells per animal the first striped mapping spilled 384 local bytes per thread during learning. Moving motor weights and decision eligibility into shared memory, aggregating spike counts per thread, and packing the activity variables reduced this to 72 local bytes per thread. Learning throughput rose from 32.69 to 66.99 billion neuron-ticks/s. The final training kernel uses 34,176 shared bytes per block. Its inference counterpart uses 1,408 shared bytes per block and zero local bytes per thread. The driver reports these resource counts on each benchmark run.

On a smaller matched workload of 128 animals × 128 cells × 64 trail episodes, median GPU **wall** times were 0.270 ms for inference and 0.338 ms for learning. The 16-thread CPU reference took 9.184 ms and 13.601 ms. These are implementation- and workload-specific timings. Energy consumption was outside the measurement.

```sh
for layout in warp block; do
    ./axion bench --backend gpu --layout "$layout" --task trail --brains 4096 --neurons 128 --episodes 128 --repeats 9
done
./axion bench --backend gpu --task trail --brains 128 --neurons 4096 --episodes 32 --repeats 9
./axion bench --backend gpu --task trail --brains 1 --neurons 1048576 --episodes 16 --repeats 9
for backend in gpu cpu; do
    ./axion bench --backend "$backend" --task trail --brains 128 --neurons 128 --episodes 64 --repeats 5
done
```

## Correctness

`make test-gpu` checks packed integration against a scalar signed-synapse sum, delayed credit, compensation of silent and highly active cells, sensory isolation, action-dependent world transitions, conditioning, reversal, checkpoint continuation, corruption detection, and inheritance. It compares every stored weight and each animal's counts of spikes, ticks and wins between CUDA and the CPU across all supported widths and learning modes. Maximum-width fixtures exercise action sums beyond 32-bit range. The CPU checks also passed under Clang AddressSanitizer and UndefinedBehaviorSanitizer.
