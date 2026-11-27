CXX ?= c++
CUDA_CLANG ?= clang++
CUDA_ARCH ?= sm_120
CUDA ?= 1
NATIVE ?= 1
BUILD ?= build
LIFE_FAMILIES = life assays actors memories outcomes appraisals foodactors trails travel mazes springbeds mapactors sequence_field sequence_actors sequence_routes sequence_mazes sequence_springs sequence_appraisals junctions
kernel_sequence_field = trails
kernel_sequence_actors = mapactors
kernel_sequence_routes = travel
kernel_sequence_mazes = mazes
kernel_sequence_springs = springbeds
kernel_sequence_appraisals = appraisals
kernel_junctions = junctions
FAMILIES = foraging lessons routes drawing scenes dialogue vocal_brood recollection causes development composition procedure grounding floodplain siphons runnels mind genesis $(LIFE_FAMILIES)
IMAGE_EXT = $(if $(filter 1,$(NATIVE)),cubin,ptx)
IMAGES = $(addprefix $(BUILD)/$(CUDA_ARCH)/,$(addsuffix .$(IMAGE_EXT),$(FAMILIES)))
.SECONDARY: $(IMAGES)
ifeq ($(NATIVE),1)
# A native release embeds cubins. Make can rebuild a changed cubin from its
# source dependencies while discarding the textual assembly after assembling.
.INTERMEDIATE: $(IMAGES:.cubin=.ptx)
endif
.DELETE_ON_ERROR:
CPPFLAGS += -Iinclude
CXXFLAGS ?= -O3
HOSTFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -pthread
LDLIBS += -ldl -pthread
COMMON = $(addprefix $(BUILD)/,cpu.o gpu.o checkpoint.o evolution.o tutor.o replay.o life.o actor.o dialogue.o recollection.o develop.o compose.o procedure.o ground.o floodplain.o mind.o genesis.o journal.o journal_script.o)
ifeq ($(CUDA),1)
COMMON += $(BUILD)/$(CUDA_ARCH)/module-$(NATIVE).o
endif

.PHONY: all axion test test-gpu test-journal test-development test-composition test-procedures trim clean
all: axion

axion: $(BUILD)/main.o $(COMMON)
	@set -eu; out=$$(mktemp ./.axion-link.XXXXXX); trap 'rm -f -- "$$out"' EXIT; \
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o "$$out"; chmod 755 "$$out"; mv -f -- "$$out" "$@"

$(BUILD)/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOSTFLAGS) -MMD -MP -MF $(@:.o=.d) -MQ $@ -c $< -o $@

$(BUILD)/$(CUDA_ARCH)/assays.ptx: src/breeding.cuh
$(BUILD)/$(CUDA_ARCH)/assays.ptx $(BUILD)/$(CUDA_ARCH)/memories.ptx: src/conditioning.cuh
$(BUILD)/$(CUDA_ARCH)/memories.ptx: src/reinstatement.cuh
$(BUILD)/$(CUDA_ARCH)/junctions.ptx: src/conditioning.cuh src/traveller.cuh src/junctions.cuh include/axion/passage.hpp
$(BUILD)/$(CUDA_ARCH)/actors.ptx: src/actor.cuh include/axion/actor_batch.hpp
$(BUILD)/$(CUDA_ARCH)/appraisals.ptx $(BUILD)/$(CUDA_ARCH)/sequence_appraisals.ptx: src/subsistence.cuh src/field_food.cuh src/valuation.cuh src/conditioning.cuh
$(BUILD)/$(CUDA_ARCH)/travel.ptx $(BUILD)/$(CUDA_ARCH)/sequence_routes.ptx: src/conditioning.cuh src/traveller.cuh src/crossings.cuh
$(BUILD)/$(CUDA_ARCH)/mazes.ptx $(BUILD)/$(CUDA_ARCH)/sequence_mazes.ptx: src/conditioning.cuh src/traveller.cuh src/terrain.cuh src/labyrinth.cuh
$(BUILD)/$(CUDA_ARCH)/springbeds.ptx $(BUILD)/$(CUDA_ARCH)/sequence_springs.ptx: src/conditioning.cuh src/traveller.cuh src/terrain.cuh src/migration.cuh include/axion/passage.hpp
$(BUILD)/$(CUDA_ARCH)/mapactors.ptx $(BUILD)/$(CUDA_ARCH)/sequence_actors.ptx: src/actor.cuh include/axion/actor_batch.hpp
$(BUILD)/$(CUDA_ARCH)/foodactors.ptx: src/actor.cuh include/axion/actor_batch.hpp
$(addprefix $(BUILD)/$(CUDA_ARCH)/,$(addsuffix .ptx,$(LIFE_FAMILIES))): $(BUILD)/$(CUDA_ARCH)/%.ptx: src/life.cu src/life_brain.cuh src/somatic_core.cuh src/somatic_genome.cuh src/associations.cuh src/assemblies.cuh src/outcomes.cuh src/wayfinding.cuh src/temporal.cuh include/axion/actor.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -DAX_FAMILY_$(or $(kernel_$*),$*) $(if $(kernel_$*),-DAX_SEQUENCE) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/%.ptx: src/kernels.cu src/grid.cuh include/axion/model.hpp include/axion/arena.hpp include/axion/lesson.hpp include/axion/recall.hpp include/axion/routes.hpp include/axion/workshop.hpp include/axion/planning.hpp include/axion/archive.hpp include/axion/detector.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -DAX_FAMILY_$(or $(kernel_$*),$*) $(if $(kernel_$*),-DAX_SEQUENCE) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/vocal_brood.ptx: src/vocal_brood.cu include/axion/recollection.hpp include/axion/habitat_view.hpp src/life.cu src/life_brain.cuh src/somatic_core.cuh src/somatic_genome.cuh src/associations.cuh src/assemblies.cuh src/outcomes.cuh src/wayfinding.cuh src/temporal.cuh include/axion/dialogue.hpp include/axion/auditory.hpp include/axion/actor_batch.hpp include/axion/actor.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/recollection.cu src/recollection.cuh src/memory_proof.cuh src/attention.cuh src/attention_school.cuh src/attention_batch.cuh include/axion/attention.hpp include/axion/recollection.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/dialogue.ptx: src/dialogue.cu src/recollection.cuh src/attention.cuh include/axion/attention.hpp include/axion/recollection.hpp include/axion/dialogue.hpp include/axion/auditory.hpp include/axion/actor_batch.hpp include/axion/actor.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/scenes.ptx: src/field_journal.cu include/axion/census.hpp include/axion/journal.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/imagery.cuh src/imagery_world.cuh
$(BUILD)/$(CUDA_ARCH)/dialogue.ptx: src/imagery.cuh
$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/binding.cuh src/binding_world.cuh include/axion/binding.hpp
$(BUILD)/$(CUDA_ARCH)/dialogue.ptx: src/binding.cuh include/axion/binding.hpp
$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/binding_apparatus.cuh
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx vocal_brood.ptx recollection.ptx): src/conjunction.cuh
$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/conjunction_world.cuh src/conjunction_batch.cuh src/conjunction_certificate.cuh
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx vocal_brood.ptx recollection.ptx): include/axion/report.hpp
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx recollection.ptx): src/report.cuh
$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/report_world.cuh
$(BUILD)/$(CUDA_ARCH)/vocal_brood.ptx: src/culture.cuh src/report.cuh
$(BUILD)/$(CUDA_ARCH)/vocal_brood.ptx: src/estuary.cuh src/concept_warp.cuh src/concept.cuh
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx vocal_brood.ptx recollection.ptx): include/axion/concept.hpp
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx vocal_brood.ptx): include/axion/causal.hpp
$(addprefix $(BUILD)/$(CUDA_ARCH)/,dialogue.ptx recollection.ptx): src/concept.cuh
$(BUILD)/$(CUDA_ARCH)/recollection.ptx: src/concept_world.cuh src/concept_batch.cuh src/concept_warp.cuh
$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/runnels.cu include/axion/nerve_pages.hpp include/axion/floodplain.hpp include/axion/braid_parallax.hpp src/braid_parallax.cuh src/braid_perspective.cuh include/axion/braid_view.hpp src/braid_inquiry.cuh include/axion/braid_inquiry.hpp src/braid_recollection.cuh include/axion/braid_discourse.hpp include/axion/braid_episode.hpp src/braid_culture.cuh include/axion/braid_culture.hpp src/braid_dialogue.cuh include/axion/braid_talk.hpp src/braid_studies.cuh include/axion/braid_trials.hpp include/axion/braid_route.hpp include/axion/braid.hpp src/rattle_trials.cuh include/axion/rattle.hpp include/axion/rattle_trials.hpp include/axion/runnel_trace.hpp src/runnel_keepers.cuh src/runnel_physics.cuh src/runnel_analogy.cuh include/axion/flume.hpp include/axion/runnel.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/siphons.ptx: src/siphons.cu include/axion/nerve_pages.hpp include/axion/braid_route.hpp include/axion/braid.hpp include/axion/backwater.hpp include/axion/rattle.hpp src/runnel_reference.cuh src/runnel_physics.cuh src/runnel_analogy.cuh include/axion/flume.hpp include/axion/wicks.hpp include/axion/runnel.hpp $(wildcard include/axion/practice*.hpp) $(wildcard src/practice*.cuh) $(wildcard include/axion/linkage*.hpp) src/linkage_cells.cuh src/linkage_design.cuh include/axion/linkage_design.hpp src/linkage_physics.cuh src/effort_core.cuh src/effort_bundle.cuh include/axion/effort_bundle.hpp include/axion/effort.hpp $(wildcard src/siphon*.cuh) include/axion/hollow_image.hpp include/axion/hollow_words.hpp src/grounding_eye.cuh include/axion/grounding.hpp include/axion/ceramic_words.hpp include/axion/siphon_words.hpp include/axion/press.hpp include/axion/hollow.hpp include/axion/clay.hpp include/axion/siphon.hpp include/axion/floodplain.hpp include/axion/material.hpp include/axion/solvent.hpp include/axion/worksite.hpp include/axion/foresight.hpp include/axion/cortex.hpp include/axion/pods.hpp include/axion/recollection.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: src/floodplain.cu include/axion/nerve_pages.hpp include/axion/braid_view.hpp include/axion/braid_episode.hpp include/axion/braid_gate.hpp include/axion/braid_route.hpp include/axion/braid.hpp include/axion/rattle.hpp src/runnel_analogy.cuh include/axion/flume.hpp include/axion/runnel.hpp src/runnel_physics.cuh include/axion/wicks.hpp src/wick_physics.cuh $(wildcard include/axion/practice*.hpp) $(wildcard src/practice*.cuh) $(wildcard include/axion/linkage*.hpp) src/linkage_cells.cuh src/linkage_design.cuh include/axion/linkage_design.hpp src/linkage_physics.cuh src/effort_core.cuh src/effort_bundle.cuh include/axion/effort_bundle.hpp include/axion/effort.hpp src/hollow_contest.hpp include/axion/press.hpp include/axion/hollow.hpp src/flood_tray.hpp include/axion/material.hpp include/axion/mill.hpp include/axion/artifacts.hpp include/axion/foresight.hpp include/axion/worksite.hpp include/axion/solvent.hpp include/axion/clay.hpp include/axion/siphon.hpp include/axion/field_voice.hpp $(wildcard src/flood*.cuh) src/somatic_core.cuh src/somatic_genome.cuh src/grounding_eye.cuh include/axion/cortex.hpp include/axion/pods.hpp include/axion/floodplain.hpp include/axion/grounding.hpp include/axion/life.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/grounding.ptx: src/grounding.cu $(wildcard src/grounding*.cuh) include/axion/grounding.hpp include/axion/life.hpp include/axion/model.hpp include/axion/recollection.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/procedure.ptx: src/procedure.cu $(wildcard src/procedure*.cuh) include/axion/report.hpp src/garden_motion.cuh src/lantern.cuh src/agency.cuh src/causal.cuh src/concept.cuh src/causal_warp.cuh src/concept_warp.cuh include/axion/causal.hpp include/axion/concept.hpp include/axion/procedure.hpp include/axion/model.hpp include/axion/life.hpp include/axion/recollection.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/composition.ptx: src/composition.cu $(wildcard src/composition*.cuh) include/axion/composition.hpp include/axion/life.hpp include/axion/recollection.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/development.ptx: src/development.cu $(wildcard src/sequence*.cuh) include/axion/sequence.hpp include/axion/life.hpp include/axion/model.hpp include/axion/recollection.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/causes.ptx: src/causes.cu src/causal.cuh src/causal_warp.cuh src/causal_world.cuh src/agency.cuh src/agency_dialogue.cuh src/garden_motion.cuh src/lantern.cuh src/concept_warp.cuh src/concept.cuh include/axion/causal.hpp include/axion/concept.hpp include/axion/report.hpp include/axion/recollection.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@
$(BUILD)/$(CUDA_ARCH)/vocal_brood.ptx: include/axion/census.hpp include/axion/grounding.hpp include/axion/procedure.hpp include/axion/composition.hpp include/axion/sequence.hpp src/commons.cuh src/binding.cuh src/binding_apparatus.cuh src/imagery.cuh src/recollection.cuh include/axion/binding.hpp include/axion/attention.hpp include/axion/commons.hpp
$(BUILD)/$(CUDA_ARCH)/causes.ptx: src/orchard.cuh include/axion/commons.hpp include/axion/dialogue.hpp include/axion/actor.hpp

$(BUILD)/journal_script.o: src/journal_script.S tools/compound_journal.py tools/context_journal.py tools/detritus_journal.py tools/capillary_journal.py tools/inquiry_journal.py tools/retina_journal.py tools/inlet_journal.py tools/drift_journal.py tools/braid_journal.py tools/mind_journal.py tools/genesis_journal.py tools/roadmap_card.py tools/journal.py tools/creature_cards.py tools/population_cards.py
	@mkdir -p $(@D)
	$(CXX) -c $< -o $@

$(BUILD)/assemble: tools/assemble.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(HOSTFLAGS) $< -l:libcuda.so.1 -o $@

$(BUILD)/$(CUDA_ARCH)/%.cubin: $(BUILD)/$(CUDA_ARCH)/%.ptx $(BUILD)/assemble
	$(BUILD)/assemble $(CUDA_ARCH) $< $@

$(BUILD)/$(CUDA_ARCH)/module-$(NATIVE).o: src/module.S $(IMAGES)
	$(CXX) -DAX_NATIVE=$(NATIVE) -Wa,-I,$(@D) -c $< -o $@

$(BUILD)/tests.o: tests/tests.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOSTFLAGS) -MMD -MP -MF $(@:.o=.d) -MQ $@ -c $< -o $@

$(BUILD)/tests-$(CUDA)-$(CUDA_ARCH)-$(NATIVE): $(BUILD)/tests.o $(COMMON)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: $(BUILD)/tests-$(CUDA)-$(CUDA_ARCH)-$(NATIVE)
	$<

test-gpu: $(BUILD)/tests-$(CUDA)-$(CUDA_ARCH)-$(NATIVE)
	$< --gpu

test-journal: axion
	python3 tests/journal.py
	python3 tests/creature_cards.py

test-development: axion
	python3 tests/development.py

test-composition: axion
	python3 tests/composition.py

test-procedures: axion
	python3 tests/procedure.py

.PHONY: test-floodplain test-grounding test-pods test-growth test-materials test-shoal test-worksites test-brine test-siphons test-clay test-press test-hollows test-effort test-linkages test-practice test-runnels test-backwaters test-braids test-parallax test-neural-dispatch
test-floodplain test-grounding test-pods test-growth test-materials test-shoal test-worksites test-brine test-siphons test-clay test-press test-hollows test-effort test-linkages test-practice test-runnels test-backwaters test-braids test-parallax: axion
	python3 tests/$(@:test-%=%).py

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/drift_dialogue.cuh include/axion/drift.hpp src/field_class.cuh include/axion/field_class.hpp src/apprentice.cuh include/axion/apprentice.hpp include/axion/regulation.hpp include/axion/feedback_contact.hpp

.PHONY: test-field-voices
test-field-voices: axion
	python3 tests/field_voices.py

.PHONY: test-gate-trial
test-gate-trial: axion
	python3 tests/gate_trial.py

test-neural-dispatch: axion
	python3 tests/neural_dispatch.py

$(addprefix $(BUILD)/$(CUDA_ARCH)/,floodplain.ptx runnels.ptx siphons.ptx): include/axion/drift_marker.hpp

$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: include/axion/gate_trial.hpp src/drift_legacy.cuh include/axion/drift.hpp include/axion/regulation.hpp include/axion/apprentice.hpp include/axion/feedback_contact.hpp include/axion/shore.hpp include/axion/braid_discourse.hpp include/axion/braid_talk.hpp

# Retain the runnable program, host objects and individual device kernels.
trim:
	@for file in $(BUILD)/sm_*/module-[01].o $(BUILD)/tests-[01]-sm_*-[01]; do \
	    if test -f "$$file"; then rm -- "$$file"; fi; \
	done
ifeq ($(NATIVE),1)
	@for assembly in "$(BUILD)/$(CUDA_ARCH)"/*.ptx; do \
	    native=$${assembly%.ptx}.cubin; \
	    if test -f "$$assembly" && test -s "$$native" && ! test "$$assembly" -nt "$$native"; then rm -- "$$assembly"; fi; \
	done
endif
	@python3 tools/trim-build.py ./axion "$(BUILD)"

clean:
	$(RM) -r $(BUILD) axion

-include $(wildcard $(BUILD)/*.d)

.PHONY: test-apprentice
test-apprentice: axion
	python3 tests/apprentice.py

.PHONY: test-drift test-inlets
$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: include/axion/inlet.hpp src/flood_inlet.cuh src/inlet_history.cuh include/axion/fovea.hpp include/axion/retina.hpp include/axion/retina_study.hpp src/flood_retina.cuh src/flood_sounding.cuh
$(BUILD)/$(CUDA_ARCH)/runnels.ptx: include/axion/inlet.hpp src/inlet_interview.cuh include/axion/fovea.hpp include/axion/retina.hpp include/axion/retina_study.hpp include/axion/cascade.hpp src/cascade.cuh
test-drift: axion
	python3 tests/drift.py

test-inlets: axion
	python3 tests/inlets.py

.PHONY: test-inquiry
.PHONY: test-utterances
test-utterances: axion
	python3 tests/utterances.py

.PHONY: test-audience
test-audience: axion
	python3 tests/audience.py

$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: include/axion/audience.hpp include/axion/utterance.hpp include/axion/narrative_world.hpp include/axion/narrative.hpp src/flood_ink.cuh src/flood_capillary.cuh src/flood_detritus.cuh include/axion/material_context.hpp include/axion/detritus.hpp include/axion/capillary_field.hpp include/axion/capillary.hpp include/axion/capillary_course.hpp include/axion/capillary_sheet.hpp
$(BUILD)/$(CUDA_ARCH)/runnels.ptx: include/axion/audience.hpp include/axion/audience_course.hpp include/axion/audience_batch.hpp src/audience.cuh
$(BUILD)/$(CUDA_ARCH)/runnels.ptx: include/axion/utterance.hpp include/axion/narrative.hpp include/axion/narrative_image.hpp include/axion/narrative_recall.hpp include/axion/narrative_world.hpp include/axion/narrative_school.hpp src/narrative.cuh src/chorus.cuh src/capillary.cuh src/material_context.cuh include/axion/context_study.hpp include/axion/context_exposure.hpp include/axion/material_context.hpp include/axion/capillary_manual.hpp include/axion/capillary_play.hpp include/axion/capillary_voice.hpp include/axion/capillary_field.hpp include/axion/capillary_tutor.hpp include/axion/capillary.hpp include/axion/capillary_sheet.hpp include/axion/capillary_course.hpp
test-inquiry: axion
	python3 tests/inquiry.py

.PHONY: test-retina
test-retina: axion
	python3 tests/retina.py

.PHONY: test-capillaries
test-capillaries: axion
	python3 tests/capillaries.py

.PHONY: test-detritus
test-detritus: axion
	python3 tests/detritus.py

.PHONY: test-material-context
test-material-context: axion
	python3 tests/material_context.py

.PHONY: test-retention
test-retention: axion
	python3 tests/retention.py

$(addprefix $(BUILD)/$(CUDA_ARCH)/,floodplain.ptx runnels.ptx): include/axion/catalysis.hpp

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/material_signal.cuh include/axion/material_signal.hpp

.PHONY: test-catalysis
test-catalysis: axion
	python3 tests/catalysis.py

$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: src/chemical_wake.cuh include/axion/material_signal.hpp

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/compound_sign.cuh include/axion/compound_sign.hpp

.PHONY: test-compound
test-compound: axion
	python3 tests/compound.py

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/signal_voice.cuh include/axion/signal_voice.hpp

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: src/compound_conversation.cuh

$(BUILD)/$(CUDA_ARCH)/runnels.ptx: include/axion/clarification.hpp

.PHONY: test-signal-voice
test-signal-voice: axion
	python3 tests/signal_voice.py

$(BUILD)/$(CUDA_ARCH)/floodplain.ptx: src/field_hands.cuh include/axion/material_hands.hpp include/axion/compound_sign.hpp

$(BUILD)/$(CUDA_ARCH)/mind.ptx: src/mind.cu src/mind_core.cuh src/mind_hippocampus.cuh src/mind_causal.cuh src/mind_neocortex.cuh src/mind_spatial.cuh src/mind_world.cuh include/axion/mind.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/$(CUDA_ARCH)/genesis.ptx: src/genesis.cu src/genesis_dna.cuh src/genesis_world.cuh src/genesis_brain.cuh include/axion/genesis.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=sm_120 -nocudainc -nocudalib -S $< -o $@

.PHONY: test-mind test-genesis
test-mind: axion
	python3 tests/mind.py

test-genesis: axion
	python3 tests/genesis.py
