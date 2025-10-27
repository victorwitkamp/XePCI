#ifndef XEPCI_HPP
#define XEPCI_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

// Gen12/Xe Register Offsets (from Linux xe driver research)
// GT Configuration and Status
#define GEN12_GT_THREAD_STATUS          0x13800
#define GEN12_GT_GEOMETRY_DSS_ENABLE    0x913C

// Forcewake Management
#define GEN12_FORCEWAKE_GT              0x13810
#define GEN12_FORCEWAKE_ACK_GT          0x13D84
#define GEN12_FORCEWAKE_MEDIA           0x13E80
#define GEN12_FORCEWAKE_ACK_MEDIA       0x13EF4
#define GEN12_FORCEWAKE_RENDER          0x13E90
#define GEN12_FORCEWAKE_ACK_RENDER      0x13EF8

// GGTT (Global Graphics Translation Table) Registers
#define GEN12_GGTT_BASE                 0x108100
#define GEN12_PGTBL_CTL                 0x02020
#define GEN12_GGC                       0x50      // PCI config register

// Ring Buffer Registers (RCS0 - Render Command Streamer)
#define GEN12_RING_HEAD_RCS0            0x02000
#define GEN12_RING_TAIL_RCS0            0x02030
#define GEN12_RING_START_RCS0           0x02038
#define GEN12_RING_CTL_RCS0             0x02034
#define GEN12_RING_MODE_RCS0            0x0229C

// Ring Control Bits
#define RING_VALID                      (1 << 0)
#define RING_IDLE                       (1 << 2)
#define RING_WAIT                       (1 << 11)

// GuC (Graphics Micro Controller) Registers
#define GEN12_GUC_STATUS                0xC000
#define GEN12_GUC_WOPCM_SIZE            0xC050
#define GEN12_DMA_GUC_WOPCM_OFFSET      0xC340
#define GEN12_GUC_GGTT_ADDR             0xC380
#define GEN12_GUC_CTL                   0xC064

// Interrupt Registers
#define GEN12_GT_INTR_DW0               0x190000
#define GEN12_GT_INTR_DW1               0x190004
#define GEN12_GFX_MSTR_IRQ              0x190010

// Power Management
#define GEN12_RC_STATE                  0x138104
#define GEN12_RC_CONTROL                0x138108

// Mirror BAR offsets (device identification)
#define INTEL_GEN12_MIRROR_BASE         0x0
#define INTEL_DEVICE_ID_OFFSET          0x0  // First register often readable

// Forcewake domains
#define FORCEWAKE_GT_BIT                (1 << 0)
#define FORCEWAKE_RENDER_BIT            (1 << 1)
#define FORCEWAKE_MEDIA_BIT             (1 << 2)

// MI Command Definitions (from RESEARCH.md Section 4.7)
#define MI_NOOP                         0x00
#define MI_BATCH_BUFFER_END             (0x0A << 23)
#define MI_BATCH_BUFFER_START           (0x31 << 23)
#define MI_STORE_DWORD_IMM              (0x20 << 23)
#define MI_LOAD_REGISTER_IMM            (0x22 << 23)
#define MI_FLUSH_DW                     (0x26 << 23)

// Forward declarations for structures
struct XeRing;
struct XeBufferObject;
struct XeGGTT;

// Ring Buffer Structure (from RESEARCH.md Section 4.5)
struct XeRing {
    IOBufferMemoryDescriptor *mem;    // Ring buffer memory
    volatile UInt32 *vaddr;           // Virtual address
    UInt64 gttOffset;                 // GTT offset for GPU access
    UInt32 size;                      // Ring size in bytes
    UInt32 head;                      // Read pointer
    UInt32 tail;                      // Write pointer
    bool initialized;                 // Initialization state
};

// Buffer Object Structure (from RESEARCH.md Section 4.9)
struct XeBufferObject {
    IOBufferMemoryDescriptor *mem;    // Backing memory
    UInt64 gttOffset;                 // GTT mapping offset
    UInt32 size;                      // Buffer size
    UInt32 flags;                     // Buffer flags
    bool pinned;                      // Pinned state
};

// GGTT Management Structure (from RESEARCH.md Section 4.4)
struct XeGGTT {
    UInt64 baseAddr;                  // GGTT base address
    UInt64 size;                      // GGTT size
    UInt32 numEntries;                // Number of GGTT entries
    UInt32 nextFreeOffset;            // Next free GTT offset (simple allocator)
    bool initialized;                 // Initialization state
};

class org_yourorg_XePCI : public IOService {
    OSDeclareDefaultStructors(org_yourorg_XePCI)

private:
    IOPCIDevice *pciDev;
    IOMemoryMap  *bar0Map;
    volatile UInt32 *bar0Ptr; // mapped MMIO pointer
    IOBufferMemoryDescriptor *scratchBuf;
    
    // Device info
    UInt16 deviceId;
    UInt16 revisionId;
    
    // Forcewake state
    bool forcewakeActive;
    
    // GGTT (Global Graphics Translation Table)
    XeGGTT ggtt;
    
    // Ring buffer for command submission
    XeRing renderRing;
    
    // Sequence number for command tracking
    UInt32 seqno;
    
    // Acceleration readiness flag
    bool accelReady;

public:
    virtual bool init(OSDictionary *prop = 0) override;
    virtual void free(void) override;
    virtual IOService* probe(IOService *provider, SInt32 *score) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;

private:
    // BAR mapping
    bool mapBARs();
    void unmapBARs();
    
    // Register access helpers (from RESEARCH.md Section 5.1)
    inline UInt32 readReg(UInt32 offset) {
        if (!bar0Ptr) return 0xFFFFFFFF;
        return bar0Ptr[offset / 4];
    }
    
    inline void writeReg(UInt32 offset, UInt32 value) {
        if (!bar0Ptr) return;
        bar0Ptr[offset / 4] = value;
    }
    
    // Wait for register bit with timeout
    bool waitForRegisterBit(UInt32 offset, UInt32 mask, UInt32 value, UInt32 timeoutMs);
    
    // Device identification
    bool identifyDevice();
    const char* getDeviceName(UInt16 devId);
    
    // Forcewake management (from RESEARCH.md Section 4.3)
    bool acquireForcewake(UInt32 domains);
    void releaseForcewake(UInt32 domains);
    
    // GT configuration readout
    void readGTConfiguration();
    
    // Legacy register dump
    void dumpRegisters();
    
    // GGTT (Global Graphics Translation Table) Management (from RESEARCH.md Section 4.4)
    bool initGGTT();
    void cleanupGGTT();
    UInt64 allocateGTTSpace(UInt32 size);
    
    // Ring Buffer Management (from RESEARCH.md Section 4.5)
    bool initRingBuffer(XeRing *ring, UInt32 size);
    void cleanupRingBuffer(XeRing *ring);
    bool writeRingCommand(XeRing *ring, UInt32 *cmds, UInt32 numDwords);
    void updateRingTail(XeRing *ring);
    
    // Command Submission Framework (from RESEARCH.md Section 4.6)
    bool submitMINoop();
    bool waitForIdle(UInt32 timeoutMs);
    UInt32 getNextSeqno();
    
    // Buffer Object Management (from RESEARCH.md Section 4.9)
    XeBufferObject* createBufferObject(UInt32 size, UInt32 flags);
    void destroyBufferObject(XeBufferObject *bo);
    bool pinBufferObject(XeBufferObject *bo);
    
    // Interrupt handling preparation
    bool setupInterrupts();
    void cleanupInterrupts();
    
    // GuC firmware loading preparation (from RESEARCH.md Section 4.8)
    bool prepareGuCFirmware();
    
    // Power management
    bool enablePowerWells();
    void disablePowerWells();
    
    // Acceleration readiness check
    bool checkAccelerationReadiness();
};

#endif // XEPCI_HPP
