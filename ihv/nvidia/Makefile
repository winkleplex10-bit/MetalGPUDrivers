# NvidiaGSP.kext — Blackwell GB203 (RTX 5080) Phase N1 enumerate
#
# Darwin + macOS SDK: `make kext` → build/NvidiaGSP.kext
# No portable unit test yet (PCI / GSP are Darwin-only).

IHV := .
INC := -I$(IHV)/include -I$(IHV)/match

BUILD := build
KEXT_NAME := NvidiaGSP
KEXT_BUNDLE := $(BUILD)/$(KEXT_NAME).kext
KEXT_SRCS := \
	match/NvidiaController.cpp \
	kext/NvidiaGSP.cpp

CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror

.PHONY: all kext clean help

all: kext

help:
	@echo "make kext  — Darwin only; NvidiaGSP.kext (N1 enumerate, no GSP boot)"

kext:
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "kext link requires Darwin + macOS SDK"; \
		exit 1; \
	fi
	mkdir -p $(KEXT_BUNDLE)/Contents/MacOS
	SDK=$$(xcrun --sdk macosx --show-sdk-path); \
	KERN=$$SDK/System/Library/Frameworks/Kernel.framework/Headers; \
	xcrun --sdk macosx clang++ -mkernel -std=gnu++17 -fapple-kext \
		-fno-builtin -fno-exceptions -fno-rtti \
		-Wno-deprecated-declarations \
		-DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
		-isystem $$KERN $(INC) \
		-nostdlib -Wl,-kext -lkmodc++ -lkmod -lcc_kext \
		$(KEXT_SRCS) -o $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME)
	cp kext/Info.plist $(KEXT_BUNDLE)/Contents/Info.plist
	@echo "built $(KEXT_BUNDLE)"
	@ls -lh $(KEXT_BUNDLE)/Contents/MacOS/$(KEXT_NAME)

clean:
	rm -rf $(BUILD)
