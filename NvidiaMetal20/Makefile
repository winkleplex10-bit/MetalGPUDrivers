# NvidiaMetal20 GOP framebuffer kext
# x86_64 only. Does not touch GSP / BAR0.

KEXT_NAME      := NvidiaMetal20
BUNDLE_ID      := com.metalgpudrivers.NvidiaMetal20
SRC_DIR        := kext/NvidiaFB
BUILD_DIR      := build
KEXT_DIR       := $(BUILD_DIR)/$(KEXT_NAME).kext
MACOS_DIR      := $(KEXT_DIR)/Contents/MacOS

# libkmod is in the Xcode SDK, not the Command Line Tools SDK.
DEVELOPER_DIR  ?= /Applications/Xcode.app/Contents/Developer
export DEVELOPER_DIR
SDKROOT        ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
CXX            ?= $(shell DEVELOPER_DIR=$(DEVELOPER_DIR) xcrun -find clang++)

ARCH           := x86_64
MACOSX_VERSION := 10.15

KERNEL_HDRS    := $(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers

CXXFLAGS := \
	-arch $(ARCH) \
	-target $(ARCH)-apple-macos$(MACOSX_VERSION) \
	-mkernel \
	-std=gnu++17 \
	-fapple-kext \
	-fno-exceptions \
	-fno-rtti \
	-fno-builtin \
	-fno-common \
	-mno-red-zone \
	-DKERNEL \
	-DKERNEL_PRIVATE \
	-DDRIVER_PRIVATE \
	-DAPPLE \
	-DNeXT \
	-isysroot $(SDKROOT) \
	-isystem $(KERNEL_HDRS) \
	-I$(SRC_DIR) \
	-Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations

LDFLAGS := \
	-arch $(ARCH) \
	-target $(ARCH)-apple-macos$(MACOSX_VERSION) \
	-mkernel \
	-fapple-kext \
	-nostdlib \
	-lkmodc++ \
	-lkmod \
	-Xlinker -kext \
	-Xlinker -export_dynamic \
	-isysroot $(SDKROOT)

SOURCES := $(SRC_DIR)/NvidiaGopFramebuffer.cpp
OBJECTS := $(BUILD_DIR)/NvidiaGopFramebuffer.o

.PHONY: all clean kext

all: kext

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/NvidiaGopFramebuffer.o: $(SOURCES) $(SRC_DIR)/NvidiaGopFramebuffer.hpp $(SRC_DIR)/NvidiaPciIds.h
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $(SOURCES)

$(MACOS_DIR)/$(KEXT_NAME): $(OBJECTS)
	mkdir -p $(MACOS_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)
	cp $(SRC_DIR)/Info.plist $(KEXT_DIR)/Contents/Info.plist

kext: $(MACOS_DIR)/$(KEXT_NAME)
	@echo "Built $(KEXT_DIR)"
	@ls -lh $(MACOS_DIR)/$(KEXT_NAME)

clean:
	rm -rf $(BUILD_DIR)
