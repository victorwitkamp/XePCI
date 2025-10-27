// kexts/XeService.hpp — IOService + IOUserClient (minimal, safe, read-mostly)

#ifndef XESERVICE_HPP
#define XESERVICE_HPP

#include <IOKit/IOService.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOUserClient.h>

enum {
  kMethodCreateBuffer = 0,
  kMethodSubmit       = 1,
  kMethodWait         = 2,
  kMethodReadReg      = 3,
  kMethodGetDeviceInfo = 4,
  kMethodGetGTConfig  = 5,
};

// Ring Control Bits (duplicated from XePCI.hpp for XeService modularity)
#define RING_VALID                      (1 << 0)
#define RING_IDLE                       (1 << 2)

// Simple buffer tracking structure
struct XeBufferHandle {
  IOBufferMemoryDescriptor *mem;
  uint64_t cookie;
  uint32_t size;
};

class XeService;

class XeUserClient final : public IOUserClient {
  OSDeclareDefaultStructors(XeUserClient)
private:
  task_t      clientTask {nullptr};
  XeService*  providerSvc {nullptr};
public:
  bool initWithTask(task_t owningTask, void* securityID, UInt32 type) override;
  bool start(IOService* provider) override;
  void stop(IOService* provider) override;
  IOReturn clientClose() override;
  IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* args,
                          IOExternalMethodDispatch* disp, OSObject* target, void* ref) override;
};

class XeService final : public IOService {
  OSDeclareDefaultStructors(XeService)
private:
  IOPCIDevice *pci {nullptr};
  IOMemoryMap *bar0 {nullptr};
  volatile uint32_t *mmio {nullptr};
  
  // Device information
  uint16_t deviceId {0};
  uint16_t revisionId {0};
  
  // Buffer tracking (simple array for now)
  static constexpr int kMaxBuffers = 16;
  XeBufferHandle buffers[kMaxBuffers];
  int bufferCount {0};
  
  // Acceleration state
  bool accelReady {false};
  
public:
  bool init(OSDictionary* prop = nullptr) override;
  IOService* probe(IOService* provider, SInt32* score) override;
  bool start(IOService* provider) override;
  void stop(IOService* provider) override;

  // UserClient hook
  IOReturn newUserClient(task_t task, void* securityID, UInt32 type,
                         OSDictionary* properties, IOUserClient** handler) override;

  // Exposed helpers for the user client:
  IOReturn ucReadRegs(uint32_t count, uint32_t* out, uint32_t* outCount);
  IOReturn ucCreateBuffer(uint32_t bytes, uint64_t* outCookie);
  IOReturn ucSubmitNoop();
  IOReturn ucWait(uint32_t timeoutMs);
  IOReturn ucGetDeviceInfo(uint32_t* info, uint32_t* outCount);
  IOReturn ucGetGTConfig(uint32_t* config, uint32_t* outCount);

private:
  // Helper methods
  inline uint32_t readReg(uint32_t offset) {
    if (!mmio) return 0xFFFFFFFF;
    return mmio[offset / 4];
  }
  
  inline void writeReg(uint32_t offset, uint32_t value) {
    if (!mmio) return;
    mmio[offset / 4] = value;
  }
  
  bool initAcceleration();
  void cleanupBuffers();
};

#endif // XESERVICE_HPP
