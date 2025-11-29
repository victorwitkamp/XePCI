#pragma once
#include <stdint.h>

namespace XeHW {

// ============================================================================
// Engine / Ring Registers (Gen12)
// ============================================================================

// Verified from Linux dump:
constexpr uint32_t RCS0_BASE          = 0x00002000;

// Common Gen11+/Gen12 ring register layout (relative to engine base)
constexpr uint32_t RCS0_RING_TAIL     = RCS0_BASE + 0x30;  // dword tail
constexpr uint32_t RCS0_RING_HEAD     = RCS0_BASE + 0x34;  // dword head
constexpr uint32_t RCS0_RING_CTL      = RCS0_BASE + 0x2C;  // size/enable bits
constexpr uint32_t RCS0_MI_MODE       = RCS0_BASE + 0x9C;  // optional (read-only)

constexpr uint32_t GFX_MODE           = 0x00002500;        // graphics mode

// Page-table control (PGTBL_CTL). 0x2020 is a long-standing location.
constexpr uint32_t PGTBL_CTL          = 0x00002020;

// Typical aperture size (256MB from lspci BAR2)
constexpr uint32_t GGTT_ApertureBytes = 256u * 1024u * 1024u;

// MI opcodes
constexpr uint32_t MI_NOOP            = 0x00000000;
constexpr uint32_t MI_BATCH_BUFFER_END= 0x0A000000;

// ============================================================================
// Forcewake Registers (Gen12 Raptor Lake)
// Verified from raptor_lake_regs.txt dump
// ============================================================================

// Forcewake MT pair (commonly used since Gen9+)
constexpr uint32_t FORCEWAKE_REQ      = 0x000A188;         // _MT
constexpr uint32_t FORCEWAKE_ACK      = 0x000A18C;         // _MT

// ============================================================================
// Power Management Registers (from raptor_lake_regs.txt)
// ============================================================================

// Render P-state control
constexpr uint32_t GEN6_RP_CONTROL         = 0x0000A024;  // value: 0x00000400
constexpr uint32_t GEN6_RPNSWREQ           = 0x0000A008;  // value: 0x3198c000
constexpr uint32_t GEN6_RP_DOWN_TIMEOUT    = 0x0000A010;
constexpr uint32_t GEN6_RP_INTERRUPT_LIMITS= 0x0000A014;
constexpr uint32_t GEN6_RP_UP_THRESHOLD    = 0x0000A02C;
constexpr uint32_t GEN6_RP_UP_EI           = 0x0000A068;
constexpr uint32_t GEN6_RP_DOWN_EI         = 0x0000A06C;
constexpr uint32_t GEN6_RP_IDLE_HYSTERSIS  = 0x0000A070;

// RC state control
constexpr uint32_t GEN6_RC_STATE           = 0x0000A094;  // value: 0x00040000
constexpr uint32_t GEN6_RC_CONTROL         = 0x0000A090;  // value: 0x00040000
constexpr uint32_t GEN6_RC1_WAKE_RATE_LIMIT= 0x0000A098;
constexpr uint32_t GEN6_RC6_WAKE_RATE_LIMIT= 0x0000A09C;
constexpr uint32_t GEN6_RC_EVALUATION_INTERVAL = 0x0000A0A8;
constexpr uint32_t GEN6_RC_IDLE_HYSTERSIS  = 0x0000A0AC;  // value: 0x00000019
constexpr uint32_t GEN6_RC_SLEEP           = 0x0000A0B0;
constexpr uint32_t GEN6_RC1e_THRESHOLD     = 0x0000A0B4;
constexpr uint32_t GEN6_RC6_THRESHOLD      = 0x0000A0B8;
constexpr uint32_t GEN6_RC_VIDEO_FREQ      = 0x0000A00C;

// PM interrupt registers
constexpr uint32_t GEN6_PMIER              = 0x0004402C;
constexpr uint32_t GEN6_PMIMR              = 0x00044024;
constexpr uint32_t GEN6_PMINTRMSK          = 0x0000A168;  // value: 0x80000000

// ============================================================================
// Power Well Control Registers (HSW+, from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t HSW_PWR_WELL_CTL1       = 0x00045400;  // value: 0x0000000d
constexpr uint32_t HSW_PWR_WELL_CTL2       = 0x00045404;  // value: 0x0000000f
constexpr uint32_t HSW_PWR_WELL_CTL3       = 0x00045408;
constexpr uint32_t HSW_PWR_WELL_CTL4       = 0x0004540C;  // value: 0x00000005
constexpr uint32_t HSW_PWR_WELL_CTL5       = 0x00045410;
constexpr uint32_t HSW_PWR_WELL_CTL6       = 0x00045414;

// ============================================================================
// Fence Registers (for tiled memory, from raptor_lake_regs.txt)
// 32 fence registers at 0x100000-0x1000FC
// ============================================================================

constexpr uint32_t FENCE_REG_BASE          = 0x00100000;
constexpr uint32_t FENCE_REG_COUNT         = 32;
// Each fence has START (base + n*8) and END (base + n*8 + 4)
inline constexpr uint32_t FENCE_START(uint32_t n) { return FENCE_REG_BASE + (n * 8); }
inline constexpr uint32_t FENCE_END(uint32_t n)   { return FENCE_REG_BASE + (n * 8) + 4; }

// ============================================================================
// Display Pipeline Registers - DDI/Transcoder (from raptor_lake_regs.txt)
// ============================================================================

// DDI Function Control (per pipe)
constexpr uint32_t PIPE_DDI_FUNC_CTL_A     = 0x00060400;  // value: 0x8a100006 (enabled, DP SST, 10bpc, x4)
constexpr uint32_t PIPE_DDI_FUNC_CTL_B     = 0x00061400;
constexpr uint32_t PIPE_DDI_FUNC_CTL_C     = 0x00062400;
constexpr uint32_t PIPE_DDI_FUNC_CTL_EDP   = 0x0006F400;

// DDI Buffer Control
constexpr uint32_t DDI_BUF_CTL_A           = 0x00064000;  // value: 0x80000006 (enabled, x4)
constexpr uint32_t DDI_BUF_CTL_B           = 0x00064100;
constexpr uint32_t DDI_BUF_CTL_C           = 0x00064200;
constexpr uint32_t DDI_BUF_CTL_D           = 0x00064300;
constexpr uint32_t DDI_BUF_CTL_E           = 0x00064400;

// DP Transport Control
constexpr uint32_t DP_TP_CTL_A             = 0x00064040;
constexpr uint32_t DP_TP_CTL_B             = 0x00064140;
constexpr uint32_t DP_TP_CTL_C             = 0x00064240;
constexpr uint32_t DP_TP_CTL_D             = 0x00064340;
constexpr uint32_t DP_TP_CTL_E             = 0x00064440;

// DP Transport Status
constexpr uint32_t DP_TP_STATUS_B          = 0x00064144;
constexpr uint32_t DP_TP_STATUS_C          = 0x00064244;
constexpr uint32_t DP_TP_STATUS_D          = 0x00064344;
constexpr uint32_t DP_TP_STATUS_E          = 0x00064444;

// ============================================================================
// Display Timing Registers - Pipe A (from raptor_lake_regs.txt @ 2560x1600)
// ============================================================================

// Horizontal timing
constexpr uint32_t HTOTAL_A                = 0x00060000;  // value: 0x0a9f09ff (2560 active, 2720 total)
constexpr uint32_t HBLANK_A                = 0x00060004;  // value: 0x0a9f09ff
constexpr uint32_t HSYNC_A                 = 0x00060008;  // value: 0x0a4f0a2f (2608 start, 2640 end)

// Vertical timing
constexpr uint32_t VTOTAL_A                = 0x0006000C;  // value: 0x06df063f (1600 active, 1760 total)
constexpr uint32_t VBLANK_A                = 0x00060010;  // value: 0x06df063f
constexpr uint32_t VSYNC_A                 = 0x00060014;  // value: 0x06480642 (1603 start, 1609 end)
constexpr uint32_t VSYNCSHIFT_A            = 0x00060028;

// Data/Link M/N values (for DP link rate calculation)
constexpr uint32_t PIPEA_DATA_M1           = 0x00060030;  // value: 0x7e4415a9
constexpr uint32_t PIPEA_DATA_N1           = 0x00060034;  // value: 0x00800000
constexpr uint32_t PIPEA_LINK_M1           = 0x00060040;  // value: 0x0011056a
constexpr uint32_t PIPEA_LINK_N1           = 0x00060044;  // value: 0x00080000

// ============================================================================
// Display Timing Registers - Pipe B
// ============================================================================

constexpr uint32_t HTOTAL_B                = 0x00061000;
constexpr uint32_t HBLANK_B                = 0x00061004;
constexpr uint32_t HSYNC_B                 = 0x00061008;
constexpr uint32_t VTOTAL_B                = 0x0006100C;
constexpr uint32_t VBLANK_B                = 0x00061010;
constexpr uint32_t VSYNC_B                 = 0x00061014;
constexpr uint32_t VSYNCSHIFT_B            = 0x00061028;
constexpr uint32_t PIPEB_DATA_M1           = 0x00061030;
constexpr uint32_t PIPEB_DATA_N1           = 0x00061034;
constexpr uint32_t PIPEB_LINK_M1           = 0x00061040;
constexpr uint32_t PIPEB_LINK_N1           = 0x00061044;

// ============================================================================
// Display Timing Registers - Pipe C
// ============================================================================

constexpr uint32_t HTOTAL_C                = 0x00062000;
constexpr uint32_t HBLANK_C                = 0x00062004;
constexpr uint32_t HSYNC_C                 = 0x00062008;
constexpr uint32_t VTOTAL_C                = 0x0006200C;
constexpr uint32_t VBLANK_C                = 0x00062010;
constexpr uint32_t VSYNC_C                 = 0x00062014;
constexpr uint32_t VSYNCSHIFT_C            = 0x00062028;
constexpr uint32_t PIPEC_DATA_M1           = 0x00062030;
constexpr uint32_t PIPEC_DATA_N1           = 0x00062034;
constexpr uint32_t PIPEC_LINK_M1           = 0x00062040;
constexpr uint32_t PIPEC_LINK_N1           = 0x00062044;

// ============================================================================
// Display Timing Registers - EDP Pipe
// ============================================================================

constexpr uint32_t HTOTAL_EDP              = 0x0006F000;
constexpr uint32_t HBLANK_EDP              = 0x0006F004;
constexpr uint32_t HSYNC_EDP               = 0x0006F008;
constexpr uint32_t VTOTAL_EDP              = 0x0006F00C;
constexpr uint32_t VBLANK_EDP              = 0x0006F010;
constexpr uint32_t VSYNC_EDP               = 0x0006F014;
constexpr uint32_t VSYNCSHIFT_EDP          = 0x0006F028;
constexpr uint32_t PIPEEDP_DATA_M1         = 0x0006F030;
constexpr uint32_t PIPEEDP_DATA_N1         = 0x0006F034;
constexpr uint32_t PIPEEDP_LINK_M1         = 0x0006F040;
constexpr uint32_t PIPEEDP_LINK_N1         = 0x0006F044;

// ============================================================================
// Pipe/Plane Source Size Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t PIPEASRC                = 0x0006001C;  // value: 0x09ff063f (2560, 1600)
constexpr uint32_t PIPEBSRC                = 0x0006101C;
constexpr uint32_t PIPECSRC                = 0x0006201C;

// ============================================================================
// Pipe Configuration Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t PIPEACONF               = 0x00070008;  // value: 0xc0000000 (enabled, active)
constexpr uint32_t PIPEBCONF               = 0x00071008;
constexpr uint32_t PIPECCONF               = 0x00072008;
constexpr uint32_t PIPEEDPCONF             = 0x0007F008;

// ============================================================================
// Display Plane Control Registers (from raptor_lake_regs.txt)
// ============================================================================

// Plane A (Primary)
constexpr uint32_t DSPACNTR                = 0x00070180;  // value: 0x84000400 (enabled, pipe A)
constexpr uint32_t DSPASTRIDE              = 0x00070188;  // value: 0x00000014 (20 bytes - stride)
constexpr uint32_t DSPASURF                = 0x0007019C;  // value: 0x0ca40000 (surface address)
constexpr uint32_t DSPATILEOFF             = 0x000701A4;  // tile offset

// Plane B
constexpr uint32_t DSPBCNTR                = 0x00071180;
constexpr uint32_t DSPBSTRIDE              = 0x00071188;
constexpr uint32_t DSPBSURF                = 0x0007119C;
constexpr uint32_t DSPBTILEOFF             = 0x000711A4;

// Plane C
constexpr uint32_t DSPCCNTR                = 0x00072180;
constexpr uint32_t DSPCSTRIDE              = 0x00072188;
constexpr uint32_t DSPCSURF                = 0x0007219C;
constexpr uint32_t DSPCTILEOFF             = 0x000721A4;

// ============================================================================
// Panel Fitter Registers (from raptor_lake_regs.txt)
// ============================================================================

// Pipe A scaler
constexpr uint32_t PFA_CTL_1               = 0x00068080;
constexpr uint32_t PFA_WIN_POS             = 0x00068070;
constexpr uint32_t PFA_WIN_SIZE            = 0x00068074;

// Pipe B scaler
constexpr uint32_t PFB_CTL_1               = 0x00068880;
constexpr uint32_t PFB_WIN_POS             = 0x00068870;
constexpr uint32_t PFB_WIN_SIZE            = 0x00068874;

// Pipe C scaler
constexpr uint32_t PFC_CTL_1               = 0x00069080;
constexpr uint32_t PFC_WIN_POS             = 0x00069070;
constexpr uint32_t PFC_WIN_SIZE            = 0x00069074;

// ============================================================================
// Transcoder Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t TRANS_HTOTAL_A          = 0x000E0000;
constexpr uint32_t TRANS_HBLANK_A          = 0x000E0004;
constexpr uint32_t TRANS_HSYNC_A           = 0x000E0008;
constexpr uint32_t TRANS_VTOTAL_A          = 0x000E000C;
constexpr uint32_t TRANS_VBLANK_A          = 0x000E0010;
constexpr uint32_t TRANS_VSYNC_A           = 0x000E0014;
constexpr uint32_t TRANS_VSYNCSHIFT_A      = 0x000E0028;
constexpr uint32_t TRANSACONF              = 0x000F0008;
constexpr uint32_t FDI_RXA_MISC            = 0x000F0010;
constexpr uint32_t FDI_RXA_TUSIZE1         = 0x000F0030;
constexpr uint32_t FDI_RXA_IIR             = 0x000F0014;
constexpr uint32_t FDI_RXA_IMR             = 0x000F0018;

// ============================================================================
// Clock Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t SPLL_CTL                = 0x00046020;
constexpr uint32_t LCPLL_CTL               = 0x00130040;
constexpr uint32_t WRPLL_CTL1              = 0x00046040;
constexpr uint32_t WRPLL_CTL2              = 0x00046060;

// Port clock selection
constexpr uint32_t PORT_CLK_SEL_A          = 0x00046100;
constexpr uint32_t PORT_CLK_SEL_B          = 0x00046104;
constexpr uint32_t PORT_CLK_SEL_C          = 0x00046108;
constexpr uint32_t PORT_CLK_SEL_D          = 0x0004610C;
constexpr uint32_t PORT_CLK_SEL_E          = 0x00046110;

// Pipe clock selection
constexpr uint32_t PIPE_CLK_SEL_A          = 0x00046140;  // value: 0x10000000
constexpr uint32_t PIPE_CLK_SEL_B          = 0x00046144;
constexpr uint32_t PIPE_CLK_SEL_C          = 0x00046148;

// ============================================================================
// Watermark Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t WM_PIPE_A               = 0x00045100;
constexpr uint32_t WM_PIPE_B               = 0x00045104;
constexpr uint32_t WM_PIPE_C               = 0x00045200;
constexpr uint32_t WM_LP1                  = 0x00045108;
constexpr uint32_t WM_LP2                  = 0x0004510C;
constexpr uint32_t WM_LP3                  = 0x00045110;
constexpr uint32_t WM_LP1_SPR              = 0x00045120;
constexpr uint32_t WM_LP2_SPR              = 0x00045124;
constexpr uint32_t WM_LP3_SPR              = 0x00045128;
constexpr uint32_t WM_MISC                 = 0x00045260;  // value: 0x20000000
constexpr uint32_t WM_SR_CNT               = 0x00045264;  // value: 0x027695bb
constexpr uint32_t PIPE_WM_LINETIME_A      = 0x00045270;  // value: 0x00000013
constexpr uint32_t PIPE_WM_LINETIME_B      = 0x00045274;
constexpr uint32_t PIPE_WM_LINETIME_C      = 0x00045278;
constexpr uint32_t WM_DBG                  = 0x00045280;  // value: 0x70000000

// ============================================================================
// Backlight / PWM Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t BLC_PWM_CPU_CTL2        = 0x00048250;
constexpr uint32_t BLC_PWM_CPU_CTL         = 0x00048254;
constexpr uint32_t BLC_PWM2_CPU_CTL2       = 0x00048350;
constexpr uint32_t BLC_PWM2_CPU_CTL        = 0x00048354;
constexpr uint32_t BLC_MISC_CTL            = 0x00048360;
constexpr uint32_t BLC_PWM_PCH_CTL1        = 0x000C8250;  // value: 0x80000000 (enabled)
constexpr uint32_t BLC_PWM_PCH_CTL2        = 0x000C8254;  // value: 0x00004b00
constexpr uint32_t UTIL_PIN_CTL            = 0x00048400;

// ============================================================================
// Panel Power Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t PCH_PP_STATUS           = 0x000C7200;  // value: 0x80000008 (on, not ready)
constexpr uint32_t PCH_PP_CONTROL          = 0x000C7204;  // value: 0x00000067 (backlight enabled, panel on)
constexpr uint32_t PCH_PP_ON_DELAYS        = 0x000C7208;  // value: 0x07d00001
constexpr uint32_t PCH_PP_OFF_DELAYS       = 0x000C720C;  // value: 0x01f40001
constexpr uint32_t PCH_PP_DIVISOR          = 0x000C7210;

// ============================================================================
// Other Display Registers (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t SFUSE_STRAP             = 0x000C2014;  // display enabled
constexpr uint32_t PIXCLK_GATE             = 0x000C6020;
constexpr uint32_t SDEISR                  = 0x000C4000;  // value: 0x00010000

// ============================================================================
// RC6 Residency (from raptor_lake_regs.txt)
// ============================================================================

constexpr uint32_t RC6_RESIDENCY_TIME      = 0x00138108;  // value: 0x230cfa53

// ============================================================================
// Working Display Mode from Raptor Lake dump: 2560x1600@60Hz
// ============================================================================
namespace RaptorLakeMode {
    // Parsed from PIPEASRC value 0x09ff063f
    constexpr uint32_t ACTIVE_WIDTH        = 2560;
    constexpr uint32_t ACTIVE_HEIGHT       = 1600;
    
    // Parsed from HTOTAL_A value 0x0a9f09ff
    constexpr uint32_t HTOTAL              = 2720;  // 0x0a9f + 1
    
    // Parsed from VTOTAL_A value 0x06df063f
    constexpr uint32_t VTOTAL              = 1760;  // 0x06df + 1
    
    // Parsed from HSYNC_A value 0x0a4f0a2f
    constexpr uint32_t HSYNC_START         = 2608;  // 0x0a2f + 1
    constexpr uint32_t HSYNC_END           = 2640;  // 0x0a4f + 1
    
    // Parsed from VSYNC_A value 0x06480642
    constexpr uint32_t VSYNC_START         = 1603;  // 0x0642 + 1
    constexpr uint32_t VSYNC_END           = 1609;  // 0x0648 + 1
    
    // Calculated: Pixel clock = (htotal * vtotal * refresh_rate)
    // 2720 * 1760 * 60 = 287,232,000 Hz ≈ 287.2 MHz
    constexpr uint32_t PIXEL_CLOCK_KHZ     = 287232;
}

} // namespace XeHW
