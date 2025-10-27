# XePCI - Proof of Concept Implementation

## Overview

This PoC implements the foundational elements for Intel Xe GPU driver development on macOS, based on the research documented in [RESEARCH.md](RESEARCH.md).

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

5. **GGTT Initialization** (RESEARCH.md Section 4.4)
   - Map GGTT aperture
   - Allocate scratch pages
   - Insert test entries

6. **Ring Buffer Setup** (RESEARCH.md Section 4.5)
   - Allocate ring buffer memory
   - Program ring registers
   - Initialize head/tail pointers

7. **Command Submission** (RESEARCH.md Section 4.6)
   - Submit MI_NOOP commands
   - Implement seqno tracking
   - Wait for completion

8. **GuC Firmware Loading** (RESEARCH.md Section 4.8)
   - Load firmware from filesystem
   - Initialize GuC
   - Verify initialization

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

### XePCI.cpp

**Initialization Flow:**
1. PCI device enumeration and vendor check
2. BAR0 mapping (MMIO region)
3. Device identification
4. Forcewake test (PoC)
5. GT configuration readout
6. Service registration

**Safety Features:**
- Null pointer checks before all register access
- Timeout-based polling (1000ms default)
- Graceful degradation if forcewake fails
- Proper cleanup in stop()

## Usage and Expected Output

When the kext loads, you should see output like:

```
XePCI: init
XePCI: probe vendor=0x8086 device=0x4680
XePCI: start
XePCI: BAR0 mapped at 0x[address], size=[size] bytes
XePCI: Device identified: Intel Raptor Lake (0x4680), Revision: 0x[rev]
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
XePCI: PoC completed successfully
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
- **Raptor Lake** (Gen 12.7): 0x4600-0x4693
- **Alder Lake** (Gen 12): 0x46A0-0x46B3, 0x4626-0x462A
- **Tiger Lake** (Gen 12): 0x9A40-0x9A78

### Register Access Pattern

```cpp
// From RESEARCH.md Section 5.1
inline UInt32 readReg(UInt32 offset) {
    if (!bar0Ptr) return 0xFFFFFFFF;
    return bar0Ptr[offset / 4];  // Divide by 4 for 32-bit indexing
}
```

All register offsets are byte-aligned but accessed as 32-bit words.

## Safety Considerations

⚠️ **Important Safety Notes:**

1. **Read-Only Operations**: This PoC primarily reads registers and only writes to forcewake control registers, which are safe to access.

2. **Forcewake Degradation**: If forcewake acquisition fails, the driver continues but logs a warning. Some registers may return incorrect values without forcewake.

3. **No Hardware Initialization**: This PoC does NOT:
   - Initialize the GPU
   - Load firmware
   - Configure display outputs
   - Modify GPU state (beyond forcewake)

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

- **v0.1-poc** (2025-10-27): Initial PoC with register access and forcewake
