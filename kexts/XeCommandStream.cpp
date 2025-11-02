#include "XeCommandStream.hpp"

void XeCommandStream::logRcs0State() const {
  if (!m) return;
  ForcewakeGuard fw(m);
  uint32_t head = rd(XeHW::RCS0_RING_HEAD);
  uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
  uint32_t ctl  = rd(XeHW::RCS0_RING_CTL);
  uint32_t gfx  = rd(XeHW::GFX_MODE);
  IOLog("XeCS: RCS0 head=0x%08x tail=0x%08x ctl=0x%08x gfx=0x%08x\n", head, tail, ctl, gfx);
}

IOReturn XeCommandStream::submitNoop(IOBufferMemoryDescriptor* bo) {
  if (!m || !bo) return kIOReturnBadArgument;

  auto *va = (uint32_t*)bo->getBytesNoCopy();
  if (!va) return kIOReturnNoMemory;

  // Minimal batch
  va[0] = XeHW::MI_NOOP;
  va[1] = XeHW::MI_BATCH_BUFFER_END;

  // Read-only sanity first
  {
    ForcewakeGuard fw(m);
    uint32_t head = rd(XeHW::RCS0_RING_HEAD);
    uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
    uint32_t ctl  = rd(XeHW::RCS0_RING_CTL);
    IOLog("XeCS: (pre) head=0x%08x tail=0x%08x ctl=0x%08x\n", head, tail, ctl);
  }

#if 0
  // === Enable this after ring offsets look sane in logRcs0State() ===
  {
    ForcewakeGuard fw(m);
    uint32_t tail = rd(XeHW::RCS0_RING_TAIL);
    tail += 2 * sizeof(uint32_t);     // advance by our 2 dwords
    wr(XeHW::RCS0_RING_TAIL, tail);
    uint32_t newTail = rd(XeHW::RCS0_RING_TAIL);
    IOLog("XeCS: (post) tail=0x%08x\n", newTail);
  }
#endif

  return kIOReturnSuccess;
}
