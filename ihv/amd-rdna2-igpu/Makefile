# RaphaelIGPU.kext — AMD Raphael RDNA2 iGPU
#
# Product: macOS 26, WhateverGreen absent (see docs/BUILD-RULES.md).
# Linux / this Cloud Agent: `make test` (ATOM / VFCT parser only).
# Darwin + macOS 26 SDK: `make kext` produces RaphaelIGPU.kext and RaphaelFB.kext.

IHV := .
HOST := ../../host
INC := -I$(IHV)/include -I$(IHV)/display -I$(IHV)/match -I$(IHV)/submit -I$(HOST)/iofb

BUILD := build
TEST_BIN := $(BUILD)/atom_parse_test
ATOM_SRCS := display/atom_parse.cpp display/atom_parse_test.cpp

KEXT_NAME := RaphaelIGPU
KEXT_BUNDLE := $(BUILD)/$(KEXT_NAME).kext
KEXT_SRCS := \
	match/RaphaelController.cpp \
	display/atom_parse.cpp \
	display/RaphaelConnectorNub.cpp \
	display/RaphaelDmub.cpp \
	display/RaphaelDcnModeset.cpp \
	submit/RaphaelAccelerator.cpp \
	kext/RaphaelIGPU.cpp

FW_CACHE := firmware/cache
FW_BASE ?= https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/plain
LICENSE_URL ?= https://raw.githubusercontent.com/thesofproject/linux-firmware/master/LICENSE.amdgpu
FW_BLOBS := dcn_3_1_5_dmcub.bin psp_13_0_5_toc.bin psp_13_0_5_ta.bin

FB_KEXT_NAME := RaphaelFB
FB_KEXT_BUNDLE := $(BUILD)/$(FB_KEXT_NAME).kext
FB_KEXT_SRCS := \
	$(HOST)/iofb/IOFBLinearShell.cpp \
	display/RaphaelFramebuffer.cpp \
	kext/RaphaelFB.cpp

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror

.PHONY: all test kext clean help fetch-firmware

all: test

help:
	@echo "make test            — ATOM / VFCT connector parser (portable)"
	@echo "make fetch-firmware  — download linux-firmware AMD blobs (not committed)"
	@echo "make kext            — Darwin + macOS 26 SDK; RaphaelIGPU.kext + RaphaelFB.kext + Resources"

fetch-firmware:
	mkdir -p $(FW_CACHE)
	curl -fsSL -o firmware/LICENSE.amdgpu $(LICENSE_URL)
	curl -fsSL -o $(FW_CACHE)/dcn_3_1_5_dmcub.bin $(FW_BASE)/amdgpu/dcn_3_1_5_dmcub.bin
	curl -fsSL -o $(FW_CACHE)/psp_13_0_5_toc.bin $(FW_BASE)/amdgpu/psp_13_0_5_toc.bin
	curl -fsSL -o $(FW_CACHE)/psp_13_0_5_ta.bin $(FW_BASE)/amdgpu/psp_13_0_5_ta.bin
	@ls -l firmware/LICENSE.amdgpu $(FW_CACHE)/dcn_3_1_5_dmcub.bin \
		$(FW_CACHE)/psp_13_0_5_toc.bin $(FW_CACHE)/psp_13_0_5_ta.bin

test:
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) $(ATOM_SRCS) -o $(TEST_BIN)
	$(TEST_BIN)

kext:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "kext link requires Darwin + macOS SDK; use make test here"; \
		exit 1; \
	fi
	mkdir -p $(KEXT_BUNDLE)/Contents/MacOS $(FB_KEXT_BUNDLE)/Contents/MacOS
	SDK=$$(xcrun --sdk macosx --show-sdk-path); \
	KERN=$$SDK/System/Library/Frameworks/Kernel.framework/Headers; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) \
		-nostdlib -Wl,-kext -lkmodc++ -lkmod -lcc_kext \
		$(KEXT_SRCS) -o $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME)
	cp kext/Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	mkdir -p $(KEXT_BUNDLE)/Contents/Resources
	@if [ ! -f $(FW_CACHE)/dcn_3_1_5_dmcub.bin ]; then $(MAKE) fetch-firmware; fi
	cp firmware/LICENSE.amdgpu $(KEXT_BUNDLE)/Contents/Resources/LICENSE.amdgpu
	@for b in $(FW_BLOBS); do \
		if [ -f $(FW_CACHE)/$$b ]; then cp $(FW_CACHE)/$$b $(KEXT_BUNDLE)/Contents/Resources/$$b; \
		else echo "WARNING: missing $(FW_CACHE)/$$b — modeset will GOP-wrap"; fi; \
	done
	SDK=$$(xcrun --sdk macosx --show-sdk-path); \
	KERN=$$SDK/System/Library/Frameworks/Kernel.framework/Headers; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) \
		-nostdlib -Wl,-kext -Wl,-undefined,dynamic_lookup \
		-lkmodc++ -lkmod -lcc_kext \
		$(FB_KEXT_SRCS) -o $(FB_KEXT_BUNDLE)/Contents/MacOS/$(FB_KEXT_NAME)
	cp kext/RaphaelFB-Info.plist $(FB_KEXT_BUNDLE)/Contents/Info.plist
	@echo "built $(KEXT_BUNDLE) and $(FB_KEXT_BUNDLE)"

clean:
	rm -rf $(BUILD) $(FW_CACHE)
