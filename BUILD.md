# Building XePCI Lilu Plugin

This document describes how to build the XePCI kernel extension (kext) for macOS.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Detailed Build Instructions](#detailed-build-instructions)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)
- [CI/CD](#cicd)

---

## Prerequisites

### Required Software

1. **macOS** - Version 10.15 (Catalina) or later recommended
2. **Xcode Command Line Tools** - For compilation
3. **MacKernelSDK** - Kernel headers for kernel extension development
4. **Lilu SDK** - Required headers for Lilu plugin development

### Installing Prerequisites

#### 1. Install Xcode Command Line Tools

```bash
xcode-select --install
```

Verify installation:
```bash
xcode-select -p
# Should output: /Library/Developer/CommandLineTools
```

#### 2. Install MacKernelSDK

Clone and install the MacKernelSDK:

```bash
# Clone MacKernelSDK
git clone https://github.com/acidanthera/MacKernelSDK.git /tmp/MacKernelSDK

# Install to standard SDK location
sudo mkdir -p /Library/Developer/SDKs
sudo cp -R /tmp/MacKernelSDK /Library/Developer/SDKs/

# Verify installation
ls /Library/Developer/SDKs/MacKernelSDK/Headers/
```

#### 3. Install Lilu SDK Headers

Clone the Lilu repository and install headers:

```bash
# Clone Lilu
git clone https://github.com/acidanthera/Lilu.git /tmp/Lilu

# Install complete SDK to standard location
# This copies Lilu/, hde/, and capstone/ to /usr/local/include
# The symlinks in Lilu/Headers/ (hde64.h -> ../hde/hde64.h, capstone -> ../capstone/include)
# will correctly resolve to the adjacent directories
sudo mkdir -p /usr/local/include
sudo cp -R /tmp/Lilu/Lilu /usr/local/include/
sudo cp -R /tmp/Lilu/hde /usr/local/include/
sudo cp -R /tmp/Lilu/capstone /usr/local/include/

# Verify installation
ls /usr/local/include/Lilu/Headers/
ls /usr/local/include/hde/
ls /usr/local/include/capstone/include/
```

Expected files in Headers directory:
- `plugin_start.hpp`
- `kern_api.hpp`
- `kern_util.hpp`
- `hde32.h` (symlink to ../hde/hde32.h)
- `hde64.h` (symlink to ../hde/hde64.h)
- `capstone` (symlink to ../capstone/include)
- etc.

**Note:** The Headers directory contains symlinks that point to `../hde/` and `../capstone/`. Installing Lilu/, hde/, and capstone/ directories to `/usr/local/include` ensures these symlinks resolve correctly.

---

## Quick Start

For users who want to build immediately:

```bash
# Clone the repository (if not already done)
git clone https://github.com/victorwitkamp/XePCI.git
cd XePCI

# Build release version
make release

# Or build debug version with symbols
make debug

# The built kext will be in: build/XePCI.kext
```

---

## Detailed Build Instructions

### Building Release Version

The release version is optimized for performance:

```bash
make release
```

This will:
1. Create the `build/` directory
2. Compile `kexts/XePCI_LiluPlugin.cpp` with optimization flags
3. Link the kernel extension binary
4. Create the kext bundle structure
5. Copy and configure `Info.plist`

Output: `build/XePCI.kext/`

### Building Debug Version

The debug version includes debug symbols and is not optimized:

```bash
make debug
```

This is useful for:
- Debugging with lldb
- Getting detailed crash reports
- Development and testing

### Build Output

After a successful build, you'll have:

```
build/
└── XePCI.kext/
    └── Contents/
        ├── Info.plist       # Bundle configuration
        └── MacOS/
            └── XePCI        # Kernel extension binary
```

---

## Build Options

### Custom Lilu SDK Path

If you installed Lilu headers to a different location:

```bash
make release LILU_SDK=/path/to/lilu
```

**Note:** The path should point to the Lilu directory (e.g., `/usr/local/include/Lilu`), not to the Headers subdirectory.

### Architecture Selection

By default, builds target x86_64. To specify architecture:

```bash
make release ARCH=x86_64
```

Note: Apple Silicon (arm64) kernel extensions require additional configuration and signing.

### Verbose Build

To see all compilation commands:

```bash
make release V=1
```

---

## Build Targets

The Makefile provides several targets:

### Primary Targets

- `make` or `make all` - Build release version (default)
- `make release` - Build optimized release version
- `make debug` - Build debug version with symbols
- `make clean` - Remove all build artifacts
- `make help` - Display help information

### Installation Targets (Requires sudo)

- `make install` - Install kext to `/Library/Extensions/`
- `make uninstall` - Remove kext from system
- `make load` - Load the kext into the kernel
- `make unload` - Unload the kext from the kernel
- `make test-load` - Test load kext without installing
- `make status` - Check if kext is currently loaded

---

## Troubleshooting

### Common Build Errors

#### Error: libkern/libkern.h not found

```
fatal error: 'libkern/libkern.h' file not found
```

**Solution:** Install MacKernelSDK (see [Prerequisites](#prerequisites)). The Makefile requires MacKernelSDK headers to be available at `/Library/Developer/SDKs/MacKernelSDK`.

#### Error: Lilu headers not found

```
fatal error: 'Headers/plugin_start.hpp' file not found
```

**Solution:** Install Lilu SDK headers (see [Prerequisites](#prerequisites))

#### Error: No SDK found

```
error: SDK "macosx" cannot be located
```

**Solution:** Install Xcode Command Line Tools:
```bash
xcode-select --install
```

#### Error: clang++ not found

**Solution:** Ensure Xcode Command Line Tools are properly installed:
```bash
xcode-select --install
xcode-select -p
```

### Build Verification

To verify your build is valid:

```bash
# Check if the binary exists and is a valid Mach-O
file build/XePCI.kext/Contents/MacOS/XePCI

# Should output something like:
# build/XePCI.kext/Contents/MacOS/XePCI: Mach-O 64-bit kext bundle x86_64

# Check bundle structure
ls -laR build/XePCI.kext/

# Verify Info.plist
plutil -lint build/XePCI.kext/Contents/Info.plist

# Check linked libraries
otool -L build/XePCI.kext/Contents/MacOS/XePCI
```

### Clean Build

If you encounter issues, try a clean build:

```bash
make clean
make release
```

---

## CI/CD

### GitHub Actions

The repository includes a GitHub Actions workflow that automatically builds the kext on every push and pull request.

**Workflow file:** `.github/workflows/build.yml`

The workflow:
1. Runs on macOS latest
2. Installs Lilu SDK headers
3. Builds both release and debug versions
4. Verifies the build output
5. Creates build artifacts (zip files)
6. Uploads artifacts for download

### Downloading CI Artifacts

1. Go to the **Actions** tab in the GitHub repository
2. Click on a workflow run
3. Scroll down to **Artifacts**
4. Download `XePCI-kext-[commit-sha].zip`

---

## Advanced Topics

### Customizing the Build

You can modify the `Makefile` to customize:
- Compiler flags (`CXXFLAGS`)
- Linker flags (`LDFLAGS`)
- SDK paths
- Bundle identifier
- Version number

### Manual Compilation

If you prefer to build without Make:

```bash
# Create directories
mkdir -p build/XePCI.kext/Contents/MacOS

# Compile
clang++ -std=c++17 -arch x86_64 \
  -fno-rtti -fno-exceptions -fno-builtin -fno-common \
  -mkernel -nostdlib -nostdinc++ \
  -D__KERNEL__ -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE \
  -I/usr/local/include \
  -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/Kernel.framework/Headers \
  -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include \
  -O2 -c kexts/XePCI_LiluPlugin.cpp -o build/XePCI.o

# Link
clang++ -arch x86_64 -Xlinker -kext \
  -nostdlib -lkmod -lkmodc++ -lcc_kext \
  -Xlinker -export_dynamic \
  -Xlinker -no_deduplicate \
  -o build/XePCI.kext/Contents/MacOS/XePCI build/XePCI.o

# Copy Info.plist
cp kexts/Info.plist build/XePCI.kext/Contents/
```

---

## Next Steps

After building, see [TESTING.md](TESTING.md) for instructions on:
- Installing the kext
- Testing on local Mac
- Debugging and troubleshooting
- Using the xectl command-line tool

---

## Additional Resources

- [Lilu Documentation](https://github.com/acidanthera/Lilu)
- [Apple Kernel Programming Guide](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/)
- [XePCI Project README](README.md)
- [POC Documentation](POC.md)
