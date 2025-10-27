#ifndef XEPCI_HPP
#define XEPCI_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

// Gen12/Xe Register Offsets (from Linux xe driver research)
#define GEN12_GT_THREAD_STATUS          0x13800
#define GEN12_GT_GEOMETRY_DSS_ENABLE    0x913C
#define GEN12_FORCEWAKE_GT              0x13810
#define GEN12_FORCEWAKE_ACK_GT          0x13D84
#define GEN12_FORCEWAKE_MEDIA           0x13E80
#define GEN12_FORCEWAKE_ACK_MEDIA       0x13EF4

// Mirror BAR offsets (device identification)
#define INTEL_GEN12_MIRROR_BASE         0x0
#define INTEL_DEVICE_ID_OFFSET          0x0  // First register often readable

// Forcewake domains
#define FORCEWAKE_GT_BIT                (1 << 0)
#define FORCEWAKE_RENDER_BIT            (1 << 1)
#define FORCEWAKE_MEDIA_BIT             (1 << 2)

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
};

#endif // XEPCI_HPP
