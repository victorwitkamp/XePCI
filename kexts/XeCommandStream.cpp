#include "XeCommandStream.hpp"
#include "XeService.hpp"
#include "XeBootArgs.hpp"

// Maximum safe MMIO offset
constexpr uint32_t kCSMaxOffset = 0x00FFFFFF;

// Safe register read
static inline uint32_t safeRd(volatile uint32_t* m, uint32_t off) {
  if (!m) return 0xDEADBEEF;
  if (off > kCSMaxOffset) return 0xBAD0FFFF;
  return m[off >> 2];
}

void XeCommandStream::logRcs0State() const {
  XeLog("XeCS::logRcs0State: starting\n");
  
  if (!m) {
    XeLog("XeCS::logRcs0State: ERROR - mmio is null\n");
    return;
  }

  if (gXeBoot.disableCommandStream || gXeBoot.strictSafe) {
    XeLog("XeCS::logRcs0State: SKIP - disabled by boot flags\n");
    return;
  }

  // Validate register offsets before access
  if (XeHW::RCS0_RING_HEAD > kCSMaxOffset || 
      XeHW::RCS0_RING_TAIL > kCSMaxOffset ||
      XeHW::RCS0_RING_CTL > kCSMaxOffset) {
    XeLog("XeCS::logRcs0State: ERROR - ring register offsets out of range\n");
    return;
  }

  // Acquire forcewake to safely read engine registers
  XeLog("XeCS::logRcs0State: acquiring forcewake for register access\n");
  ForcewakeGuard fw(m);
  
  if (!fw.isAcquired()) {
    if (gXeBoot.disableForcewake) {
      XeLog("XeCS::logRcs0State: forcewake disabled, reading without it\n");
    } else {
      XeLog("XeCS::logRcs0State: WARNING - failed to acquire forcewake, reads may be unreliable\n");
    }
  }

  // Read RCS0 ring buffer state using documented registers
  XeLog("XeCS::logRcs0State: reading ring registers...\n");
  
  uint32_t ringHead = safeRd(m, XeHW::RCS0_RING_HEAD);
  uint32_t ringTail = safeRd(m, XeHW::RCS0_RING_TAIL);
  uint32_t ringCtl = safeRd(m, XeHW::RCS0_RING_CTL);
  
  // Check for read errors
  if (ringHead == 0xDEADBEEF || ringTail == 0xDEADBEEF || ringCtl == 0xDEADBEEF) {
    XeLog("XeCS::logRcs0State: ERROR - null mmio during read\n");
    return;
  }
  if (ringHead == 0xBAD0FFFF || ringTail == 0xBAD0FFFF || ringCtl == 0xBAD0FFFF) {
    XeLog("XeCS::logRcs0State: ERROR - offset out of range\n");
    return;
  }

  XeLog("XeCS::logRcs0State: HEAD=0x%08x TAIL=0x%08x CTL=0x%08x\n",
        ringHead, ringTail, ringCtl);
  
  // Read optional registers (may not be valid on all hardware)
  if (XeHW::RCS0_MI_MODE <= kCSMaxOffset && XeHW::GFX_MODE <= kCSMaxOffset) {
    uint32_t miMode = safeRd(m, XeHW::RCS0_MI_MODE);
    uint32_t gfxMode = safeRd(m, XeHW::GFX_MODE);
    XeLog("XeCS::logRcs0State: MI_MODE=0x%08x GFX_MODE=0x%08x\n", miMode, gfxMode);
  }

  // Decode ring control register
  bool ringEnabled = (ringCtl & (1u << 0)) != 0;
  uint32_t ringSize = (ringCtl >> 12) & 0x1FF; // Ring size in pages
  XeLog("XeCS::logRcs0State: ring %s, size=%u pages\n",
        ringEnabled ? "ENABLED" : "DISABLED", ringSize);
  
  XeLog("XeCS::logRcs0State: completed\n");
}

IOReturn XeCommandStream::submitNoop(IOBufferMemoryDescriptor* bo) {
  XeLog("XeCS::submitNoop: starting\n");
  
  // Validate parameters
  if (!m) {
    XeLog("XeCS::submitNoop: ERROR - mmio is null\n");
    return kIOReturnNotReady;
  }
  
  if (!bo) {
    XeLog("XeCS::submitNoop: ERROR - buffer is null\n");
    return kIOReturnBadArgument;
  }

  if (gXeBoot.disableCommandStream || gXeBoot.strictSafe) {
    XeLog("XeCS::submitNoop: SKIP - disabled by boot flags\n");
    return kIOReturnNotReady;
  }

  // Get buffer virtual address with validation
  void* rawAddr = bo->getBytesNoCopy();
  if (!rawAddr) {
    XeLog("XeCS::submitNoop: ERROR - failed to get buffer address\n");
    return kIOReturnNoMemory;
  }
  
  // Validate buffer size
  uint64_t bufSize = bo->getLength();
  if (bufSize < 16) {
    XeLog("XeCS::submitNoop: ERROR - buffer too small (%llu bytes)\n", 
          (unsigned long long)bufSize);
    return kIOReturnNoSpace;
  }
  
  XeLog("XeCS::submitNoop: buffer at %p, size=%llu bytes\n", rawAddr, (unsigned long long)bufSize);

  // Write MI_NOOP + MI_BATCH_BUFFER_END to the buffer
  volatile uint32_t* cmds = (volatile uint32_t*)rawAddr;
  cmds[0] = XeHW::MI_NOOP;
  cmds[1] = XeHW::MI_NOOP;
  cmds[2] = XeHW::MI_NOOP;
  cmds[3] = XeHW::MI_BATCH_BUFFER_END;
  OSSynchronizeIO();

  XeLog("XeCS::submitNoop: wrote NOOP batch: [0]=0x%08x [1]=0x%08x [2]=0x%08x [3]=0x%08x\n",
        cmds[0], cmds[1], cmds[2], cmds[3]);

  // Log current ring state before any submission
  XeLog("XeCS::submitNoop: logging ring state...\n");
  logRcs0State();

  // NOTE: Actual ring tail update and batch execution is NOT implemented yet
  // This requires proper GGTT setup and ring initialization first
  // For now, we just prepare the buffer and log the state

  XeLog("XeCS::submitNoop: completed (batch prepared, not executed)\n");
  return kIOReturnSuccess;
}
