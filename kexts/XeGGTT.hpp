#pragma once
#include <IOKit/IOLib.h>
#include "xe_hw_offsets.hpp"

class XeGGTT {
public:
  static bool probe(volatile uint32_t* mmio) {
    if (!mmio) return false;
    uint32_t pgtbl = mmio[XeHW::PGTBL_CTL >> 2];
    IOLog("XeGGTT: PGTBL_CTL @0x%08x = 0x%08x\n", XeHW::PGTBL_CTL, pgtbl);
    return true;
  }
};
