# RaphaelIGPU.kext — AMD Raphael RDNA2 iGPU
#
# Linux / this Cloud Agent: `make test` (ATOM / VFCT parser only).
# Darwin + macOS SDK: `make kext` produces RaphaelIGPU.kext and RaphaelFB.kext.

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
	submit/RaphaelAccelerator.cpp \
	kext/RaphaelIGPU.cpp

FB_KEXT_NAME := RaphaelFB
FB_KEXT_BUNDLE := $(BUILD)/$(FB_KEXT_NAME).kext
FB_KEXT_SRCS := \
	$(HOST)/iofb/IOFBLinearShell.cpp \
	display/RaphaelFramebuffer.cpp \
	kext/RaphaelFB.cpp

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror

.PHONY: all test kext xcodebuild clean help

all: test

help:
	@echo "make test       — ATOM / VFCT connector parser (portable)"
	@echo "make kext       — Darwin Makefile; RaphaelIGPU.kext + RaphaelFB.kext"
	@echo "make xcodebuild — xcodebuild Release for both .xcodeproj targets"

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

xcodebuild:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "xcodebuild requires Darwin"; \
		exit 1; \
	fi
	xcodebuild -project RaphaelIGPU.xcodeproj -scheme RaphaelIGPU -configuration Release \
		-derivedDataPath "$(CURDIR)/build/DerivedData-IGPU" build
	xcodebuild -project RaphaelFB.xcodeproj -scheme RaphaelFB -configuration Release \
		-derivedDataPath "$(CURDIR)/build/DerivedData-FB" build
	@echo "built build/xcode/Release/RaphaelIGPU.kext and RaphaelFB.kext"

clean:
	rm -rf $(BUILD)
