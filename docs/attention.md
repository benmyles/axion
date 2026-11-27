# Asking about an open scene or a previous sighting

The world can change while both animals have their eyes open. The questioner’s spoken request can make its partner retrieve the last reliable sighting before it looks again. Run `./axion talk`, or open a moving ecology with `./axion talk --world examples/confluence-late.eco`. The original observer used below and its successor with learned motion predictions are both available. [Questioner](../examples/inquirer.mind) · [Observer](../examples/observer.mind) · [Successor](imagery.md)

```text
/scene west north
Where is the food?
animal> west
Where is the water?
animal> north
/scene east south
Where was the water?
animal> north
Where was the food?
animal> west
Where is the food?
animal> east
Where was the water?
animal> north
Where is the water?
animal> south
```

## How the pair learns

The caller has eight additional word-sensitive fields. A form can change its emitted pitch as a function of its previous pitch. Four-bit synapses excite two competing output cells. The witness hears the pitch through a separate pair of synaptic projections onto two attention muscles. Their firing determines whether the witness samples the open eye or reinstates activity from its visual memory. Its existing spiking body still chooses the resource and emits the compass word. The interface lowercases and hashes each token. A request can include an unknown token or place the resource name before, between or after the source words. The only word meanings supplied by the program are in the classroom and independent scorer. [Caller and receiver contacts](../src/attention.cuh) · [Public sensory pathway](../src/dialogue.cu)

The classroom trains the witness’s pitch convention through feedback on its own movements. The caller then tries pitches, hears feedback about the resulting movement, and changes a contact according to its inherited 32-bit rule. Both programs started with zero update bits in the selected cohorts. The mutation and selection loop operated on CUDA. New words, contacts and prior activity stay in the individual checkpoint. `/teach-focus yesterday remember` and `/teach-focus yesterday look` provide two successive lessons in the same lifetime. There are eight source fields. Recruiting a ninth form can displace a previous binding. [CUDA classroom](../src/attention_school.cuh) · [Study driver](../tools/attention-study.py)

Human electrocorticography has identified responses specific to composing minimal phrases. Recent single-neuron recordings also report activity associated with several linguistic features during natural speech. These observations motivated testing word order with exactly the same words and testing effects of word sequences after meanings changed. The experiment implements a finite two-state composition task. General syntax, a productive vocabulary and unrestricted conversation remain open. [Murphy et al., 2022](https://pubmed.ncbi.nlm.nih.gov/35232761/) · [Human single-neuron recordings, 2026](https://www.nature.com/articles/s41586-026-10691-5)

## Results on different speakers, scenes and words

Six selected populations ran 192 generations of 256 candidate pairs, with four new teaching environments per candidate and generation. All six champions executed 12,288/12,288 eight-word sequences after the associations changed in their held-out environments. The strongest final individuals from six matched drift populations also performed well at 12,279/12,288, with 11,259/12,288 on single words after reassignment. Selection improved consistency in this small classroom. Disabling synaptic change eliminated acquisition. The released pair correctly resolved 32,768/32,768 reassigned 128-word sequences across eight fresh sets of 512 teaching environments. Replacing action-dependent feedback with random food pulses yielded 1,985/32,768. [Selected cohorts](attention/selected.csv) · [Drift](attention/drift.csv) · [Held-out assessments](attention/held-out.csv)

The public text experiment independently moved the resources and scored the words emitted by the full animals. It first checked that externally visible locations differed from previously seen ones. The two orders of *look* and *switch* have identical words and opposite meanings. The animal answered 128/128 for each order. Across 128 further utterances it retained the parity of a 128-word sequence containing only *switch* and *keep*. These chains contained no word that could reset the result to a known source. The pair learned a new source word on 64/64 probes and revised its use on 64/64 after a second lesson. A new eight-form vocabulary with an independently reversed pitch convention reproduced the order and parity results. [Public interviews](attention/spoken.csv) · [Renamed vocabulary and reversed dialect](attention/renamed.csv)

With pitch transmission suppressed, the witness still answered visible questions while all 16 explicit recall questions in two matched streams gave the wrong bearing. Erasing visual reinstatement preserved visible answers and eliminated the 16 correct remembered answers. Inverting pitch also disrupted the task. These interventions act between the question and the answer. [Muted pitch](attention/spoken-muted.csv) · [Erased visual activity](attention/spoken-erased.csv) · [Inverted pitch](attention/spoken-inverted.csv)

In a separately scored living habitat, the reporter moved to new positions and let the ecology advance between questions. Its prior report was scored against the measured currents at the last inspected location. Current reports used the currents at the new viewpoint. On each of two habitat checkpoints, the pair gave all 128 prior reports and all 128 current reports correctly, including silence before a first sighting. In 155 prior reports across the two habitats, the remembered direction differed from the current direction. [Original habitat](attention/habitat.csv) · [Habitat after 131,072 ticks](attention/late-habitat.csv)

The habitat continued from tick 131,072 to 196,608. Its population grew from 417 to 527, with 145,700 births, 145,590 deaths and 11,272,366 moves over the interval. A reporter can now follow a living resident's viewpoint using its physical slot, founder, birth tick and generation. In eight sessions the selected residents moved through four to seven distinct sites each. The pair answered all 252 questions asked while attached to a living resident. One tracked animal died during a later advance. The reporter then used its last viewpoint for four more questions. The observers’ answers and movements have separate records. [Resident habitat](../examples/confluence-residents.eco) · [Tracked observations and answers](attention/resident-views.csv)

## The cost of choosing a source

For fixed hearing, eight acquired forms can be compiled into 16 one-bit conditional transitions. The word transitions, emitted pitch and the receiver’s two muscle responses fit in one 32-bit word. Each pair retains its own 16-byte visual program and contact state. Vision continues to learn on every exposed observation. The complete direct peripheral pair uses 80 bytes, and the compiled representation uses 20. Teaching new source words requires recompiling the frozen word projections. [CUDA implementation](../src/attention_batch.cuh)

The RTX PRO 2000 Blackwell laptop ran 1,048,576 independent peripheral pairs for 64 decisions each. The direct and compiled kernels agreed on all 67,108,864 scored outcomes and the checksum of their activity and learned visual state. The median of three device-timed runs was 5.64 billion decisions/s direct and 24.04 billion/s compiled. Each decision includes a word-conditioned transition, pitch reception, source selection and visual state update. This benchmark excludes cortical spiking, text I/O, vocabulary compilation and reporting. The separate full-body exchange benchmark is in the communication report. [Raw times and checksums](attention/scale.csv) · [Full-body benchmark](conversation.md#blackwell-execution)

```sh
./axion attention --listener examples/interpreter.mind --speaker examples/witness.mind --origin inert --seed 4993 --generations 192 --candidates 256 --worlds 4 --cycles 8 --length 8 --save-listener reader.mind --save-speaker observer.mind
./axion talk --listener reader.mind --speaker observer.mind
python3 tools/attention-study.py interview
python3 tools/habitat-interview.py --recall --world examples/confluence-late.eco
python3 tools/habitat-interview.py --recall --follow --world examples/confluence-residents.eco
./axion attention --listener examples/inquirer.mind --speaker examples/observer.mind --generations 0 --replicas 1048576 --decisions 64
```
