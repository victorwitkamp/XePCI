#ifndef XEPCI_HPP
#define XEPCI_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

class org_yourorg_XePCI : public IOService {
    OSDeclareDefaultStructors(org_yourorg_XePCI)

private:
    IOPCIDevice *pciDev;
    IOMemoryMap  *bar0Map;
    volatile UInt32 *bar0Ptr; // mapped MMIO pointer
    IOBufferMemoryDescriptor *scratchBuf;

public:
    virtual bool init(OSDictionary *prop = 0) override;
    virtual void free(void) override;
    virtual IOService* probe(IOService *provider, SInt32 *score) override;
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;

private:
    bool mapBARs();
    void unmapBARs();
    void dumpRegisters(); // simple read/write exercise
};

#endif // XEPCI_HPP
