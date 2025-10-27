# XePCI - Proof of Concept Implementation

## Overview

This PoC implements the foundational elements for Intel Xe GPU driver development on macOS, based on the research documented in [RESEARCH.md](RESEARCH.md).

**Target Hardware**: 
- **Device**: ASUS GI814JI with Intel Raptor Lake HX
- **GPU Device ID**: 0xA788
- **Revision**: B-0 (0x04)
- **Configuration**: HX 8P+16E CPU with 32EU iGPU
- **Architecture**: Gen 12.2 (Xe-LP)

## Implementation Status

### ✅ Completed (This PoC)

1. **Register Access Framework** (RESEARCH.md Section 5.1)
   - Inline register read/write helpers
   - Safe register access with null pointer checks
   - Timeout-based register polling mechanism

2. **Device Identification** (RESEARCH.md Section 5.5)
   - PCI device ID and revision readout
   - Device name lookup for Raptor Lake, Alder Lake, and Tiger Lake
   - Vendor ID validation (Intel = 0x8086)

3. **Forcewake Management** (RESEARCH.md Section 4.3)
   - Basic forcewake acquisition for GT domain
   - Acknowledgment polling with timeout
   - Safe forcewake release on shutdown

4. **GT Configuration Readout** (RESEARCH.md Section 4.2)
   - Thread status register reading
   - DSS (Dual Subslice) enable register reading
   - Basic EU (Execution Unit) counting

### 🔄 Planned Next Steps

5. **GGTT Initialization** (RESEARCH.md Section 4.4) - ✅ Framework prepared
   - GGTT structure defined
   - Initialization stub implemented
   - GTT space allocation framework ready

6. **Ring Buffer Setup** (RESEARCH.md Section 4.5) - ✅ Framework prepared
   - Ring buffer structure implemented
   - Memory allocation working
   - Command writing framework ready
   - Ring register programming prepared

7. **Command Submission** (RESEARCH.md Section 4.6) - ✅ Framework prepared
   - MI_NOOP command framework implemented
   - Sequence number tracking added
   - Wait mechanism prepared

8. **GuC Firmware Loading** (RESEARCH.md Section 4.8) - ✅ Framework prepared
   - GuC preparation stub implemented
   - Status register reading added
   - Firmware loading framework ready

9. **Buffer Object Management** (RESEARCH.md Section 4.9) - ✅ Implemented
   - BO creation and destruction working
   - Pinning framework prepared
   - Memory tracking in place

10. **XeService Enhancement** - ✅ Completed
    - Buffer tracking implemented
    - Device info retrieval added
    - GT configuration readout added
    - Acceleration readiness check added

## Code Structure

### XePCI.hpp

**Register Definitions:**
```cpp
// Gen12/Xe Register Offsets (from Linux xe driver)
#define GEN12_GT_THREAD_STATUS          0x13800
#define GEN12_GT_GEOMETRY_DSS_ENABLE    0x913C
#define GEN12_FORCEWAKE_GT              0x13810
#define GEN12_FORCEWAKE_ACK_GT          0x13D84
```

**Key Methods:**
- `readReg(offset)` - Safe register read
- `writeReg(offset, value)` - Safe register write
- `waitForRegisterBit()` - Poll register with timeout
- `acquireForcewake()` - Request forcewake domain
- `releaseForcewake()` - Release forcewake domain
- `identifyDevice()` - Read and display device info
- `readGTConfiguration()` - Display GT configuration
- `initGGTT()` - Initialize Global Graphics Translation Table (preparation)
- `initRingBuffer()` - Initialize command ring buffer
- `submitMINoop()` - Submit MI_NOOP command (preparation)
- `createBufferObject()` - Allocate GPU buffer object
- `checkAccelerationReadiness()` - Check if acceleration framework is ready

### XePCI.cpp

**Initialization Flow:**
1. PCI device enumeration and vendor check
2. BAR0 mapping (MMIO region)
3. Device identification
4. Forcewake test (PoC)
5. GT configuration readout
6. Power well enablement (preparation)
7. GGTT initialization (preparation)
8. Ring buffer setup (memory allocation)
9. Interrupt framework preparation
10. GuC firmware preparation
11. Acceleration readiness check
12. Service registration

**Safety Features:**
- Null pointer checks before all register access
- Timeout-based polling (1000ms default)
- Graceful degradation if forcewake fails
- Proper cleanup in stop()

## Usage and Expected Output

When the kext loads on the target device (0xA788 - Raptor Lake HX), you should see output like:

```
XePCI: init
XePCI: probe vendor=0x8086 device=0xa788
XePCI: start
XePCI: BAR0 mapped at 0x[address], size=[size] bytes
XePCI: Device identified: Intel Raptor Lake HX (32EU) (0xa788), Revision: 0x04
XePCI: *** TARGET DEVICE DETECTED ***
XePCI: Raptor Lake HX 8P+16E with 32EU configuration
XePCI: Revision B-0 (expected) confirmed
XePCI: === Starting PoC - Forcewake Test ===
XePCI: Acquiring forcewake for domains 0x1
XePCI: GT forcewake acknowledged
XePCI: Forcewake acquired successfully
XePCI: === Reading GT Configuration ===
XePCI: GT_THREAD_STATUS (0x13800) = 0x[value]
XePCI: GT_GEOMETRY_DSS_ENABLE (0x913c) = 0x[value]
XePCI: Estimated enabled DSS units: [count]
XePCI: Releasing forcewake for domains 0x1
XePCI: Forcewake released
XePCI: === Legacy Register Dump ===
XePCI: reg[0x0000]=0x[value]
XePCI: reg[0x0100]=0x[value]
XePCI: reg[0x1000]=0x[value]
XePCI: scratch buffer allocated (4KB)
XePCI: === Initializing Acceleration Support ===
XePCI: Power wells enabled
XePCI: GGTT init (preparation stub)
XePCI: GGC register = 0x[value]
XePCI: WARNING - GGTT initialization skipped (preparation only)
XePCI: Initializing ring buffer (size=4096)
XePCI: Ring buffer allocated at 0x[address]
XePCI: WARNING - Ring buffer initialization skipped (preparation only)
XePCI: Interrupt setup (preparation stub)
XePCI: WARNING - Interrupt setup skipped (preparation only)
XePCI: GuC firmware preparation (stub)
XePCI: GuC status = 0x[value]
XePCI: WARNING - GuC preparation skipped (preparation only)
XePCI: Checking acceleration readiness
XePCI: Ring buffer not initialized (preparation mode)
XePCI: Basic acceleration framework is ready
XePCI: Acceleration framework prepared but not fully active
XePCI: PoC completed successfully with acceleration preparation
```

## Technical Details

### Forcewake Mechanism

Forcewake prevents the GPU from power-gating specific domains while they're being accessed:

1. **Request**: Write `0x00010001` to `GEN12_FORCEWAKE_GT`
   - Bit 0: Request forcewake
   - Bit 16: Shadow bit (required for acknowledgment)

2. **Acknowledge**: Poll `GEN12_FORCEWAKE_ACK_GT` bit 0 until set

3. **Release**: Write `0x00010000` to `GEN12_FORCEWAKE_GT`

### Device ID Recognition

Supports the following Intel GPU families:
- **Raptor Lake HX** (Gen 12.2) - **PRIMARY TARGET**: 0xA788 (32EU, HX 8P+16E)
- **Raptor Lake** (Gen 12.7): 0x4600-0x4693
- **Alder Lake** (Gen 12): 0x46A0-0x46B3, 0x4626-0x462A
- **Tiger Lake** (Gen 12): 0x9A40-0x9A78

The driver specifically targets **device 0xA788** (Raptor Lake HX on ASUS GI814JI) with revision B-0 (0x04).

### Register Access Pattern

```cpp
// From RESEARCH.md Section 5.1
inline UInt32 readReg(UInt32 offset) {
    if (!bar0Ptr) return 0xFFFFFFFF;
    return bar0Ptr[offset / 4];  // Divide by 4 for 32-bit indexing
}
```

All register offsets are byte-aligned but accessed as 32-bit words.

### Acceleration Framework

The acceleration framework provides preparation for future GPU compute and rendering support:

**Components:**

1. **GGTT (Global Graphics Translation Table)**
   - Manages GPU virtual address space
   - Maps system memory to GPU-accessible addresses
   - Currently in preparation mode (reads GGC register)

2. **Ring Buffer Management**
   - 4KB ring buffer allocated for command submission
   - Supports writing MI commands to ring
   - Head/tail pointer tracking implemented
   - Hardware programming prepared but not active

3. **Command Submission**
   - MI_NOOP command framework implemented
   - Sequence number tracking for completion
   - Batch buffer preparation ready

4. **Buffer Objects (BO)**
   - Memory descriptor allocation working
   - GTT mapping framework prepared
   - Pin/unpin support ready

5. **Power Management**
   - Power well control prepared
   - RC state monitoring available
   - Integration with forcewake

6. **Interrupt Framework**
   - Interrupt register definitions added
   - Handler preparation stubbed
   - Ready for future implementation

7. **GuC Firmware Support**
   - GuC status register reading
   - WOPCM allocation framework
   - Firmware loading preparation

**XeService Integration:**

The XeService component provides user-space access:
- Buffer creation and tracking (up to 16 buffers)
- Device information queries
- GT configuration readout
- Register dumps
- Acceleration readiness status

**Command-line Tool (xectl):**

```bash
sudo ./xectl info       # Display device info and acceleration status
sudo ./xectl regdump    # Dump register values
sudo ./xectl gtconfig   # Show GT configuration
sudo ./xectl mkbuf 4096 # Create a 4KB buffer
sudo ./xectl noop       # Submit MI_NOOP (prepared)
```

## Safety Considerations

⚠️ **Important Safety Notes:**

1. **Read-Mostly Operations**: This implementation primarily reads registers and allocates memory. Hardware initialization is prepared but not active.

2. **Forcewake Degradation**: If forcewake acquisition fails, the driver continues but logs a warning. Some registers may return incorrect values without forcewake.

3. **Preparation Mode**: The acceleration framework is prepared but NOT active:
   - GGTT reads configuration but doesn't initialize
   - Ring buffers allocate memory but don't program hardware
   - Commands are written to memory but not submitted to GPU
   - Interrupts are not enabled
   - GuC firmware is not loaded

4. **Testing Hardware**: Always test on non-critical hardware with a fallback display option.

## References to RESEARCH.md

This implementation directly uses patterns from:
- **Section 4.2**: Register access abstraction (Linux xe driver)
- **Section 4.3**: Forcewake and power management
- **Section 5.1**: XePCI implementation roadmap
- **Section 5.2**: Critical register offsets (Gen12/Xe)
- **Section 5.4**: Error handling patterns
- **Section 5.5**: Reference device IDs

## Next Development Phase

To continue development based on this PoC:

1. **Verify PoC Output**: Check system logs for successful initialization
2. **Implement GGTT**: Use patterns from RESEARCH.md Section 4.4
3. **Create Ring Buffer**: Follow RESEARCH.md Section 4.5
4. **Submit Test Commands**: Use MI_NOOP pattern from Section 4.7
5. **Add IOUserClient**: Enable user-space interaction

## Build Instructions

(To be added based on project build system - Xcode or kmutil)

## Testing Checklist

- [ ] Kext loads without panic
- [ ] Device is identified correctly
- [ ] BAR0 is mapped successfully
- [ ] Forcewake is acquired (or gracefully degrades)
- [ ] GT configuration is readable
- [ ] Register values are non-zero and reasonable
- [ ] Kext unloads cleanly
- [ ] No system instability after unload

## Known Limitations

1. **Platform Support**: Only tested concepts, not validated on actual hardware
2. **Forcewake Domains**: Only implements GT domain, not RENDER or MEDIA
3. **Error Recovery**: Limited error recovery mechanisms
4. **Diagnostic Output**: Relies on IOLog, needs structured logging
5. **Configuration**: No runtime configuration options yet

## Version History

- **v0.2-accel-prep** (2025-10-27): Acceleration framework preparation
  - Added GGTT initialization framework
  - Added ring buffer management
  - Added command submission preparation
  - Added buffer object management
  - Enhanced XeService with device info and GT config
  - Added power management and interrupt preparation
  - Added GuC firmware loading framework
- **v0.1-poc** (2025-10-27): Initial PoC with register access and forcewake
