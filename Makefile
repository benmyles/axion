CXX ?= c++
CUDA_CLANG ?= clang++
CUDA_ARCH ?= sm_120
CUDA ?= 1
NATIVE ?= 1
BUILD ?= build
IMAGE_EXT = $(if $(filter 1,$(NATIVE)),cubin,ptx)
IMAGES = $(BUILD)/$(CUDA_ARCH)/genesis.$(IMAGE_EXT)
.SECONDARY: $(IMAGES)
ifeq ($(NATIVE),1)
.INTERMEDIATE: $(IMAGES:.cubin=.ptx)
endif
.DELETE_ON_ERROR:

CPPFLAGS += -Iinclude
CXXFLAGS ?= -O3
HOSTFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -pthread
LDLIBS += -ldl -pthread
COMMON = $(addprefix $(BUILD)/,genesis.o journal.o journal_script.o)
ifeq ($(CUDA),1)
COMMON += $(BUILD)/$(CUDA_ARCH)/module-$(NATIVE).o
endif

.PHONY: all axion test test-genesis trim clean
all: axion

axion: $(BUILD)/main.o $(COMMON)
	@set -eu; out=$$(mktemp ./.axion-link.XXXXXX); trap 'rm -f -- "$$out"' EXIT; \
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o "$$out"; chmod 755 "$$out"; mv -f -- "$$out" "$@"

$(BUILD)/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(HOSTFLAGS) -MMD -MP -MF $(@:.o=.d) -MQ $@ -c $< -o $@

$(BUILD)/$(CUDA_ARCH)/genesis.ptx: src/genesis.cu src/genesis_dna.cuh src/genesis_world.cuh src/genesis_brain.cuh include/axion/genesis.hpp include/axion/model.hpp
	@mkdir -p $(@D)
	$(CUDA_CLANG) -std=c++20 -O3 -fno-verbose-asm $(CPPFLAGS) --cuda-device-only --cuda-gpu-arch=$(CUDA_ARCH) -nocudainc -nocudalib -S $< -o $@

$(BUILD)/journal_script.o: src/journal_script.S tools/genesis_journal.py tools/journal.py
	@mkdir -p $(@D)
	$(CXX) -c $< -o $@

$(BUILD)/assemble: tools/assemble.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(HOSTFLAGS) $< -l:libcuda.so.1 -o $@

$(BUILD)/$(CUDA_ARCH)/%.cubin: $(BUILD)/$(CUDA_ARCH)/%.ptx $(BUILD)/assemble
	$(BUILD)/assemble $(CUDA_ARCH) $< $@

$(BUILD)/$(CUDA_ARCH)/module-$(NATIVE).o: src/module.S $(IMAGES)
	$(CXX) -DAX_NATIVE=$(NATIVE) -Wa,-I,$(@D) -c $< -o $@

test: test-genesis
test-genesis: axion
	python3 tests/genesis.py

trim:
	@for file in $(BUILD)/sm_*/module-[01].o; do \
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
