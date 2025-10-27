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
};

#endif // XESERVICE_HPP
