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

# ---- Sources (remove XeUserClient.* if you don't have them yet) ----
SOURCES = \
  kexts/XeService.cpp \
  kexts/XeUserClient.cpp

HEADERS = \
  kexts/XeService.hpp \
  kexts/XeUserClient.hpp

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
	-arch $(ARCH)

LDFLAGS = -Xlinker -kext \
	-nostdlib -lkmod -lkmodc++ -lcc_kext \
	-Xlinker -export_dynamic -Xlinker -no_deduplicate \
	-arch $(ARCH)

OBJS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# ---- Targets ----
.PHONY: all clean install uninstall load unload status test-load release debug help

all: release

help:
	@echo "XePCI stand-alone PCI kext"
	@echo "Targets: release (default), debug, install, uninstall, load, unload, status, clean"

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

# ---- Install / Uninstall (Big Sur+ flow) ----
install: $(KEXT_DIR)
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	- sudo kmutil unload -b $(BUNDLE_ID) >/dev/null 2>&1 || true
	rm -rf /Library/Extensions/$(KEXT_NAME)
	cp -R $(KEXT_DIR) /Library/Extensions/
	chown -R root:wheel /Library/Extensions/$(KEXT_NAME)
	chmod -R 755 /Library/Extensions/$(KEXT_NAME)
	@echo "Rebuilding kernel cache..."
	kmutil install --volume-root / --update-all || kextcache -i / || touch /Library/Extensions/
	@echo "Installed /Library/Extensions/$(KEXT_NAME)"

uninstall:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	- sudo kmutil unload -b $(BUNDLE_ID) >/dev/null 2>&1 || true
	rm -rf /Library/Extensions/$(KEXT_NAME)
	kmutil install --volume-root / --update-all || kextcache -i / || touch /Library/Extensions/
	@echo "Removed $(KEXT_NAME)"

# ---- Quick load/unload for dev without copying ----
test-load: $(KEXT_DIR)
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kmutil load -p $(KEXT_DIR)

load:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kmutil load -p /Library/Extensions/$(KEXT_NAME)

unload:
	@if [ "$$(id -u)" != "0" ]; then echo "Run with sudo"; exit 1; fi
	kmutil unload -b $(BUNDLE_ID) || true

status:
	kmutil showloaded | grep -i $(BUNDLE_ID) || echo "$(BUNDLE_ID) not loaded"
