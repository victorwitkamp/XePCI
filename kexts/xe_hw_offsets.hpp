#pragma once
#include <stdint.h>

namespace XeHW {

// Verified from your Linux dump:
constexpr uint32_t RCS0_BASE          = 0x00002000;

// Common Gen11+/Gen12 ring register layout (relative to engine base)
constexpr uint32_t RCS0_RING_TAIL     = RCS0_BASE + 0x30;  // dword tail
constexpr uint32_t RCS0_RING_HEAD     = RCS0_BASE + 0x34;  // dword head
constexpr uint32_t RCS0_RING_CTL      = RCS0_BASE + 0x2C;  // size/enable bits
constexpr uint32_t RCS0_MI_MODE       = RCS0_BASE + 0x9C;  // optional (read-only)

// If you find exact offset from your dumps, adjust this:
constexpr uint32_t GFX_MODE           = 0x00002500;        // best-guess; safe to read

// Forcewake MT pair (commonly used since Gen9+). If your read logs show 0, try the non-MT pair:
//  - FORCEWAKE:     0x0A180
//  - FORCEWAKE_ACK: 0x0A184
constexpr uint32_t FORCEWAKE_REQ      = 0x000A188;         // _MT
constexpr uint32_t FORCEWAKE_ACK      = 0x000A18C;         // _MT

// Page-table control (PGTBL_CTL). 0x2020 is a long-standing location.
// We only log this for now.
constexpr uint32_t PGTBL_CTL          = 0x00002020;

// Typical aperture size (adjust later if your dumps show different)
constexpr uint32_t GGTT_ApertureBytes = 256u * 1024u * 1024u;

// MI opcodes
constexpr uint32_t MI_NOOP            = 0x00000000;
constexpr uint32_t MI_BATCH_BUFFER_END= 0x0A000000;

} // namespace XeHW
