#pragma once
#include "XeBootArgs.hpp"

class ForcewakeGuard {
public:
  explicit ForcewakeGuard(volatile uint32_t* /*mmio*/) : m(nullptr) {
    // strict safe mode: never acquire forcewake
  }
  ~ForcewakeGuard() {
    // strict safe mode: never release forcewake
  }
private:
  volatile uint32_t* m {nullptr};
};
