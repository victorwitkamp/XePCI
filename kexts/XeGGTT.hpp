#pragma once
#include "XeService.hpp"
#include "xe_hw_offsets.hpp"

class XeGGTT {
public:
  static bool probe(volatile uint32_t* mmio) {
    if (!mmio) return false;
    uint32_t pgtbl = mmio[XeHW::PGTBL_CTL >> 2];
    if (pgtbl == 0) {
      XeLog("XeGGTT: PGTBL_CTL=0 (appears disabled)\n");
    } else {
      uint64_t base = (uint64_t)pgtbl & ~0xFFFULL; // page-aligned base guess
      XeLog("XeGGTT: PGTBL_CTL=0x%08x base=0x%llx\n", pgtbl, (unsigned long long)base);
    }
    return true;
  }
};
