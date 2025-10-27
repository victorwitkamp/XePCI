#ifndef XEUSERCLIENT_HPP
#define XEUSERCLIENT_HPP

#include <IOKit/IOUserClient.h>
#include "XeService.hpp"

class XeUserClient final : public IOUserClient {
  OSDeclareDefaultStructors(XeUserClient)

private:
  task_t     clientTask {nullptr};
  XeService* providerSvc {nullptr};

  // Static dispatchers used by IOExternalMethodDispatch
  static IOReturn sCreateBuffer(OSObject* target, void* ref, IOExternalMethodArguments* args);
  static IOReturn sSubmit      (OSObject* target, void* ref, IOExternalMethodArguments* args);
  static IOReturn sWait        (OSObject* target, void* ref, IOExternalMethodArguments* args);
  static IOReturn sReadRegs    (OSObject* target, void* ref, IOExternalMethodArguments* args);

  static const IOExternalMethodDispatch sMethods[];

public:
  // IOUserClient overrides
  bool     initWithTask(task_t owningTask, void* securityID, UInt32 type) override;
  bool     start(IOService* provider) override;
  IOReturn clientClose() override;

  IOReturn externalMethod(uint32_t selector,
                          IOExternalMethodArguments* args,
                          IOExternalMethodDispatch* dispatch,
                          OSObject* target, void* ref) override;
};

// Factory used by XeService::newUserClient()
extern "C" IOUserClient* XeCreateUserClient(XeService* provider, task_t task, void* secID, UInt32 type);

#endif // XEUSERCLIENT_HPP
