#ifndef XESERVICE_HPP
#define XESERVICE_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOUserClient.h>
#include <libkern/c++/OSArray.h>   // MacKernelSDK C++ header path
#include "XeBootArgs.hpp"

// Central logging helper (Task 2). Declared here for use across kext.
void XeLog(const char* fmt, ...) __attribute__((format(printf,1,2)));

// IOUserClient selector IDs (keep in one place)
enum {
  kMethodCreateBuffer = 0,   // in:  [0]=bytes (u64)    out: [0]=cookie (u64)
  kMethodSubmit       = 1,   // in:  (none)             out: (none)  -- NOOP (stub)
  kMethodWait         = 2,   // in:  [0]=timeout_ms     out: (none)
  kMethodReadReg      = 3,   // in:  (none)             out: up to 8 u64 dwords
  kMethodGetGTConfig  = 4,   // in:  (none)             out: GT config (power wells, display, RC state)
  kMethodGetDisplayInfo = 5, // in:  (none)             out: Display pipe/plane info
};

// Maximum safe MMIO offset to prevent out-of-bounds access
// BAR0 is 16MB (0x1000000) based on lspci data
constexpr uint32_t kMaxSafeMMIOOffset = 0x00FFFFFF;

class XeUserClient; // fwd

class XeService final : public IOService {
  OSDeclareDefaultStructors(XeService)

private:
  // PCI / MMIO
  IOPCIDevice           *pci  {nullptr};
  IOMemoryMap           *bar0 {nullptr};
  volatile uint32_t     *mmio {nullptr};
  uint64_t              bar0Length {0};  // Track BAR0 size for bounds checking

  // Minimal BO registry (kernel-only cookies)
  OSArray               *m_boList {nullptr}; // holds IOBufferMemoryDescriptor*

  // Helpers
  IOBufferMemoryDescriptor* boFromCookie(uint64_t cookie);
  
  // Internal logging helpers for GPU state
  void logPowerState();
  void logDisplayState();

public:
  // IOService
  bool        init(OSDictionary* props = nullptr) override;
  IOService*  probe(IOService* provider, SInt32* score) override;
  bool        start(IOService* provider) override;
  void        stop(IOService* provider) override;
  void        free() override;

  // IOUserClient factory
  IOReturn    newUserClient(task_t task, void* secID, UInt32 type,
                            OSDictionary* props, IOUserClient** out) override;

  // Methods used by the user client
  IOReturn    ucCreateBuffer(uint32_t bytes, uint64_t* outCookie);
  IOReturn    ucSubmitNoop();                  // stub (will do MI_NOOP later)
  IOReturn    ucWait(uint32_t timeoutMs);      // stub
  IOReturn    ucReadRegs(uint32_t count, uint32_t* out, uint32_t* outCount);
  IOReturn    ucGetGTConfig(uint32_t* out, uint32_t* outCount);      // Read GT/power config
  IOReturn    ucGetDisplayInfo(uint32_t* out, uint32_t* outCount);   // Read display state

  // Safe MMIO accessors with bounds checking
  inline uint32_t readRegSafe(uint32_t off) const {
    // Multiple safety checks to prevent kernel panic
    if (!mmio) return 0xDEADBEEF;  // Sentinel value indicating no MMIO
    if (off > kMaxSafeMMIOOffset) return 0xBAD0FFFF;  // Offset out of range
    if (bar0Length > 0 && off >= bar0Length) return 0xBAD0FFFF;
    
    // Volatile read with memory barrier
    uint32_t val = mmio[off >> 2];
    OSSynchronizeIO();
    return val;
  }
  
  // Legacy accessor - calls safe version
  inline uint32_t readReg(uint32_t off) const {
    return readRegSafe(off);
  }
  
  inline void writeReg(uint32_t off, uint32_t val) {
    // Safety checks before any write
    if (!mmio) return;
    if (off > kMaxSafeMMIOOffset) return;
    if (bar0Length > 0 && off >= bar0Length) return;
    
    mmio[off >> 2] = val;
    OSSynchronizeIO();
  }
};

#endif // XESERVICE_HPP
