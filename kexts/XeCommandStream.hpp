#pragma once
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include "xe_hw_offsets.hpp"
#include "ForcewakeGuard.hpp"

class XeCommandStream {
public:
  explicit XeCommandStream(volatile uint32_t* mmio) : m(mmio) {}
  bool valid() const { return m != nullptr; }

  void logRcs0State() const;
  IOReturn submitNoop(IOBufferMemoryDescriptor* bo);

private:
  volatile uint32_t* m {nullptr};
  inline uint32_t rd(uint32_t off) const { return m ? m[off >> 2] : 0; }
  inline void     wr(uint32_t off, uint32_t v) { if (m) { m[off >> 2] = v; OSSynchronizeIO(); } }
};
