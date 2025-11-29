#pragma once
#include "xe_hw_offsets.hpp"
#include <IOKit/IOLib.h>

// Forward declaration for XeLog
void XeLog(const char* fmt, ...) __attribute__((format(printf,1,2)));

// Maximum safe MMIO offset (must match XeService.hpp)
constexpr uint32_t kGGTTMaxOffset = 0x00FFFFFF;

// GGTT (Graphics Global Translation Table) probing and management
// 
// SAFETY: All methods are designed to be panic-free:
// - All pointers checked before use
// - All offsets validated before access
// - Safe defaults returned on error
class XeGGTT {
public:
  // Sentinel values for error detection
  static constexpr uint32_t kErrorNullMMIO = 0xDEADBEEF;
  static constexpr uint32_t kErrorOutOfRange = 0xBAD0FFFF;

  // Safe register read with bounds checking
  static inline uint32_t safeRead(volatile uint32_t* mmio, uint32_t off) {
    if (!mmio) return kErrorNullMMIO;
    if (off > kGGTTMaxOffset) return kErrorOutOfRange;
    return mmio[off >> 2];
  }

  // Probe GGTT configuration by reading PGTBL_CTL and related registers
  static bool probe(volatile uint32_t* mmio) {
    XeLog("XeGGTT::probe: starting GGTT probe\n");
    
    if (!mmio) {
      XeLog("XeGGTT::probe: ERROR - mmio is null\n");
      return false;
    }

    // Read page table control register (with safety check)
    uint32_t pgtblCtl = safeRead(mmio, XeHW::PGTBL_CTL);
    if (pgtblCtl == kErrorNullMMIO || pgtblCtl == kErrorOutOfRange) {
      XeLog("XeGGTT::probe: ERROR - failed to read PGTBL_CTL\n");
      return false;
    }
    
    // Read power well status to check if display is powered
    uint32_t pwrWell1 = safeRead(mmio, XeHW::HSW_PWR_WELL_CTL1);
    uint32_t pwrWell2 = safeRead(mmio, XeHW::HSW_PWR_WELL_CTL2);
    
    // Log GGTT configuration
    XeLog("XeGGTT::probe: PGTBL_CTL=0x%08x\n", pgtblCtl);
    XeLog("XeGGTT::probe: PWR_WELL1=0x%08x PWR_WELL2=0x%08x\n", pwrWell1, pwrWell2);
    
    // Check first fence register (with validation)
    if (XeHW::FENCE_START(0) <= kGGTTMaxOffset && XeHW::FENCE_END(0) <= kGGTTMaxOffset) {
      uint32_t fence0Start = safeRead(mmio, XeHW::FENCE_START(0));
      uint32_t fence0End = safeRead(mmio, XeHW::FENCE_END(0));
      XeLog("XeGGTT::probe: FENCE0 start=0x%08x end=0x%08x\n", fence0Start, fence0End);
    } else {
      XeLog("XeGGTT::probe: SKIP - fence registers out of range\n");
    }
    
    XeLog("XeGGTT::probe: completed successfully\n");
    return true;
  }

  // Get GGTT aperture info
  struct GGTTInfo {
    uint32_t pgtblCtl;
    uint32_t apertureSize;
    bool     isValid;
  };

  static GGTTInfo getInfo(volatile uint32_t* mmio) {
    GGTTInfo info = {};
    
    if (!mmio) {
      XeLog("XeGGTT::getInfo: ERROR - mmio is null\n");
      return info;
    }
    
    info.pgtblCtl = safeRead(mmio, XeHW::PGTBL_CTL);
    if (info.pgtblCtl == kErrorNullMMIO || info.pgtblCtl == kErrorOutOfRange) {
      XeLog("XeGGTT::getInfo: ERROR - failed to read PGTBL_CTL\n");
      return info;
    }
    
    info.apertureSize = XeHW::GGTT_ApertureBytes; // 256MB from lspci
    info.isValid = true;
    
    XeLog("XeGGTT::getInfo: PGTBL_CTL=0x%08x aperture=%uMB\n", 
          info.pgtblCtl, info.apertureSize / (1024 * 1024));
    
    return info;
  }

  // Read fence register pair
  struct FenceInfo {
    uint32_t start;
    uint32_t end;
    bool     active;
    bool     valid;
  };

  static FenceInfo getFence(volatile uint32_t* mmio, uint32_t index) {
    FenceInfo info = {};
    
    if (!mmio) {
      XeLog("XeGGTT::getFence: ERROR - mmio is null\n");
      return info;
    }
    
    if (index >= XeHW::FENCE_REG_COUNT) {
      XeLog("XeGGTT::getFence: ERROR - index %u out of range (max %u)\n", 
            index, XeHW::FENCE_REG_COUNT - 1);
      return info;
    }
    
    uint32_t startOff = XeHW::FENCE_START(index);
    uint32_t endOff = XeHW::FENCE_END(index);
    
    if (startOff > kGGTTMaxOffset || endOff > kGGTTMaxOffset) {
      XeLog("XeGGTT::getFence: ERROR - fence %u offsets out of range\n", index);
      return info;
    }
    
    info.start = safeRead(mmio, startOff);
    info.end = safeRead(mmio, endOff);
    info.active = (info.start != 0 || info.end != 0);
    info.valid = true;
    
    XeLog("XeGGTT::getFence[%u]: start=0x%08x end=0x%08x active=%d\n",
          index, info.start, info.end, info.active);
    
    return info;
  }
};
