# Materials that creatures can build with

The mineralbank adds a two-dimensional material field to the existing floodplain. Every grain has four faces, can carry excitation and can be manipulated by an animal. A product anywhere in a construction comes from local contact with its neighbors. A new arrangement can operate while its builder is elsewhere or after the builder dies.

```sh
make -j4 NATIVE=1 CUDA_ARCH=sm_120
./axion floodplain --load examples/mineralbank.flood --mineral-flow coupled --steps 1024 --material-legacy 256 --save successors.flood
./axion floodplain --load successors.flood --work-bench 4194304 --quarry-moves 32 --work-subjects 1024 --steps 0
```

New floodplains begin with reactive minerals and hydraulic coupling. Resuming an older checkpoint preserves its original physics. Add `--materials reactive` to introduce geology into an existing bed. A new `--materials off` arena reproduces the earlier substrate for comparisons.

## What can be made

| Physical property | Local law | Consequences for construction |
| --- | --- | --- |
| Four oriented terminals | Each face is closed, positive, negative or compatible with either polarity. Opposite polarities or a compatible terminal make a link on contact. | Rotation can close one connection and open another. Chains, branches, cycles and disconnected assemblies are possible. |
| Finite excitation | Local light must match the absorption band. An open connection can conduct a unit of stored excitation down a potential difference. | A source, transport path and reaction site can occupy different places. A new contact can feed or drain a neighboring structure. |
| Conformation | Responsive grains turn their exposed contact pattern when their charge passes an upper threshold and relax below a lower threshold. | A branch can switch or retain a state after its input changes. The inventory survives the shift. |
| Local chemistry | A compatible surface can consume one unit of excitation and one unit of detritus to form two units of edible product. Energized grain pairs can exchange terminal groups and dissipate a unit of excitation. | Animals can rework future connectivity and reaction conditions. Storage, branching and repeated production need no catalog of devices. |
| Body work | Heritable muscles can rotate, pick up, place, push, pull or redistribute a pair of surface groups. Weight and existing bonds affect the energy cost. | A creature can transport a charged grain and work on another creature’s assembly. Existing buildings and manipulation skills can persist independently. |

The field uses four bytes per site for each deposited grain. Its body circuit adds 72 bytes per resident, including 24 inherited bytes, two populations of two-bit synapses, a carrying slot and lifetime observations. One extra byte per site stores the pre-contact attachment count. Pairs of GPU sites update in four alternating matchings. No global connectivity search runs inside the simulation tick. At censuses, a separate CUDA pass labels connected components. Both the number of grains and the inventory of each of the three terminal groups are conserved. Absorbed light, stored excitation, reaction consumption, dissipation and mechanical work also enter explicit ledgers.

With `--mineral-flow coupled`, the open or closed area of a face also obstructs floodwater. A folding or rotated grain can then change currents and the resources those currents carry. In a 64 × 64 continuation of the mineralbank, enabling the obstruction changed the final water stock at 2,412 sites in a 256-tick builder-free replay. Both copies retained all 164,136 units of water. The permeable control transported 172,403 units across faces during the exposure and the coupled field transported 146,939. Use `--hydraulic-trial 256` to repeat the comparison.

The complete floor can be reshaped. Its spatial extent, palette, capacities and physical laws remain finite. Larger worlds need more GPU memory. The experiments below address development within these constraints. Open-ended technical civilization, tool design on demand and human-level intelligence remain unverified.

## What emerged in the inhabited floodplain

For the first measurement we used the hydraulically permeable condition. We introduced minerals and the manipulation germline of a provisioned quarry worker into the saved shellbank. Existing residents kept their bodies, acquired somatic tissue, food contacts, stored energy and the shared watershed. After 4,096 more ticks at site capacity 512, the population of 2,751 contained a bed of 1,325 grains and another 628 carried in resident slots. It had made 114,933 rotations and 2,743 mechanically induced surface exchanges. The closing field had 365 bonds, 13 branching grains and four independent cycles. The largest connected component contained seven pieces.

After removing the animals and fixing the initial detritus and product stocks, 256 further ticks yielded 386 reactions. Insulating the same grains gave 283. Insulating only bonds incident on handled pieces also gave 283. Permuting intact grains within four-by-four neighborhoods yielded 554. Thus the closing arrangement could carry useful energy, and another arrangement of the same inventory performed better overall.

Six subsequent paired intervals compared continued manipulation with a twin starting in exactly the same state whose hands were held still. Continued manipulation made more product during three of the six later exposures. By the sixth interval the largest assembly contained eleven pieces, and the field made five fewer reactions than its still twin. Assembly size, output and survival measure different properties. [Every interval and counterfactual](materials/heritage.csv) · [Repeat the comparison](../tools/material-lineages.py)

## Measuring what an individual artifact does

```sh
./axion floodplain --load examples/mineralbank.flood --steps 0 --artifact-samples 64
```

At tick 8,576 the field contained 84 handled assemblies with 3–64 grains. We sampled the 64 largest, breaking ties by site number. For each specimen, the experiment keeps every grain, terminal group, relative position and fold state. It starts with zero stored excitation and selects two input ports and a separate output using graph distance alone. The CUDA apparatus applies darkness, two independent lamps, both lamps, delayed substrate, and two equal-length pulse orders. Each illumination has both an insulating and a fixed-conformation twin. Energy conservation is checked independently for each exposure.

Of the 64 specimens, **41** used light at the inputs to make product at the remote output. **Nine** produced nothing under either lamp alone and produced food under both lamps. **Twenty** had a later harvest that depended on which lamp went first. Holding the initial folds fixed changed the harvest for 20 specimens. The field contained 40 distinct seven-measurement responses under this protocol. For example, the five-piece specimen rooted at site 2,150 had outputs **0, 0, 0, 4** for dark, A, B and both. It also made four products when detritus first arrived after the lamps went dark. A threshold response can arise from passive transport and storage. These tests establish a physical function. Evidence that an animal intended that function would require a separate behavioral experiment.

The field plates show a measured specimen and its response. When possible, the picture selects the first two-input-only specimen in the documented sampling order. Across five consecutive snapshots, 311 individual exposures were recorded. [Five-piece specimen and paired exposures](../journal/runs/00007202-floodplain/plate.webp) · [Per-specimen responses and energy balances](materials/artifacts.csv) · [Repeat the survey](../tools/artifact-survey.py)

## Free construction in unfamiliar beds

Each worker has a mobile body and one hand in an eight-by-four field. It can move along its heritable heading, approach either side of a loose piece, exchange terminal groups or leave an existing arrangement alone. The relative source, substrate socket, polarity, connector shapes and intervening materials vary between trials. Placing material indirectly affects reproduction through nutrient production, energy expenditure and the reserves left to provision eggs.

Four populations evolved from random hand and locomotor genes for 1,024 seasons with 16,384 available stations. The fostered controls put the same provisions into eggs with unrelated viable local germlines. We then gave fresh juveniles sixteen opportunities to act in 16,384 new beds and removed them for a 96-tick exposure.

| Source–substrate separation | Rearing | Newly productive beds | Reactions in the built fields | Reactions in untouched twins |
| --- | --- | ---: | ---: | ---: |
| 3 | Provisioned | 1,428 | 154,239 | 132,780 |
| 3 | Fostered | 698 | 134,015 | 132,780 |
| 5 | Provisioned | 816 | 14,148 | 3,760 |
| 5 | Fostered | 369 | 8,112 | 3,760 |

A newly productive bed must make product where the untouched twin and an insulated twin of the built arrangement both make zero. Short-range selected workers gained 22,513 reactions in some beds and lost 1,054 in others. The five-site separation was held out from rearing. These tasks supply an existing seam and loose connectors. An evolved action can repair one contact or can spoil an already useful arrangement. The measured devices do not establish planning or invention from an empty substrate. [Per-seed results, including one and 64 interventions](materials/quarries.csv) · [Reproduction script](../tools/quarry-study.py)

## Learning through another animal’s work

The manipulating organ contains two plastic contact populations. During its own activity, a juvenile can acquire associations between its muscle volley and changes in the visible material. Seeing a neighbor change material can recruit contacts between a local cue and the visible effect. Each synaptic transition, release threshold, routing choice and orientation of the effector is heritable and mutable. The pupil has no input that identifies the demonstrator’s hidden muscle. The same contact functions run in the floodplain, the foraging workstations and the interactive tray.

In a separate nursery, a pair of oppositely polarized contacts must mate with two fixed rails to close a physical energy path. Each worker gets different muscle wiring. Its wrist contacts can learn visual consequences and its somatic motor can bias the selected action. The link between visible material class and effective rotation is reversed across islands. A successful arrangement turns available substrate into food and thereby helps pay for the next brood. Test pupils see two-link demonstrations and work on four-link racks, with new muscle wiring, new locations, and fresh physical inventories.

| Test condition | Working racks out of 131,072 |
| --- | ---: |
| Visible working demonstrations | 111,794 |
| Independently reared living demonstrators that first experimented with both material classes | 106,713 |
| Same live demonstrators, pupils’ view shuttered | 62,079 |
| Copy the original demonstrator’s physical muscle identity | 16,487 |
| Visible demonstrations, no practice with the pupil’s own muscles | 16,515 |
| Independent hidden joints at test | 60,368 |
| Fostered germlines, live demonstrators | 17,447 |

The idealized demonstrations provide a measured ceiling. Each independent live demonstrator instead used its own interventions and observed product. The apparatus reveals visible effects associated with productive constructions. The receiving animal applies its own acquired muscle map in both cases. [Every condition, including scrambled views and different amounts of experience](materials/transmission.csv) · [Run the study](../tools/material-study.py)

A third experiment passed workmanship through six successive cohorts. Each context assigns **two independently randomized polarities**. Every pupil does its own free motor calibration and can practice one work district for food. The other district can be learned by watching its immediate predecessor. Which district each pupil can practice alternates. All acquired contacts start empty at hatching. At the sixth cohort the observing chains completed both joints on **49,978/65,536** paired tests, compared with **27,334/65,536** when the view was shuttered. The independently practiced joint was nearly unaffected by shuttering. On the personally unpracticed joint, the corresponding counts were **53,873** and **30,777**. The action vocabulary is small and each pupil contributes repeated tests. This demonstrates transmission of two bounded physical skills. Whether social transmission supports a growing repertoire of inventions is open. [All six cohorts, four independent test seeds and the no-personal-practice control](materials/apprentices.csv) · [Transmission protocol](../tools/artifact-passage.py)

## The interactive workshop

```sh
./axion talk --speaker examples/joint-reader.mind --listener examples/joint-reader.mind --save-speaker pupil.mind
```

Enter `/workbench 319 2` to seat the animals at separate copies of the physical tray. `/handle-scraps 128` and `/peer-practice 128` give them independent experience of how their muscles change material. `/move-muscle 0` fires a specified pupil motor. `/show-work peer` lets the other animal choose an action. `/peer-sun 48` and `/sun 48` supply light to the peer and pupil setups respectively. The pupil can then choose its own action through `/try-work`.

`/inspect-work` reports each physical piece as location, oriented face byte, stored charge and visible spectrum. `/reach-piece N` changes the accessible socket. `/supply SEED` introduces a drawer of 32 loose grains. `/take-grain` and `/return-grain` transfer one item between the drawer and the actual hand. `/work-word newt` uses the animal's previously learned visual name to locate a uniquely matching workpiece and make its own motor choice. An ambiguous or unknown name leaves the tray alone. Opening the tray brings the actual workpieces into the ordinary visual language circuit. After `/work-requests on`, a single learned name requests a physical action, and multiword questions still use the acquired visual language. `/act-on QUESTION` lets a uniquely named spoken answer cue an intervention. Teachers and pupils pay for their actual handling interventions from body energy. Opening a fresh tray supplies new experimental material.

In the live named-object experiment an external facilitator tried eight demonstrator motors on an independent short rack, then showed the productive intervention. The same resident subsequently located the named part on all sixteen longer-rack challenges. With the demonstration it produced food on eight of eight trays, and it produced none on the eight matched trays with its view shuttered. This was a taught skill on a small apparatus. [All eight trays and both conditions](materials/spoken-work.csv)

The retained joint-reader combines the highest-energy release resident from the first provisioned workshop brood with a separately selected visual-word rule. Its original hearing law recovered zero of eight object names in the test. Eight externally schooled replicas of the selected receptive law recovered 64/64 name fields. This is a deliberate developmental intervention in a saved animal. The population in the field has yet to develop this vocabulary spontaneously. [Interactive teaching script](../tools/bench-lesson.py)

## Reproducing the retained population and resident

These four runs regenerate the two distributed checkpoints from the saved shellbank and seed 1877. The first pair introduces a quarry-reared hand into the watershed. The second pair rears a workshop hand and selects its visual-word circuit. Both resulting files matched the distributed examples byte for byte on the Blackwell laptop. The permeable setting reproduces the conditions of the original experiment.

```sh
./axion floodplain --side 64 --seed 1877 --mineral-flow permeable --quarry-seasons 1024 \
  --work-capacity 16384 --work-days 16 --work-span 2 --quarry-exposure 48 --steps 0 --export-mind quarry.mind
./axion floodplain --load examples/shellbank.flood --materials reactive --mineral-flow permeable \
  --hand-germ quarry.mind --steps 4096 --report-every 565 --save mineralbank.flood
./axion floodplain --side 64 --seed 1877 --mineral-flow permeable --work-seasons 384 \
  --work-capacity 8192 --work-days 32 --work-span 2 --steps 0 --export-mind builder.mind
./axion ground --load builder.mind --origin random --generations 48 --population 64 \
  --worlds 8 --episodes 1024 --crowd 4 --dialect english --probes 128 --save joint-reader.mind
cmp mineralbank.flood examples/mineralbank.flood
cmp joint-reader.mind examples/joint-reader.mind
```

## GPU deployment and research basis

A frozen hand compiles to **28 bytes**. For each receptive class it stores the unboosted winners and two masks describing which basal input can tie or win outright. The same readout covers exploration, variable tie breaking and all eight basal inputs. On the RTX PRO 2000 Blackwell laptop, 8,388,608 exhaustive input comparisons on 256 real residents agreed against the writable circuit. A 4,194,304-choice batch from 2,751 residents took 0.0863 ms compiled and 0.711 ms with writable tissue, using interleaved medians. The full 4,096-tick floodplain advance ran at 49.6 million site-ticks/s in the recorded run. Frozen hand decisions and whole-ecosystem advances are separate workloads, and laptop clock rates vary.

Experiments with modular origami showed that connectivity and local state can support reprogrammable mechanical functions. A 2026 physical study built cooperating cells whose behavior depended on nearby units. A November 2026 experiment used modular origami to investigate ternary mechanical computing. Our engineering inference was to give the environment local contact, reversible state and transport, then examine the resulting structures after removing their builders. The experiments above use a discrete lattice and abstract energy units. [Modular chiral metamaterials](https://www.nature.com/articles/s41586-025-08851-0) · [Decentralized modular mechanical cells](https://www.nature.com/articles/s41467-026-75166-7) · [Wang et al., ternary mechanical architectures, November 2026](https://www.nature.com/articles/s41467-026-79187-0)

A chimpanzee experiment found that observing a trained individual could enable acquisition of a sequential tool task. A 2026 study of children found that four- and five-year-olds reasoned about how social contacts can give people access to information. These studies suggested testing the content of what a witness could actually see, whether the pupil could act with unfamiliar effectors and whether information could outlast the original model. Axion’s workbench tasks differ from both experiments. [Experimental diffusion of a tool skill](https://pmc.ncbi.nlm.nih.gov/articles/PMC11132989/) · [Children’s inferences about social access to information](https://sll.stanford.edu/docs/2026_Chuey_JaraEttinger_Gweon.pdf)
