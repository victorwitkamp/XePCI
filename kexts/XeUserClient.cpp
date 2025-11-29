#include "XeUserClient.hpp"
#include <IOKit/IOLib.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(XeUserClient, IOUserClient)

// Forward declaration for XeLog
extern void XeLog(const char* fmt, ...) __attribute__((format(printf,1,2)));

// Each entry: { function, scalarInCnt, structInSize, scalarOutCnt, structOutSize }
const IOExternalMethodDispatch XeUserClient::sMethods[] = {
  /* 0 kMethodCreateBuffer  */ { (IOExternalMethodAction)&XeUserClient::sCreateBuffer,   1, 0, 1, 0 },
  /* 1 kMethodSubmit        */ { (IOExternalMethodAction)&XeUserClient::sSubmit,         0, 0, 0, 0 },
  /* 2 kMethodWait          */ { (IOExternalMethodAction)&XeUserClient::sWait,           1, 0, 0, 0 },
  /* 3 kMethodReadReg       */ { (IOExternalMethodAction)&XeUserClient::sReadRegs,       0, 0, 8, 0 },
  /* 4 kMethodGetGTConfig   */ { (IOExternalMethodAction)&XeUserClient::sGetGTConfig,    0, 0, 8, 0 },
  /* 5 kMethodGetDisplayInfo*/ { (IOExternalMethodAction)&XeUserClient::sGetDisplayInfo, 0, 0, 8, 0 },
};

bool XeUserClient::initWithTask(task_t owningTask, void*, UInt32) {
  XeLog("XeUserClient::initWithTask\n");
  if (!super::init()) {
    XeLog("XeUserClient::initWithTask: super::init failed\n");
    return false;
  }
  clientTask = owningTask;
  return true;
}

bool XeUserClient::start(IOService* provider) {
  XeLog("XeUserClient::start\n");
  if (!super::start(provider)) {
    XeLog("XeUserClient::start: super::start failed\n");
    return false;
  }
  providerSvc = OSDynamicCast(XeService, provider);
  if (!providerSvc) {
    XeLog("XeUserClient::start: ERROR - provider is not XeService\n");
    return false;
  }
  XeLog("XeUserClient::start: SUCCESS\n");
  return true;
}

IOReturn XeUserClient::clientClose() {
  XeLog("XeUserClient::clientClose\n");
  terminate();
  return kIOReturnSuccess;
}

IOReturn XeUserClient::externalMethod(uint32_t selector,
                                      IOExternalMethodArguments* args,
                                      IOExternalMethodDispatch* dispatch,
                                      OSObject* target, void* ref) {
  // Safety: validate selector
  uint32_t methodCount = (uint32_t)(sizeof(sMethods) / sizeof(sMethods[0]));
  if (selector >= methodCount) {
    XeLog("XeUserClient::externalMethod: invalid selector %u (max %u)\n", 
          selector, methodCount - 1);
    return kIOReturnUnsupported;
  }

  // Safety: validate args pointer
  if (!args) {
    XeLog("XeUserClient::externalMethod: ERROR - args is null\n");
    return kIOReturnBadArgument;
  }

  XeLog("XeUserClient::externalMethod: selector=%u\n", selector);
  
  dispatch = const_cast<IOExternalMethodDispatch*>(&sMethods[selector]);
  target   = this;
  return super::externalMethod(selector, args, dispatch, target, ref);
}

// -------------------- Static dispatchers --------------------

IOReturn XeUserClient::sCreateBuffer(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sCreateBuffer\n");
  
  // Safety: validate all pointers
  if (!t || !a) {
    XeLog("XeUserClient::sCreateBuffer: ERROR - null pointer\n");
    return kIOReturnBadArgument;
  }
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self) {
    XeLog("XeUserClient::sCreateBuffer: ERROR - invalid target\n");
    return kIOReturnBadArgument;
  }
  if (!self->providerSvc) {
    XeLog("XeUserClient::sCreateBuffer: ERROR - no provider\n");
    return kIOReturnNotReady;
  }

  // Scalar inputs are 64-bit; clamp safely to 32-bit size
  uint32_t bytes = 4096;
  if (a->scalarInputCount >= 1) {
    bytes = (uint32_t)a->scalarInput[0];
    // Clamp to reasonable limits (1 byte to 64MB)
    if (bytes == 0) bytes = 4096;
    if (bytes > 64 * 1024 * 1024) bytes = 64 * 1024 * 1024;
  }

  uint64_t cookie = 0;
  IOReturn kr = self->providerSvc->ucCreateBuffer(bytes, &cookie);
  if (kr == kIOReturnSuccess && a->scalarOutputCount >= 1) {
    a->scalarOutput[0] = cookie;
    a->scalarOutputCount = 1;
  }
  return kr;
}

IOReturn XeUserClient::sSubmit(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sSubmit\n");
  
  if (!t) return kIOReturnBadArgument;
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) {
    XeLog("XeUserClient::sSubmit: ERROR - not ready\n");
    return kIOReturnNotReady;
  }
  return self->providerSvc->ucSubmitNoop();
}

IOReturn XeUserClient::sWait(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sWait\n");
  
  if (!t || !a) return kIOReturnBadArgument;
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) {
    XeLog("XeUserClient::sWait: ERROR - not ready\n");
    return kIOReturnNotReady;
  }

  uint32_t timeoutMs = 1000;
  if (a->scalarInputCount >= 1) {
    timeoutMs = (uint32_t)a->scalarInput[0];
    // Clamp to reasonable limits (1ms to 60s)
    if (timeoutMs == 0) timeoutMs = 1000;
    if (timeoutMs > 60000) timeoutMs = 60000;
  }
  return self->providerSvc->ucWait(timeoutMs);
}

IOReturn XeUserClient::sReadRegs(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sReadRegs\n");
  
  if (!t || !a) return kIOReturnBadArgument;
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) {
    XeLog("XeUserClient::sReadRegs: ERROR - not ready\n");
    return kIOReturnNotReady;
  }

  uint32_t tmp[8] = {};
  uint32_t n = 8;
  IOReturn kr = self->providerSvc->ucReadRegs(n, tmp, &n);
  if (kr == kIOReturnSuccess) {
    uint32_t outMax = (a->scalarOutputCount < n) ? a->scalarOutputCount : n;
    for (uint32_t i = 0; i < outMax; ++i) {
      a->scalarOutput[i] = (uint64_t)tmp[i];
    }
    a->scalarOutputCount = outMax;
  }
  return kr;
}

IOReturn XeUserClient::sGetGTConfig(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sGetGTConfig\n");
  
  if (!t || !a) return kIOReturnBadArgument;
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) {
    XeLog("XeUserClient::sGetGTConfig: ERROR - not ready\n");
    return kIOReturnNotReady;
  }

  uint32_t tmp[8] = {};
  uint32_t n = 8;
  IOReturn kr = self->providerSvc->ucGetGTConfig(tmp, &n);
  if (kr == kIOReturnSuccess) {
    uint32_t outMax = (a->scalarOutputCount < n) ? a->scalarOutputCount : n;
    for (uint32_t i = 0; i < outMax; ++i) {
      a->scalarOutput[i] = (uint64_t)tmp[i];
    }
    a->scalarOutputCount = outMax;
  }
  return kr;
}

IOReturn XeUserClient::sGetDisplayInfo(OSObject* t, void*, IOExternalMethodArguments* a) {
  XeLog("XeUserClient::sGetDisplayInfo\n");
  
  if (!t || !a) return kIOReturnBadArgument;
  
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) {
    XeLog("XeUserClient::sGetDisplayInfo: ERROR - not ready\n");
    return kIOReturnNotReady;
  }

  uint32_t tmp[8] = {};
  uint32_t n = 8;
  IOReturn kr = self->providerSvc->ucGetDisplayInfo(tmp, &n);
  if (kr == kIOReturnSuccess) {
    uint32_t outMax = (a->scalarOutputCount < n) ? a->scalarOutputCount : n;
    for (uint32_t i = 0; i < outMax; ++i) {
      a->scalarOutput[i] = (uint64_t)tmp[i];
    }
    a->scalarOutputCount = outMax;
  }
  return kr;
}

// Factory used by XeService::newUserClient
extern "C" IOUserClient* XeCreateUserClient(XeService* provider, task_t task, void* secID, UInt32 type) {
  XeLog("XeCreateUserClient: creating user client\n");
  
  // Safety: validate provider
  if (!provider) {
    XeLog("XeCreateUserClient: ERROR - provider is null\n");
    return nullptr;
  }
  
  auto* uc = OSTypeAlloc(XeUserClient);
  if (!uc) {
    XeLog("XeCreateUserClient: ERROR - allocation failed\n");
    return nullptr;
  }
  
  if (!uc->initWithTask(task, secID, type)) { 
    XeLog("XeCreateUserClient: ERROR - initWithTask failed\n");
    uc->release(); 
    return nullptr; 
  }
  
  if (!uc->attach(provider)) { 
    XeLog("XeCreateUserClient: ERROR - attach failed\n");
    uc->release(); 
    return nullptr; 
  }
  
  if (!uc->start(provider)) { 
    XeLog("XeCreateUserClient: ERROR - start failed\n");
    uc->detach(provider); 
    uc->release(); 
    return nullptr; 
  }
  
  XeLog("XeCreateUserClient: SUCCESS\n");
  return uc;
}
