#pragma once
#include "XeBootArgs.hpp"
#include "xe_hw_offsets.hpp"
#include <IOKit/IOLib.h>

// Forward declaration for XeLog
void XeLog(const char* fmt, ...) __attribute__((format(printf,1,2)));

// Maximum safe MMIO offset (must match XeService.hpp)
constexpr uint32_t kForcewakeMaxOffset = 0x00FFFFFF;

// RAII guard for acquiring/releasing GPU forcewake
// Forcewake keeps the GT powered so registers can be safely accessed
// 
// SAFETY: This class is designed to be panic-free:
// - All pointers are checked before use
// - Timeout prevents infinite loops
// - Safe defaults used when operations fail
class ForcewakeGuard {
public:
  explicit ForcewakeGuard(volatile uint32_t* mmio) : m(mmio), acquired(false) {
    // Safety: skip if mmio is null
    if (!mmio) {
      XeLog("ForcewakeGuard: SKIP - mmio is null\n");
      return;
    }
    
    // Safety: skip in safe modes
    if (gXeBoot.disableForcewake || gXeBoot.strictSafe) {
      XeLog("ForcewakeGuard: SKIP - disabled by boot flags\n");
      return;
    }
    
    // Validate register offsets are in range
    if (XeHW::FORCEWAKE_REQ > kForcewakeMaxOffset || 
        XeHW::FORCEWAKE_ACK > kForcewakeMaxOffset) {
      XeLog("ForcewakeGuard: SKIP - register offsets out of range\n");
      return;
    }
    
    acquire();
  }
  
  ~ForcewakeGuard() {
    if (acquired && m) {
      release();
    }
  }

  bool isAcquired() const { return acquired; }

private:
  volatile uint32_t* m;
  bool acquired;
  
  // Sentinel values for error detection
  static constexpr uint32_t kErrorNullMMIO = 0xDEADBEEF;
  static constexpr uint32_t kErrorOutOfRange = 0xBAD0FFFF;

  // Safe register read with null check
  inline uint32_t rd(uint32_t off) const { 
    if (!m) return kErrorNullMMIO;
    if (off > kForcewakeMaxOffset) return kErrorOutOfRange;
    return m[off >> 2]; 
  }
  
  // Safe register write with null check
  inline void wr(uint32_t off, uint32_t v) { 
    if (!m) return;
    if (off > kForcewakeMaxOffset) return;
    m[off >> 2] = v; 
    OSSynchronizeIO(); 
  }

  void acquire() {
    XeLog("ForcewakeGuard: acquiring forcewake...\n");
    
    // Write forcewake request with set bit (upper 16 bits are mask)
    // Format: [31:16] = mask, [15:0] = value
    // To set bit 0: write 0x00010001
    wr(XeHW::FORCEWAKE_REQ, 0x00010001);
    
    // Poll for acknowledgment with timeout (max ~50ms to be safe)
    // Intel documentation recommends ~1ms delays between polls
    const int maxIterations = 50;
    const int delayPerIteration = 1000; // 1ms in microseconds (per Intel docs)
    
    for (int i = 0; i < maxIterations; ++i) {
      uint32_t ack = rd(XeHW::FORCEWAKE_ACK);
      
      // Check for invalid read (null mmio or out of range)
      if (ack == kErrorNullMMIO || ack == kErrorOutOfRange) {
        XeLog("ForcewakeGuard: ERROR - invalid ACK read (0x%08x)\n", ack);
        return;
      }
      
      if (ack & 0x1) {
        acquired = true;
        XeLog("ForcewakeGuard: acquired after %d iterations (ACK=0x%08x)\n", i + 1, ack);
        return;
      }
      
      IODelay(delayPerIteration);
    }
    
    XeLog("ForcewakeGuard: WARNING - timeout after %dms, continuing without forcewake\n",
          (maxIterations * delayPerIteration) / 1000);
  }

  void release() {
    if (!m) return;
    
    XeLog("ForcewakeGuard: releasing forcewake...\n");
    
    // Clear forcewake request (mask=1, value=0)
    wr(XeHW::FORCEWAKE_REQ, 0x00010000);
    acquired = false;
    
    XeLog("ForcewakeGuard: released\n");
  }
};
