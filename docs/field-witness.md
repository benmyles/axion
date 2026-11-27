# A conversation with a resident of the outer estuary

A free-living creature was born at tick 387,726 in a million-site watershed. At age two it became the first resident in a prospective search to pass eight independent questions about two contrasting scenes. It had 64 expressed somatic spiking cells, four expressed modules and a separate float-hand and speech circuit. After schooling, its own speech contacts could report what it had just seen in the watershed.

At tick 387,736 it gave the following answers. The socket names refer to positions in this individual's sensory frame. The data file contains the actual spoken waveforms and the stamp of the underlying field event. [Recorded conversation](field-witness/field-dialogue.txt) · [Portrait and retinal record of the actual speaker](../journal/runs/00010310-floodplain/plate.webp)

```text
what was in north?
witness> teal
what is in north?
witness> amber
what was in west?
witness> amber
what is in west?
witness> teal
```

## Speak to the animal

The small portable mind opens directly in the dialogue program. The archived watershed lets you interview the same animal in its physical habitat. Enter `/quit` to finish. Turns within an interview share the resident’s discourse state. Use `--save` to carry it into another run. The habitat clock holds while you speak. `--steps` determines how long the environment advances before the interview.

```sh
./axion talk --speaker examples/estuary-witness.mind --listener examples/parallax-witness.mind

zstd -d examples/witness-estuary.flood.zst -o witness.flood
./axion floodplain --load witness.flood --steps 0 --follow-witness examples/estuary-student.mind --field-questions -
```

The focal resident shares this checkpoint with 373,235 other living animals. Together they express 23,990,432 somatic spiking cells and 22,653 distinct body plans. At the interview, 300,049 animals occupied the extension. The witness occupied site (62, 903) both at discovery and eight ticks later. The direction of change across the two visible sockets can also be checked directly from the checkpoint. [Eight public questions](../examples/estuary-conversation.txt) · [Checkpoint auditor](../tools/field-voice-audit.py)

```sh
python3 tools/field-voice-audit.py visit examples/estuary-witness.mind docs/field-witness/field-dialogue.txt examples/estuary-conversation.txt --habitat witness.flood --slot 890939
```

## Finding a learner

One real individual from each of the eight most populous body plans received the same introductory lesson. None acquired a grounded noun. Of 32,768 other unselected observers, 84 learned all eight names. None passed the full diagnostic in that sample. The population samples use a seeded permutation of slot identities and include only living, sighted residents with a recorded field event. [Card representatives](field-witness/population-card-lessons.csv)

The observer then followed the wild population from tick 384,384, checking every 16 ticks. It first tested the sixteen possible four-bit name-binding rules on four independent naming lessons in isolated CUDA copies. One rule learned all eight names in all four lessons. The observer used this screen to shortlist living animals with a functioning auditory and visual pathway, then tested each candidate's own unmodified motor and speech circuits with new sound patterns. Every test required the correct answer at all four sockets before and after a visible rearrangement. The selected private field episode supplied none of these diagnostic answers.

The observer sampled 210 times and examined 137,428 candidate visits, including repeat visits. Resident 890939 passed at tick 387,728. It belonged to generation 3,196. The search observed a rare learning capacity. The ecological population received no language score or selective breeding intervention. A separate advance from the same original shore with the observer switched off reproduced the entire resulting habitat byte for byte. [Earlier coarse observations](field-witness/early-surveys.csv) · [Replay digests](field-witness/replay.csv)

```sh
zstd -d examples/outer-estuary.flood.zst -o outer.flood
./axion floodplain --load outer.flood --steps 4096 --report-every 2048 --watch-readers 16 --class-seed 82751 --class-practice 12 --brain-file compact --save discovered.flood
```

## Returning what was learned

The classroom used physical float movements and ordinary vocal demonstrations. The published lesson supplies twelve rounds of motor practice, twelve naming repetitions and sixty-four examples of each of two question forms. The learner's inherited contact rules govern the updates. The physical demonstrations were drawn from the successful diagnostic curriculum. English names and question forms were introduced after the individual was selected. [Classroom transcript to replay](../examples/estuary-lessons.txt)

The return operation checks the world seed, original slot, birth tick, ancestry, germline and native learning circuit. It compares the original portable mind, the taught copy and the speech contacts at home. A repeated return is idempotent. A conflicting change learned at home is reported before the saved world is modified. This transfers the durable parts of a 568-byte narration tissue with space for 32 acoustic types and 16 learned constructions. The genome, somatic spikes, energy, age, public discourse and visual event belong to the living animal. Adding this resident’s contacts grew the 1.57 GB habitat by 580 bytes. A byte comparison verified the entire preceding payload apart from the format flag and trailing checksum. [Return measurements](field-witness/return.csv)

The classroom animal's last demonstration put different dyes in front of it. Reconnecting its acquired language to its own field tissue changed the answers immediately. Ordinary questions in the field run the same CUDA question circuit as the portable animal. An unheard word or an unresolved meaning can produce silence, displayed as `?`. The single-resident field interview has access to its own private sight. The portable dialogue can also consult another animal. [Classroom responses](field-witness/classroom-dialogue.txt) · [Field responses](field-witness/field-dialogue.txt)

```sh
./axion talk --speaker examples/estuary-student.mind --listener examples/parallax-witness.mind --save-speaker pupil.mind < examples/estuary-lessons.txt
./axion floodplain --load discovered.flood --return-lesson examples/estuary-student.mind pupil.mind --steps 8 --follow-witness examples/estuary-student.mind --field-questions examples/estuary-conversation.txt --save continued.flood
```

At nine prespecified follow-ups, from 1 to 256 ticks after the return, the same animal answered all 72 directly observable questions correctly. Two visits shared the same sensory episode. Eight distinct episode stamps were recovered. Two later observations each recorded two sockets changing dye. By the 512-tick visit the original animal was gone. A successor occupying the same storage slot cannot answer on its behalf. [Life-course records and portable mind hashes](field-witness/lifetime.csv)

## What transferred across lessons

Each row below starts 512 independently taught copies of the same resident. The dialects vary the acoustic names, fixed words and position of the noun in each question. Each copy is also evaluated on the resident's actual field episode. An empty private memory produced zero answers throughout. [Measurements and CUDA timing](field-witness/dialects.csv)

| Physical learning history | Dialects passing all eight diagnostic questions | Correct reports out of 4,096 visible questions |
| --- | ---: | ---: |
| Successful physical curriculum, new dialects | 512 / 512 | 4,096 |
| New physical curriculum in each dialect | 10 / 512 | 684 |
| Successful curriculum with motor practice omitted | 0 / 512 | 0 |
| Another fixed physical curriculum | 0 / 512 | 2,048 |
| Successful curriculum on the later changed field episode | 512 / 512 | 4,096 |

On the later episode 1,024 paired socket changes were reported correctly on both sides of the movement. Every trace records the acoustics seed, demonstration seed, actual sensory frames, answer masks and three-bit spoken referents. The audit independently reconstructs each diagnostic arrangement and verifies every coded answer. [Later field trace](field-witness/later.tsv.gz) · [Varied physical histories](field-witness/varied.tsv.gz)

```sh
./axion floodplain --load discovered.flood --steps 0 --slot 890939 --field-retest 512 --class-practice 12 --class-seed 197767 --class-history 1826540586 --class-trace dialects.tsv
python3 tools/field-voice-audit.py class dialects.tsv
```

All 28 native regression targets passed. Native Blackwell machine code and portable sm_80 PTX produced byte-identical habitat checkpoints, portable minds and individual dialect traces on this laptop. [Recorded digests](field-witness/replay.csv)

The prospectively watched advance processed 3,506,438,144 site-ticks. The uninterrupted replay and the observed run shared the same final bytes. The packaged million-site world occupies 259,792,749 bytes in Git LFS. [Hardware timings](field-witness/throughput.csv)

The physical curriculum mattered much more than the vocabulary in these experiments. That distinction motivated separate controls for the learning history and the recalled event. Human single-neuron work on rapid visual learning and EEG work on recall of repeated experiences similarly distinguish changes during acquisition from reinstatement of a particular experience. Axion's two question forms and four-socket visual representation are engineering choices. The results establish a narrow learned conversation in a naturally born body. Four open-ended questions about causes, origins, future plans and teaching a game elicited four abstentions. General language, remote episodic recall and spontaneous acquisition of a community vocabulary remain open. [Open-ended exchange](field-witness/open-ended.txt) · [Human rapid perceptual learning](https://www.nature.com/articles/s41593-026-02496-6) · [Temporal-context reinstatement of recurring events](https://doi.org/10.1177/09567976261417325)
