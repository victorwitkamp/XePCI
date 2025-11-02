# Xe macOS iGPU Driver — Technical Overview

## Quick Start

👉 **New to the project?** See [QUICKSTART.md](QUICKSTART.md) for quick commands  
📖 **Building the kext?** See [BUILD.md](BUILD.md) for detailed instructions  
🧪 **Testing on Mac?** See [TESTING.md](TESTING.md) for complete testing guide  

---

## Current Status

🎉 **Proof of Concept (PoC) + Acceleration Framework Implemented** - See [POC.md](POC.md) for details.

The implementation demonstrates:
- ✅ PCI device enumeration and BAR0 mapping
- ✅ Device identification (Raptor Lake, Alder Lake, Tiger Lake)
- ✅ Register access framework with safety checks
- ✅ Forcewake management for safe register access
- ✅ GT configuration readout (thread status, DSS enable)
- ✅ GGTT initialization framework (preparation)
- ✅ Ring buffer allocation and management (preparation)
- ✅ Command submission framework with MI_NOOP (preparation)
- ✅ Buffer object creation and tracking
- ✅ XeService user-space interface with device info
- ✅ Power management and interrupt preparation
- ✅ GuC firmware loading preparation

**Current Phase**: Acceleration framework prepared, ready for hardware activation

**Next Steps**: Hardware initialization, firmware loading, active command submission

---

## Goal
Implement a minimal Intel Xe (Gen12, Tiger / Alder / Raptor Lake) iGPU driver for macOS to achieve:
1. PCI enumeration and MMIO access ✅ (PoC complete)
2. Firmware load (GuC/HuC) and GT power-up
3. Ring-buffer command submission (MI_NOOP → BLT → compute)
4. Optional framebuffer or IOAccelerator interface

**Target Device**: Intel Raptor Lake HX (ASUS GI814JI)
- **Device ID**: 0xA788
- **Revision**: B-0 (0x04)
- **Configuration**: HX 8P+16E with 32EU (Execution Units)
- **Architecture**: Gen 12.2 (Xe-LP)

---

## Components

### 1. **XePCI.kext**
**Purpose:** PCI attach, BAR mapping, register access.

**Functions**
- Match IGPU PCI vendor `0x8086` / device ID.
- Enable memory, I/O, bus mastering.
- Map BAR0 → MMIO region.
- Implement safe register read/dump.
- Provide interfaces for later ring management and user client.
- Expose `registerService()` for user-space connection.

---

### 2. **Firmware / GT Init**
**Purpose:** Initialize GPU core.

**Tasks**
- Load GuC / HuC firmware from user-supplied blobs.
- Power-well enable, forcewake domains.
- Program GGTT base and scratch registers.
- Initialize one render/compute ring (head/tail, context, doorbell).
- Install interrupt handler for seqno / fault / reset.

---

### 3. **Buffer Objects (BO) & GTT**
**Purpose:** Manage GPU-visible memory.

**Tasks**
- Allocate pinned pages (`IOBufferMemoryDescriptor`).
- Map into GGTT with cache attributes.
- Maintain refcount / fence / sync lists.
- Provide kernel API for BO create / destroy / map.

---

### 4. **Command Submission**
**Purpose:** Execute GPU work.

**Flow**
1. Allocate BO for batchbuffer.
2. Write command stream (`MI_NOOP`, `MI_BATCH_BUFFER_END`).
3. Write tail pointer, ring doorbell.
4. Poll seqno for completion or wait on interrupt.
5. Handle hang detection and reset.

---

### 5. **XeAccel.kext (IOAccelerator shim)**
**Purpose:** Optional user-facing accelerator.

**Responsibilities**
- Implement `IOUserClient` with methods:
  - `createBuffer`
  - `submit`
  - `wait`
  - `readRegister`
- Validate user input and translate to kernel ops.
- Expose minimal `IOAccelDevice` so macOS can enumerate an accelerator service.

---

### 6. **XeFB.kext (optional)**
**Purpose:** Dumb framebuffer for display or visual debug.

**Features**
- Derive from `IOFramebuffer`.
- Map linear buffer to `IOSurface`.
- Implement `setDisplayMode`, `getPixelInformation`, and `flushRect`.
- Optionally connect GPU BLT output.

---

### 7. **User-space Tools**
**xectl**
- Open `IOUserClient` via `IOServiceOpen`.
- Allocate buffers and submit simple batches.
- Provide CLI commands: `info`, `regdump`, `noop`, `fill`.

---

## Initialization Order

1. PCI attach → BAR0 map → read device ID.
2. Power-well enable / forcewake.
3. Load GuC / HuC firmware.
4. Init GGTT, allocate scratch / ring buffers.
5. Enable interrupts.
6. Submit `MI_NOOP` → verify seqno advancement.
7. Add BO / BLT / compute functions.
8. Optionally register IOAccelerator or framebuffer.

---

## Data Structures

```c
struct XeRing {
    volatile uint32_t *head, *tail, *start;
    uint32_t size;
};

struct XeBO {
    IOBufferMemoryDescriptor *mem;
    uint64_t gtt_offset;
};

struct XeDevice {
    IOPCIDevice *pdev;
    IOMemoryMap *bar0;
    XeRing render;
    // more fields later
};
```
---

## Kernel/User Interfaces

| Interface | Direction | Purpose |
|------------|------------|----------|
| `method 0` | user → kernel | allocate BO |
| `method 1` | user → kernel | submit batch |
| `method 2` | user → kernel | wait seqno |
| `method 3` | kernel → user | return status / regs |

---

## Build and Testing

### Quick Start

**Using Xcode:**
```bash
# Open the Xcode project
open XePCI.xcodeproj

# Build from Xcode (Cmd+B)
# The kext will be in DerivedData/XePCI/Build/Products/Release/XePCI.kext
```

**Using Command Line:**
```bash
# Build with Makefile
make release

# Or build with xcodebuild
xcodebuild -project XePCI.xcodeproj -scheme XePCI -configuration Release

# Install (requires sudo and SIP disabled)
sudo make install

# Test with xectl tool
cd userspace
clang xectl.c -framework IOKit -framework CoreFoundation -o xectl
sudo ./xectl info
```

### Documentation

- **[BUILD.md](BUILD.md)** — Complete build instructions including Xcode project and Makefile builds, prerequisites, build options, and CI/CD setup.
- **[TESTING.md](TESTING.md)** — Comprehensive testing guide for local Mac, including system preparation, installation, debugging, and safety procedures.

### Build Notes
- **Xcode Project**: Native Xcode project (`XePCI.xcodeproj`) for IDE-based development with full debugging support
- **Makefile**: Traditional command-line build system for CI/CD and automated builds
- Requires MacKernelSDK headers for compilation
- SIP must be disabled for development and testing
- Link against `IOKit` and `libkern`
- Start with **read-only MMIO**; verify register layout using Linux `i915` / `xe` sources
- Always test on non-critical hardware with fallback GPU/display
- See [BUILD.md](BUILD.md) for detailed build instructions

---

## External References
- **[POC.md](POC.md)** — Proof of Concept implementation details and technical documentation.
- **[RESEARCH.md](RESEARCH.md)** — Comprehensive code examples and implementation patterns from WhateverGreen, Lilu, NootedBlue, and Linux xe driver.
- **[BUILD.md](BUILD.md)** — Build system documentation and compilation instructions.
- **[TESTING.md](TESTING.md)** — Testing procedures and debugging guide.
- Linux `drivers/gpu/drm/xe` — register maps, GuC/HuC load, ring ops.
- Lilu / WhateverGreen — macOS kext scaffolding and patching patterns.
- NootedBlue — framebuffer spoofing and IGPU property examples.
- Mesa `iris` / `anv` — user-space shader and command-stream references.

---

## Minimal Bring-up Checklist
1. ✅ Verify PCI attach and BAR0 mapping (`XePCI.kext`).
2. ✅ Confirm register reads (device ID, GT config).
3. ✅ Enable power wells and forcewake.
4. 🔄 Map GGTT base; allocate ring buffer (framework ready).
5. 🔄 Submit `MI_NOOP` and verify seqno advance (prepared).
6. 🔄 Implement buffer objects and simple BLT test (BO framework ready).
7. ✅ Add IOUserClient interface (`XeService`).
8. ⏳ Optionally integrate framebuffer or IOAccelerator shim.

Legend: ✅ Complete | 🔄 Framework Ready | ⏳ Pending

---
