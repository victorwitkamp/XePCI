# Makefile for XePCI Lilu Plugin
# Builds the kernel extension for macOS

# Project Configuration
KEXT_NAME = XePCI.kext
BUNDLE_ID = nl.victorwitkamp.XePCI
VERSION = 1.0.0

# Directories
BUILD_DIR = build
KEXT_DIR = $(BUILD_DIR)/$(KEXT_NAME)
CONTENTS_DIR = $(KEXT_DIR)/Contents
MACOS_DIR = $(CONTENTS_DIR)/MacOS
RESOURCES_DIR = $(CONTENTS_DIR)/Resources

# Source files
SOURCES = kexts/XePCI_LiluPlugin.cpp kexts/XeService.cpp
HEADERS = kexts/XeService.hpp
PLIST = kexts/Info.plist

# Lilu SDK paths - adjust based on your Lilu installation
LILU_SDK ?= /usr/local/include/Lilu
KERNEL_SDK ?= /Library/Developer/SDKs/MacKernelSDK

# Compiler and linker flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra \
	-fno-rtti -fno-exceptions \
	-fno-builtin -fno-common \
	-mkernel -nostdlib \
	-D__KERNEL__ -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
	-DPRODUCT_NAME=XePCI -DMODULE_VERSION=\"$(VERSION)\" \
	-isysroot $(KERNEL_SDK) \
	-iframework $(KERNEL_SDK)/System/Library/Frameworks \
	-I$(KERNEL_SDK)/Headers \
	-I$(LILU_SDK) \
	-Ikexts

LDFLAGS = -Xlinker -kext \
	-nostdlib -lkmod -lkmodc++ -lcc_kext \
	-Xlinker -export_dynamic \
	-Xlinker -no_deduplicate

# Architecture support
ARCH ?= x86_64
CXXFLAGS += -arch $(ARCH)
LDFLAGS += -arch $(ARCH)

# Targets
.PHONY: all clean install uninstall debug release help

all: release

help:
	@echo "XePCI Lilu Plugin Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build release version (default)"
	@echo "  release     - Build optimized release version"
	@echo "  debug       - Build debug version with symbols"
	@echo "  clean       - Remove build artifacts"
	@echo "  install     - Install kext (requires sudo)"
	@echo "  uninstall   - Uninstall kext (requires sudo)"
	@echo ""
	@echo "Variables:"
	@echo "  LILU_SDK    - Path to Lilu headers (default: /usr/local/include/Lilu)"
	@echo "  ARCH        - Target architecture (default: x86_64)"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - Xcode Command Line Tools installed"
	@echo "  - Lilu SDK headers available at LILU_SDK path"
	@echo "  - SIP disabled for testing (csrutil disable)"

release: CXXFLAGS += -O2
release: $(KEXT_DIR)

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(KEXT_DIR)

$(KEXT_DIR): $(BUILD_DIR) $(CONTENTS_DIR) $(MACOS_DIR) $(RESOURCES_DIR)
	@echo "Building XePCI Lilu Plugin..."
	
	# Compile source files
	$(CXX) $(CXXFLAGS) -c kexts/XePCI_LiluPlugin.cpp -o $(BUILD_DIR)/XePCI_LiluPlugin.o
	$(CXX) $(CXXFLAGS) -c kexts/XeService.cpp -o $(BUILD_DIR)/XeService.o
	
	# Link kernel extension
	$(CXX) $(LDFLAGS) -o $(MACOS_DIR)/XePCI $(BUILD_DIR)/XePCI_LiluPlugin.o $(BUILD_DIR)/XeService.o
	
	# Copy Info.plist
	cp $(PLIST) $(CONTENTS_DIR)/Info.plist
	
	# Set bundle version
	/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(VERSION)" $(CONTENTS_DIR)/Info.plist || true
	
	@echo "Build complete: $(KEXT_DIR)"
	@echo "To install: sudo make install"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CONTENTS_DIR):
	mkdir -p $(CONTENTS_DIR)

$(MACOS_DIR):
	mkdir -p $(MACOS_DIR)

$(RESOURCES_DIR):
	mkdir -p $(RESOURCES_DIR)

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

install: $(KEXT_DIR)
	@echo "Installing XePCI.kext..."
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Installation requires root privileges. Run with sudo."; \
		exit 1; \
	fi
	
	# Unload if already loaded
	-kextunload /Library/Extensions/$(KEXT_NAME) 2>/dev/null
	
	# Copy to system extensions
	cp -R $(KEXT_DIR) /Library/Extensions/
	
	# Fix permissions
	chown -R root:wheel /Library/Extensions/$(KEXT_NAME)
	chmod -R 755 /Library/Extensions/$(KEXT_NAME)
	
	# Rebuild kernel cache (for macOS 11+)
	@echo "Rebuilding kernel cache (this may take a few minutes)..."
	-kmutil install --volume-root / --update-all 2>/dev/null || \
		kextcache -i / 2>/dev/null || \
		touch /Library/Extensions/
	
	@echo "Installation complete. Reboot required for changes to take effect."
	@echo "Note: Ensure SIP is disabled (csrutil status)"

uninstall:
	@echo "Uninstalling XePCI.kext..."
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Uninstallation requires root privileges. Run with sudo."; \
		exit 1; \
	fi
	
	# Unload if loaded
	-kextunload /Library/Extensions/$(KEXT_NAME) 2>/dev/null
	
	# Remove from system
	rm -rf /Library/Extensions/$(KEXT_NAME)
	
	# Rebuild kernel cache
	-kmutil install --volume-root / --update-all 2>/dev/null || \
		kextcache -i / 2>/dev/null || \
		touch /Library/Extensions/
	
	@echo "Uninstall complete"

# Debugging and testing helpers
.PHONY: load unload status test-load

load:
	@echo "Loading XePCI.kext..."
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Loading requires root privileges. Run with sudo."; \
		exit 1; \
	fi
	kextload $(KEXT_DIR) || kextload /Library/Extensions/$(KEXT_NAME)

unload:
	@echo "Unloading XePCI.kext..."
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Unloading requires root privileges. Run with sudo."; \
		exit 1; \
	fi
	kextunload /Library/Extensions/$(KEXT_NAME) || kextunload $(KEXT_DIR)

status:
	@echo "Checking XePCI.kext status..."
	@kextstat | grep -i xepci || echo "XePCI.kext is not loaded"

test-load: $(KEXT_DIR)
	@echo "Testing kext load (without installing)..."
	@if [ "$$(id -u)" != "0" ]; then \
		echo "Error: Testing requires root privileges. Run with sudo."; \
		exit 1; \
	fi
	kextutil $(KEXT_DIR)
