# Testing XePCI on Local Mac

This guide provides complete instructions for testing the XePCI Lilu plugin on a local macOS system.

## Table of Contents
- [Prerequisites](#prerequisites)
- [System Preparation](#system-preparation)
- [Installation](#installation)
- [Testing Procedures](#testing-procedures)
- [Using xectl Tool](#using-xectl-tool)
- [Debugging](#debugging)
- [Uninstallation](#uninstallation)
- [Safety and Recovery](#safety-and-recovery)

---

## ⚠️ Important Safety Warnings

**READ BEFORE PROCEEDING:**

1. **System Integrity Protection (SIP) must be disabled** - This allows loading unsigned/development kexts
2. **Hardware Risk** - Kernel extensions have direct hardware access. Test only on:
   - Development/test machines
   - Systems with backup GPU/display options
   - Hardware you can afford to potentially damage
3. **Data Backup** - Always backup important data before testing kernel extensions
4. **Kernel Panics** - Be prepared for system crashes during testing
5. **Target Hardware** - This kext is specifically for Intel Raptor Lake iGPUs (Device ID: 0xA788)

---

## Prerequisites

### Hardware Requirements

**Supported Hardware:**
- Intel Raptor Lake HX processors with integrated GPU
- Specific Device ID: 0xA788 (decimal 42888)
- Example: ASUS GI814JI laptop

**To check your GPU:**
```bash
# Check PCI devices
system_profiler SPPCIDataType | grep -A 10 "Graphics/Displays"

# Or check specific vendor/device
ioreg -l | grep -i "vendor-id.*8086"
ioreg -l | grep -i "device-id.*a788"
```

### Software Requirements

1. **macOS 10.15 (Catalina) or later**
2. **Built XePCI.kext** - See [BUILD.md](BUILD.md)
3. **Terminal access with administrative privileges**

---

## System Preparation

### 1. Disable System Integrity Protection (SIP)

SIP must be disabled to load unsigned kernel extensions.

**Steps:**

1. **Reboot into Recovery Mode:**
   - Intel Mac: Restart and hold `Cmd+R` during boot
   - Apple Silicon: Shutdown, then hold power button until "Loading startup options" appears

2. **Open Terminal in Recovery Mode:**
   - From Utilities menu, select Terminal

3. **Disable SIP:**
   ```bash
   csrutil disable
   ```

4. **Reboot normally:**
   ```bash
   reboot
   ```

5. **Verify SIP is disabled:**
   ```bash
   csrutil status
   # Should show: System Integrity Protection status: disabled.
   ```

**Security Note:** Re-enable SIP after testing by running `csrutil enable` in Recovery Mode.

### 2. Allow Kernel Extension Loading

For macOS 10.13+ (High Sierra and later):

```bash
# Check current setting
sudo spctl kext-consent status

# If needed, allow kext loading
sudo spctl kext-consent add org.yourorg.XePCI
```

### 3. Check Current Kernel Extensions

```bash
# List loaded kexts
kextstat | grep -i intel

# Check for conflicting drivers
kextstat | grep -i gpu
```

---

## Installation

### Method 1: Using Makefile (Recommended)

From the XePCI repository directory:

```bash
# Build if not already done
make release

# Install (requires sudo)
sudo make install
```

This will:
1. Copy the kext to `/Library/Extensions/`
2. Set correct permissions
3. Rebuild kernel cache
4. Prepare the kext for loading

**Note:** A reboot is recommended after installation for the kernel cache to update.

### Method 2: Manual Installation

```bash
# Copy kext to system
sudo cp -R build/XePCI.kext /Library/Extensions/

# Set ownership and permissions
sudo chown -R root:wheel /Library/Extensions/XePCI.kext
sudo chmod -R 755 /Library/Extensions/XePCI.kext

# Rebuild kernel cache
sudo kmutil install --volume-root / --update-all
# Or for older macOS:
sudo kextcache -i /

# Reboot
sudo reboot
```

### Method 3: Test Load Without Installing

For testing without system installation:

```bash
# Build the kext
make release

# Test load (requires sudo)
sudo make test-load

# Or manually:
sudo kextutil build/XePCI.kext
```

---

## Testing Procedures

### Basic Tests

#### 1. Check Kext Loading

After reboot (if using installation methods 1 or 2):

```bash
# Check if kext is loaded
kextstat | grep -i xepci

# Should show something like:
# 147  0  0xffffff7f8a000000  0x5000  0x5000  org.yourorg.XePCI (1.0.0)
```

#### 2. Check System Logs

Monitor kernel logs for XePCI messages:

```bash
# Stream kernel logs
sudo log stream --predicate 'process == "kernel"' --level debug | grep -i xepci

# Or check system log
sudo dmesg | grep -i xepci

# Check for specific messages
sudo log show --predicate 'eventMessage contains "XePCI"' --last 5m
```

Expected log messages:
- `XePCI: init`
- `XeLilu: BAR0 @ [address]`
- `XeLilu: reg[0x0]=[value]`
- `XeLilu: pluginStart`

#### 3. Verify IOKit Registration

```bash
# Check if service is registered
ioreg -l | grep -i XeService

# Detailed service info
ioreg -l -p IOService -n XeService
```

### Advanced Tests

#### 1. Device Information

Check PCI device attachment:

```bash
# Find the Intel GPU
ioreg -l | grep -B 10 -A 10 "vendor-id.*8086"

# Check if XeService attached
ioreg -l -p IOService | grep -A 20 XeService
```

#### 2. Memory Mapping

Verify BAR0 mapping:

```bash
# Check memory regions
sudo ioreg -l | grep -i "IODeviceMemory"

# Inspect PCI configuration
sudo kextutil -n -t build/XePCI.kext
```

---

## Using xectl Tool

The `xectl` command-line tool provides user-space access to the kernel extension.

### Building xectl

```bash
cd userspace
clang xectl.c -framework IOKit -framework CoreFoundation -o xectl
```

### xectl Commands

#### Get Device Info

```bash
sudo ./xectl info
```

Expected output:
```
Connected to XeService; handle=[number]
Vendor ID: 0x8086
Device ID: 0xa788
Revision:  0x04
Accel Ready: No (prepared)
```

#### Read Registers

```bash
sudo ./xectl regdump
```

Shows current register values.

#### Get GT Configuration

```bash
sudo ./xectl gtconfig
```

Shows GPU configuration (EUs, DSS, etc.)

#### Test Command Submission

```bash
sudo ./xectl noop
```

Submits a MI_NOOP command (currently a stub).

#### Create Buffer Object

```bash
sudo ./xectl mkbuf 4096
```

Creates a 4KB GPU buffer object.

### xectl Output Analysis

**Success Indicators:**
- `Connected to XeService` - Service is accessible
- Vendor ID `0x8086` - Intel vendor
- Device ID matches your hardware - Correct device
- No error messages

**Failure Indicators:**
- `Service not found` - Kext not loaded or not registered
- `Open failed` - Permission issue or service not available
- Kernel panic - Critical error in kext

---

## Debugging

### Enable Debug Output

If you built the debug version:

```bash
sudo make clean
sudo make debug
sudo make install
```

### Kernel Debugging

#### Method 1: Console Logs

```bash
# Real-time kernel log monitoring
sudo log stream --predicate 'process == "kernel"' --level debug

# Filter for XePCI
sudo log stream --predicate 'process == "kernel"' --level debug | grep XePCI
```

#### Method 2: System Log

```bash
# Check system.log
tail -f /var/log/system.log | grep -i xepci

# Or use Console.app
open /Applications/Utilities/Console.app
```

#### Method 3: dmesg

```bash
# Show kernel ring buffer
sudo dmesg | tail -100

# Filter for XePCI
sudo dmesg | grep -i xepci
```

### Common Issues

#### Kext Won't Load

**Symptom:** `kextstat` doesn't show XePCI

**Solutions:**
1. Check SIP is disabled: `csrutil status`
2. Verify kext is installed: `ls -la /Library/Extensions/XePCI.kext`
3. Check permissions: `ls -la /Library/Extensions/XePCI.kext/Contents/MacOS/XePCI`
4. Review logs: `sudo log show --predicate 'eventMessage contains "kext"' --last 5m`
5. Try manual load: `sudo kextload /Library/Extensions/XePCI.kext`

#### Kernel Panic on Load

**Symptom:** System crashes when loading kext

**Solutions:**
1. Boot in Safe Mode (hold Shift during boot)
2. Remove kext: `sudo rm -rf /Library/Extensions/XePCI.kext`
3. Rebuild cache: `sudo kextcache -i /`
4. Review panic log: `ls /Library/Logs/DiagnosticReports/`
5. Check for conflicting drivers
6. Try debug version for more information

#### XeService Not Found

**Symptom:** `xectl` reports "Service not found"

**Solutions:**
1. Verify kext is loaded: `kextstat | grep XePCI`
2. Check IOKit registry: `ioreg -l | grep XeService`
3. Verify device matching: Check if GPU device ID matches Info.plist
4. Review kernel logs for attachment failures

#### Permission Denied

**Symptom:** `xectl` fails with permission errors

**Solutions:**
1. Run with sudo: `sudo ./xectl info`
2. Check file permissions: `ls -la xectl`
3. Verify kext is loaded and registered

### Crash Report Analysis

After a kernel panic:

```bash
# Find recent panic logs
ls -lt /Library/Logs/DiagnosticReports/*.panic | head -5

# View the most recent panic
cat /Library/Logs/DiagnosticReports/[most-recent].panic

# Look for XePCI in the backtrace
grep -A 20 "XePCI" /Library/Logs/DiagnosticReports/[panic-file]
```

---

## Uninstallation

### Using Makefile

```bash
sudo make uninstall
```

### Manual Uninstallation

```bash
# Unload if loaded
sudo kextunload /Library/Extensions/XePCI.kext

# Remove from system
sudo rm -rf /Library/Extensions/XePCI.kext

# Rebuild kernel cache
sudo kmutil install --volume-root / --update-all
# Or for older macOS:
sudo kextcache -i /

# Reboot
sudo reboot
```

---

## Safety and Recovery

### Recovery from Kernel Panic Loop

If the system crashes on every boot:

1. **Boot in Safe Mode:**
   - Hold `Shift` during boot
   - This prevents third-party kexts from loading

2. **Remove the kext:**
   ```bash
   sudo rm -rf /Library/Extensions/XePCI.kext
   sudo kextcache -i /
   ```

3. **Reboot normally**

### Recovery from SIP Issues

If you need to re-enable SIP:

1. Boot into Recovery Mode (`Cmd+R`)
2. Open Terminal
3. Run: `csrutil enable`
4. Reboot

### System Snapshots (APFS)

macOS creates automatic snapshots. If needed:

```bash
# List snapshots
tmutil listlocalsnapshots /

# Revert to snapshot (from Recovery Mode)
# Utilities > Restore from Time Machine Backup
```

---

## Best Practices

1. **Incremental Testing:**
   - Test basic loading before complex operations
   - Use `test-load` before permanent installation
   - Monitor logs continuously during tests

2. **Version Control:**
   - Keep track of which kext version is installed
   - Note any code changes between tests
   - Document what works and what doesn't

3. **Hardware Monitoring:**
   - Watch GPU temperature: `sudo powermetrics --samplers smc`
   - Monitor system resources
   - Be ready to force-shutdown if needed

4. **Documentation:**
   - Log all test results
   - Save crash reports
   - Note any unusual behavior

---

## Testing Checklist

Use this checklist for systematic testing:

- [ ] SIP disabled and verified
- [ ] Kext built successfully
- [ ] Test load successful (no crash)
- [ ] Kext installed to /Library/Extensions
- [ ] System rebooted
- [ ] Kext shows in kextstat
- [ ] Kernel logs show init messages
- [ ] IOKit service registered
- [ ] xectl tool built
- [ ] xectl can connect to service
- [ ] Device info retrieved successfully
- [ ] Register reads working
- [ ] No kernel panics during basic operations
- [ ] System stable after testing
- [ ] Logs reviewed for errors

---

## Next Steps

After successful basic testing:

1. Review [POC.md](POC.md) for advanced GPU operations
2. Experiment with register access
3. Test buffer object creation
4. Attempt command submission
5. Contribute findings back to the project

---

## Support

If you encounter issues:

1. Check kernel logs carefully
2. Review [BUILD.md](BUILD.md) for build issues
3. Consult [RESEARCH.md](RESEARCH.md) for implementation details
4. Open an issue on GitHub with:
   - Hardware details
   - macOS version
   - Kernel logs
   - Steps to reproduce

---

## Additional Resources

- [Apple Kernel Programming Guide](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/)
- [IOKit Fundamentals](https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/)
- [Lilu Documentation](https://github.com/acidanthera/Lilu)
- [Intel Xe Driver (Linux)](https://www.kernel.org/doc/html/latest/gpu/xe/)
