# Xe macOS iGPU Driver — Technical Overview

## Goal
Implement a minimal Intel Xe (Gen12, Tiger / Alder / Raptor Lake) iGPU driver for macOS to achieve:
1. PCI enumeration and MMIO access
2. Firmware load (GuC/HuC) and GT power-up
3. Ring-buffer command submission (MI_NOOP → BLT → compute)
4. Optional framebuffer or IOAccelerator interface

Specifically the goal is to make it work on Intel Raptor Lake-P Xe Graphics (for ASUS GI814JI)
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

## Build Notes
- Build with Xcode or `kmutil`; SIP disabled for dev.
- Link against `IOKit` and `libkern`.
- Start with **read-only MMIO**; verify register layout using Linux `i915` / `xe` sources.
- Always test on non-critical hardware with fallback GPU/display.

---

## External References
- Linux `drivers/gpu/drm/xe` — register maps, GuC/HuC load, ring ops.
- Lilu / WhateverGreen — macOS kext scaffolding and patching patterns.
- NootedBlue — framebuffer spoofing and IGPU property examples.
- Mesa `iris` / `anv` — user-space shader and command-stream references.

---

## Minimal Bring-up Checklist
1. Verify PCI attach and BAR0 mapping (`XePCI.kext`).
2. Confirm register reads (device ID, GT config).
3. Enable power wells and forcewake.
4. Map GGTT base; allocate ring buffer.
5. Submit `MI_NOOP` and verify seqno advance.
6. Implement buffer objects and simple BLT test.
7. Add IOUserClient interface (`XeAccel.kext`).
8. Optionally integrate framebuffer or IOAccelerator shim.

---
