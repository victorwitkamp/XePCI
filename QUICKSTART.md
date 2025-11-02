# XePCI Quick Reference

Quick commands for building and testing the XePCI kernel extension.

## Prerequisites Setup

```bash
# Install Xcode (recommended) or Xcode Command Line Tools
xcode-select --install

# Install MacKernelSDK
git clone https://github.com/acidanthera/MacKernelSDK.git /tmp/MacKernelSDK
sudo mkdir -p /Library/Developer/SDKs
sudo cp -R /tmp/MacKernelSDK /Library/Developer/SDKs/
```

## Building

**Using Xcode:**
```bash
# Open project
open XePCI.xcodeproj

# Build from Xcode (Cmd+B)
# Or from command line:
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Release
```

**Using Makefile:**
```bash
# Release build (optimized)
make release

# Debug build (with symbols)
make debug

# Clean build artifacts
make clean
```

## Installation (Requires sudo and SIP disabled)

```bash
# Install to system
sudo make install

# Test load without installing
sudo make test-load

# Uninstall from system
sudo make uninstall
```

## Runtime Management

```bash
# Load the kext
sudo make load

# Unload the kext
sudo make unload

# Check if loaded
make status
```

## Testing with xectl

```bash
# Build the tool
cd userspace
clang xectl.c -framework IOKit -framework CoreFoundation -o xectl

# Get device info
sudo ./xectl info

# Read registers
sudo ./xectl regdump

# Get GT configuration
sudo ./xectl gtconfig

# Create buffer
sudo ./xectl mkbuf 4096

# Test NOOP command
sudo ./xectl noop
```

## System Preparation

```bash
# Check SIP status (should be disabled for testing)
csrutil status

# To disable SIP (from Recovery Mode):
# 1. Reboot holding Cmd+R
# 2. Open Terminal from Utilities
# 3. Run: csrutil disable
# 4. Reboot normally
```

## Debugging

```bash
# Stream kernel logs
sudo log stream --predicate 'process == "kernel"' --level debug | grep -i xepci

# Check loaded kexts
kextstat | grep -i xepci

# View IOKit registry
ioreg -l | grep -i XeService

# Check recent panic logs
ls -lt /Library/Logs/DiagnosticReports/*.panic | head -5
```

## Documentation

- **[BUILD.md](BUILD.md)** - Complete build instructions (Xcode and Makefile)
- **[TESTING.md](TESTING.md)** - Testing and debugging guide
- **[README.md](README.md)** - Project overview
- **[POC.md](POC.md)** - Proof of concept documentation
- **[RESEARCH.md](RESEARCH.md)** - Implementation research
- **[docs/XCODE_PROJECT.md](docs/XCODE_PROJECT.md)** - Xcode project structure

## Help

```bash
# Show all make targets
make help
```

## Safety Notes

⚠️ **Always test on non-production hardware**
⚠️ **Keep backups of important data**
⚠️ **Be prepared for kernel panics**
⚠️ **Have access to Recovery Mode**

For detailed instructions, see [BUILD.md](BUILD.md) and [TESTING.md](TESTING.md).
