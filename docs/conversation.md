# Grounded conversation on a spiking circuit

Run `./axion talk` to speak with two CUDA individuals. One hears the user's words and issues a request through its muscles. The other hears that request, orients to a view and emits one of four compass words. Two resource locations, a previously discussed referent, prior sightings and learned words constitute the current conversational domain. The original questioner and speaker are available for reproducing the speech experiments below. The default pair also chooses between a physical view and a prior sighting using an acquired pitch convention. [Original questioner](../examples/interpreter.mind) · [Original speaker](../examples/discourse-speaker.mind) · [Visual memory](recollection.md) · [Present and remembered views](attention.md)

```sh
make -j NATIVE=1 CUDA_ARCH=sm_120
./axion talk
```

```text
/scene west north
Where is the food now?
animal> west
And the other one?
animal> north
Again?
animal> north
/teach brook 2
Completed 192 pointing trials.
Where is the brook today?
animal> north
```

The first bearing in a scene is food and the second is water. A pointing lesson associates a new word form with a view through actual choices and their consequences. The auditory circuit can retain 16 word forms. To keep new learning, start a session with `./axion talk --save-listener pupil.mind --save-speaker witness.mind`. Resume with `./axion talk --listener pupil.mind --speaker witness.mind`. The individual files preserve the neural state, acquired contacts, last emitted request and pending feedback.

## How the world teaches communication

Initial peer experiments give one animal a private observation and the other access to four physical exits. An audible discharge has four possible forms. A closed actuator gives silence. The mapping from perceived objects to sensory ensembles, and from objects to useful exit movements, varies across worlds. The listener has no direct access to the speaker's observation. A successful movement feeds both animals. There is no target call in the peer-learning environment. In the two-attribute environment the listener also receives a private question. Its first muscle discharge is the question the speaker actually hears.

A human intracranial study of an interactive question-answer task found distinguishable planning and articulation dynamics. A 2026 infant study tracked gaze during an adult's meal preparation and found attention concentrated on relevant objects and actions. A separate word-learning experiment examined how infants used gaze to learn an unfamiliar word. Axion's engineering hypothesis was that preparing a request, selecting a physical view, and producing a call could be separately learnable operations whose combination supports a new exchange. These experiments test that hypothesis in a finite artificial world. [Human question-answer recordings](https://pmc.ncbi.nlm.nih.gov/articles/PMC12937506/) · [Infant attention during everyday activity](https://pubmed.ncbi.nlm.nih.gov/41603413/) · [Word learning through gaze](https://journals.sagepub.com/doi/10.1177/00238309251395278)

Every pair owns two spiking tissues, two motor histories, two sets of heritable update programs and their own learned contacts. Acoustic transmission, the eyes, the physical outcomes, local plasticity and breeding execute on CUDA. Whole-field viewing presents both attributes. Active gaze requires the speaker to choose an orientation before it can see one of the two views. The side on which each view lies varies across worlds. Silencing a wire, rotating the delivered sound or fixing the gaze changes an actual sensory path. [Coupled GPU environment](../src/dialogue.cu) · [Shared neuron and motor implementation](../src/actor.cuh) · [GPU inheritance](../src/vocal_brood.cu)

## Conventions born between peers

At the first baseline, 128 pairs of naive receptor–motor learners made 1,416/4,096 correct choices after 256 shared interactions. Silencing the call reduced it to 1,024/4,096. After the exits changed their score fell to 1,080/4,096. Previously trained bodies and different population donors gave a wide range of results. [Peer comparisons](dialogue/peer-baseline.csv) · [Two-way comparisons](dialogue/question-baseline.csv)

Six 512-generation nurseries began with the same untrained receptor–motor founders under different seeds. A generation evaluated 128 candidate pairs in four matched private worlds per candidate. Each speaker and listener inherited its own mutable body and learning programs. Reproduction selected pairs on acquisition and on revision after the exits changed. Drift controls used the same inheritance and mutation with unselected parents. The subsequent common garden hatched individuals from the selected hereditary programs with their prior acquired contacts cleared. [Selected cohorts](dialogue/vocal-select.csv) · [Drift cohorts](dialogue/vocal-drift.csv) · [Speaker germline](../examples/signaller.mind) · [Listener germline](../examples/receiver.mind)

The best of six selected cohorts was confirmed across eight further seeds. Groups of eight pairs shared exactly the same sensory geometry and exits. With the partner they had learned alongside, the animals made **130,520/131,072** correct choices after reversal. Delivering a different pair's call for the very same scene gave **67,328/131,072**. Silencing the channel gave 31,136/131,072, and changing the transmitted symbols gave 33,496/131,072. When both individuals had frozen nervous systems throughout their lifetimes, the four-seed result was 16,384/65,536. The population's reproductive history improved the ability to establish a local convention. The resulting conventions varied between partners. [Matched familiar and unfamiliar partners](dialogue/strangers.csv) · [Both partners frozen](dialogue/strangers-neither.csv) · [Listener alone learns](dialogue/strangers-listener.csv) · [Speaker alone learns](dialogue/strangers-speaker.csv)

## A first meeting after joint play

A stable older animal that had learned four object-specific movements through food could provide a consistent signal. A young listener learned all four. Across four new seeds the matched receptor–motor donors made **32,768/32,768** revised choices with the channel open and 8,192/32,768 with it closed. Changing both partners continuously was much harder. For two attributes, ordinary simultaneous presentation and unrestricted peer fitness alone also left most populations close to chance. [Stable partners](dialogue/handling-1-listener.csv) · [Plastic partners](dialogue/handling-1-both.csv) · [Two-attribute baseline](dialogue/question-baseline.csv) · [Evolved gaze](dialogue/gaze-local.csv) · [Heritable eligibility window](dialogue/gaze-inherited.csv)

In a separate, explicitly tutored environment each animal can experience the consequences of individual actions. A requester tries its muscles against a need. A witness tries orienting to a sound, and handles an object for a consequence. The other animal experiences sounds followed by successful or unsuccessful retrieval. The teacher defines a culture during these exercises. The spiking populations acquire the mappings. Subsequent tests put the actions together as a real exchange. No correct word, hidden target or desired gaze enters either brain at test time.

Under `--scenes withheld`, the low bits of the two bearings partition the 16 resource arrangements into disjoint halves. Both questions occur in both halves. Across eight seeds and 4,096 independently raised pairs, their first joint test on the held-out half yielded **131,072/131,072** correct choices. Silencing the question or fixing the speaker's gaze reduced the score to **65,536/131,072**. Silencing its answer reduced it to **32,768/131,072**. A childlike developmental sequence here means an explicitly constructed curriculum of simpler physical contingencies. [Held-out resource arrangements and interventions](dialogue/unseen-scenes.csv) · [Direct and revised exchanges after practice](dialogue/joint-play-local.csv)

## Remembering what the question referred to

The questioner's own last emitted call is retained in three spare bits of its existing peripheral event word. A word receptor and that efference copy can jointly activate one of 16 sensory ensembles. The cortical meaning of that activation has to be learned. During play the cultured word forms for repeating a referent and switching referents are followed by different physical outcomes, depending on the earlier exchange. The same string can consequently cause either direction of gaze. Goal identity stays private to the environment.

Eight more seeds produced **254,976/262,144** correct referential replies after guided play on held-out resource arrangements. Muting the efference copy gave **131,072/262,144**. Silencing the answer gave 65,536/262,144. The denominator counts related choices within each lifetime. [GPU controls](dialogue/reference-transfer.csv)

A separate text experiment moved both resources between every question. The released pair answered **680/680** questions across chains of 2, 8, 32 and 128 turns. Eight separately raised pairs answered **5,194/5,440**. Training four pairs with the meanings of “again” and “other” exchanged also changed their responses through the same text interface. The external scorer records what repeating the previous bearing or keeping the first referent would have achieved. [Released individual pair](dialogue/long-interview.csv) · [Independent rearing](dialogue/long-transfer.csv) · [Reversed reference vocabulary](dialogue/reversed-reference-meanings.csv) · [Text experiment](../tools/reference-study.py)

## Learning new words by pointing

The text adapter splits words, folds ASCII case and turns spellings into numerical sensory stimuli. Both animals compute their choices with the same spiking motor populations used by the ecological lineages. The four spoken strings label physical actuator settings. The runtime uses local spiking inference and online synaptic updates.

A four-bit word projection alone succeeded on “Where is the water?” but an unrelated trailing word could overwrite the request. A denser sensory code and choosing the strongest motor discharge in a turn gave limited improvement. The released questioner adds 16 auditory receptive fields. Each has a 32-bit word-form fingerprint, four candidate projections with four-bit efficacies and a local activation level. The circuit occupies **108 bytes**, including its inherited signed update table. The complete questioner occupies 460 device bytes, and the speaker occupies 352. Their individual files occupy 484 and 376 bytes respectively. [Single-receptor baseline](dialogue/raised-interviews.csv) · [Distributed features and peak motor evidence](dialogue/suffix-population-peak.csv) · [Auditory circuit](../include/axion/auditory.hpp)

During a pointing lesson, both previously discussed referents occur while the animal tries different auditory projections. The speaker hears an actual neural request, produces an orienting movement, and the auditory contacts receive the physical match or mismatch. The spelling carries no built-in meaning. A word form can subsequently release the learned request even with unfamiliar words before and after it. When a turn contains no recognized form, the vocal channel stays quiet. On ordinary text this is a small grounded vocabulary with a large amount of surrounding speech left uninterpreted.

In the first implementation, two different projections of “brook” both reached the maximum efficacy of 15 despite directing the eyes differently. The founder update table gave a large gain for occasional successes. The current founder uses a smaller positive and larger negative increment. The strongest of a receptive field's projections determines its activation. These updates are genetic and mutate in the vocal nursery. The earlier and corrected experiment records are retained. [Earlier pointing results](dialogue/word-teaching.csv) · [Original cohort transfer](dialogue/word-transfer.csv) · [Corrected individual](dialogue/word-teaching-after.csv)

Eight newly raised pairs each learned 12 additional names. Before teaching, all 1,536 unfamiliar queries produced silence. The first test after the lessons yielded **1,511/1,536** correct replies. Closing and reopening the individual files yielded 1,514/1,536 in a reshuffled test. Reassigning half the names through new pointing lessons yielded **1,527/1,536**, with 192/192 correct on the original named questions and 192/192 on the reference questions. In four additional matched rearing experiments an inert auditory update table gave **0/768** correct replies to the new names following the lessons. The new words name the two available views. The two-view environment bounds the concepts that can be taught here. [Pointing transfer](dialogue/word-transfer-after.csv) · [Inert learning rule](dialogue/word-inert-after.csv) · [Independent scorer](../tools/word-study.py)

## Interviewing a living habitat

```sh
./axion talk --world examples/confluence.eco --save-world later.eco
```

Use `/visit 3 8` to move the viewpoint and `/advance 128` to run 128 real ecological ticks. The world advances the original animal, plant, neural and resource kernels. Four directional sensory currents sum local food biomass and active springs within eight sites, weighted by distance on the torus. The strongest current supplies the bearing visible in that modality. A cardinal tie resolves in clockwise order from north. The language pair must still select the requested view and report it. The journal records the four readings for each modality separately from the animal's words.

Eight sessions sampled 64 different viewpoint visits in a world that began at tick 32,768 and advanced by different amounts. The speaker's **128/128** reports agreed with the maxima of the independently scored sensory currents. The answers included all four bearings. Eighty of the 128 questions required distinguishing two different bearings. The same habitat then continued through ticks 65,536 and 131,072, when 417 animals survived and the deepest lineage spanned 483 generations. In another eight interviews at the later checkpoint the pair made 128/128 correct reports, including 84 with distinct food and spring bearings. The attached pair observes this habitat from a movable viewpoint. The resident creatures continue to move through their own controllers. [Recorded readings and spoken reports](dialogue/living-interviews.csv) · [CUDA sensory field](../src/vocal_brood.cu) · [Habitat checkpoint](../examples/confluence.eco) · [Later world](../examples/confluence-late.eco) · [Continued ecology](dialogue/ecology-timeline.csv) · [Later interviews](dialogue/late-habitat.csv)

## Reproduce the comparisons

```sh
tools/dialogue-study.py --speakers examples/signaller.mind --listeners examples/receiver.mind --pairs 512 --practice 64 --community-size 8 --seeds 15349 34123 57947 110597
tools/reference-study.py --raise-pair --seeds 191399 379997 --lengths 2 8 32 128
tools/word-study.py --raise-pair --seeds 11939 40063
tools/habitat-interview.py --world examples/confluence.eco
```

Independent roles can have different tissue sizes or organs. The batched path strides through the actual compact state of each individual. CUDA graphs replay the sensory, neural and physical operations for eight successive exchanges at a time. External scripts generate experiments and score reports. The runtime has no learned text model outside the two simulated organisms.


## Choosing a convention without a tutor

Two founders from a newly generated ecology supplied the genomes. Six independent cohorts evolved for 1,024 generations using only the consequences of the pairs’ choices. After the physical food doors changed, the selected pairs made 195,248/196,608 correct choices across the six runs. Descendants bred under random parental sampling made 48,512/196,608. On eight fresh world seeds the chosen structured-optics pair scored 131,072/131,072 after local experience. [Selected cohorts](dialogue/unreared-select.csv) · [Drift](dialogue/unreared-drift.csv) · [Fresh worlds](dialogue/unreared-fresh-both.csv)

Each animal still heard and moved when it received the food pulse earned by another pair on the same tick. This preserved the timing and number of pulses across the colony. It removed the causal connection between an animal’s own choice and its meal. The pairs then made 16,384 correct choices out of 65,536, the chance score for four doors. Suppressing food consequences, or disabling reward-dependent motor contacts in either animal, also produced 16,384/65,536. [Reassigned meals](dialogue/consequences-yoked.csv) · [Suppressed meals](dialogue/consequences-none.csv) · [Speaker contacts only](dialogue/contacts-speaker.csv) · [Listener contacts only](dialogue/contacts-listener.csv)

Shuffling all 32 sensory addresses independently for each role removed the regularity in the original optical wiring. Transplanting the first elite into these worlds produced 71,424/131,072 correct choices. Six new cohorts evolved directly with shuffled wiring and made 196,520/196,608 correct choices. The released pair scored 131,072/131,072 on eight fresh seeds. Exchanging its speaker with another already-trained group sharing the same wiring gave 97,408/131,072. Separately selected families introduced to one another **before** practice also acquired a working convention on every one of 196,608 revised choices. The absolute vocabulary depends on shared experience. [Optical transfer](dialogue/shuffled-lark.csv) · [Selection](dialogue/shuffled-selection.csv) · [Fresh confirmation](dialogue/shuffled-confirmation.csv) · [New partners](dialogue/new-partners.csv)

## Blackwell execution

A native run on the RTX PRO 2000 Blackwell laptop allocated **1,048,576 independent pairs**, each with 352 speaker bytes and 460 questioner bytes. The allocated neural and experimental work buffers totaled **1,086,619,648 bytes**. Each pair answered 128 frozen questions about the same sensory world. Across six CUDA event measurements the median was **50.99 million exchanges/s**. At 65,536 pairs the median was **65.99 million exchanges/s**. Each exchange includes preparing a request, orienting, answering and making the receiver's physical choice. The largest test scored 134,217,728/134,217,728. This is a throughput workload of replicated trained states on a shared task. It excludes text segmentation, disk I/O, host statistics and post-run illustration. Block reductions send eight counters and an information estimate per 128 pairs to the host, plus one selected pair's vocal counts. [Batch sizes, repeats, allocations and GPU times](dialogue/scale.csv)
