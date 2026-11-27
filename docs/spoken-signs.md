# Asking another animal what it saw

In an encounter with its view of the pressure junctions closed, a forager heard one sound from a partner that privately saw the food cue. It asked for the other port’s reading, received a word, went to the southeast feeder and described its own movement. This pair also acquired a four-way material code when the southeast cue was entirely absent during its original rearing. The two stages are separate. The animals first acquired the physical foraging code from meals and later learned a human-provided acoustic vocabulary in individual, paid demonstrations. [4K field plate](compound-signs/transfer.webp) · [Material rearing and omitted-cue replication](compound-signs.md)

```text
What did you see?
speaker> tap ring
pupil> tick?
speaker> ring
/cap-ask pupil Where is the food?
pupil> south east
```

Only “tap” reached the listener on the first transmission. Its own learned contacts associated the proposed “tick?” with the second junction. The witness consulted its private observation and replied “ring.” Changing only the witness’s reply to the word for the other state caused the same listener to report “south west.” The receiving animal had no optical access to the junctions or the original cue. The console shows both sides of the damaged link. [Factual exchange](compound-signs/unseen-live-partial-1-actual.txt) · [Changed witness evidence](compound-signs/unseen-live-partial-1-exchanged.txt)

## What the animals acquired

Each pupil paid to operate the same pair of pigment junctions and to move a limb in opposed directions. A calibrated resonator paired a sound and a spoken question with one physical event at a time. In 128 demonstrations per domain each pupil performed 320 pressure strokes and 127 or 128 physical limb strokes. The teacher supplied zero two-word examples, zero destinations paired with the pressure words, and zero examples of questions between pupils. The evidence records the actual movements, heating cost, spikes and signed synaptic changes. A teacher can name the same physical events differently, or an operator can teach a new sound through the workshop. [Six independent pairs of tuition records](compound-signs/tuition.csv)

The learned organ occupies 96 bytes in an enrolled individual. It holds the waveforms heard through its inherited ear, their signed sensory and motor contacts, two recognized question patterns and seven recent admissions to episodic memory. An animal can carry the organ home or retain a private occurrence across saving and reopening its individual checkpoint. Both directions of the speech link visit those contacts. The human-facing console renders the actual pulses as words. General sentence interpretation and the acquisition of new conversation protocols remain open problems. [Local auditory substrate](../include/axion/signal_voice.hpp) · [Return and replay tests](../tests/signal_voice.py)

## When is a question worth asking?

The listener evaluates all optical states still compatible with the sounds it received. Each state passes through its own frozen spiking motor circuit using the same noise. If all plausible states produce the same movement it can act immediately. Otherwise it can pay to utter one of its learned pressure words whose answer would distinguish different movements. Its genome affects the preference and threshold for asking. This method of comparing possible actions is built into the simulator. The identity of the sound, its meaning, the reply, and the destination reached depend on the animals’ separate histories. Both agents spend energy in the interactive workshop.

| Link to the same listener | Questions sent | Answers heard | Moved | Stocked feeder reached |
| --- | ---: | ---: | ---: | ---: |
| Both sounds available, up to two questions | 0 | 0 | 4,096 | 3,730 |
| One sound available, no questions | 0 | 0 | 328 | 162 |
| One sound available, one factual answer | 3,768 | 3,768 | 4,096 | 3,730 |
| One sound available, answer withheld | 3,768 | 0 | 328 | 162 |
| One sound available, opposite observation | 3,768 | 3,768 | 4,096 | 162 |
| Independent loss of both sounds, no questions | 0 | 0 | 1,112 | 986 |
| Independent loss of both sounds, one question allowed | 2,984 | 2,984 | 3,156 | 2,846 |
| Independent loss of both sounds, two questions allowed | 3,924 | 3,924 | 4,096 | 3,730 |

Every row uses the same 1,024 fixtures with four matched physical alternatives per fixture. The zero-budget control computes precisely the same motor uncertainty as the conditions that allow a question. With the input link muted or the witness’s view closed, the pair recovered no food. With both sounds clear but the receiver taught the opposite dialect, it reached two of the 4,096 stocked feeders. In the factual, one-word condition the listener correctly described all 4,096 of its own movements, including 366 that failed to reach food. [All 57 matched experiments](compound-signs/spoken.csv) · [Single missing word](compound-signs/unseen-partial-1-actual-00.tsv.gz) · [Opposite observation](compound-signs/unseen-partial-1-exchanged-00.tsv.gz) · [No questions](compound-signs/unseen-partial-0-actual-00.tsv.gz)

Reversing the order of the first two sounds preserved performance. The receiver can treat these sounds as a set of two features. Both question forms were taught as whole utterances. Evidence from toddlers learning variable labels and experiments on how bonobos communicate about a hidden reward motivated varying the language and the information each participant could obtain. These studies concern real behaviors that this small task only samples. The lab supplies the vocabulary, turn-taking mechanism, opportunity to question and resource arrangement. Open conversation and child-level generalization have yet to be demonstrated. [Casey and Lew-Williams, Cognition 2026](https://doi.org/10.1016/j.cognition.2026.106647) · [Townrow and Krupenye, PNAS 2025](https://doi.org/10.1073/pnas.2412450122)

## Run the encounter

```sh
./axion talk --speaker examples/compound-spoken-writer.mind --listener examples/compound-spoken-reader.mind < examples/compound-clarification.txt
./axion talk --speaker examples/compound-spoken-writer.mind --listener examples/compound-foreign-reader.mind < examples/compound-spoken-conversation.txt
python3 tools/spoken-signs.py /var/tmp/axion-new-spoken-study --subject unseen
python3 tools/spoken_audit.py docs/compound-signs/unseen-patchy-2-actual-00.tsv.gz --channel patchy --questions 2
make test-signal-voice
```

In an interactive session the first seven setup commands in the example feed each separate animal, establish two sealed ports and close the receiver’s view. Use `/cap-private speaker se 961` to provide the witness with a new event and ask the witness or the receiver about its own experience. An unfamiliar phrasing can elicit silence. [Complete interactive setup](../examples/compound-clarification.txt)
