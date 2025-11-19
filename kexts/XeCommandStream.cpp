#include "XeCommandStream.hpp"
#include "XeService.hpp"
#include "XeBootArgs.hpp"

void XeCommandStream::logRcs0State() const {
  if (!m) return;
  if (gXeBoot.disableCommandStream) {
    XeLog("XeCS: logRcs0State skipped (nocs flag)\n");
    return;
  }
  ForcewakeGuard fw(m); // RAII: acquire forcewake for duration of this scope
  (void)fw; // suppress unused variable warning - used for RAII
  uint32_t head = rd(XeHW::RCS0_RING_HEAD);
  uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
  uint32_t ctl  = rd(XeHW::RCS0_RING_CTL);
  uint32_t gfx  = rd(XeHW::GFX_MODE);
  if (ctl == 0) {
    XeLog("XeCS: WARNING RCS0_RING_CTL=0 (ring disabled?) head=0x%08x tail=0x%08x gfx=0x%08x\n", head, tail, gfx);
  } else {
    XeLog("XeCS: RCS0 head=0x%08x tail=0x%08x ctl=0x%08x gfx=0x%08x\n", head, tail, ctl, gfx);
  }
}

IOReturn XeCommandStream::submitNoop(IOBufferMemoryDescriptor* bo) {
  XeLog("XeCS: submitNoop invoked (nocs=%d)\n", gXeBoot.disableCommandStream);
  if (gXeBoot.disableCommandStream) return kIOReturnNotReady;
  if (!m || !bo) return kIOReturnBadArgument;

  auto *va = (uint32_t*)bo->getBytesNoCopy();
  if (!va) return kIOReturnNoMemory;

  // Minimal batch
  va[0] = XeHW::MI_NOOP;
  va[1] = XeHW::MI_BATCH_BUFFER_END;

  // Read-only sanity first
  {
    ForcewakeGuard fw(m); // RAII: acquire forcewake for duration of this scope
    (void)fw; // suppress unused variable warning - used for RAII
    uint32_t head = rd(XeHW::RCS0_RING_HEAD);
    uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
    uint32_t ctl  = rd(XeHW::RCS0_RING_CTL);
    XeLog("XeCS: (pre) head=0x%08x tail=0x%08x ctl=0x%08x\n", head, tail, ctl);
  }

#if 0
  // === Enable this after ring offsets look sane in logRcs0State() ===
  {
    ForcewakeGuard fw(m); // RAII: acquire forcewake for duration of this scope
    (void)fw; // suppress unused variable warning - used for RAII
    uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
    tail += 2 * sizeof(uint32_t);     // advance by our 2 dwords
    wr(XeHW::RCS0_RING_TAIL, tail);
    uint32_t newTail = rd(XeHW::RCS0_RING_TAIL);
    XeLog("XeCS: (post) tail=0x%08x\n", newTail);
  }
#endif

  return kIOReturnSuccess;
}
