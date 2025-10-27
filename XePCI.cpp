#include "XePCI.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_yourorg_XePCI, IOService)

bool org_yourorg_XePCI::init(OSDictionary *props) {
    if (!super::init(props)) return false;
    pciDev = nullptr;
    bar0Map = nullptr;
    bar0Ptr = nullptr;
    scratchBuf = nullptr;
    deviceId = 0;
    revisionId = 0;
    forcewakeActive = false;
    IOLog("XePCI: init\n");
    return true;
}

void org_yourorg_XePCI::free() {
    IOLog("XePCI: free\n");
    if (scratchBuf) {
        scratchBuf->release();
        scratchBuf = nullptr;
    }
    super::free();
}

IOService* org_yourorg_XePCI::probe(IOService *provider, SInt32 *score) {
    IOLog("XePCI: probe\n");
    IOPCIDevice *dev = OSDynamicCast(IOPCIDevice, provider);
    if (!dev) return nullptr;

    // Check vendor/device
    UInt16 vendor = dev->configRead16(kIOPCIConfigVendorID);
    UInt16 device = dev->configRead16(kIOPCIConfigDeviceID);
    IOLog("XePCI: probe vendor=0x%04x device=0x%04x\n", vendor, device);
    
    // Verify this is an Intel device
    if (vendor != 0x8086) {
        IOLog("XePCI: not an Intel device, skipping\n");
        return nullptr;
    }

    return super::probe(provider, score);
}

bool org_yourorg_XePCI::start(IOService *provider) {
    IOLog("XePCI: start\n");
    if (!super::start(provider)) return false;

    pciDev = OSDynamicCast(IOPCIDevice, provider);
    if (!pciDev) {
        IOLog("XePCI: provider is not IOPCIDevice\n");
        return false;
    }

    // Enable device memory & bus mastering
    pciDev->setMemoryEnable(true);
    pciDev->setIOEnable(true);
    pciDev->setBusMasterEnable(true);

    if (!mapBARs()) {
        IOLog("XePCI: failed to map BARs\n");
        return false;
    }

    // Identify device
    if (!identifyDevice()) {
        IOLog("XePCI: failed to identify device\n");
        return false;
    }

    // PoC: Attempt to acquire forcewake (read-only check)
    IOLog("XePCI: === Starting PoC - Forcewake Test ===\n");
    if (acquireForcewake(FORCEWAKE_GT_BIT)) {
        IOLog("XePCI: Forcewake acquired successfully\n");
        
        // Read GT configuration while forcewake is active
        readGTConfiguration();
        
        // Release forcewake
        releaseForcewake(FORCEWAKE_GT_BIT);
        IOLog("XePCI: Forcewake released\n");
    } else {
        IOLog("XePCI: WARNING - Forcewake not acquired (may not be required on this platform)\n");
        // Still try to read configuration
        readGTConfiguration();
    }
    
    // Legacy register dump for comparison
    dumpRegisters();

    // Optional scratch buffer for future BO prototyping
    scratchBuf = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryPhysicallyContiguous | kIOMemoryKernelUserShared,
        4096, // 4k
        0);
    if (scratchBuf) {
        IOLog("XePCI: scratch buffer allocated (4KB)\n");
    }

    // Publish service so user clients can open later
    registerService();

    IOLog("XePCI: PoC completed successfully\n");
    return true;
}

void org_yourorg_XePCI::stop(IOService *provider) {
    IOLog("XePCI: stop\n");
    
    // Ensure forcewake is released
    if (forcewakeActive) {
        releaseForcewake(FORCEWAKE_GT_BIT);
    }
    
    unmapBARs();
    super::stop(provider);
}

bool org_yourorg_XePCI::mapBARs() {
    if (!pciDev) return false;

    // BAR 0 typical for Intel GT MMIO
    bar0Map = pciDev->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
    if (!bar0Map) {
        IOLog("XePCI: mapDeviceMemoryWithRegister failed for BAR0\n");
        return false;
    }

    // Get virtual pointer
    bar0Ptr = (volatile UInt32 *)bar0Map->getVirtualAddress();
    if (!bar0Ptr) {
        IOLog("XePCI: getVirtualAddress returned NULL\n");
        return false;
    }

    IOLog("XePCI: BAR0 mapped at %p, size=%llu bytes\n", 
          (void*)bar0Ptr, bar0Map->getLength());
    return true;
}

void org_yourorg_XePCI::unmapBARs() {
    if (bar0Map) {
        bar0Map->release();
        bar0Map = nullptr;
        bar0Ptr = nullptr;
    }
}

bool org_yourorg_XePCI::waitForRegisterBit(UInt32 offset, UInt32 mask, UInt32 value, UInt32 timeoutMs) {
    UInt32 timeout = timeoutMs;
    while (timeout--) {
        UInt32 regValue = readReg(offset);
        if ((regValue & mask) == value) {
            return true;
        }
        IOSleep(1); // Sleep 1ms
    }
    return false;
}

bool org_yourorg_XePCI::identifyDevice() {
    // Read device ID from PCI config space
    deviceId = pciDev->configRead16(kIOPCIConfigDeviceID);
    revisionId = pciDev->configRead8(kIOPCIConfigRevisionID);
    
    const char* name = getDeviceName(deviceId);
    IOLog("XePCI: Device identified: %s (0x%04x), Revision: 0x%02x\n", 
          name, deviceId, revisionId);
    
    return true;
}

const char* org_yourorg_XePCI::getDeviceName(UInt16 devId) {
    // From RESEARCH.md Section 5.5 - Reference Device IDs
    switch (devId) {
        // Raptor Lake
        case 0x4600: case 0x4601: case 0x4602: case 0x4603:
        case 0x4680: case 0x4681: case 0x4682: case 0x4683:
        case 0x4690: case 0x4691: case 0x4692: case 0x4693:
            return "Intel Raptor Lake";
        
        // Alder Lake
        case 0x46A0: case 0x46A1: case 0x46A2: case 0x46A3:
        case 0x46A6: case 0x46A8: case 0x46AA: case 0x462A:
        case 0x4626: case 0x4628: case 0x46B0: case 0x46B1:
        case 0x46B2: case 0x46B3:
            return "Intel Alder Lake";
        
        // Tiger Lake
        case 0x9A49: case 0x9A40: case 0x9A59: case 0x9A60:
        case 0x9A68: case 0x9A70: case 0x9A78:
            return "Intel Tiger Lake";
        
        default:
            return "Unknown Intel GPU";
    }
}

bool org_yourorg_XePCI::acquireForcewake(UInt32 domains) {
    // From RESEARCH.md Section 4.3 - Forcewake implementation
    if (!bar0Ptr) return false;
    
    IOLog("XePCI: Acquiring forcewake for domains 0x%x\n", domains);
    
    // Write to forcewake request register
    if (domains & FORCEWAKE_GT_BIT) {
        writeReg(GEN12_FORCEWAKE_GT, 0x00010001); // Request GT domain
        
        // Wait for acknowledgment with timeout
        if (waitForRegisterBit(GEN12_FORCEWAKE_ACK_GT, 0x1, 0x1, 1000)) {
            IOLog("XePCI: GT forcewake acknowledged\n");
            forcewakeActive = true;
            return true;
        } else {
            IOLog("XePCI: WARNING - GT forcewake not acknowledged (timeout)\n");
            return false;
        }
    }
    
    return false;
}

void org_yourorg_XePCI::releaseForcewake(UInt32 domains) {
    if (!bar0Ptr || !forcewakeActive) return;
    
    IOLog("XePCI: Releasing forcewake for domains 0x%x\n", domains);
    
    if (domains & FORCEWAKE_GT_BIT) {
        writeReg(GEN12_FORCEWAKE_GT, 0x00010000); // Release GT domain
        forcewakeActive = false;
    }
}

void org_yourorg_XePCI::readGTConfiguration() {
    // From RESEARCH.md Section 4.2 - Read GT configuration
    IOLog("XePCI: === Reading GT Configuration ===\n");
    
    // Read thread status (EU configuration)
    UInt32 threadStatus = readReg(GEN12_GT_THREAD_STATUS);
    IOLog("XePCI: GT_THREAD_STATUS (0x%05x) = 0x%08x\n", 
          GEN12_GT_THREAD_STATUS, threadStatus);
    
    // Read DSS enable register
    UInt32 dssEnable = readReg(GEN12_GT_GEOMETRY_DSS_ENABLE);
    IOLog("XePCI: GT_GEOMETRY_DSS_ENABLE (0x%05x) = 0x%08x\n", 
          GEN12_GT_GEOMETRY_DSS_ENABLE, dssEnable);
    
    // Count enabled EUs (very simplified)
    int enabledDss = __builtin_popcount(dssEnable & 0xFFFF);
    IOLog("XePCI: Estimated enabled DSS units: %d\n", enabledDss);
}

void org_yourorg_XePCI::dumpRegisters() {
    if (!bar0Ptr) {
        IOLog("XePCI: no BAR0 pointer\n");
        return;
    }

    IOLog("XePCI: === Legacy Register Dump ===\n");
    
    // Read a few safe registers for sanity check
    UInt32 r0  = readReg(0x0000);
    UInt32 r1  = readReg(0x0100);
    UInt32 r2  = readReg(0x1000);

    IOLog("XePCI: reg[0x0000]=0x%08x\n", r0);
    IOLog("XePCI: reg[0x0100]=0x%08x\n", r1);
    IOLog("XePCI: reg[0x1000]=0x%08x\n", r2);
}

