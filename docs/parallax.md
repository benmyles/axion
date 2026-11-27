# Parallax

Two witnesses watch the same float move. To find out what occupies its own north socket, the speaker asks one witness about east and the other about south. One says `jade`, the other says `amber`. The speaker reports `teal` in its own frame in both conversations. Each animal uses its own learned acoustic constructions. No language model participates.

```sh
./axion talk --speaker examples/parallax-reader.mind --listener examples/parallax-witness.mind < examples/parallax-conversation.txt
```

The first two eyes and their shared names are available interactively from the saved checkpoints. The example mounts a third independently raised body, gives each witness a different instrument, and repeats the same work from the same starting arrangement. The first speaker’s view of the outcome is covered. The reader learned the two correspondences from public replies on shared boards. [Conversation](../examples/parallax-conversation.txt) · [Teaching inputs](../examples/parallax-lessons.txt)

## What changed

Position and pigment can be permuted independently between individuals. A four-place instrument has 24 position arrangements and 24 pigment arrangements. If one eye sees the bijective board (A), another publicly describes board (B), and (P) maps sockets between them, then the corresponding pigment map is (H=B\circ P\circ A^{-1}). This leaves exactly 24 compatible pairs. A second view can distinguish some of them. Showing the same board repeatedly leaves the original ambiguity intact.

Each acquaintance occupies 16 bytes, including its 64-bit acoustic identity. The resident keeps the shared board and a 24-bit set of exclusions. The existing inherited plasticity truth table decides which conflicts inhibit or release a candidate. The learner can choose the next socket by counting how many surviving hypotheses predict different replies. If an old alignment becomes inconsistent, an inherited recovery gate can use the most recent exchange to start again. Four acquaintances fit in formerly unused words of the 568-byte event-language organ. Old minds and habitats load directly. [Optics and candidate update](../include/axion/braid_view.hpp) · [Public evidence and witness choice](../src/braid_perspective.cuh)

The optical registration is available to the simulated instrument and to the independent grader. During calibration the learner receives its own sight and the public sounds from its partner. During a covered test it can use its own opening view, motor expectation, prior conversation and the one reply it requested. A carrier’s motion and any obstruction follow the same physical rules as in the preceding watershed. [Float mechanics](braids.md)

## What the tests establish

Five shared panels gave the speaker four questions on the first panel and one possible question on each of the next four. Independent bodies sampled from the workshop lineage then encountered new motor sequences, new public requests, and paired working or jammed final strokes. Each test case supplied two witnesses with independently chosen optical wirings. The strict score requires the opening reply, the addressed noun, the witness’s actual word and the speaker’s final answer all to agree with the physical event.

| Treatment, five panels | Fully grounded exchanges out of 16,384 | Both witnesses correct on a conflicting pair, out of 7,712 |
| --- | ---: | ---: |
| Learned partner frames | 11,208 | 4,114 |
| Read all names in the speaker’s own coordinates | 870 | 0 |
| Exchange the two witnesses’ acoustic identities | 656 | 0 |
| Repeat one calibration panel | 0 | 0 |
| Mute the return | 0 | 0 |

The learner resolved one frame for 705 of the 1,024 acquaintances. All 705 matched the physical wiring. After a subsequent lens change it achieved 630 fully grounded answers across 16,384 probes. Further comparisons raised that to 9,662. Of 607 frames it then resolved uniquely, 606 matched the changed instrument. The separate voicebank lineage achieved 172 grounded exchanges out of 16,384 at the same exposure. These lineages differ genetically and have different selection histories. [Experiment and control totals](parallax/experiments.csv) · [Five-view evidence](parallax/five-views.tsv.gz) · [Separate lineage](parallax/other-lineage.tsv.gz)

### Choosing whom to ask

With three panels and a one-call budget, the speaker completed 2,039 of 8,192 decisions. Always asking the first or the second witness yielded 1,273 and 1,102. In 2,039 decisions only one witness could have supplied a fully grounded answer, and the speaker chose that witness 1,871 times. In 273 of those decisions both witnesses retained equally many candidate frames. The speaker still chose correctly on 248.

In fourteen scenes the useful witness changed with the noun despite the physical event remaining the same. The speaker successfully directed questions to both witnesses within each of those fourteen scenes. Across five calibration panels the one-call total reached 6,703 of 8,192. Each choice uses the learned coverage of the current referent before hearing a reply. The fixed-recipient comparisons use separate copies with the same history. Each source is consulted separately during grading. [Three-view evidence](parallax/three-views.tsv.gz) · [Independent grading](parallax/audit.json)

### Replaying the result

The audit enumerates all 576 possible joint wirings at an anchor, reconstructs the candidate state from the ordered public calibration replies, replays physical movements and checks each reported reference. It recomputes the one-call choices from the information available before the return. The selected-source exchange is checked against the corresponding single-source copy. [Auditor](../tools/parallax-audit.py)

```sh
make -j3 CXX=/usr/bin/g++ CUDA_CLANG=/usr/bin/clang++ CUDA_ARCH=sm_120 NATIVE=1
make test-parallax CUDA_ARCH=sm_120 NATIVE=1
./axion floodplain --load examples/floatbank.flood --steps 0 --braid-perspectives 512 --braid-examples 48 --braid-views 3 --braid-view-budget 1 --braid-seed 72497 --braid-view-trace observations.tsv
python3 tools/parallax-audit.py observations.tsv
python3 tools/parallax-audit.py docs/parallax/*.tsv.gz
```

The native Blackwell kernels and portable PTX for the habitat and conversation modules produced identical bytes for the habitat, three learned minds and two complete case tables when exercised on the laptop. All 25 native regression targets passed. [Replay digests](parallax/replay.csv)

## The free-living watershed

The released habitat at tick 368,000 has 71,722 living bodies, 4,603,072 expressed spiking cells and 192 combinations of retinal orientation and pigment wiring. At the final census, 65,973 residents retained a visual episode. Individual wiring and heading now affect the optical input used for naming, hearing-associated movement, local pull calibration and pressure contacts. The physical carbon and mineral accounts are conserved across checkpoint continuation. The final 4,096 ticks advanced 1,073,741,824 site-ticks in 24.306 seconds on the RTX PRO 2000 Blackwell laptop, or 44.18 million site-ticks/s. [Watershed measurements](parallax/watershed.csv) · [Population field guide](../journal/creature-card.webp)

```sh
./axion floodplain --load examples/parallax.flood --steps 1024 --save next.flood
```

The public comparisons and question grammar are taught in the separate conversational experiments. The wild census measures the sensory substrate and its use in physical work. Evidence for a wild community independently acquiring these conversations remains outstanding. Reference is limited to four positions, four pigments and the most recent event. Inconsistent or deceptive replies and general conversation remain open problems.

## Research context

Zheng and Sun varied the relationship between an addressee’s own view and the views of two speakers. Their participants’ referential judgments depended on how those perspectives were distributed. A 2026 intervention in children also studied the effects of practicing another person’s frame of reference during block construction. These results motivated isolating the identity of the speaker, the current referent and the information available before a reply. The permutation model and its memory layout are engineering choices. [Three-party reference experiment](https://pubmed.ncbi.nlm.nih.gov/40088564/) · [Visuospatial training study](https://pubmed.ncbi.nlm.nih.gov/41757403/)
