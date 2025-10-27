#include "XePCI.hpp"

#define super IOService
OSDefineMetaClassAndStructors(org_yourorg_XePCI, IOService)

bool org_yourorg_XePCI::init(OSDictionary *props) {
    if (!super::init(props)) return false;
    pciDev = nullptr;
    bar0Map = nullptr;
    bar0Ptr = nullptr;
    scratchBuf = nullptr;
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

    // Optional: check vendor/device here and bump score
    uint32_t vendor = dev->configRead16(kIOPCIConfigVendorID);
    uint32_t device = dev->configRead16(kIOPCIConfigDeviceID);
    IOLog("XePCI: probe vendor=0x%04x device=0x%04x\n", vendor, device);

    // allow matching to continue; use default score
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

    // enable device memory & bus mastering
    pciDev->setMemoryEnable(true);
    pciDev->setIOEnable(true);
    pciDev->setBusMasterEnable(true);

    if (!mapBARs()) {
        IOLog("XePCI: failed to map BARs\n");
        return false;
    }

    // optional scratch buffer for BO prototyping
    scratchBuf = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryPhysicallyContiguous | kIOMemoryKernelUserShared,
        4096, // 4k
        0);
    if (scratchBuf) {
        IOLog("XePCI: scratch buffer allocated\n");
    }

    // quick register dump for sanity
    dumpRegisters();

    // publish service so user clients can open later
    registerService();

    IOLog("XePCI: started\n");
    return true;
}

void org_yourorg_XePCI::stop(IOService *provider) {
    IOLog("XePCI: stop\n");
    unmapBARs();
    super::stop(provider);
}

bool org_yourorg_XePCI::mapBARs() {
    if (!pciDev) return false;

    // BAR 0 typical for Intel GT MMIO (but check your device)
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

    IOLog("XePCI: BAR0 mapped at %p\n", (void*)bar0Ptr);
    return true;
}

void org_yourorg_XePCI::unmapBARs() {
    if (bar0Map) {
        bar0Map->release();
        bar0Map = nullptr;
        bar0Ptr = nullptr;
    }
}

void org_yourorg_XePCI::dumpRegisters() {
    if (!bar0Ptr) {
        IOLog("XePCI: no BAR0 pointer\n");
        return;
    }

    // Read a few registers that commonly exist / are harmless to probe:
    // WARNING: register offsets are device-specific — use Linux i915 as canonical source.
    // Here we just read offsets 0x0000, 0x0100, 0x1000 as a sanity check.
    UInt32 r0  = bar0Ptr[0x0 / 4];
    UInt32 r1  = bar0Ptr[0x100 / 4];
    UInt32 r2  = bar0Ptr[0x1000 / 4];

    IOLog("XePCI: reg[0x0000]=0x%08x\n", (unsigned)r0);
    IOLog("XePCI: reg[0x0100]=0x%08x\n", (unsigned)r1);
    IOLog("XePCI: reg[0x1000]=0x%08x\n", (unsigned)r2);

    // Example: write-then-read test to a non-critical register — **only if you know it's safe**.
    // Do NOT blindly write registers on unknown hardware. This code is commented for safety:
    //
    // UInt32 saved = bar0Ptr[0x10/4];
    // bar0Ptr[0x10/4] = saved ^ 0x1;
    // IOLog("XePCI: toggled reg[0x10] -> 0x%08x\n", (unsigned)bar0Ptr[0x10/4]);
    // bar0Ptr[0x10/4] = saved;
}

