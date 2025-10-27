#include "XeUserClient.hpp"
#include <IOKit/IOLib.h>

#define super IOUserClient
OSDefineMetaClassAndStructors(XeUserClient, IOUserClient)

// --- Method table ---
// Each entry: { function, scalarInCnt, structInSize, scalarOutCnt, structOutSize }
const IOExternalMethodDispatch XeUserClient::sMethods[] = {
  /* 0 kMethodCreateBuffer */ { (IOExternalMethodAction)&XeUserClient::sCreateBuffer, 1, 0, 1, 0 },
  /* 1 kMethodSubmit       */ { (IOExternalMethodAction)&XeUserClient::sSubmit,       0, 0, 0, 0 },
  /* 2 kMethodWait         */ { (IOExternalMethodAction)&XeUserClient::sWait,         1, 0, 0, 0 },
  /* 3 kMethodReadReg      */ { (IOExternalMethodAction)&XeUserClient::sReadRegs,     0, 0, 8, 0 },
};

bool XeUserClient::initWithTask(task_t owningTask, void*, UInt32) {
  if (!super::init()) return false;
  clientTask = owningTask;
  return true;
}

bool XeUserClient::start(IOService* provider) {
  if (!super::start(provider)) return false;
  providerSvc = OSDynamicCast(XeService, provider);
  if (!providerSvc) return false;
  return true;
}

IOReturn XeUserClient::clientClose() {
  // Userspace closed the connection
  terminate();
  return kIOReturnSuccess;
}

IOReturn XeUserClient::externalMethod(uint32_t selector,
                                      IOExternalMethodArguments* args,
                                      IOExternalMethodDispatch* dispatch,
                                      OSObject* target, void* ref) {
  if (selector >= (uint32_t)(sizeof(sMethods) / sizeof(sMethods[0])))
    return kIOReturnUnsupported;

  // Route to our dispatch table; IOUserClient will validate arg counts.
  dispatch = const_cast<IOExternalMethodDispatch*>(&sMethods[selector]);
  target   = this;
  return super::externalMethod(selector, args, dispatch, target, ref);
}

// -------------------- Static dispatchers --------------------

IOReturn XeUserClient::sCreateBuffer(OSObject* t, void*, IOExternalMethodArguments* a) {
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) return kIOReturnNotReady;

  // Inputs are 64-bit scalars; we downcast carefully to 32-bit bytes if needed.
  uint32_t bytes = a->scalarInputCount >= 1 ? (uint32_t)a->scalarInput[0] : 4096;

  uint64_t cookie = 0;
  IOReturn kr = self->providerSvc->ucCreateBuffer(bytes, &cookie);
  if (kr == kIOReturnSuccess) {
    a->scalarOutput[0]  = cookie;
    a->scalarOutputCount = 1;
  }
  return kr;
}

IOReturn XeUserClient::sSubmit(OSObject* t, void*, IOExternalMethodArguments* a) {
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) return kIOReturnNotReady;

  // For now this is a stub: it will submit NOOP once rings are implemented.
  return self->providerSvc->ucSubmitNoop();
}

IOReturn XeUserClient::sWait(OSObject* t, void*, IOExternalMethodArguments* a) {
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) return kIOReturnNotReady;

  uint32_t timeoutMs = a->scalarInputCount >= 1 ? (uint32_t)a->scalarInput[0] : 1000;
  return self->providerSvc->ucWait(timeoutMs);
}

IOReturn XeUserClient::sReadRegs(OSObject* t, void*, IOExternalMethodArguments* a) {
  auto self = OSDynamicCast(XeUserClient, t);
  if (!self || !self->providerSvc) return kIOReturnNotReady;

  // Ask provider for up to 8 safe dwords; provider decides exact offsets.
  uint32_t tmp[8] = {};
  uint32_t count  = 8;
  IOReturn kr = self->providerSvc->ucReadRegs(count, tmp, &count);
  if (kr == kIOReturnSuccess) {
    // Return them as 64-bit scalars (IOKit ABI); zero-extend.
    for (uint32_t i = 0; i < count; ++i) a->scalarOutput[i] = (uint64_t)tmp[i];
    a->scalarOutputCount = count;
  }
  return kr;
}

// Factory used by XeService::newUserClient()
extern "C" IOUserClient* XeCreateUserClient(XeService* provider, task_t task, void* secID, UInt32 type) {
  auto* uc = OSTypeAlloc(XeUserClient);
  if (!uc) return nullptr;
  if (!uc->initWithTask(task, secID, type)) { uc->release(); return nullptr; }
  if (!uc->attach(provider))                { uc->release(); return nullptr; }
  if (!uc->start(provider))                 { uc->detach(provider); uc->release(); return nullptr; }
  return uc;
}
