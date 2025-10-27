# Intel Xe macOS Driver Research - Code Implementation References

This document provides research and code examples from other macOS kernel extensions (kexts) and Linux driver implementations for Intel GPUs, specifically focusing on patterns applicable to the XePCI project for Intel Raptor Lake Xe Graphics.

## Table of Contents
- [1. WhateverGreen - Intel iGPU Implementation](#1-whatevergreen---intel-igpu-implementation)
- [2. Lilu - Kernel Extension Framework](#2-lilu---kernel-extension-framework)
- [3. NootedBlue - Intel iGPU Framebuffer](#3-nootedblue---intel-igpu-framebuffer)
- [4. Linux xe Driver - Command Submission & Register Access](#4-linux-xe-driver---command-submission--register-access)
- [5. Key Patterns and Takeaways](#5-key-patterns-and-takeaways)

---

## 1. WhateverGreen - Intel iGPU Implementation

**Repository:** https://github.com/acidanthera/WhateverGreen

WhateverGreen is a Lilu plugin that provides various patches for Intel, AMD, and NVIDIA GPUs. The Intel-specific code is in `kern_igfx.cpp` and `kern_igfx.hpp`.

### 1.1 PCI Device Matching and Initialization

WhateverGreen uses Lilu's patching framework rather than direct IOPCIDevice attachment. However, it contains valuable patterns for Intel GPU device detection and register access.

**Key Files:**
- `WhateverGreen/kern_igfx.cpp` - Intel graphics framework patches
- `WhateverGreen/kern_igfx.hpp` - Headers and structures
- `WhateverGreen/kern_fb.hpp` - Framebuffer definitions

### 1.2 Framebuffer Structure Definitions

```cpp
// From kern_fb.hpp - Intel Framebuffer structures for different generations

// Ice Lake LP (Gen 11)
struct FramebufferICLLP {
    uint32_t framebufferId;
    uint8_t  fMobile;
    uint8_t  fPipeCount;
    uint8_t  fPortCount;
    uint8_t  fFBMemoryCount;
    uint32_t fStolenMemorySize;
    uint32_t fFramebufferMemorySize;
    uint32_t fUnifiedMemorySize;
    uint32_t flags;
    // ... connector info
};

// Connector types
enum ConnectorType : uint32_t {
    ConnectorZero      = 0x0,
    ConnectorDummy     = 0x1,
    ConnectorLVDS      = 0x2,
    ConnectorDigitalDVI = 0x4,
    ConnectorSVID      = 0x8,
    ConnectorVGA       = 0x10,
    ConnectorDP        = 0x400,
    ConnectorHDMI      = 0x800,
    ConnectorAnalogDVI = 0x2000,
};
```

### 1.3 Register Access Patterns

```cpp
// Westmere register definitions from kern_igfx.hpp
static constexpr uint32_t WESTMERE_TXA_CTL = 0x60100;
static constexpr uint32_t WESTMERE_RXA_CTL = 0xF000C;
static constexpr uint32_t WESTMERE_LINK_WIDTH_MASK = 0xFFC7FFFF;

// Modern register access would follow similar patterns:
// Read: uint32_t value = bar0Ptr[REGISTER_OFFSET / 4];
// Write: bar0Ptr[REGISTER_OFFSET / 4] = value;
```

### 1.4 Device Property Patching

WhateverGreen patches various device properties for framebuffer configuration:

```cpp
// Platform ID matching (determines which framebuffer to use)
// Example platform IDs for different generations:
// - Ice Lake: 0x8A520000, 0x8A530000, 0x8A5C0000
// - Tiger Lake: 0x9A490000, 0x9A600000
// - Alder Lake: 0x4600, 0x4680, 0x4690
```

### 1.5 Memory Management

WhateverGreen patches stolen memory size and framebuffer memory allocation:

```cpp
struct FramebufferPatchFlags {
    uint8_t FPFStolenMemorySize         :1;
    uint8_t FPFFramebufferMemorySize    :1;
    uint8_t FPFUnifiedMemorySize        :1;
    // ...
};

// Typical values:
// fStolenMemorySize: 0x00000000 (calculated from DVMT)
// fFramebufferMemorySize: 0x00009000 (36 KB)
```

---

## 2. Lilu - Kernel Extension Framework

**Repository:** https://github.com/acidanthera/Lilu

Lilu provides the foundation for runtime kernel patching. While XePCI doesn't use Lilu's patching features directly, its architecture provides excellent patterns for kext development.

### 2.1 Plugin Architecture Pattern

```cpp
// Lilu plugin initialization pattern
class MyPlugin {
public:
    void init() {
        // Register callbacks
        lilu.onPatcherLoadForce([](void *user, KernelPatcher &patcher) {
            static_cast<MyPlugin *>(user)->processKernel(patcher);
        }, this);
    }
    
    void processKernel(KernelPatcher &patcher) {
        // Kernel patching logic
    }
};
```

### 2.2 IOKit Service Interaction

```cpp
// Safe IOService property access pattern (from Lilu)
OSObject *prop = service->getProperty("property-name");
if (prop) {
    OSData *data = OSDynamicCast(OSData, prop);
    if (data && data->getLength() == expectedSize) {
        // Use data safely
    }
}
```

### 2.3 Kext Structure Best Practices

From Lilu's Info.plist structure:
```xml
<key>OSBundleLibraries</key>
<dict>
    <key>com.apple.kpi.bsd</key>
    <string>12.0</string>
    <key>com.apple.kpi.dsep</key>
    <string>12.0</string>
    <key>com.apple.kpi.iokit</key>
    <string>12.0</string>
    <key>com.apple.kpi.libkern</key>
    <string>12.0</string>
    <key>com.apple.kpi.mach</key>
    <string>12.0</string>
    <key>com.apple.kpi.unsupported</key>
    <string>12.0</string>
</dict>
```

### 2.4 Boot Arguments and Configuration

```cpp
// Boot argument parsing pattern
static bool debugEnabled = false;

void parseBootArgs() {
    uint32_t debugFlags = 0;
    if (PE_parse_boot_argn("-mydbg", &debugFlags, sizeof(debugFlags))) {
        debugEnabled = true;
    }
}
```

---

## 3. NootedBlue - Intel iGPU Framebuffer

**Repository:** https://github.com/Zormeister/NootedBlue (archived)
**Alternative:** https://github.com/ChefKissInc/NootedRed (AMD, similar patterns)

NootedBlue was an attempt to support newer Intel iGPUs (Gen11/Gen12+) on macOS. The project is archived but provides valuable insights.

### 3.1 Platform ID Configuration

```cpp
// Platform ID selection for Ice Lake / Tiger Lake
// These IDs determine which framebuffer personality macOS loads
static const uint32_t supportedPlatformIds[] = {
    // Ice Lake
    0xFF050000, // Ice Lake - LPDDR4X
    0x8A510000, // Ice Lake - U GT2
    0x8A520000, // Ice Lake - U GT2
    0x8A530000, // Ice Lake - U GT2
    
    // Tiger Lake (Gen 12)
    0x9A490000, // Tiger Lake - UP3 GT2
    0x9A400000, // Tiger Lake - H GT2
    0x9A600000, // Tiger Lake - H GT1
};
```

### 3.2 DeviceProperty Injection

Key properties needed for Intel iGPU initialization:

```cpp
// Properties to inject via DeviceProperties
struct IGPUProperties {
    // Platform ID
    uint32_t AAPL_ig_platform_id;
    
    // Device ID spoofing (if needed)
    uint32_t device_id;
    
    // Memory allocation
    uint32_t framebuffer_stolenmem;  // e.g., 0x00003001
    uint32_t framebuffer_fbmem;      // e.g., 0x00009000
    
    // Connector patches
    uint8_t framebuffer_patch_enable;
    OSData *framebuffer_con0_enable;  // Per-connector enable
    OSData *framebuffer_con1_enable;
    OSData *framebuffer_con2_enable;
};
```

### 3.3 Framebuffer Patching Approach

```cpp
// Framebuffer patching requires finding the correct framebuffer
// in the AppleIntelFramebufferController kext and modifying it

// 1. Find framebuffer by platform ID
// 2. Patch connector configurations
// 3. Patch memory sizes
// 4. Enable/disable specific connectors

// Example connector patch:
struct ConnectorInfo {
    uint32_t index;     // Connector index (0-2)
    uint32_t busId;     // PCH bus ID (0x00, 0x01, 0x02, etc.)
    uint32_t pipe;      // Pipe assignment
    uint32_t type;      // ConnectorType
    uint32_t flags;     // Connector flags
};
```

---

## 4. Linux xe Driver - Command Submission & Register Access

**Repository:** Linux kernel mainline - `drivers/gpu/drm/xe/`
**Documentation:** https://docs.kernel.org/gpu/xe/

The Linux xe driver is the official Intel driver for Xe architecture (Gen12+).

### 4.1 Device Initialization Flow

```c
// From Linux xe driver initialization pattern
// File: drivers/gpu/drm/xe/xe_device.c

int xe_device_probe(struct pci_dev *pdev) {
    struct xe_device *xe;
    
    // 1. Enable PCI device
    pci_enable_device(pdev);
    pci_set_master(pdev);
    
    // 2. Map MMIO regions (BAR 0)
    xe->mmio.regs = pci_iomap(pdev, 0, 0);
    
    // 3. Read device info
    xe->info.devid = pdev->device;
    xe->info.platform = xe_get_platform(pdev->device);
    
    // 4. Initialize GT (Graphics Technology)
    xe_gt_init(xe);
    
    // 5. Load firmware (GuC/HuC)
    xe_guc_init(&xe->gt.uc.guc);
    
    return 0;
}
```

### 4.2 Register Access Abstraction

```c
// Register read/write helpers
// File: drivers/gpu/drm/xe/xe_mmio.h

static inline u32 xe_mmio_read32(struct xe_gt *gt, u32 reg) {
    return readl(gt->mmio.regs + reg);
}

static inline void xe_mmio_write32(struct xe_gt *gt, u32 reg, u32 val) {
    writel(val, gt->mmio.regs + reg);
}

// Register definitions
// File: drivers/gpu/drm/xe/regs/xe_gt_regs.h
#define GEN12_GT_THREAD_STATUS      _MMIO(0x13800)
#define GEN12_GT_GEOMETRY_DSS_ENABLE _MMIO(0x913c)
```

### 4.3 Forcewake and Power Management

```c
// Forcewake is required before accessing certain registers
// File: drivers/gpu/drm/xe/xe_force_wake.c

enum forcewake_domains {
    FORCEWAKE_GT    = BIT(0),
    FORCEWAKE_RENDER = BIT(1),
    FORCEWAKE_MEDIA  = BIT(2),
};

int xe_force_wake_get(struct xe_gt *gt, enum forcewake_domains domains) {
    // Write to forcewake request register
    xe_mmio_write32(gt, FORCEWAKE_REQ, domains);
    
    // Poll acknowledgment register
    return xe_mmio_wait_for_ack(gt, FORCEWAKE_ACK, domains, TIMEOUT);
}

void xe_force_wake_put(struct xe_gt *gt, enum forcewake_domains domains) {
    xe_mmio_write32(gt, FORCEWAKE_REQ, 0);
}
```

### 4.4 GGTT (Global Graphics Translation Table) Setup

```c
// GGTT initialization
// File: drivers/gpu/drm/xe/xe_ggtt.c

int xe_ggtt_init(struct xe_ggtt *ggtt) {
    // 1. Read GGTT base address from PCI config
    pci_read_config_dword(pdev, GGC, &ggc);
    ggtt->gsm_base = ggc & 0xFFF00000;
    
    // 2. Map GGTT aperture
    ggtt->gsm = ioremap(ggtt->gsm_base, ggtt->size);
    
    // 3. Clear GGTT (set all entries to scratch page)
    for (i = 0; i < ggtt->num_entries; i++) {
        xe_ggtt_set_pte(ggtt, i, scratch_pte);
    }
    
    return 0;
}

// Insert buffer into GGTT
void xe_ggtt_insert_entries(struct xe_ggtt *ggtt, 
                            u64 start, 
                            struct sg_table *pages,
                            enum xe_cache_level cache_level) {
    u64 pte_encode = gen12_pte_encode(0, cache_level);
    
    for_each_sg(pages->sgl, sg, pages->nents, i) {
        u64 addr = sg_dma_address(sg);
        writeq(pte_encode | addr, &ggtt->gsm[start++]);
    }
}
```

### 4.5 Command Submission Architecture

```c
// Ring buffer / execution queue setup
// File: drivers/gpu/drm/xe/xe_execlist.c

struct xe_ring {
    void *vaddr;              // Virtual address
    dma_addr_t dma_addr;      // DMA address
    u32 size;                 // Ring size
    u32 head;                 // Read pointer
    u32 tail;                 // Write pointer
};

int xe_ring_init(struct xe_ring *ring, u32 size) {
    // Allocate coherent memory for ring buffer
    ring->vaddr = dma_alloc_coherent(dev, size, 
                                     &ring->dma_addr, 
                                     GFP_KERNEL);
    ring->size = size;
    ring->head = 0;
    ring->tail = 0;
    
    // Program ring registers
    xe_mmio_write32(gt, RING_HEAD, 0);
    xe_mmio_write32(gt, RING_TAIL, 0);
    xe_mmio_write32(gt, RING_START, ring->dma_addr);
    xe_mmio_write32(gt, RING_CTL, RING_VALID | size);
    
    return 0;
}
```

### 4.6 Command Buffer Submission

```c
// Submit commands to GPU
// File: drivers/gpu/drm/xe/xe_exec.c

int xe_exec_submit(struct xe_exec_queue *q, struct xe_exec *exec) {
    struct xe_ring *ring = &q->ring;
    u32 *cs;  // Command stream pointer
    u32 length = exec->num_dwords * 4;
    
    // Check for available space
    if (ring_space(ring) < length)
        return -ENOSPC;
    
    // Write commands to ring
    cs = ring->vaddr + ring->tail;
    memcpy(cs, exec->cmds, length);
    
    // Update tail pointer
    ring->tail = (ring->tail + length) & (ring->size - 1);
    
    // Ring doorbell (trigger submission)
    xe_mmio_write32(gt, RING_TAIL, ring->tail);
    
    // Add fence for completion tracking
    xe_fence_emit(exec->fence, ring->tail);
    
    return 0;
}
```

### 4.7 MI Commands (Machine Instructions)

```c
// Common MI commands for Intel GPUs
// File: drivers/gpu/drm/xe/instructions/xe_mi_commands.h

#define MI_NOOP                 0x00
#define MI_BATCH_BUFFER_END     0x0A << 23
#define MI_BATCH_BUFFER_START   0x31 << 23
#define MI_STORE_DWORD_IMM      0x20 << 23
#define MI_LOAD_REGISTER_IMM    0x22 << 23
#define MI_FLUSH_DW             0x26 << 23

// Example: Simple batch buffer
u32 batch[] = {
    MI_NOOP,
    MI_NOOP,
    MI_STORE_DWORD_IMM | (4 - 2),  // Length in dwords - 2
    target_addr_low,
    target_addr_high,
    0xDEADBEEF,  // Value to store
    MI_BATCH_BUFFER_END,
};
```

### 4.8 GuC (Graphics Micro Controller) Firmware Loading

```c
// GuC firmware initialization
// File: drivers/gpu/drm/xe/xe_guc.c

int xe_guc_init(struct xe_guc *guc) {
    const struct firmware *fw;
    
    // 1. Request firmware from filesystem
    request_firmware(&fw, "i915/tgl_guc_70.bin", dev);
    
    // 2. Allocate memory for firmware
    guc->fw.vma = xe_bo_create_pin_map(xe, fw->size, 
                                       XE_BO_CREATE_VRAM_IF_DGFX);
    
    // 3. Copy firmware to GPU memory
    memcpy(guc->fw.vma->map, fw->data, fw->size);
    
    // 4. Program GuC registers
    xe_mmio_write32(gt, GUC_WOPCM_SIZE, guc->fw.size);
    xe_mmio_write32(gt, DMA_GUC_WOPCM_OFFSET, guc->fw.offset);
    xe_mmio_write32(gt, GUC_GGTT_ADDR, guc->fw.vma->addr);
    
    // 5. Start GuC
    xe_mmio_write32(gt, GUC_CTL, GUC_CTL_START);
    
    // 6. Wait for initialization
    return xe_guc_wait_for_init_complete(guc);
}
```

### 4.9 Memory Object (BO) Management

```c
// Buffer Object creation
// File: drivers/gpu/drm/xe/xe_bo.c

struct xe_bo *xe_bo_create(struct xe_device *xe, size_t size, u32 flags) {
    struct xe_bo *bo;
    
    // Allocate BO structure
    bo = kzalloc(sizeof(*bo), GFP_KERNEL);
    
    // Allocate backing pages
    if (flags & XE_BO_CREATE_VRAM_IF_DGFX) {
        bo->vram_allocation = xe_vram_alloc(xe, size);
    } else {
        bo->pages = drm_gem_get_pages(&bo->gem, size);
    }
    
    // Initialize DMA mapping
    bo->dma_addr = dma_map_sg(xe->dev, bo->pages, 
                              bo->num_pages, DMA_BIDIRECTIONAL);
    
    return bo;
}

// Pin BO into GGTT
int xe_bo_pin(struct xe_bo *bo) {
    u64 offset = xe_ggtt_allocate(xe->ggtt, bo->size);
    xe_ggtt_insert_entries(xe->ggtt, offset, bo->pages, 
                          bo->cache_level);
    bo->ggtt_offset = offset;
    return 0;
}
```

---

## 5. Key Patterns and Takeaways

### 5.1 XePCI Implementation Roadmap

Based on the research, here's a suggested implementation order for XePCI:

1. **PCI Enumeration & BAR Mapping** (Already done in XePCI.cpp)
   - Match Intel vendor ID (0x8086)
   - Map BAR0 for MMIO access
   - Enable memory, I/O, and bus mastering

2. **Register Access Framework**
   ```cpp
   // Add to XePCI.hpp
   inline uint32_t readReg(uint32_t offset) {
       return bar0Ptr[offset / 4];
   }
   
   inline void writeReg(uint32_t offset, uint32_t value) {
       bar0Ptr[offset / 4] = value;
   }
   ```

3. **Forcewake Implementation**
   ```cpp
   bool acquireForcewake(uint32_t domains);
   void releaseForcewake(uint32_t domains);
   ```

4. **GGTT Initialization**
   ```cpp
   bool initGGTT();
   uint64_t allocateGTTSpace(size_t size);
   void mapToGTT(uint64_t offset, IOBufferMemoryDescriptor *mem);
   ```

5. **Ring Buffer Setup**
   ```cpp
   struct XeRing {
       IOBufferMemoryDescriptor *mem;
       volatile uint32_t *vaddr;
       uint64_t gtt_offset;
       uint32_t size;
       uint32_t head;
       uint32_t tail;
   };
   
   bool initRingBuffer(XeRing *ring, uint32_t size);
   ```

6. **Command Submission**
   ```cpp
   bool submitCommands(uint32_t *cmds, uint32_t num_dwords);
   bool waitForIdle();
   ```

7. **Firmware Loading** (GuC/HuC)
   ```cpp
   bool loadGuCFirmware(const char *path);
   bool waitForGuCReady();
   ```

8. **IOUserClient Interface**
   ```cpp
   class XeUserClient : public IOUserClient {
       // Provide user-space access to:
       // - Buffer allocation
       // - Command submission
       // - Register dumps
   };
   ```

### 5.2 Critical Register Offsets (Gen12/Xe)

```cpp
// Power Management
#define GEN12_FORCEWAKE_GT          0x13810
#define GEN12_FORCEWAKE_ACK_GT      0x13D84

// GGTT Control
#define GEN12_GGTT_BASE             0x108100
#define GEN12_GGC                   0x50      // PCI config

// Ring Registers (RCS0 - Render Command Streamer)
#define GEN12_RING_HEAD_RCS0        0x02000
#define GEN12_RING_TAIL_RCS0        0x02030
#define GEN12_RING_START_RCS0       0x02038
#define GEN12_RING_CTL_RCS0         0x02034

// GuC
#define GEN12_GUC_WOPCM_SIZE        0xC050
#define GEN12_GUC_STATUS            0xC000
```

### 5.3 Memory Allocation Strategy

```cpp
// Follow this pattern for GPU buffers
IOBufferMemoryDescriptor *allocateGPUBuffer(size_t size) {
    return IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryPhysicallyContiguous | kIOMemoryKernelUserShared,
        size,
        PAGE_SIZE  // alignment
    );
}
```

### 5.4 Error Handling

```cpp
// Always check for errors and log appropriately
if (!bar0Ptr) {
    IOLog("XePCI: ERROR - Failed to map BAR0\n");
    return false;
}

// Timeout pattern for register polling
uint32_t timeout = 1000; // ms
while (timeout--) {
    if (readReg(STATUS_REG) & READY_BIT)
        break;
    IOSleep(1);
}
if (timeout == 0) {
    IOLog("XePCI: ERROR - Timeout waiting for ready\n");
    return false;
}
```

### 5.5 Reference Device IDs

```cpp
// Intel Xe Graphics Device IDs

// Raptor Lake HX - TARGET DEVICE for XePCI
// ASUS GI814JI: HX 8P+16E with 32EU, Revision B-0
#define INTEL_RPL_HX_TARGET \
    0xA788  // Raptor Lake HX (32EU) - Primary target device

// Raptor Lake (standard mobile/desktop)
#define INTEL_RPL_P_IDS \
    0x4600, 0x4601, 0x4602, 0x4603, \
    0x4680, 0x4681, 0x4682, 0x4683, \
    0x4690, 0x4691, 0x4692, 0x4693

// Alder Lake
#define INTEL_ADL_P_IDS \
    0x46A0, 0x46A1, 0x46A2, 0x46A3, \
    0x46A6, 0x46A8, 0x46AA, 0x462A, \
    0x4626, 0x4628, 0x46B0, 0x46B1, \
    0x46B2, 0x46B3

// Tiger Lake
#define INTEL_TGL_IDS \
    0x9A49, 0x9A40, 0x9A59, 0x9A60, \
    0x9A68, 0x9A70, 0x9A78
```

---

## 6. Additional Resources

### Official Documentation
- **Intel Graphics Programmer Reference Manuals (PRMs)**: https://01.org/linuxgraphics/documentation/hardware-specification-prms
- **Linux DRM/Xe Documentation**: https://docs.kernel.org/gpu/xe/
- **Apple IOKit Fundamentals**: https://developer.apple.com/library/archive/documentation/DeviceDrivers/Conceptual/IOKitFundamentals/

### Community Resources
- **Acidanthera GitHub**: https://github.com/acidanthera
- **Dortania OpenCore Guide**: https://dortania.github.io/OpenCore-Install-Guide/
- **Intel iGPU Patching Guide**: https://github.com/acidanthera/WhateverGreen/blob/master/Manual/

### Source Code Repositories
- **WhateverGreen**: https://github.com/acidanthera/WhateverGreen
- **Lilu**: https://github.com/acidanthera/Lilu
- **NootedBlue (archived)**: https://github.com/Zormeister/NootedBlue
- **Linux Kernel - drivers/gpu/drm/xe**: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/xe
- **Linux Kernel - drivers/gpu/drm/i915**: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/gpu/drm/i915

---

## Conclusion

This research provides a foundation for implementing the XePCI driver for Intel Raptor Lake Xe Graphics. Key takeaways:

1. **Start Simple**: Begin with register reads and device identification
2. **Incremental Development**: Add features one at a time (forcewake → GGTT → ring → commands)
3. **Learn from Linux**: The xe driver is the canonical reference for register offsets and initialization sequences
4. **macOS Patterns**: WhateverGreen and Lilu show how to structure macOS kexts properly
5. **Safety First**: Always implement timeouts, error checking, and graceful degradation

The next steps for XePCI should focus on:
- Implementing forcewake for safe register access
- Initializing GGTT for memory management
- Setting up a basic ring buffer
- Submitting simple MI_NOOP commands
- Building up to more complex operations

Remember to test incrementally on non-critical hardware with fallback display options available.
