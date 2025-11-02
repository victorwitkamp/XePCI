# Makefile — XePCI (stand-alone PCI kext, no Lilu)

# ---- Project ----
KEXT_NAME    = XePCI.kext
BUNDLE_ID    = nl.victorwitkamp.XePCI
VERSION      = 1.0.0

# ---- Layout ----
BUILD_DIR     = build
KEXT_DIR      = $(BUILD_DIR)/$(KEXT_NAME)
CONTENTS_DIR  = $(KEXT_DIR)/Contents
MACOS_DIR     = $(CONTENTS_DIR)/MacOS
RES_DIR       = $(CONTENTS_DIR)/Resources
PLIST         = kexts/Info.plist

# ---- Sources ----
SOURCES = \
    kexts/XeService.cpp \
    kexts/XeUserClient.cpp \
    kexts/ForcewakeGuard.cpp \
    kexts/XeGGTT.cpp \
    kexts/XeCommandStream.cpp

HEADERS = \
    kexts/XeService.hpp \
    kexts/XeUserClient.hpp \
    kexts/ForcewakeGuard.hpp \
    kexts/XeGGTT.hpp \
    kexts/XeCommandStream.hpp \
    kexts/xe_hw_offsets.hpp

# ---- SDK / toolchain ----
KERNEL_SDK ?= /Library/Developer/SDKs/MacKernelSDK
CXX        = clang++
ARCH      ?= x86_64

CXXFLAGS = -std=c++17 -Wall -Wextra \
	-fno-rtti -fno-exceptions -fno-builtin -fno-common \
	-mkernel -nostdlib \
	-D__KERNEL__ -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
	-DPRODUCT_NAME=XePCI -DMODULE_VERSION=\"$(VERSION)\" \
	-isysroot $(KERNEL_SDK) \
	-iframework $(KERNEL_SDK)/System/Library/Frameworks \
	-I$(KERNEL_SDK)/Headers \
	-Ikexts \
	-arch $(ARCH) \
	-target x86_64-apple-macos10.15 \
	-fvisibility=hidden \
	-Wno-unused-parameter -Wno-missing-field-initializers

LDFLAGS = -Xlinker -kext \
	-nostdlib -lkmod -lkmodc++ -lcc_kext \
	-Xlinker -export_dynamic -Xlinker -no_deduplicate \
	-arch $(ARCH) \
	-target x86_64-apple-macos10.15

OBJS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# ---- Targets ----
.PHONY: all clean install uninstall load unload status test-load release debug help

all: release

help:
	@echo "XePCI stand-alone PCI kext"
	@echo "Targets: release (default), debug, install, uninstall, load, unload, status, clean, test-load"

release: CXXFLAGS += -O2
release: $(KEXT_DIR)

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(KEXT_DIR)

# Bundle
$(KEXT_DIR): $(BUILD_DIR) $(CONTENTS_DIR) $(MACOS_DIR) $(RES_DIR) $(OBJS)
	@echo "Linking kext..."
	$(CXX) $(LDFLAGS) -o $(MACOS_DIR)/XePCI $(OBJS)
	@echo "Copying Info.plist..."
	cp $(PLIST) $(CONTENTS_DIR)/Info.plist
	/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(VERSION)" $(CONTENTS_DIR)/Info.plist || true
	/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $(VERSION)" $(CONTENTS_DIR)/Info.plist || true
	@echo "Build complete: $(KEXT_DIR)"

# Objects
$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dirs
$(BUILD_DIR) $(CONTENTS_DIR) $(MACOS_DIR) $(RES_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

# ---- Install / Uninstall ----
install: $(KEXT_DIR)
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	- kmutil unload -b $(BUNDLE_ID) >/dev/null 2>&1 || true
	rm -rf /Library/Extensions/$(KEXT_NAME)
	cp -R $(KEXT_DIR) /Library/Extensions/
	chown -R root:wheel /Library/Extensions/$(KEXT_NAME)
	chmod -R 755 /Library/Extensions/$(KEXT_NAME)
	@echo "Rebuilding kernel cache..."
	kmutil install --volume-root / --update-all || kextcache -i / || touch /Library/Extensions/
	@echo "Installed /Library/Extensions/$(KEXT_NAME)"

uninstall:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	- kmutil unload -b $(BUNDLE_ID) >/dev/null 2>&1 || true
	rm -rf /Library/Extensions/$(KEXT_NAME)
	kmutil install --volume-root / --update-all || kextcache -i / || touch /Library/Extensions/
	@echo "Removed $(KEXT_NAME)"

# ---- Quick loads ----
test-load: $(KEXT_DIR)
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kextutil -v 6 $(KEXT_DIR) || true

load:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kmutil load -p /Library/Extensions/$(KEXT_NAME)

unload:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kmutil unload -b $(BUNDLE_ID) || true

status:
	kmutil showloaded | grep -i $(BUNDLE_ID) || echo "$(BUNDLE_ID) not loaded"
