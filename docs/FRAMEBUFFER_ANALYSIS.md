# Framebuffer Implementation Analysis

## Source Repository Analysis

This document analyzes the implementation approach from [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext), which claims to have a working framebuffer for Intel Tiger Lake (Iris Xe) graphics on macOS.

### Repository Status (as of 2025-11-29)

Per their README, they have achieved:
- **Phase 1-4 COMPLETED**: Working framebuffer, display pipeline, Tiger Lake eDP panel lit
- **Phase 5 COMPLETED (major part)**: Accelerator framework with Metal showing "Supported"

Target device: **Intel Tiger Lake GPU (8086:9A49)** - different from our **8086:A788** (Raptor Lake).

---

## Architecture Overview

### Class Hierarchy

```
IOPCIDevice
    └── FakeIrisXEFramebuffer (inherits IOFramebuffer)
            └── FakeIrisXEAccelerator (attaches as IOAccelerator)
                    └── FakeIrisXEAcceleratorUserClient
```

### Key Files

| File | Purpose |
|------|---------|
| `FakeIrisXEFramebuffer.hpp/.cpp` | IOFramebuffer subclass, display pipeline, GGTT mapping |
| `FakeIrisXEAccelerator.hpp/.cpp` | IOAccelerator-like service for 2D operations |
| `FakeIrisXEAccelShared.h` | Shared kernel/user ring buffer protocol |
| `Info.plist` | Bundle matching and personalities |

---

## Key Implementation Techniques

### 1. PCI Power Management Sequence

The `initPowerManagement()` function performs a comprehensive power-up sequence:

```cpp
// 1. Force PCI D0 state
pmcsr &= ~0x3; // Force D0
pciDevice->configWrite16(0x84, pmcsr);

// 2. Disable GT power gating
safeMMIOWrite(GT_PG_ENABLE, safeMMIORead(GT_PG_ENABLE) & ~0x1);

// 3. Enable Power Well 1 (Render)
safeMMIOWrite(PWR_WELL_CTL_1, safeMMIORead(PWR_WELL_CTL_1) | 0x2);
safeMMIOWrite(PWR_WELL_CTL_1, safeMMIORead(PWR_WELL_CTL_1) | 0x4);
// Poll PWR_WELL_STATUS (0x45408) for bit 30

// 4. Enable Power Well 2 (Display)
safeMMIOWrite(PWR_WELL_CTL_2, safeMMIORead(PWR_WELL_CTL_2) | 0x1);
// Poll for 0xFF state

// 5. Forcewake sequence
safeMMIOWrite(FORCEWAKE_RENDER_CTL, 0x000F000F);
// Poll FORCEWAKE_ACK_RENDER (0x130044) for ACK bit

// 6. Enable MBUS
safeMMIOWrite(MBUS_DBOX_CTL_A, 0xb1038c02);

// 7. Enable display clocks
safeMMIOWrite(LCPLL1_CTL, 0xcc000000);
safeMMIOWrite(TRANS_CLK_SEL_A, 0x10000000);
```

### 2. Register Offsets (Tiger Lake)

**Power Management:**
```cpp
GT_PG_ENABLE        = 0xA218
PUNIT_PG_CTRL       = 0xA2B0
PWR_WELL_CTL_1      = 0x45400
PWR_WELL_CTL_2      = 0x45404
PWR_WELL_STATUS     = 0x45408
FORCEWAKE_RENDER_CTL = 0xA188
FORCEWAKE_ACK_RENDER = 0x130044
```

**Display Pipeline:**
```cpp
// Pipe A timings
HTOTAL_A = 0x60000
HBLANK_A = 0x60004
HSYNC_A  = 0x60008
VTOTAL_A = 0x6000C
VBLANK_A = 0x60010
VSYNC_A  = 0x60014
PIPE_SRC_A = 0x6001C

// Transcoder/Pipe control
TRANS_CONF_A = 0x70008
PIPECONF_A   = 0x70008

// Plane control
PLANE_CTL_1_A    = 0x70180
PLANE_STRIDE_1_A = 0x70188
PLANE_POS_1_A    = 0x7018C
PLANE_SIZE_1_A   = 0x70190
PLANE_SURF_1_A   = 0x7019C

// DDI
DDI_BUF_CTL_A = 0x64000

// Panel power
PP_CONTROL = 0x64004
PP_STATUS  = 0x64024

// Backlight
BXT_BLC_PWM_CTL1  = 0xC8250
BXT_BLC_PWM_FREQ1 = 0xC8250
BXT_BLC_PWM_DUTY1 = 0xC8254
```

### 3. Framebuffer Memory Allocation

```cpp
// Allocate 64KB-aligned framebuffer
uint32_t alignedSize = (rawSize + 0xFFFF) & ~0xFFFF;

framebufferMemory = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
    kernel_task,
    kIODirectionInOut | kIOMemoryKernelUserShared,
    alignedSize,
    0x000000003FFFF000ULL   // Below 1GB, 4KB aligned
);

framebufferMemory->prepare();
```

### 4. GGTT (Graphics Translation Table) Mapping

```cpp
// Map BAR1 (GTTMMADR) to access GGTT
uint64_t bar1Lo = pciDevice->configRead32(0x18) & ~0xF;
uint64_t bar1Hi = pciDevice->configRead32(0x1C);
uint64_t gttPhys = (bar1Hi << 32) | bar1Lo;

// Map 16MB GGTT aperture
IOMemoryDescriptor* gttDesc = 
    IOMemoryDescriptor::withPhysicalAddress(gttPhys, 0x1000000, kIODirectionInOut);
IOMemoryMap* gttMap = gttDesc->map();
volatile uint64_t* ggtt = (volatile uint64_t*)gttMap->getVirtualAddress();

// Write PTEs for each 4KB page
uint64_t pte = (physAddr & ~0xFFFULL) | 0x3;  // Present + writable
ggtt[index] = pte;
```

### 5. Display Mode Configuration

The implementation provides a single 1920x1080@60Hz mode:

```cpp
IOItemCount getDisplayModeCount() { return 1; }

IOReturn getDisplayModes(IODisplayModeID* modes) {
    modes[0] = 1;  // Mode ID 1
    return kIOReturnSuccess;
}

IOReturn getInformationForDisplayMode(IODisplayModeID mode, 
                                      IODisplayModeInformation* info) {
    info->maxDepthIndex = 0;
    info->nominalWidth  = 1920;
    info->nominalHeight = 1080;
    info->refreshRate   = (60 << 16);  // Fixed point
    return kIOReturnSuccess;
}

IOReturn getPixelInformation(..., IOPixelInformation* info) {
    info->pixelType = kIO32ARGBPixelFormat;
    info->bitsPerPixel = 32;
    info->bytesPerRow = 1920 * 4;
    info->activeWidth = 1920;
    info->activeHeight = 1080;
    // ARGB masks...
    return kIOReturnSuccess;
}
```

### 6. Pipe/Transcoder Timing Setup

```cpp
// 1920x1080@60Hz CVT timing
h_active = 1920, h_frontporch = 88, h_sync = 44, h_backporch = 148
v_active = 1080, v_frontporch = 4, v_sync = 5, v_backporch = 36

// Program timing registers
wr(HTOTAL_A, ((h_total - 1) << 16) | (h_active - 1));
wr(VTOTAL_A, ((v_total - 1) << 16) | (v_active - 1));
// ... HBLANK, VBLANK, HSYNC, VSYNC similarly

// Enable pipe
pipeconf |= (1u << 31);  // Enable
pipeconf |= (1u << 30);  // Progressive
wr(PIPECONF_A, pipeconf);

// Enable transcoder
trans |= (1u << 31);
wr(TRANS_CONF_A, trans);
```

### 7. Plane Configuration

```cpp
// Stride in 64-byte blocks: 1920 pixels * 4 bytes = 7680 bytes = 120 blocks
uint32_t strideBlocks = 7680 / 64;  // 120 blocks
wr(PLANE_STRIDE_1_A, strideBlocks);

// Position and size
wr(PLANE_POS_1_A, 0x00000000);
wr(PLANE_SIZE_1_A, ((height - 1) << 16) | (width - 1));

// Surface address (GGTT offset)
wr(PLANE_SURF_1_A, fbGGTTOffset);

// Plane control: enable, ARGB8888, linear
uint32_t planeCtl = (1u << 31) | (6u << 24);  // Enable + ARGB8888
wr(PLANE_CTL_1_A, planeCtl);
```

### 8. Accelerator Ring Buffer Protocol

```cpp
// Shared header structure
struct XEHdr {
    uint32_t magic;     // 0x53524558 'XERS'
    uint32_t version;   // 1
    uint32_t capacity;  // ring size
    uint32_t head;      // producer (user)
    uint32_t tail;      // consumer (kernel)
};

// Command structure
struct XECmd {
    uint32_t opcode;    // XE_CMD_CLEAR, XE_CMD_RECT, etc.
    uint32_t bytes;     // payload size
    uint32_t ctxId;
    uint32_t reserved;
};
```

---

## Differences from XePCI

| Aspect | XePCI (Current) | Reference |
|--------|-----------------|-----------|
| Target GPU | Raptor Lake (A788) | Tiger Lake (9A49) |
| Base class | IOService | IOFramebuffer |
| Display | None | Full pipeline |
| GGTT | Stub | Working mapping |
| Forcewake | Stub | Full sequence |
| Accelerator | None | Ring buffer 2D |

---

## Recommendations for XePCI

### Short-term (Incremental)

1. **Document register differences** between Tiger Lake and Raptor Lake
2. **Implement IOFramebuffer subclass** as optional parallel path
3. **Port power management sequence** with Raptor Lake offsets
4. **Add GGTT mapping logic** to existing XeGGTT

### Medium-term

1. **Enable display pipeline** for a single mode
2. **Implement plane configuration** for basic framebuffer
3. **Add accelerator framework** for 2D operations

### Long-term

1. **Multi-mode support**
2. **Metal/OpenGL integration research**
3. **GuC firmware loading**

---

## Register Mapping Notes

The reference implementation targets Tiger Lake (Gen12). Raptor Lake (also Gen12.x) shares many registers but some offsets may differ. Key areas to verify:

- Power well control registers
- Display engine base addresses
- GGTT aperture size and location
- DDI configuration

Intel PRM (Programmer's Reference Manual) Volumes 15-17 contain the definitive register documentation.

---

## Tiger Lake Register Dump Summary

The reference repository includes a `regs (Copy).txt` file containing a complete register dump from a working Tiger Lake system. Key categories include:

### Interrupt Registers (0x44xxx)
- Master IRQ, GT ISR/IMR/IIR/IER, Display pipe/port/misc interrupts

### Power Well Registers (0x454xx)
- `PWR_WELL_BIOS/DRIVER/KVM/DEBUG` at 0x45400-0x4540C

### Display PLL Registers (0x6Cxxx, 0x46xxx)
- DPLL status, config, control registers
- CD clock, LC PLL registers

### Plane Registers (0x70xxx-0x73xxx)
- Per-pipe, per-plane control, stride, position, size, surface, watermarks
- Cursor registers
- Pipe scaler registers

### Transcoder Registers (0x60xxx-0x63xxx)
- Timing: HTOTAL, HBLANK, HSYNC, VTOTAL, VBLANK, VSYNC
- DDI function control, MSA misc
- Data/Link M/N values

### Other Display Registers
- MBUS control, watermark misc, PSR registers

For detailed register offsets and research methodology, see [RESEARCH_GUIDE.md](RESEARCH_GUIDE.md).

---

## References

1. [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext)
2. Linux i915 driver source
3. Intel PRM (Programmer's Reference Manual) Volumes 15-17 (Tiger Lake/Raptor Lake)
