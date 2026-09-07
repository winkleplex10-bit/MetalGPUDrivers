# RaphaelIGPU.kext — AMD Raphael RDNA2 iGPU
#
# Linux / this Cloud Agent: `make test` (ATOM parser only).
# Darwin + macOS SDK: `make kext` produces RaphaelIGPU.kext.

IHV := .
HOST := ../../host
INC := -I$(IHV)/include -I$(IHV)/display -I$(IHV)/match -I$(IHV)/submit -I$(HOST)/iofb

BUILD := build
TEST_BIN := $(BUILD)/atom_parse_test
ATOM_SRCS := display/atom_parse.cpp display/atom_parse_test.cpp

KEXT_NAME := RaphaelIGPU
KEXT_BUNDLE := $(BUILD)/$(KEXT_NAME).kext
KEXT_SRCS := \
	$(HOST)/iofb/IOFBLinearShell.cpp \
	match/RaphaelController.cpp \
	display/atom_parse.cpp \
	display/RaphaelConnectorNub.cpp \
	display/RaphaelFramebuffer.cpp \
	submit/RaphaelAccelerator.cpp \
	kext/RaphaelIGPU.cpp

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror

.PHONY: all test kext clean help

all: test

help:
	@echo "make test  — ATOM connector parser (portable)"
	@echo "make kext  — Darwin only; needs macOS Kernel.framework SDK"

test:
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) $(ATOM_SRCS) -o $(TEST_BIN)
	$(TEST_BIN)

kext:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "kext link requires Darwin + macOS SDK; use make test here"; \
		exit 1; \
	fi
	mkdir -p $(KEXT_BUNDLE)/Contents/MacOS
	SDK=$$(xcrun --sdk macosx --show-sdk-path); \
	KERN=$$SDK/System/Library/Frameworks/Kernel.framework/Headers; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) \
		-nostdlib -Wl,-kext -lkmodc++ -lkmod -lcc_kext \
		$(KEXT_SRCS) -o $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME)
	cp kext/Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	@echo "built $(KEXT_BUNDLE)"

clean:
	rm -rf $(BUILD)
