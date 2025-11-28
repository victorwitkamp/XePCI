#pragma once
#include "XeService.hpp"
#include "xe_hw_offsets.hpp"

class XeGGTT {
public:
  static bool probe(volatile uint32_t* /*mmio*/) {
    // strict safe mode: do nothing
    return true;
  }
};
