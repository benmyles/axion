# Learning what a sound refers to

The learner sees two moving eight-channel retinal images and hears a caption. Nobody identifies which patch is being named. A single animal encounters many scenes. Each acoustic prototype acquires a set of candidate visual fields from their co-occurrence. Its inherited two-bit synaptic transition and readout tables determine which fields survive. Ordered changes in position, brightness and motion let it distinguish possible relations. At most 256 ambiguous caption episodes and 64 conversational exchanges are retained.

The physical training room supplies word boundaries, a three-token declarative utterance and a finite vocabulary. The 576 visual candidate circuits cover conjunctions of up to three of eight inputs. There are 24 candidate relational comparisons. A learner can ignore inputs, admit multiple hypotheses or remain silent. The laboratory can select the local update law by what its offspring correctly infer and by their ability to supply a missing word. The floodplain uses ecological reproduction independently of this laboratory score.

```sh
./axion talk --speaker examples/marsh-reader.mind
```

Enter the following lines in order. The first puts two objects on a physical retina. The two-digit number is a light pattern and the two decimal numbers are prior and current positions in an eight-by-eight field.

```text
/tabletop 00@0>0 09@3>3
/point moth
who is west of vole?
and vole?
is vole west moth?
```

The saved animal points to site 0, says *moth*, says *moth* again using the relation from its last answer and then responds *no*. Putting both objects on the table teaches no names. External lessons can use `/overhear`, `/ground-example QUESTION | REPLY` and `/ground-replay`. The physical codes in a shuffled culture, its active perceptual channels, word order and relation conventions differ across worlds. A checkpoint can retain the acquired word fields, episodic references, question frames and the previous grounded proposition.

When two different names both fit, the animal can request confirmation. Try `/tabletop 00@0>0 02@2>2 01@7>7`, then ask `who is west of vole?`. It asks about one of the possible names. Its learned affirmative or negative sound selects the intended participant. Changing the scene clears the pending question. The previous confirmed proposition remains available for follow-up.

In a separate GPU test, a private speaker intention was drawn uniformly from the labels that were physically true. Across 96 shuffled cultures, the selected learning rule identified **17,576 of 17,884** intended referents, made zero wrong commitments and emitted 22,012 clarification questions. Random feedback reduced correct identifications to 7,759. The expected total from guessing uniformly among the true answers was 7,823.15. Clarification uses a fixed dialogue controller over learned acoustic and visual bindings. It requires shared affirmative and negative conventions.

## Transfer and controls

Training uses only even-parity light patterns. All test appearances have odd parity. A separate hash withholds complete unordered pairs of noun meanings for particular relations, including their reversal. The evaluator independently reads the physical positions and identities. A recovered noun must match its target field on all 256 visual patterns with both its possible and certain masks. Wrong statements and unanswered questions are counted separately.

Three independent breeding runs started from random learning rules. Across 192 new cultures their selected rules recovered all 1,536 noun fields. They correctly judged **33,025 of 48,084** held-out statements and answered **4,155 of 6,214** uniquely answerable subject questions. Progress depended on the breeding run. The specific rule carried by the marsh reader, tested separately in 96 unfamiliar cultures, judged 23,964 of 23,992 statements correctly, abstained on 28 and gave no wrong judgments. Breaking scene–speech pairing removed noun recovery across all three selected rules. Preserving co-occurrence while randomly reversing half the captions retained the nouns and reduced correct judgments to 483 of 48,084. Drift recovered zero complete noun fields in the new cultures. [Independent studies](grounding/cross-situational.csv)

```sh
./axion ground --origin random --population 96 --generations 32 --episodes 512 --worlds 96 --dialect shuffled --seed 91571 --save new-reader.mind
python3 tools/grounding-study.py --output cross-situational.csv
python3 tests/grounding.py
```

A 2026 evaluation of cross-situational learning on egocentric data found that a small fraction of captions carrying a visible referent supplied much of the learning signal. Visual representations learned from developmental video also underperformed pretrained ones. Experimental work on verb learning also describes how linguistic context and statistical evidence across scenes can constrain different parts of a meaning. These motivated ambiguous exposures and explicit controls over alignment and order. Performance on this small synthetic language measures a particular kind of grounding. Spontaneous acquisition of human language, open conversation and child-level cognition remain unverified. [Egocentric cross-situational learning](https://arxiv.org/abs/2610.31842) · [Experimental verb learning](https://www.la.utexas.edu/users/dil/papers/2025_Chen_CogSci.pdf)
