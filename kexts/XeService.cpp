#include "XeService.hpp"
#include "xe_hw_offsets.hpp"
#include "XeGGTT.hpp"
#include "XeCommandStream.hpp"
#include "XeBootArgs.hpp"
#include "ForcewakeGuard.hpp"

#include <IOKit/IOLib.h>            // IOLog, kprintf
#include <kern/debug.h>              // panic

#include <IOKit/IOBufferMemoryDescriptor.h> // for IOBufferMemoryDescriptor

#include <stdarg.h>   // va_list, va_start, va_end

// Factory from XeUserClient.cpp
extern "C" IOUserClient* XeCreateUserClient(class XeService* provider,
                                            task_t task, void* secID, UInt32 type);

#define super IOService

// Central logging helper implementation (Task 2)
// Uses IOLog directly with format string forwarding (kernel-safe)
void XeLog(const char* fmt, ...) {
  // IOLog supports printf-style formatting directly
  // We use a simple wrapper that calls IOLog and kprintf
  va_list ap;
  va_start(ap, fmt);
  // IOLogv is available in kernel for variadic logging
  IOLogv(fmt, ap);
  va_end(ap);
}
OSDefineMetaClassAndStructors(XeService, IOService)

bool XeService::init(OSDictionary* props) {
  XeLog("XePCI: ======== GPU BRING-UP START ========\n");
  XeLog("XePCI: Step 1/7: Initializing XeService\n");
  
  if (!super::init(props)) {
    XeLog("XePCI: ERROR - super::init failed\n");
    return false;
  }
  
  // Parse boot args early
  XeParseBootArgs();
  XeLog("XePCI: Boot flags parsed: verbose=%d noforcewake=%d nocs=%d strictsafe=%d\n",
        gXeBoot.verbose, gXeBoot.disableForcewake, gXeBoot.disableCommandStream, gXeBoot.strictSafe);
  
  XeLog("XePCI: Step 1/7: COMPLETE - XeService initialized\n");
  return true;
}

IOService* XeService::probe(IOService* provider, SInt32* score) {
  XeLog("XePCI: Step 2/7: Probing PCI device\n");
  
  auto dev = OSDynamicCast(IOPCIDevice, provider);
  if (!dev) {
    XeLog("XePCI: ERROR - Provider is not IOPCIDevice\n");
    return nullptr;
  }

  uint16_t v = dev->configRead16(kIOPCIConfigVendorID);
  uint16_t d = dev->configRead16(kIOPCIConfigDeviceID);
  uint8_t rev = dev->configRead8(kIOPCIConfigRevisionID);
  
  XeLog("XePCI: PCI Device: Vendor=0x%04x Device=0x%04x Revision=0x%02x\n", v, d, rev);
  
  if (v == 0x8086 && d == 0xA788) {
    *score += 1000;
    XeLog("XePCI: MATCH - Intel Raptor Lake iGPU (8086:A788) detected, score boosted\n");
  } else {
    XeLog("XePCI: Device mismatch - expected 8086:A788, got %04x:%04x\n", v, d);
  }
  
  XeLog("XePCI: Step 2/7: COMPLETE - Probe finished with score=%d\n", *score);
  return super::probe(provider, score);
}

bool XeService::start(IOService* provider) {
  XeLog("XePCI: Step 3/7: Starting service and mapping BAR0\n");
  
  if (!super::start(provider)) {
    XeLog("XePCI: ERROR - super::start failed\n");
    return false;
  }

  pci = OSDynamicCast(IOPCIDevice, provider);
  if (!pci) {
    XeLog("XePCI: ERROR - provider is not IOPCIDevice\n");
    return false;
  }

  // Enable PCI features with error handling
  XeLog("XePCI: Enabling PCI memory space, I/O space, and bus mastering\n");
  pci->setMemoryEnable(true);
  pci->setIOEnable(true);
  pci->setBusMasterEnable(true);

  // Map BAR0 (GTTMMADR - Graphics Translation Table Memory Mapped Address Range)
  XeLog("XePCI: Mapping BAR0 (GTTMMADR)...\n");
  bar0 = pci->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
  if (!bar0) {
    XeLog("XePCI: ERROR - failed to map BAR0\n");
    return false;
  }

  // Store BAR0 length for bounds checking
  bar0Length = bar0->getLength();
  if (bar0Length == 0) {
    XeLog("XePCI: WARNING - BAR0 length is 0, using default max offset\n");
    bar0Length = kMaxSafeMMIOOffset + 1;
  }

  mmio = reinterpret_cast<volatile uint32_t*>(bar0->getVirtualAddress());
  if (!mmio) {
    XeLog("XePCI: ERROR - BAR0 virtual address is null\n");
    bar0->release();
    bar0 = nullptr;
    return false;
  }
  
  XeLog("XePCI: BAR0 mapped at virtual address %p, size=%llu bytes\n", 
        (void*)mmio, (unsigned long long)bar0Length);
  XeLog("XePCI: Step 3/7: COMPLETE - BAR0 mapped successfully\n");

  // Step 4: Read PCI configuration
  XeLog("XePCI: Step 4/7: Reading PCI configuration\n");
  uint16_t v = pci->configRead16(kIOPCIConfigVendorID);
  uint16_t d = pci->configRead16(kIOPCIConfigDeviceID);
  uint8_t  r = pci->configRead8(kIOPCIConfigRevisionID);
  uint16_t subsysVendor = pci->configRead16(kIOPCIConfigSubSystemVendorID);
  uint16_t subsysDevice = pci->configRead16(kIOPCIConfigSubSystemID);
  
  XeLog("XePCI: Vendor=0x%04x Device=0x%04x Revision=0x%02x\n", v, d, r);
  XeLog("XePCI: Subsystem Vendor=0x%04x Subsystem Device=0x%04x\n", subsysVendor, subsysDevice);
  XeLog("XePCI: Step 4/7: COMPLETE - PCI config read\n");

  // Step 5: Read initial MMIO registers (safe offsets only)
  XeLog("XePCI: Step 5/7: Reading initial MMIO registers\n");
  uint32_t reg0 = mmio[0x0 >> 2];
  uint32_t reg4 = mmio[0x4 >> 2];
  uint32_t reg10 = mmio[0x10 >> 2];
  uint32_t reg100 = mmio[0x100 >> 2];
  
  XeLog("XePCI: MMIO[0x0000]=0x%08x MMIO[0x0004]=0x%08x\n", reg0, reg4);
  XeLog("XePCI: MMIO[0x0010]=0x%08x MMIO[0x0100]=0x%08x\n", reg10, reg100);
  XeLog("XePCI: Step 5/7: COMPLETE - Initial MMIO read\n");

  // Step 6: Probe power state and GT configuration (if not in strict safe mode)
  XeLog("XePCI: Step 6/7: Probing GPU power and configuration\n");
  if (gXeBoot.strictSafe) {
    XeLog("XePCI: SKIP - strictsafe mode active, skipping advanced probing\n");
  } else {
    // Read power management state
    logPowerState();
    
    // Probe GGTT configuration
    XeLog("XePCI: Probing GGTT configuration...\n");
    XeGGTT::probe(mmio);
    
    // Read display pipeline state
    logDisplayState();
  }
  XeLog("XePCI: Step 6/7: COMPLETE - GPU probing finished\n");

  // Step 7: Initialize buffer object registry and register service
  XeLog("XePCI: Step 7/7: Registering service\n");
  m_boList = OSArray::withCapacity(8);
  if (!m_boList) {
    XeLog("XePCI: ERROR - failed to allocate BO list\n");
    return false;
  }
  
  XeLog("XePCI: BO registry initialized with capacity=8\n");
  registerService();
  
  XeLog("XePCI: Step 7/7: COMPLETE - Service registered\n");
  XeLog("XePCI: ======== GPU BRING-UP COMPLETE ========\n");
  XeLog("XePCI: Summary: Device %04x:%04x rev 0x%02x ready (strictsafe=%d)\n", 
        v, d, r, gXeBoot.strictSafe);
  
  return true;
}

// Log power management state using documented registers
void XeService::logPowerState() {
  XeLog("XePCI: --- Power State ---\n");
  
  uint32_t pwrWell1 = readReg(XeHW::HSW_PWR_WELL_CTL1);
  uint32_t pwrWell2 = readReg(XeHW::HSW_PWR_WELL_CTL2);
  uint32_t pwrWell3 = readReg(XeHW::HSW_PWR_WELL_CTL3);
  uint32_t pwrWell4 = readReg(XeHW::HSW_PWR_WELL_CTL4);
  
  XeLog("XePCI: PWR_WELL_CTL1=0x%08x (BIOS)\n", pwrWell1);
  XeLog("XePCI: PWR_WELL_CTL2=0x%08x (Driver)\n", pwrWell2);
  XeLog("XePCI: PWR_WELL_CTL3=0x%08x (KVM)\n", pwrWell3);
  XeLog("XePCI: PWR_WELL_CTL4=0x%08x (Debug)\n", pwrWell4);
  
  uint32_t rcState = readReg(XeHW::GEN6_RC_STATE);
  uint32_t rcControl = readReg(XeHW::GEN6_RC_CONTROL);
  uint32_t rpControl = readReg(XeHW::GEN6_RP_CONTROL);
  
  XeLog("XePCI: RC_STATE=0x%08x RC_CONTROL=0x%08x RP_CONTROL=0x%08x\n",
        rcState, rcControl, rpControl);
  
  uint32_t fwAck = readReg(XeHW::FORCEWAKE_ACK);
  uint32_t pmIntMsk = readReg(XeHW::GEN6_PMINTRMSK);
  uint32_t rc6Res = readReg(XeHW::RC6_RESIDENCY_TIME);
  
  XeLog("XePCI: FORCEWAKE_ACK=0x%08x PMINTRMSK=0x%08x RC6_RESIDENCY=0x%08x\n",
        fwAck, pmIntMsk, rc6Res);
  
  XeLog("XePCI: --- Power State End ---\n");
}

// Log display pipeline state using documented registers
void XeService::logDisplayState() {
  XeLog("XePCI: --- Display State ---\n");
  
  // Pipe A configuration
  uint32_t pipeConf = readReg(XeHW::PIPEACONF);
  uint32_t ddiFuncCtl = readReg(XeHW::PIPE_DDI_FUNC_CTL_A);
  uint32_t ddiBufCtl = readReg(XeHW::DDI_BUF_CTL_A);
  
  bool pipeEnabled = (pipeConf & 0x80000000) != 0;
  bool pipeActive = (pipeConf & 0x40000000) != 0;
  bool ddiEnabled = (ddiFuncCtl & 0x80000000) != 0;
  
  XeLog("XePCI: PIPEACONF=0x%08x (enabled=%d active=%d)\n", 
        pipeConf, pipeEnabled, pipeActive);
  XeLog("XePCI: PIPE_DDI_FUNC_CTL_A=0x%08x (enabled=%d)\n", ddiFuncCtl, ddiEnabled);
  XeLog("XePCI: DDI_BUF_CTL_A=0x%08x\n", ddiBufCtl);
  
  // Timing registers
  uint32_t htotal = readReg(XeHW::HTOTAL_A);
  uint32_t vtotal = readReg(XeHW::VTOTAL_A);
  uint32_t pipeSrc = readReg(XeHW::PIPEASRC);
  
  uint32_t hActive = (htotal & 0xFFFF) + 1;
  uint32_t hTotal = ((htotal >> 16) & 0xFFFF) + 1;
  uint32_t vActive = (vtotal & 0xFFFF) + 1;
  uint32_t vTotal = ((vtotal >> 16) & 0xFFFF) + 1;
  uint32_t srcWidth = ((pipeSrc >> 16) & 0xFFFF) + 1;
  uint32_t srcHeight = (pipeSrc & 0xFFFF) + 1;
  
  XeLog("XePCI: HTOTAL_A=0x%08x (active=%u total=%u)\n", htotal, hActive, hTotal);
  XeLog("XePCI: VTOTAL_A=0x%08x (active=%u total=%u)\n", vtotal, vActive, vTotal);
  XeLog("XePCI: PIPEASRC=0x%08x (width=%u height=%u)\n", pipeSrc, srcWidth, srcHeight);
  
  // Plane control
  uint32_t dspaCntr = readReg(XeHW::DSPACNTR);
  uint32_t dspaStride = readReg(XeHW::DSPASTRIDE);
  uint32_t dspaSurf = readReg(XeHW::DSPASURF);
  
  bool planeEnabled = (dspaCntr & 0x80000000) != 0;
  XeLog("XePCI: DSPACNTR=0x%08x (enabled=%d)\n", dspaCntr, planeEnabled);
  XeLog("XePCI: DSPASTRIDE=0x%08x DSPASURF=0x%08x\n", dspaStride, dspaSurf);
  
  // Panel power status
  uint32_t ppStatus = readReg(XeHW::PCH_PP_STATUS);
  uint32_t ppControl = readReg(XeHW::PCH_PP_CONTROL);
  
  bool panelOn = (ppStatus & 0x80000000) != 0;
  XeLog("XePCI: PCH_PP_STATUS=0x%08x (panel=%s)\n", ppStatus, panelOn ? "ON" : "OFF");
  XeLog("XePCI: PCH_PP_CONTROL=0x%08x\n", ppControl);
  
  // Backlight
  uint32_t blcPwm1 = readReg(XeHW::BLC_PWM_PCH_CTL1);
  uint32_t blcPwm2 = readReg(XeHW::BLC_PWM_PCH_CTL2);
  
  bool blEnabled = (blcPwm1 & 0x80000000) != 0;
  XeLog("XePCI: BLC_PWM_PCH_CTL1=0x%08x (enabled=%d)\n", blcPwm1, blEnabled);
  XeLog("XePCI: BLC_PWM_PCH_CTL2=0x%08x\n", blcPwm2);
  
  XeLog("XePCI: --- Display State End ---\n");
}

void XeService::stop(IOService* provider) {
  XeLog("XePCI: Stopping XeService...\n");
  
  // Release any BOs left around
  if (m_boList) {
    uint32_t count = m_boList->getCount();
    XeLog("XePCI: Releasing %u buffer objects\n", count);
    for (unsigned i = 0; i < count; ++i) {
      auto *md = OSDynamicCast(IOBufferMemoryDescriptor, m_boList->getObject(i));
      if (md) md->release();
    }
    m_boList->release();
    m_boList = nullptr;
  }

  if (bar0) { 
    XeLog("XePCI: Releasing BAR0 mapping\n");
    bar0->release(); 
    bar0 = nullptr; 
    mmio = nullptr; 
  }
  
  if (pci) { 
    XeLog("XePCI: Disabling bus mastering\n");
    pci->setBusMasterEnable(false); 
  }
  
  XeLog("XePCI: XeService stopped\n");
  super::stop(provider);
}

void XeService::free() {
  XeLog("XePCI: Freeing XeService\n");
  super::free();
}

IOReturn XeService::newUserClient(task_t task, void* secID, UInt32 type,
                                  OSDictionary* props, IOUserClient** out) {
  XeLog("XePCI: Creating new user client (type=%u)\n", type);
  (void)props; // silence unused warning in CI
  IOUserClient* uc = XeCreateUserClient(this, task, secID, type);
  if (!uc) {
    XeLog("XePCI: ERROR - failed to create user client\n");
    return kIOReturnNoResources;
  }
  *out = uc;
  XeLog("XePCI: User client created successfully\n");
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
  XeLog("XePCI: ucCreateBuffer: requested %u bytes\n", bytes);
  
  if (!m_boList) {
    XeLog("XePCI: ucCreateBuffer: ERROR - BO list not ready\n");
    return kIOReturnNotReady;
  }

  // Page-align (4 KiB)
  uint32_t sz = (bytes + 0xFFFu) & ~0xFFFu;
  XeLog("XePCI: ucCreateBuffer: allocating %u bytes (page-aligned)\n", sz);

  auto *md = IOBufferMemoryDescriptor::withOptions(
      kIOMemoryKernelUserShared | kIODirectionInOut,
      sz, page_size);

  if (!md) {
    XeLog("XePCI: ucCreateBuffer: ERROR - allocation failed\n");
    return kIOReturnNoResources;
  }

  if (!m_boList->setObject(md)) {
    XeLog("XePCI: ucCreateBuffer: ERROR - failed to add to BO list\n");
    md->release();
    return kIOReturnNoResources;
  }

  uint64_t cookie = m_boList->getCount(); // 1..N
  *outCookie = cookie;

  XeLog("XePCI: ucCreateBuffer: SUCCESS - cookie=%llu size=%u vaddr=%p\n",
        (unsigned long long)cookie, sz, md->getBytesNoCopy());

  return kIOReturnSuccess;
}

IOReturn XeService::ucSubmitNoop() {
  XeLog("XePCI: ucSubmitNoop: starting\n");
  
  if (!mmio) {
    XeLog("XePCI: ucSubmitNoop: ERROR - mmio not ready\n");
    return kIOReturnNotReady;
  }

  // Allocate a tiny 4K batch (kernel-user shared so we can write commands)
  XeLog("XePCI: ucSubmitNoop: allocating 4K command buffer\n");
  auto *md = IOBufferMemoryDescriptor::withOptions(
      kIOMemoryKernelUserShared | kIODirectionInOut, 4096, page_size);
  if (!md) {
    XeLog("XePCI: ucSubmitNoop: ERROR - buffer allocation failed\n");
    return kIOReturnNoResources;
  }

  XeCommandStream cs(mmio);
  XeLog("XePCI: ucSubmitNoop: calling XeCommandStream::submitNoop\n");
  IOReturn kr = cs.submitNoop(md);
  
  XeLog("XePCI: ucSubmitNoop: result=0x%x\n", kr);
  md->release();
  return kr;
}

IOReturn XeService::ucWait(uint32_t timeoutMs) {
  XeLog("XePCI: ucWait: timeout=%u ms (stub)\n", timeoutMs);
  // Stub: pretend completion (will become HWSP poll later)
  return kIOReturnSuccess;
}

IOReturn XeService::ucReadRegs(uint32_t count, uint32_t* out, uint32_t* outCount) {
  XeLog("XePCI: ucReadRegs: requested %u registers\n", count);
  
  if (!mmio || !out || !outCount) {
    XeLog("XePCI: ucReadRegs: ERROR - invalid parameters\n");
    return kIOReturnNotReady;
  }

  // Expanded allow-list of safe, read-only registers using documented Raptor Lake offsets
  // These are verified from raptor_lake_regs.txt dump
  static const uint32_t kSafeOffs[] = {
    // Basic device registers
    0x0000, 0x0004, 0x0010, 0x0014, 0x0100, 0x0104,
    // Power management (from XeHW namespace - read-only safe)
    XeHW::GEN6_RP_CONTROL,           // 0xA024
    XeHW::GEN6_RC_STATE,             // 0xA094
    // Power wells
    XeHW::HSW_PWR_WELL_CTL1,         // 0x45400
    XeHW::HSW_PWR_WELL_CTL2,         // 0x45404
  };

  uint32_t avail = static_cast<uint32_t>(sizeof(kSafeOffs) / sizeof(kSafeOffs[0]));
  uint32_t n = (count < avail) ? count : avail;

  XeLog("XePCI: ucReadRegs: reading %u of %u available registers\n", n, avail);
  
  for (uint32_t i = 0; i < n; ++i) {
    out[i] = readReg(kSafeOffs[i]);
    XeLog("XePCI: ucReadRegs: [%u] offset=0x%04x value=0x%08x\n", i, kSafeOffs[i], out[i]);
  }

  *outCount = n;
  return kIOReturnSuccess;
}

// Read GT configuration using documented Raptor Lake registers
IOReturn XeService::ucGetGTConfig(uint32_t* out, uint32_t* outCount) {
  XeLog("XePCI: ucGetGTConfig: reading GT configuration\n");
  
  if (!mmio || !out || !outCount) {
    XeLog("XePCI: ucGetGTConfig: ERROR - invalid parameters\n");
    return kIOReturnNotReady;
  }

  // Read power management and GT state using documented registers
  XeLog("XePCI: ucGetGTConfig: reading power well registers...\n");
  out[0] = readReg(XeHW::HSW_PWR_WELL_CTL1);
  out[1] = readReg(XeHW::HSW_PWR_WELL_CTL2);
  
  XeLog("XePCI: ucGetGTConfig: reading RC state registers...\n");
  out[2] = readReg(XeHW::GEN6_RC_STATE);
  out[3] = readReg(XeHW::GEN6_RC_CONTROL);
  out[4] = readReg(XeHW::GEN6_RP_CONTROL);
  
  XeLog("XePCI: ucGetGTConfig: reading forcewake/PM registers...\n");
  out[5] = readReg(XeHW::FORCEWAKE_ACK);
  out[6] = readReg(XeHW::GEN6_PMINTRMSK);
  out[7] = readReg(XeHW::RC6_RESIDENCY_TIME);

  *outCount = 8;

  XeLog("XePCI: ucGetGTConfig: PWR_WELL1=0x%08x PWR_WELL2=0x%08x\n", out[0], out[1]);
  XeLog("XePCI: ucGetGTConfig: RC_STATE=0x%08x RC_CTRL=0x%08x RP_CTRL=0x%08x\n", 
        out[2], out[3], out[4]);
  XeLog("XePCI: ucGetGTConfig: FWAKE_ACK=0x%08x PMINTRMSK=0x%08x RC6_RES=0x%08x\n",
        out[5], out[6], out[7]);

  return kIOReturnSuccess;
}

// Read display pipeline configuration using documented Raptor Lake registers
IOReturn XeService::ucGetDisplayInfo(uint32_t* out, uint32_t* outCount) {
  XeLog("XePCI: ucGetDisplayInfo: reading display configuration\n");
  
  if (!mmio || !out || !outCount) {
    XeLog("XePCI: ucGetDisplayInfo: ERROR - invalid parameters\n");
    return kIOReturnNotReady;
  }

  // Read display state for Pipe A using documented registers
  XeLog("XePCI: ucGetDisplayInfo: reading pipe configuration...\n");
  out[0] = readReg(XeHW::PIPEACONF);
  out[1] = readReg(XeHW::PIPE_DDI_FUNC_CTL_A);
  out[2] = readReg(XeHW::DDI_BUF_CTL_A);
  out[3] = readReg(XeHW::DSPACNTR);
  
  XeLog("XePCI: ucGetDisplayInfo: reading timing registers...\n");
  out[4] = readReg(XeHW::HTOTAL_A);
  out[5] = readReg(XeHW::VTOTAL_A);
  out[6] = readReg(XeHW::PIPEASRC);
  out[7] = readReg(XeHW::PCH_PP_STATUS);

  *outCount = 8;

  // Parse and log display state
  bool pipeEnabled = (out[0] & 0x80000000) != 0;
  bool ddiEnabled = (out[1] & 0x80000000) != 0;
  bool planeEnabled = (out[3] & 0x80000000) != 0;
  
  uint32_t width = ((out[6] >> 16) & 0xFFFF) + 1;
  uint32_t height = (out[6] & 0xFFFF) + 1;
  
  uint32_t hActive = (out[4] & 0xFFFF) + 1;
  uint32_t hTotal = ((out[4] >> 16) & 0xFFFF) + 1;
  uint32_t vActive = (out[5] & 0xFFFF) + 1;
  uint32_t vTotal = ((out[5] >> 16) & 0xFFFF) + 1;

  XeLog("XePCI: ucGetDisplayInfo: PIPEACONF=0x%08x DDI_FUNC=0x%08x DDI_BUF=0x%08x DSPACNTR=0x%08x\n",
        out[0], out[1], out[2], out[3]);
  XeLog("XePCI: ucGetDisplayInfo: Pipe=%s DDI=%s Plane=%s\n",
        pipeEnabled ? "ON" : "OFF", ddiEnabled ? "ON" : "OFF", planeEnabled ? "ON" : "OFF");
  XeLog("XePCI: ucGetDisplayInfo: Resolution=%ux%u Timing H=%u/%u V=%u/%u\n",
        width, height, hActive, hTotal, vActive, vTotal);
  XeLog("XePCI: ucGetDisplayInfo: PCH_PP_STATUS=0x%08x\n", out[7]);

  return kIOReturnSuccess;
}
