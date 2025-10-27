// kexts/XeService.cpp — minimal attach + IOUserClient with 4 methods

#include "XeService.hpp"

#define super IOService
OSDefineMetaClassAndStructors(XeService, IOService)
OSDefineMetaClassAndStructors(XeUserClient, IOUserClient)

bool XeService::init(OSDictionary* prop) {
  if (!super::init(prop)) return false;
  return true;
}

IOService* XeService::probe(IOService* provider, SInt32* score) {
  auto dev = OSDynamicCast(IOPCIDevice, provider);
  if (!dev) return nullptr;
  uint16_t v = dev->configRead16(kIOPCIConfigVendorID);
  uint16_t d = dev->configRead16(kIOPCIConfigDeviceID);
  // Prefer 8086:A788 strongly
  if (v == 0x8086 && d == 0xA788) *score += 1000;
  return super::probe(provider, score);
}

bool XeService::start(IOService* provider) {
  if (!super::start(provider)) return false;
  pci = OSDynamicCast(IOPCIDevice, provider);
  if (!pci) return false;

  pci->setMemoryEnable(true);
  pci->setIOEnable(true);
  pci->setBusMasterEnable(true);

  bar0 = pci->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
  if (!bar0) return false;

  mmio = reinterpret_cast<volatile uint32_t*>(bar0->getVirtualAddress());
  if (!mmio) return false;

  IOLog("XeService: BAR0 @ %p (8086:A788)\n", (void*)mmio);

  // Safe sanity reads only
  uint32_t r0 = mmio[0x0000/4];
  uint32_t r1 = mmio[0x0100/4];
  uint32_t r2 = mmio[0x1000/4];
  IOLog("XeService: reg[0x0000]=0x%08x reg[0x0100]=0x%08x reg[0x1000]=0x%08x\n", r0, r1, r2);

  registerService(); // allow IOUserClient to open
  return true;
}

void XeService::stop(IOService* provider) {
  if (bar0) { bar0->release(); bar0=nullptr; mmio=nullptr; }
  super::stop(provider);
}

IOReturn XeService::newUserClient(task_t task, void* securityID, UInt32 type,
                                  OSDictionary* props, IOUserClient** handler) {
  auto* client = OSTypeAlloc(XeUserClient);
  if (!client) return kIOReturnNoMemory;
  if (!client->initWithTask(task, securityID, type)) { client->release(); return kIOReturnNoMemory; }
  if (!client->attach(this)) { client->release(); return kIOReturnNoResources; }
  if (!client->start(this)) { client->detach(this); client->release(); return kIOReturnError; }
  *handler = client;
  return kIOReturnSuccess;
}

// ===== UserClient side =====

bool XeUserClient::initWithTask(task_t owningTask, void*, UInt32) {
  if (!super::init()) return false;
  clientTask = owningTask;
  return true;
}

bool XeUserClient::start(IOService* provider) {
  if (!super::start(provider)) return false;
  providerSvc = OSDynamicCast(XeService, provider);
  return providerSvc != nullptr;
}

void XeUserClient::stop(IOService* provider) {
  super::stop(provider);
}

IOReturn XeUserClient::clientClose() {
  terminate();
  return kIOReturnSuccess;
}

IOReturn XeUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments* args,
                                      IOExternalMethodDispatch*, OSObject*, void*) {
  if (!providerSvc) return kIOReturnNotReady;

  switch (selector) {
    case kMethodCreateBuffer: {
      uint32_t bytes = args->scalarInputCount ? static_cast<uint32_t>(args->scalarInput[0]) : 4096;
      uint64_t cookie = 0;
      IOReturn kr = providerSvc->ucCreateBuffer(bytes, &cookie);
      if (kr == kIOReturnSuccess) {
        args->scalarOutput[0] = cookie;
        args->scalarOutputCount = 1;
      }
      return kr;
    }
    case kMethodSubmit: {
      // For now: submit a NOOP batch from kernel-owned buffer (stub)
      return providerSvc->ucSubmitNoop();
    }
    case kMethodWait: {
      uint32_t to = args->scalarInputCount ? static_cast<uint32_t>(args->scalarInput[0]) : 1000;
      return providerSvc->ucWait(to);
    }
    case kMethodReadReg: {
      uint32_t out[8] = {};
      uint32_t count = 8;
      IOReturn kr = providerSvc->ucReadRegs(count, out, &count);
      if (kr == kIOReturnSuccess) {
        for (uint32_t i=0; i<count; ++i) args->scalarOutput[i] = out[i];
        args->scalarOutputCount = count;
      }
      return kr;
    }
    default:
      return kIOReturnUnsupported;
  }
}

// ===== Provider methods used by the user client =====

IOReturn XeService::ucReadRegs(uint32_t count, uint32_t* out, uint32_t* outCount) {
  if (!mmio) return kIOReturnNotReady;
  uint32_t n = (count > 8) ? 8 : count;
  // Return a few fixed, safe offsets
  const uint32_t offs[8] = {0x0, 0x4, 0x100, 0x104, 0x10, 0x14, 0x1000, 0x1004};
  for (uint32_t i=0; i<n; ++i) out[i] = mmio[offs[i]/4];
  *outCount = n;
  return kIOReturnSuccess;
}

IOReturn XeService::ucCreateBuffer(uint32_t /*bytes*/, uint64_t* outCookie) {
  // Stub: return a dummy cookie; real impl should allocate IOBufferMemoryDescriptor and track it.
  *outCookie = 0xB0B0B0B0ULL;
  return kIOReturnSuccess;
}

IOReturn XeService::ucSubmitNoop() {
  // Stub: no real GPU submit yet — just return success to let the userspace flow work.
  // Replace with ring programming & MI_NOOP batch when ready.
  return kIOReturnSuccess;
}

IOReturn XeService::ucWait(uint32_t /*timeoutMs*/) {
  // Stub: pretend completion arrived.
  return kIOReturnSuccess;
}
