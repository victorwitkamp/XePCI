# Building XePCI Kernel Extension

This document describes how to build the XePCI kernel extension (kext) for macOS.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Building with Xcode](#building-with-xcode)
- [Building with Makefile](#building-with-makefile)
- [Build Options](#build-options)
- [Troubleshooting](#troubleshooting)
- [CI/CD](#cicd)

---

## Prerequisites

### Required Software

1. **macOS** - Version 10.15 (Catalina) or later recommended
2. **Xcode** (optional) - For IDE-based development and debugging
   - Full Xcode installation from the Mac App Store (recommended), OR
   - Xcode Command Line Tools only (for command-line builds)
3. **MacKernelSDK** - Kernel headers for kernel extension development

### Installing Prerequisites

#### 1. Install Xcode or Command Line Tools

**Option A: Full Xcode (Recommended for development)**
1. Install Xcode from the Mac App Store
2. Open Xcode and accept the license agreement
3. Install additional components when prompted

**Option B: Command Line Tools Only (For command-line builds)**
```bash
xcode-select --install
```

Verify installation:
```bash
xcode-select -p
# Should output: /Applications/Xcode.app/Contents/Developer (for full Xcode)
# Or: /Library/Developer/CommandLineTools (for Command Line Tools only)
```

#### 2. Install MacKernelSDK

Clone and install the MacKernelSDK:

```bash
# Clone MacKernelSDK
git clone https://github.com/acidanthera/MacKernelSDK.git /tmp/MacKernelSDK

# Install to standard SDK location
sudo mkdir -p /Library/Developer/SDKs
sudo cp -R /tmp/MacKernelSDK /Library/Developer/SDKs/

# Clean up temporary directory
rm -rf /tmp/MacKernelSDK

# Verify installation
ls /Library/Developer/SDKs/MacKernelSDK/Headers/
```

**Note:** XePCI is a standalone kernel extension and does not require Lilu SDK.

---

## Quick Start

### Using Xcode (Recommended)

```bash
# Clone the repository (if not already done)
git clone https://github.com/victorwitkamp/XePCI.git
cd XePCI

# Open in Xcode
open XePCI.xcodeproj

# Build from Xcode:
# - Select the XePCI scheme
# - Choose Product > Build (Cmd+B) for release build
# - Or Product > Build For > Running (Cmd+Shift+R) for debug build

# The built kext will be in: DerivedData/XePCI/Build/Products/Release/XePCI.kext
```

### Using Command Line

For users who prefer command-line builds or don't have full Xcode installed:

```bash
# Clone the repository (if not already done)
git clone https://github.com/victorwitkamp/XePCI.git
cd XePCI

# Build release version with Makefile
make release

# Or build debug version with symbols
make debug

# The built kext will be in: build/XePCI.kext
```

---

## Building with Xcode

XePCI includes a native Xcode project for development with full IDE integration.

### Opening the Project

```bash
cd XePCI
open XePCI.xcodeproj
```

Or double-click `XePCI.xcodeproj` in Finder.

### Building in Xcode

1. **Select the Target and Scheme:**
   - Target: XePCI
   - Scheme: XePCI (should be selected by default)

2. **Choose Build Configuration:**
   - For **Debug** build: Product > Build For > Running (⌘⇧R)
   - For **Release** build: Product > Build (⌘B)

3. **Build Output Location:**
   - Debug: `DerivedData/XePCI/Build/Products/Debug/XePCI.kext`
   - Release: `DerivedData/XePCI/Build/Products/Release/XePCI.kext`

### Building from Command Line with xcodebuild

You can also build the Xcode project from the command line:

```bash
# Build release version
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Release

# Build debug version
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Debug

# Clean build
xcodebuild -project XePCI.xcodeproj -scheme XePCI clean

# Build and archive
xcodebuild -project XePCI.xcodeproj -scheme XePCI archive
```

### Xcode Project Structure

The Xcode project is organized into logical groups:

```
XePCI.xcodeproj/
├── project.pbxproj           # Project configuration
└── xcshareddata/
    └── xcschemes/
        └── XePCI.xcscheme    # Build scheme

Project Groups:
├── Sources/                   # C++ implementation files
│   ├── XeService.cpp
│   ├── XeUserClient.cpp
│   ├── ForcewakeGuard.cpp
│   ├── XeGGTT.cpp
│   └── XeCommandStream.cpp
├── Headers/                   # Header files
│   ├── XeService.hpp
│   ├── XeUserClient.hpp
│   ├── ForcewakeGuard.hpp
│   ├── XeGGTT.hpp
│   ├── XeCommandStream.hpp
│   └── xe_hw_offsets.hpp
└── Resources/                 # Resources
    └── Info.plist
```

### Xcode Build Settings

The Xcode project is configured with the following key settings:

**Compiler Flags:**
- C++ Standard: C++17
- Kernel flags: `-mkernel`, `-fno-rtti`, `-fno-exceptions`, `-fno-builtin`, `-fno-common`, `-nostdlib`
- Architecture: x86_64
- Deployment Target: macOS 10.15

**Linker Flags:**
- `-lkmod`, `-lkmodc++`, `-lcc_kext`

**SDK Paths:**
- Kernel SDK: `/Library/Developer/SDKs/MacKernelSDK`
- Header Search Paths: `$(PROJECT_DIR)/kexts`, MacKernelSDK Headers

**Preprocessor Macros:**
- `__KERNEL__`
- `KERNEL`
- `KERNEL_PRIVATE`
- `DRIVER_PRIVATE`
- `PRODUCT_NAME=XePCI`
- `MODULE_VERSION="1.0.0"`
- `DEBUG=1` (Debug builds only)

### Debugging with Xcode

Xcode provides excellent debugging support for kernel extensions:

1. **Set Breakpoints:** Click in the gutter next to any line of code
2. **Two-Machine Debugging:** Configure kernel debugging on a second Mac
3. **View Logs:** Use Console.app to view kernel logs
4. **LLDB Integration:** Full LLDB debugger support

See [TESTING.md](TESTING.md) for detailed debugging instructions.

---

## Building with Makefile

For users who prefer command-line builds or CI/CD environments, XePCI includes a traditional Makefile.

### Building Release Version

The release version is optimized for performance:

```bash
make release
```

This will:
1. Create the `build/` directory
2. Compile all `.cpp` files with optimization flags
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

### Makefile Options

#### Custom SDK Path

If you installed MacKernelSDK to a different location:

```bash
make release KERNEL_SDK=/path/to/MacKernelSDK
```

### Architecture Selection

By default, builds target x86_64. To specify architecture:

```bash
make release ARCH=x86_64
```

Note: Apple Silicon (arm64) kernel extensions require additional configuration and signing.

### Xcode Configuration

To customize Xcode build settings:

1. Open `XePCI.xcodeproj` in Xcode
2. Select the XePCI project in the navigator
3. Select the XePCI target
4. Go to Build Settings tab
5. Modify settings as needed:
   - Search for specific settings using the search box
   - Add custom preprocessor macros
   - Modify compiler or linker flags
   - Change SDK paths

### Makefile Targets

The Makefile provides several targets for various tasks:

#### Primary Targets

- `make` or `make all` - Build release version (default)
- `make release` - Build optimized release version
- `make debug` - Build debug version with symbols
- `make clean` - Remove all build artifacts
- `make help` - Display help information

#### Installation Targets (Requires sudo)

- `make install` - Install kext to `/Library/Extensions/`
- `make uninstall` - Remove kext from system
- `make load` - Load the kext into the kernel
- `make unload` - Unload the kext from the kernel
- `make test-load` - Test load kext without installing
- `make status` - Check if kext is currently loaded

---

## Troubleshooting

### Common Build Errors

#### Error: MacKernelSDK headers not found

```
fatal error: 'libkern/libkern.h' file not found
```

**Solution:** Install MacKernelSDK (see [Prerequisites](#prerequisites)). 

- For Xcode builds: Verify the `KERNEL_FRAMEWORK_HEADERS` build setting points to `/Library/Developer/SDKs/MacKernelSDK/Headers`
- For Makefile builds: Verify the `KERNEL_SDK` variable in the Makefile or set it via `make KERNEL_SDK=/path/to/sdk`

#### Error: Xcode project won't open

**Solution:** Ensure Xcode is properly installed:
```bash
# Check Xcode installation
xcode-select -p

# If needed, select the Xcode installation
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

# Accept Xcode license if prompted
sudo xcodebuild -license accept
```

#### Error: SDK "macosx" cannot be located

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
