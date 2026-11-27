# The outer estuary

At tick 384,384 the watershed contains 381,959 independent animals expressing 24,566,304 somatic spiking cells. It runs on a single RTX PRO 2000 Blackwell laptop GPU. The last 8,192 ticks advanced 8,589,934,592 site-ticks in 137.966 seconds. This includes water, material transport, growth, feeding, sensory input, recurrent activity, reproduction and selection. [Field measurements](neural-execution/wild-watershed.csv)

## An island opens

The population continued from the Parallax checkpoint for 8,192 ticks. At tick 376,192 we opened 786,432 adjoining sites, keeping all 71,371 living residents on their original 512 × 512 shore. The added territory began with water, minerals and energy, and zero animals. Movement wraps across opposite borders. The furthest distance below is the shortest number of cardinal steps to the original shore, including paths around those borders. Animals received the ordinary physical consequences of their actions throughout. [Measured settlement at each 1,024-tick interval](neural-execution/settlement.csv)

| Final survey | Original shore | New territory |
| --- | ---: | ---: |
| Living residents | 74,927 | 307,032 |
| Expressed somatic spiking cells | 4,808,128 | 19,758,176 |
| Largest living soma in cells | 288 | 352 |
| Distinct expressed body plans | 5,224 | 17,137 |
| Sites carrying an eastward or southward fibre edge | 151,559 | 381,373 |
| Maximum depth in generations | 3,302 | 3,317 |

Only 778 body plans occurred on both shores. All living residents descended from one recorded founder. The most distant occupied site was 512 steps from the original shore. These are spatial and genealogical observations. A common-garden or reciprocal transplant would be needed to assign morphological differences to local adaptation. Contemporary range-expansion studies use that distinction when evaluating phenotypes at a range edge. [Independent checkpoint census](neural-execution/population.json) · [Clark et al., *Evolution*, 2026](https://doi.org/10.1093/evolut/qpag015)

The mean soma at the final census contained 64.32 expressed cells. The 24.57 million cells belong to the entire population. The archived world is a complete checkpoint. Unpack it once to continue evolving it. The independent survey reads its population, neural extents, fibre edges and 32 × 32 density map directly from the saved bytes.

```sh
zstd -d examples/outer-estuary.flood.zst -o estuary.flood
./axion floodplain --load estuary.flood --survey-origin 512 --steps 4096 --save next.flood
python3 tools/frontier-audit.py estuary.flood --origin 512 --opened 376192
```

To reproduce the expansion from its ancestor, run the following commands. The inner checkpoint has SHA-256 `9a46fc81318f6b3dbd7e307049e424c68de7fcfb10da8df7cb05572b459a83d4`. The final unpacked checkpoint has SHA-256 `7efb9ab8c3d904885d9d03875773f95fc319abc1e2883e6f5f19f830aed15071`.

```sh
./axion floodplain --load examples/parallax.flood --steps 8192 --report-every 1024 --brain-file compact --save inner.flood
./axion floodplain --load inner.flood --extend 1024 --steps 8192 --report-every 1024 --brain-file compact --save replay.flood
```

## Variable-size cortices on CUDA

A CUDA lane updates 32 spiking cells using bitwise operations. A living cortex can express any number of these bands from 2 to 32. The new scheduler reads 32 neighbouring sites, prefix-sums their live widths with warp shuffles and fits complete residents into successive warps. Each lane finds the start and owner of its resident by ballot, rank and masked bit selection. An irregular subgroup reduces motor votes in hardware. The same evolved instruction tape runs in both execution modes. Dispatch requires zero global work-queue bytes. [CUDA implementation](../src/flood_neural_dispatch.cuh) · [NVIDIA PTX bit selection](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#integer-arithmetic-instructions-fns) · [NVIDIA PTX warp reductions](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#parallel-synchronization-and-communication-instructions-redux-sync)

On the 512 × 512 ancestral watershed, 71,722 animals occupied 143,846 active bands. Reserving a full warp per live animal assigned 2,295,104 lane slots. Packing these same animals used 271,872 lane slots. In five interleaved timing samples, 64 consecutive neural updates took a median 139.997 ms with fixed widths and 31.378 ms packed, a 4.46× speedup. The benchmark used two dense shadow habitats under identical sensory conditions. The resulting 184,811,520 bytes of tissue, body state, motor output and auditory history matched. Two smaller founder populations ran faster with fixed widths. Automatic dispatch accounts for the population shape and habitat size. [Kernel timing and crossover points](neural-execution/cortical-timing.csv)

| Production execution on the same 512 × 512 habitat | Median million site-ticks/s | Reserved CUDA neural arena |
| --- | ---: | ---: |
| Fixed widths, dense tissue | 41.49 | 128 MiB |
| Packed widths, dense tissue | 65.05 | 128 MiB |
| Packed widths, paged tissue | 64.55 | 16.28 MiB |

Each entry is the median of five 768-tick advances from the same saved world on the RTX PRO 2000 Blackwell Generation Laptop GPU, native sm_120, Linux driver 620.70.06 and Clang 23.1.5. Execution order reversed on alternating repetitions. Timings cover the simulation interval. The CSV also records the reporting loop's elapsed time. The neural arena includes the persistent device slab, directory, claims and per-slot metadata. Whole-process memory, temporary CUDA allocations and the operating system's placement of spill pages are additional. [All fifteen timings](neural-execution/watershed-timing.csv)

The two basal bands reside on the device. Higher bands share 1 KiB pages across 64 slot identities. An untouched row can represent its exact initial tissue with its genome and width. A bit mask per slot marks materialized changes. A zero read from an absent page allocates nothing. When the regular slab fills, a CUDA lane can address a stable spill page through the Linux pageable-memory facility. A periodic device census reclaims empty pages, moves live pages back into the regular slab and lets the operating system discard unused system pages. Automatic storage selection checks device support and projected byte savings. [Neural page implementation](../src/flood_nerve_pages.cuh) · [NVIDIA pageable-memory support](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/unified-memory.html)

The million-site expansion reserved **51,908,608 device bytes** for this arena against a **536,870,912-byte** dense representation. At the end, spill entries occupied 3,108 logical system pages of 4,096 bytes each within a 503,316,480-byte virtual reservation. Two collections discarded 55,995 unused system pages cumulatively. This particular arena was sized when new territory opened. Reloading a mature checkpoint can choose a different initial slab.

The optional compact checkpoint encodes each row as a developmental prefix or an explicit prefix through its highest nonzero band. This preserves retired and altered dormant tissue, including holes. An original 518,262,260-byte replay became 392,715,548 bytes, and conversion to the dense file recovered every byte. The million-site checkpoint uses 34,727,768 bytes for neural state in a 1,570,895,180-byte habitat file. Git LFS stores the complete Zstandard-compressed habitat in 255,958,815 bytes. [Replay digests](neural-execution/replay.csv)

```sh
make -j3 CXX=/usr/bin/g++ CUDA_CLANG=/usr/bin/clang++ CUDA_ARCH=sm_120 NATIVE=1
make test-neural-dispatch test-gate-trial CUDA_ARCH=sm_120 NATIVE=1
./axion floodplain --load examples/parallax.flood --steps 0 --brain-bench 64
./axion floodplain --load estuary.flood --steps 256 --brain-dispatch packed --brain-memory paged --brain-file compact --save continued.flood
```

`--brain-pool-pages` sets the number of spare on-device pages. Zero exercises the spill path. `--brain-collect` sets the collection interval in ticks, with zero disabling collection. `--brain-file dense` restores the original wire representation. All 27 native regression targets passed, including conversion of legacy fixed-width residents and retention of dormant state in developing residents. A second full expansion reproduced all 1,570,895,180 bytes at 62.38 million site-ticks/s. On this laptop native Blackwell machine code and baseline sm_80 PTX produced identical habitat files, the perspective and gate-intervention traces, and a saved conversational speaker. The GPU kernels perform neural inference, plasticity, interventions and field dynamics. Host code handles checkpoint encoding, memory management and reporting.

## Does acquired wiring help a wild animal?

Every cell has two mutable gate bits. The genome determines which inputs they route and how experience can change them. We enrolled residents at least one tick old whose continuously present natal bands differed from their genome-defined gate image. For each starting resident we compared three assignments in copies of the same functioning habitat. Each copy contains all three arms, and across three rotations each individual experiences each arm exactly once.

- **Intact** keeps the resident's acquired gates.
- **Reverted** resets the gates in those bands to their developmental values.
- **Rotated** shifts the paired difference masks within each band, preserving the number and co-occurrence of acquired A and B edits. Different cells receive the changes.

The CUDA intervention leaves spikes, recurrent memory, the genome, energy, anatomy, acquired sensory circuits and the physical world intact at enrollment. All arms resume ordinary plasticity on the next tick. A sham carries out the same enrollment and assignment with zero changes. Records match surviving individuals by slot and birth tick, including animals that moved. A subsequent occupant of the slot contributes zero retained energy for the original resident. The archived outcome includes every eligible starting animal, including those that died. [Device intervention](../src/floodplain.cu) · [Independent per-bit test](../tests/gate_trial.py)

| Starting field and observation horizon | Enrolled per arm across rotations | Surviving intact | Surviving reverted | Surviving rotated |
| --- | ---: | ---: | ---: | ---: |
| Original watershed, 64 ticks | 70,719 | 38,385 | 38,321 | 37,909 |
| Inner shore, 64 ticks | 70,619 | 37,782 | 37,803 | 37,537 |
| Outer estuary, 1 tick | 376,603 | 372,838 | 372,837 | 372,832 |
| Outer estuary, 64 ticks | 376,603 | 213,150 | 213,130 | 212,080 |
| Outer estuary, 64 ticks, second assignment seed | 376,603 | 213,200 | 213,057 | 212,178 |
| Outer estuary, 128 ticks | 376,603 | 137,924 | 137,898 | 136,933 |
| Outer estuary, 64 ticks, sham | 376,603 | 213,174 | 213,174 | 213,174 |

In the first outer-estuary step, moving the gate changes altered the last action of 9,224 paired survivors. Restoring birth settings altered 1,927. At 64 ticks the intact configuration retained 1,070 more animals than rotation and 20 more than reversion. The sham produced the same survival, retained energy and last action for every paired animal. At the inner shore, reversion retained 21 more animals than intact. The effect of retaining acquired gates relative to birth settings is small and changes sign across these environments. [Outcome totals](neural-execution/gate-outcomes.csv) · [Paired outcomes](neural-execution/gate-pairs.csv)

The eight most abundant body plans were selected from the starting census before comparing their outcomes. With the two assignment seeds, retaining acquired gates yielded 30 and 20 fewer survivors than reversion among 73,801 original-shore residents. It yielded 50 and 163 more among 302,802 residents already living beyond that shore. The rotated group lost survivors relative to intact in both regions at both seeds. The location differences are small, and these are repeat interventions on the same checkpoint. [Region and body-plan results](neural-execution/gate-groups.csv)

The worlds interact after assignment. A resident's outcome can depend on a neighbour's altered actions as well as its own. These populations share ancestry. The experiment measures consequences of naturally accumulated gate state, which can include autonomous changes from the inherited program. Establishing an adaptive memory of a particular experience would require changing that experience and testing subsequent behaviour. Human single-neuron recordings during rapid visual learning and mouse single-synapse imaging during motor learning motivate examining history, location and recurrent state separately. [Armendariz et al., *Nature Neuroscience*, 2026](https://www.nature.com/articles/s41593-026-02496-6) · [Wright et al., *Science*, 2025](https://doi.org/10.1126/science.ads4706)

```sh
for phase in 0 1 2; do
  ./axion floodplain --load estuary.flood --steps 64 --report-every 16 --gate-trial crossover --gate-seed 41971 --gate-phase "$phase" --gate-trace "phase-$phase.tsv"
done
python3 tools/gate-audit.py phase-0.tsv phase-1.tsv phase-2.tsv
python3 tools/gate-audit.py docs/neural-execution/trials/outer-64-41971-crossover-{0,1,2}.tsv.gz
```

Pass `--habitat estuary.flood --origin 512` to the auditor to pair outcomes by the eight most numerous original body plans and by the starting region. Each trace contains the assigned arm, acquired and changed bit counts, starting energy, retained energy and last action per original identity. The repository includes all 21 traces and their digests. [Trace index](neural-execution/gate-files.csv)

The taught speaker and witness remain available for grounded dialogue with independent visual perspectives. The free-living field has yet to generate that teaching history or demonstrate general conversation. [Talk to the Parallax speaker](parallax.md)
