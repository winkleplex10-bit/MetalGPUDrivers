# NvidiaGSP.kext — Blackwell GB203 (RTX 5080) Phase N1 enumerate
#
# Darwin + macOS SDK:
#   make kext        → build/NvidiaGSP.kext
#   make xcodebuild  → build/xcode/Release/NvidiaGSP.kext
# Or open NvidiaGSP.xcodeproj in Xcode.

IHV := .
INC := -I$(IHV)/include -I$(IHV)/match

BUILD := build
KEXT_NAME := NvidiaGSP
KEXT_BUNDLE := $(BUILD)/$(KEXT_NAME).kext
MACOS_DIR := $(KEXT_BUNDLE)/Contents/MacOS

CXX_SRCS := match/NvidiaController.cpp kext/NvidiaGSP.cpp
C_SRCS := kext/NvidiaGSP_info.c

.PHONY: all kext xcodebuild clean help

all: kext

help:
	@echo "make kext       — Darwin Makefile build → build/NvidiaGSP.kext"
	@echo "make xcodebuild — xcodebuild Release → build/xcode/Release/NvidiaGSP.kext"

kext:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "kext link requires Darwin + macOS SDK"; \
		exit 1; \
	fi
	mkdir -p $(BUILD) $(MACOS_DIR)
	SDK=$$(xcrun --sdk macosx --show-sdk-path); \
	KERN=$$SDK/System/Library/Frameworks/Kernel.framework/Headers; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-Wno-deprecated-declarations \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) -c match/NvidiaController.cpp -o $(BUILD)/NvidiaController.o; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-Wno-deprecated-declarations \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) -c kext/NvidiaGSP.cpp -o $(BUILD)/NvidiaGSP.o; \
	xcrun --sdk macosx clang -mkernel -std=gnu17 -fno-builtin \
		-Wno-deprecated-declarations \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN -c kext/NvidiaGSP_info.c -o $(BUILD)/NvidiaGSP_info.o; \
	xcrun --sdk macosx clang++ -mkernel -fapple-kext -nostdlib \
		-Wl,-kext -lkmodc++ -lkmod -lcc_kext \
		-o $(MACOS_DIR)/$(KEXT_NAME) \
		$(BUILD)/NvidiaController.o $(BUILD)/NvidiaGSP.o $(BUILD)/NvidiaGSP_info.o
	cp kext/Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	@echo "built $(KEXT_BUNDLE)"
	@ls -lh $(MACOS_DIR)/$(KEXT_NAME)

xcodebuild:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "xcodebuild requires Darwin"; \
		exit 1; \
	fi
	xcodebuild -project NvidiaGSP.xcodeproj -scheme NvidiaGSP -configuration Release \
		-derivedDataPath "$(CURDIR)/build/DerivedData" build
	@echo "built build/xcode/Release/NvidiaGSP.kext"

clean:
	rm -rf $(BUILD)
