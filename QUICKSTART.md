# XePCI Quick Reference

Quick commands for building and testing the XePCI Lilu plugin.

## Prerequisites Setup

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Lilu SDK headers
git clone https://github.com/acidanthera/Lilu.git /tmp/Lilu
sudo mkdir -p /usr/local/include/Lilu
sudo cp -R /tmp/Lilu/Lilu/Headers /usr/local/include/Lilu/
```

## Building

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

- **[BUILD.md](BUILD.md)** - Complete build instructions
- **[TESTING.md](TESTING.md)** - Testing and debugging guide
- **[README.md](README.md)** - Project overview
- **[POC.md](POC.md)** - Proof of concept documentation
- **[RESEARCH.md](RESEARCH.md)** - Implementation research

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
