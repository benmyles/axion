# Predicting the result of an action before taking it

The original conversational pair could report where food was and where it had last seen it. When asked what direction would result from a clockwise turn, it scored 0/128. The same happened for an opposite bearing. The successor observed sixteen movements of a four-position apparatus. It then answered 128/128 of both kinds of questions through its vocal muscles. Its visual memory still referred to the real sighting after a hypothetical movement. [Original transfer test](attention/untrained-reasoning.csv) · [Same questions after observing movements](imagery/transfer.csv) · [Trained individual](../examples/dreamer.mind)

```text
/scene west north
Where is the food?
animal> west
Where would the food be after clockwise?
animal> north
Where is the food now?
animal> west
Where would the food be after clockwise clockwise?
animal> east
/show-motion blink west south
Where would the food be after blink?
animal> south
```

## One local rule for two tasks

In one human intracranial study, hippocampal ripples coincided with planning sequences and compositional representations in neocortex. A separate human study manipulated transition structure to examine the hippocampus’s contribution to relational learning and inference. These findings motivated an engineering question. Could the local rule evolved to retain a visual observation also learn action-conditioned associations, then propagate activity through them in a new order? [Human planning sequences, 2026](https://www.nature.com/articles/s41593-026-02291-3) · [Relational structure learning and inference, 2026](https://www.nature.com/articles/s41562-026-02627-w)

The new circuit uses the existing evolved visual rule without another generation of breeding. Six action-sensitive fields add **48 bytes** to the witness. Each has a four-byte sound fingerprint and sixteen two-bit contacts. In the apparatus, a movement exposes a departure bearing, the action’s sound and the observed arrival bearing. Activity at the departure and arrival cells changes the contacts locally. The world retains the physical law. Both animals can hear an action word in an interview. Each recognized sound triggers a wave through the witness’s corresponding contacts. Its partner’s normal request still selects the resource and the source of evidence. A projected outcome enters the existing vocal circuit. [Plastic contacts and replay](../src/imagery.cuh) · [Physical training apparatus](../src/imagery_world.cuh)

The four possible starting activities can propagate together while the witness is still hearing the question. Their reachable outputs fit into the four nibbles of a 16-bit relation. Four integer products distribute destination spikes to the active source rows. A bitwise union completes one imagined step. The operation also represents branching and empty paths. The regression test checks every destination relation against every possible incoming four-cell activity in each row. [Packed composition](../src/imagery.cuh) · [Exhaustive row test](../tests/tests.cpp)

## Unfamiliar worlds and changed laws

Eight held-out seeds each generated 256 private apparatuses. In one family each action had a randomly permuted set of destinations. In another, different starting sites could converge on the same destination. Each animal observed sixteen individual movements covering the four starting states and four actions. The laws then changed, and it could observe another sixteen movements. Test sequences were drawn after these individual observations. No trajectory or sequence endpoint was supplied to the circuit during acquisition.

| After the physical laws changed | Permutations / 32,768 | Many-to-one maps / 32,768 |
| --- | ---: | ---: |
| Intact prediction, 64 actions | 32,768 | 32,768 |
| Frozen at birth | 8,510 | 8,258 |
| Arrivals reassigned to another trial | 8,163 | 9,567 |
| Actuator sounds muted during replay | 8,510 | 8,258 |
| Same actions replayed in reverse order | 11,033 | 10,786 |
| Learning frozen at the change | 8,125 | 8,072 |

Displacing arrival signals preserves their counts within every sixteen-movement teaching sweep. Reversing an action sequence preserves its complete set of sounds. Both manipulations allow the distinction between storing observed outcomes and correctly composing them in order to be measured. [Permuted worlds](imagery/permutation.csv) · [Many-to-one worlds](imagery/arbitrary.csv)

In an independent public interview, the experimenter named four unfamiliar actions and showed their effects using `/show-motion`. The experimenter generated the laws separately from the CUDA classroom. The speaker correctly followed 64/64 new 32-action sequences after learning and another 64/64 after all four laws were reassigned, for each world family. It correctly distinguished both orders on 128/128 deliberately noncommuting action pairs per order in each family. It stayed silent when asked about an unobserved transition of a familiar sound, then answered after a single demonstration. Suppressing replay preserved every visible report and reduced revised 32-action accuracy to 3/16 on the matched permutation streams. [Spoken permutations](imagery/spoken-permutation.csv) · [Spoken many-to-one actions](imagery/spoken-arbitrary.csv) · [Replay intervention](imagery/spoken-muted.csv)

This motion-only individual scored **0/128** when asked about an exchange between two objects, a location two sightings back, or an earlier shared location. These questions use the same four possible answers. A later individual learned cross-object counterfactuals by observing and changing paired arrangements. [Original transfer trials](imagery/transfer.csv) · [Object-binding experiments](bindings.md)

Eight interviews followed real residents through the ecology that began at tick 196,608. Current, previous and hypothetical clockwise reports each scored 128/128 against the sampled food and water currents. In each category 126 questions came from the position of a still-living resident and two from the final viewpoint after a resident died. [Observed currents, identities and spoken forecasts](imagery/residents.csv)

## Batched replay on Blackwell

Four learned action relations compile into eight bytes of readouts. After the laws change, the kernel reacquires the transitions and recompiles these eight bytes from the new synaptic state. The direct and compiled executions agreed on each individual world's scores. At 32,768 active apparatuses, the GPU taught and retaught every individual, then evaluated singles, pairs and 32-action sequences for a total of 17,825,792 replay steps. Across three seeds the median was 1.462 ms direct and 0.542 ms compiled. Both timings include simulated physical practice, forecasting and on-device scoring. Host I/O and the separate spiking vocal pathway are outside this benchmark. [Times and counts](imagery/scale.csv)

```sh
./axion imagine --load examples/observer.mind --observations 16 --save my-dreamer.mind
./axion talk --speaker my-dreamer.mind
python3 tools/imagery-study.py assay --dynamics arbitrary --length 64
python3 tools/imagery-study.py interview --dynamics permutation --length 32 --probes 8
python3 tools/habitat-interview.py --follow --recall --predict --world examples/confluence-residents.eco
```
