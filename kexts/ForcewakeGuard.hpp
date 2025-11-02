#pragma once
#include <IOKit/IOLib.h>
#include "xe_hw_offsets.hpp"

class ForcewakeGuard {
public:
  explicit ForcewakeGuard(volatile uint32_t* mmio) : m(mmio) {
    if (!m) return;
    write32(XeHW::FORCEWAKE_REQ, 0x0000FFFFu); // request all domains
    OSSynchronizeIO();
    // quick spin for ack
    for (uint32_t i = 0; i < 100000; ++i) {
      if ((read32(XeHW::FORCEWAKE_ACK) & 0x0000FFFFu) == 0x0000FFFFu) break;
    }
  }
  ~ForcewakeGuard() {
    if (!m) return;
    write32(XeHW::FORCEWAKE_REQ, 0x00000000u);
    OSSynchronizeIO();
  }
private:
  volatile uint32_t* m {nullptr};
  inline uint32_t read32(uint32_t off)  const { return m[off >> 2]; }
  inline void     write32(uint32_t off, uint32_t v) { m[off >> 2] = v; OSSynchronizeIO(); }
};
