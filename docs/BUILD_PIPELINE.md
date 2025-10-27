# Build Pipeline Documentation

This document explains the build pipeline infrastructure for the XePCI Lilu plugin.

## Overview

The XePCI project now includes a complete build pipeline that consists of:
1. **Makefile** - Local build system
2. **GitHub Actions** - Automated CI/CD
3. **Documentation** - BUILD.md, TESTING.md, QUICKSTART.md

## Build Pipeline Architecture

```
┌─────────────────┐
│  Source Code    │
│  kexts/*.cpp    │
└────────┬────────┘
         │
         ├──────────────────┐
         │                  │
    ┌────▼────┐      ┌──────▼─────┐
    │  Local  │      │  GitHub    │
    │  Build  │      │  Actions   │
    │ (Make)  │      │   (CI)     │
    └────┬────┘      └──────┬─────┘
         │                  │
         ├──────────────────┤
         │                  │
    ┌────▼──────────────────▼────┐
    │     XePCI.kext Bundle       │
    │  Contents/MacOS/XePCI       │
    │  Contents/Info.plist        │
    └─────────────────────────────┘
```

## Components

### 1. Makefile

**Location:** `Makefile`

**Purpose:** Local development and building

**Key Features:**
- Configurable SDK paths (LILU_SDK, KERNEL_SDK)
- Multiple build targets (release, debug, clean)
- Installation and management targets (install, uninstall, load, unload)
- Testing targets (test-load, status)
- Help documentation (make help)

**Build Process:**
```bash
make release
│
├─> Compile XePCI_LiluPlugin.cpp → XePCI.o
│   └─> Flags: -O2 -std=c++17 -mkernel -nostdlib
│       -I/usr/local/include/Lilu
│       -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/...
│
├─> Link XePCI.o → XePCI (binary)
│   └─> Flags: -Xlinker -kext -lkmod -lkmodc++ -lcc_kext
│
├─> Create bundle structure
│   ├─> build/XePCI.kext/Contents/MacOS/XePCI
│   └─> build/XePCI.kext/Contents/Info.plist
│
└─> Success
```

**Variables:**
- `LILU_SDK` - Path to Lilu headers (default: /usr/local/include/Lilu)
- `KERNEL_SDK` - Path to macOS SDK
- `ARCH` - Target architecture (default: x86_64)
- `BUILD_DIR` - Build output directory (default: build)

### 2. GitHub Actions Workflow

**Location:** `.github/workflows/build.yml`

**Purpose:** Automated CI/CD on every push and PR

**Trigger Events:**
- Push to `main`, `develop`, or `copilot/**` branches
- Pull requests to `main` or `develop`
- Manual workflow dispatch

**Jobs:**

#### Job 1: build (Release)
```yaml
Steps:
1. Checkout repository
2. Setup Xcode (latest-stable)
3. Install build dependencies (Xcode CLI tools)
4. Clone and install Lilu SDK
5. Verify build environment
6. Build release version
7. Verify build output
8. Package artifacts (.zip)
9. Upload artifacts (30 day retention)
10. Generate build summary
```

#### Job 2: build-debug (Debug)
```yaml
Steps:
1. Checkout repository
2. Setup Xcode (latest-stable)
3. Clone and install Lilu SDK
4. Build debug version
5. Verify debug build
6. Package debug artifacts (.zip)
7. Upload debug artifacts (30 day retention)
```

**Artifacts:**
- `XePCI-kext-[commit-sha].zip` - Release build
- `XePCI-kext-debug-[commit-sha].zip` - Debug build

**Retention:** 30 days

### 3. Documentation

#### BUILD.md (326 lines)
- Prerequisites and installation
- Quick start guide
- Detailed build instructions
- Build options and customization
- Troubleshooting common issues
- CI/CD explanation
- Advanced topics

#### TESTING.md (576 lines)
- Safety warnings and prerequisites
- System preparation (SIP, kext consent)
- Three installation methods
- Testing procedures (basic and advanced)
- xectl tool usage
- Debugging strategies
- Recovery procedures
- Testing checklist

#### QUICKSTART.md (130 lines)
- Quick reference card
- Common commands
- Prerequisites setup
- Building, installing, testing
- Debugging quick tips

## Build Configurations

### Release Build

**Command:** `make release`

**Compiler Flags:**
```
-O2                    # Optimization level 2
-std=c++17             # C++17 standard
-arch x86_64           # 64-bit architecture
-fno-rtti              # No run-time type info
-fno-exceptions        # No exceptions
-fno-builtin           # No builtin functions
-fno-common            # No common variables
-mkernel               # Kernel mode
-nostdlib              # No standard library
-nostdinc++            # No standard C++ includes
-D__KERNEL__           # Kernel build definition
-DKERNEL               # Kernel definition
-DKERNEL_PRIVATE       # Kernel private APIs
-DDRIVER_PRIVATE       # Driver private APIs
```

**Linker Flags:**
```
-Xlinker -kext         # Kernel extension linking
-nostdlib              # No standard library
-lkmod                 # Link kernel module support
-lkmodc++              # Link kernel C++ support
-lcc_kext              # Link kext support
-Xlinker -export_dynamic      # Export symbols
-Xlinker -no_deduplicate      # No deduplication
```

**Output:**
- Optimized binary (smaller, faster)
- No debug symbols
- Suitable for production use

### Debug Build

**Command:** `make debug`

**Additional Flags:**
```
-g                     # Generate debug symbols
-O0                    # No optimization
-DDEBUG                # Debug mode definition
```

**Output:**
- Unoptimized binary (larger, slower)
- Full debug symbols
- Suitable for debugging with lldb

## Dependencies

### Required

1. **Xcode Command Line Tools**
   - Provides: clang++, ld, SDK
   - Install: `xcode-select --install`

2. **Lilu SDK Headers**
   - Provides: plugin_start.hpp, kern_api.hpp
   - Install: Clone Lilu, copy headers to /usr/local/include/Lilu

### Optional

1. **macOS SDK**
   - Location: /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
   - Provides: Kernel.framework, system headers

## Build Artifacts

### Directory Structure

```
build/
└── XePCI.kext/                    # Main kext bundle
    └── Contents/
        ├── Info.plist             # Bundle metadata
        │   ├── CFBundleIdentifier: org.yourorg.XePCI
        │   ├── CFBundleVersion: 1.0.0
        │   ├── IOKitPersonalities
        │   └── OSBundleLibraries
        │
        └── MacOS/
            └── XePCI              # Kernel extension binary
                                   # Mach-O 64-bit kext bundle
```

### Build Products

After successful build:
- `build/XePCI.o` - Compiled object file (intermediate)
- `build/XePCI.kext/` - Complete kext bundle (final)

After `make clean`:
- All build/ directory contents removed

### Verification Commands

```bash
# Check bundle structure
ls -laR build/XePCI.kext/

# Verify binary
file build/XePCI.kext/Contents/MacOS/XePCI
# Output: Mach-O 64-bit kext bundle x86_64

# Check Info.plist
plutil -lint build/XePCI.kext/Contents/Info.plist
# Output: OK

# Inspect linked libraries
otool -L build/XePCI.kext/Contents/MacOS/XePCI
```

## CI/CD Workflow Details

### Automated Checks

1. **Build Verification:**
   - Binary exists at correct path
   - Info.plist is valid XML
   - Bundle structure is correct
   - File type is Mach-O kext bundle

2. **Environment Verification:**
   - Xcode version
   - SDK availability
   - Lilu headers installed

3. **Artifact Generation:**
   - Zip creation
   - Upload to GitHub
   - Download URL in workflow summary

### Workflow Outputs

**On Success:**
- Green checkmark on commit
- Build summary in workflow log
- Downloadable artifacts (30 days)

**On Failure:**
- Red X on commit
- Error messages in workflow log
- Email notification (if configured)

## Integration with Lilu

### Header Dependencies

From `kexts/XePCI_LiluPlugin.cpp`:
```cpp
#include <Headers/plugin_start.hpp>    // Lilu
#include <Headers/kern_api.hpp>        // Lilu
```

### Required Lilu Headers

- `plugin_start.hpp` - Plugin initialization macros
- `kern_api.hpp` - Kernel API wrappers (DBGLOG, etc.)
- `kern_util.hpp` - Utility functions

### Lilu Plugin Configuration

```cpp
PluginConfiguration ADDPR(config) = {
  "org.yourorg.XePCI",                          // Plugin ID
  parseModuleVersion("1.0.0"),                  // Version
  LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery,
  nullptr, 0,                                   // arrAddress
  nullptr, 0,                                   // patchInfos
  pluginStart,                                  // Start callback
  pluginStop                                    // Stop callback
};
```

## Testing the Build System

### Local Testing

```bash
# Test Makefile
make help              # Should display help
make clean             # Should succeed
make release           # Should build kext (requires Lilu SDK)

# Test build output
ls build/XePCI.kext/   # Should exist
file build/XePCI.kext/Contents/MacOS/XePCI  # Should be Mach-O

# Test without Lilu SDK (will fail gracefully)
make release LILU_SDK=/nonexistent
# Should show clear error about missing headers
```

### CI Testing

Push to a branch or create a PR:
```bash
git push origin feature-branch
```

Check GitHub Actions tab for:
- Build status (green/red)
- Build logs
- Artifacts availability

## Troubleshooting the Build System

### Issue: "Lilu headers not found"

**Solution:**
```bash
# Install Lilu SDK
git clone https://github.com/acidanthera/Lilu.git /tmp/Lilu
sudo mkdir -p /usr/local/include/Lilu
sudo cp -R /tmp/Lilu/Lilu/Headers /usr/local/include/Lilu/

# Or specify custom path
make release LILU_SDK=/path/to/lilu/headers
```

### Issue: "SDK not found"

**Solution:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Check SDK location
ls /Library/Developer/CommandLineTools/SDKs/
```

### Issue: "GitHub Actions fails on Lilu clone"

**Cause:** Network issues or rate limiting

**Solution:** Workflow includes retry logic and caching

### Issue: "Makefile doesn't find clang++"

**Solution:**
```bash
# Ensure Xcode CLI tools are installed
xcode-select --install

# Check compiler
which clang++
clang++ --version
```

## Extending the Build System

### Adding New Targets

Edit `Makefile`:
```makefile
my-target: dependency
	@echo "Running my target..."
	# Commands here
```

### Adding CI Checks

Edit `.github/workflows/build.yml`:
```yaml
- name: My Check
  run: |
    echo "Running my check..."
    # Commands here
```

### Customizing Build Flags

Edit `Makefile` variables:
```makefile
CXXFLAGS += -DMY_CUSTOM_FLAG
LDFLAGS += -lmycustomlib
```

## Maintenance

### Updating Lilu SDK Version

```bash
# In CI workflow
- name: Clone Lilu SDK
  run: |
    git clone --depth=1 --branch v1.6.7 https://github.com/acidanthera/Lilu.git /tmp/Lilu
```

### Updating macOS/Xcode Version

```yaml
# In workflow
- name: Setup Xcode
  uses: maxim-lobanov/setup-xcode@v1
  with:
    xcode-version: '15.0'
```

### Adding New Source Files

Edit `Makefile`:
```makefile
SOURCES = kexts/XePCI_LiluPlugin.cpp kexts/NewFile.cpp
```

## Best Practices

1. **Always run `make clean` before building for distribution**
2. **Test both release and debug builds**
3. **Verify CI passes before merging PRs**
4. **Keep Lilu SDK updated**
5. **Document any build system changes**
6. **Test on clean environment periodically**

## References

- [Apple Kernel Programming Guide](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/)
- [Lilu Documentation](https://github.com/acidanthera/Lilu)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [GNU Make Manual](https://www.gnu.org/software/make/manual/)
