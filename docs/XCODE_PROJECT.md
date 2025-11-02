# XePCI Xcode Project Conversion

This document describes the Xcode project structure created for the XePCI kernel extension.

## Overview

The XePCI project has been converted to use Xcode's native project format while maintaining backward compatibility with the existing Makefile build system. This provides developers with:

- **IDE Integration**: Full Xcode IDE support with code completion, navigation, and syntax highlighting
- **Debugging Support**: Native LLDB debugger integration for kernel debugging
- **Build Flexibility**: Both Xcode and Makefile builds supported
- **Industry Standard**: Aligns with common macOS kext development practices (following examples from Lilu, WhateverGreen, VoodooI2C)

## Project Structure

```
XePCI.xcodeproj/
├── project.pbxproj                 # Main project configuration
└── xcshareddata/
    └── xcschemes/
        └── XePCI.xcscheme         # Build scheme definition
```

### File Organization

The Xcode project organizes files into logical groups:

**Sources Group** (`kexts/`):
- `XeService.cpp` - Main IOService driver implementation
- `XeUserClient.cpp` - User-space communication interface
- `ForcewakeGuard.cpp` - Power management for register access
- `XeGGTT.cpp` - Graphics Translation Table management
- `XeCommandStream.cpp` - Command buffer and ring management

**Headers Group** (`kexts/`):
- `XeService.hpp` - Service class declarations
- `XeUserClient.hpp` - User client interface
- `ForcewakeGuard.hpp` - Forcewake management
- `XeGGTT.hpp` - GGTT interface
- `XeCommandStream.hpp` - Command stream interface
- `xe_hw_offsets.hpp` - Hardware register offsets

**Resources Group** (`kexts/`):
- `Info.plist` - Bundle metadata and IOKit personalities

## Build Configurations

### Debug Configuration
- Optimization: None (`-O0`)
- Debug symbols: Full DWARF
- Preprocessor defines: `DEBUG=1`, `__KERNEL__`, `KERNEL`, etc.
- Output: `DerivedData/XePCI/Build/Products/Debug/XePCI.kext`

### Release Configuration
- Optimization: `-O2`
- Debug symbols: DWARF with dSYM
- Preprocessor defines: `__KERNEL__`, `KERNEL`, etc.
- Output: `DerivedData/XePCI/Build/Products/Release/XePCI.kext`

## Build Settings

### Compiler Settings
- **Language Standard**: C++17
- **Kernel Flags**: `-mkernel`, `-fno-rtti`, `-fno-exceptions`, `-fno-builtin`, `-fno-common`, `-nostdlib`
- **Architecture**: x86_64
- **Deployment Target**: macOS 10.15

### SDK and Headers
- **Kernel SDK**: `/Library/Developer/SDKs/MacKernelSDK`
- **Header Search Paths**: 
  - `$(PROJECT_DIR)/kexts`
  - `/Library/Developer/SDKs/MacKernelSDK/Headers`

### Linker Settings
- **Libraries**: `-lkmod`, `-lkmodc++`, `-lcc_kext`
- **Product Type**: Kernel Extension (`.kext`)

## Comparison with Other Kexts

The XePCI Xcode project structure follows the same patterns as established macOS kext projects:

### Similar to Lilu
- Single target for kext
- Debug/Release configurations
- MacKernelSDK integration
- Shared scheme for CI/CD

### Similar to WhateverGreen
- Logical file grouping (Sources, Headers, Resources)
- Standard build phases (Headers, Sources, Resources, Frameworks)
- Kernel extension build settings

### Similar to VoodooI2C
- Standalone kext (not a Lilu plugin)
- IOKit integration
- User client support

## Building

### With Xcode IDE
```bash
open XePCI.xcodeproj
# Build with ⌘B (Release) or ⌘⇧R (Debug)
```

### With xcodebuild (Command Line)
```bash
# Release build
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Release

# Debug build
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Debug
```

### With Makefile (Legacy Support)
```bash
# Still fully supported
make release  # or make debug
```

## Version Control

The `.gitignore` file is configured to:
- ✅ Track `project.pbxproj` (project configuration)
- ✅ Track `xcshareddata/xcschemes/` (shared schemes)
- ❌ Ignore `xcuserdata/` (user-specific settings)
- ❌ Ignore `DerivedData/` (build output)

## Benefits

1. **IDE Integration**: Full code navigation, completion, and refactoring support
2. **Debugging**: Native LLDB integration for kernel debugging
3. **Build Flexibility**: Choose between Xcode or Makefile builds
4. **Team Development**: Shared schemes work across different machines
5. **Industry Standard**: Follows established macOS kext development practices
6. **CI/CD Compatible**: Both build systems work in automated environments

## Migration Notes

The Makefile build system remains fully functional and produces identical output. Developers can:
- Continue using `make` commands
- Switch to Xcode for IDE features
- Use both interchangeably

No changes to source code were required for this conversion - only build infrastructure was added.

## References

This conversion was based on studying the structure of:
- [Lilu](https://github.com/acidanthera/Lilu)
- [WhateverGreen](https://github.com/acidanthera/WhateverGreen)
- [VoodooI2C](https://github.com/VoodooI2C/VoodooI2C)
- [VoodooPS2](https://github.com/acidanthera/VoodooPS2)

## Verification

To verify the project structure:

```bash
# Check project exists
ls -la XePCI.xcodeproj/

# Verify scheme
ls -la XePCI.xcodeproj/xcshareddata/xcschemes/

# Count source files (should be 5)
grep -c "\.cpp in Sources" XePCI.xcodeproj/project.pbxproj | awk '{print $1/2}'

# Count header files (should be 6)
grep -c "\.hpp in Headers" XePCI.xcodeproj/project.pbxproj | awk '{print $1/2}'

# Test Makefile still works
make clean && make release
```

## Future Enhancements

Potential improvements for the future:
- Add unit test target
- Add documentation generation target
- Add code signing configuration
- Add distribution/packaging target
- Add CI/CD workflow for xcodebuild
