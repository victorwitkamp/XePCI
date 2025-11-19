#include "XeService.hpp"
#include "xe_hw_offsets.hpp"
#include "XeGGTT.hpp"
#include "XeCommandStream.hpp"

#include <IOKit/IOLib.h>            // IOLog, kprintf
#include <kern/debug.h>              // panic

#include <IOKit/IOBufferMemoryDescriptor.h> // for IOBufferMemoryDescriptor

// Factory from XeUserClient.cpp
extern "C" IOUserClient* XeCreateUserClient(class XeService* provider,
                                            task_t task, void* secID, UInt32 type);

#define super IOService

// Mirror logs to both system log and on-screen (where possible)
#define XE_LOG(fmt, ...) do { \
  IOLog(fmt, ##__VA_ARGS__); \
  kprintf(fmt, ##__VA_ARGS__); \
} while (0)
OSDefineMetaClassAndStructors(XeService, IOService)

bool XeService::init(OSDictionary* props) {
  if (!super::init(props)) return false;
  return true;
}

IOService* XeService::probe(IOService* provider, SInt32* score) {
  auto dev = OSDynamicCast(IOPCIDevice, provider);
  if (!dev) return nullptr;

  uint16_t v = dev->configRead16(kIOPCIConfigVendorID);
  uint16_t d = dev->configRead16(kIOPCIConfigDeviceID);
  if (v == 0x8086 && d == 0xA788) *score += 1000;  // prefer our device
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

  // Simple read-only sanity (no GT writes yet)
  uint16_t v = pci->configRead16(kIOPCIConfigVendorID);
  uint16_t d = pci->configRead16(kIOPCIConfigDeviceID);
  uint8_t  r = pci->configRead8(kIOPCIConfigRevisionID);

  uint32_t r0 = readReg(0x0000);
  uint32_t r1 = readReg(0x0100);
  uint32_t r2 = readReg(0x1000);

  XE_LOG("XeService: attach %04x:%04x rev 0x%02x BAR0=%p regs: [0]=0x%08x [0x100]=0x%08x [0x1000]=0x%08x\n",
    v, d, r, (void*)mmio, r0, r1, r2);

  // --- New: lightweight HW probes (safe reads) ---
  XeGGTT::probe(mmio);
  {
    XeCommandStream cs(mmio);
    cs.logRcs0State(); // logs head/tail/ctl/gfx; no writes
  }

  // Minimal BO registry
  m_boList = OSArray::withCapacity(8);
  if (!m_boList) return false;

  registerService();  // allow IOUserClient to open us

  // Intentionally panic after completing the last startup action,
  // so all logs above are flushed and visible on-screen during the panic.
  // XE_LOG("XeService: intentional panic for bring-up diagnostics\n");
  // panic("XePCI intentional panic: vendor=%04x device=%04x rev=0x%02x regs:[0]=0x%08x [0x100]=0x%08x [0x1000]=0x%08x",
  //   v, d, r, r0, r1, r2);

  return true; // unreachable
}

void XeService::stop(IOService* provider) {
  // Release any BOs left around
  if (m_boList) {
    for (unsigned i = 0; i < m_boList->getCount(); ++i) {
      auto *md = OSDynamicCast(IOBufferMemoryDescriptor, m_boList->getObject(i));
      if (md) md->release();
    }
    m_boList->release();
    m_boList = nullptr;
  }

  if (bar0) { bar0->release(); bar0 = nullptr; mmio = nullptr; }
  if (pci)  { pci->setBusMasterEnable(false); }
  super::stop(provider);
}

void XeService::free() {
  super::free();
}

IOReturn XeService::newUserClient(task_t task, void* secID, UInt32 type,
                                  OSDictionary* props, IOUserClient** out) {
  (void)props; // silence unused warning in CI
  IOUserClient* uc = XeCreateUserClient(this, task, secID, type);
  if (!uc) return kIOReturnNoResources;
  *out = uc;
  return kIOReturnSuccess;
}

// -------------------------- BO helpers --------------------------

IOBufferMemoryDescriptor* XeService::boFromCookie(uint64_t cookie) {
  if (!cookie || !m_boList) return nullptr;
  uint64_t idx = cookie - 1;
  if (idx >= m_boList->getCount()) return nullptr;
  return OSDynamicCast(IOBufferMemoryDescriptor, m_boList->getObject((unsigned)idx));
}

// ----------------------- UserClient methods ---------------------

IOReturn XeService::ucCreateBuffer(uint32_t bytes, uint64_t* outCookie) {
  if (!m_boList) return kIOReturnNotReady;

  // Page-align (4 KiB)
  uint32_t sz = (bytes + 0xFFFu) & ~0xFFFu;

  auto *md = IOBufferMemoryDescriptor::withOptions(
      kIOMemoryKernelUserShared | kIODirectionInOut,
      sz, page_size);

  if (!md) return kIOReturnNoResources;

  if (!m_boList->setObject(md)) {
    md->release();
    return kIOReturnNoResources;
  }

  uint64_t cookie = m_boList->getCount(); // 1..N
  *outCookie = cookie;

  XE_LOG("XeService: BO created cookie=%llu size=%u\n",
    (unsigned long long)cookie, sz);

  return kIOReturnSuccess;
}

IOReturn XeService::ucSubmitNoop() {
  if (!mmio) return kIOReturnNotReady;

  // Allocate a tiny 4K batch (kernel-user shared so we can write commands)
  auto *md = IOBufferMemoryDescriptor::withOptions(
      kIOMemoryKernelUserShared | kIODirectionInOut, 4096, page_size);
  if (!md) return kIOReturnNoResources;

  XeCommandStream cs(mmio);
  IOReturn kr = cs.submitNoop(md); // currently read-only path (no tail bump yet)
  md->release();
  return kr;
}

IOReturn XeService::ucWait(uint32_t /*timeoutMs*/) {
  // Stub: pretend completion (will become HWSP poll later)
  return kIOReturnSuccess;
}

IOReturn XeService::ucReadRegs(uint32_t count, uint32_t* out, uint32_t* outCount) {
  if (!mmio || !out || !outCount) return kIOReturnNotReady;

  // Fixed allow-list of safe, read-only dwords (global + RCS0 ring + GGTT ctl)
  static const uint32_t kSafeOffs[] = {
    // global sampler
    0x0000, 0x0004, 0x0010, 0x0014,
    0x0100, 0x0104,
    0x1000, 0x1004,

    // ring regs (relative constants expanded to absolute in xe_hw_offsets.hpp)
    XeHW::RCS0_RING_HEAD,
    XeHW::RCS0_RING_TAIL,
    XeHW::RCS0_RING_CTL,

    // misc
    XeHW::GFX_MODE,
    XeHW::PGTBL_CTL,
  };

  uint32_t avail = static_cast<uint32_t>(sizeof(kSafeOffs) / sizeof(kSafeOffs[0]));
  uint32_t n = (count < avail) ? count : avail;

  for (uint32_t i = 0; i < n; ++i)
    out[i] = readReg(kSafeOffs[i]);

  *outCount = n;
  return kIOReturnSuccess;
}
