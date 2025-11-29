# Raptor Lake GPU Research Guide

This document outlines the research methodology and data to collect for enabling Intel Raptor Lake (8086:A788) GPU support, based on the approach used by the [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext) project for Tiger Lake.

---

## Research Methodology

The reference project's success is based on:

1. **Register Dumps** - Capturing MMIO register values from a working system (Linux i915)
2. **Intel PRM (Programmer's Reference Manual)** - Volumes 15-17 for display/GT documentation
3. **Linux i915 Source** - Open source driver with detailed register definitions
4. **Iterative Testing** - Small incremental changes with safety checks

---

## Data Collection Tasks for Raptor Lake

### 1. Basic Device Information

Collect from your Raptor Lake system:

```bash
# Device IDs
lspci -nn | grep VGA

# PCI config space
lspci -xxx -s <device>

# BAR addresses
lspci -vvv -s <device>
```

**Expected data:**
- Device ID: 8086:A788
- BAR0 (GTTMMADR) physical address
- BAR1 (GMADR) physical address
- BAR2 (IO) if present

### 2. Register Dump from Linux i915

Use `intel_reg` tool from intel-gpu-tools:

```bash
# Install intel-gpu-tools
sudo apt install intel-gpu-tools

# Dump all registers
sudo intel_reg dump --all > raptor_lake_regs.txt

# Or specific register ranges
sudo intel_reg read 0x45400  # Power well control
sudo intel_reg read 0xA188   # Forcewake
```

---

## Register Categories to Research

Based on the Tiger Lake dump, collect the same categories for Raptor Lake:

### Power Management & Forcewake

| Register Name | TGL Offset | Description | Value to Check |
|---------------|------------|-------------|----------------|
| `PWR_WELL_BIOS` | 0x45400 | Power well BIOS | Enabled state |
| `PWR_WELL_DRIVER` | 0x45404 | Power well driver | Enabled state |
| `PWR_WELL_KVM` | 0x45408 | Power well KVM | Status |
| `PWR_WELL_DEBUG` | 0x4540C | Power well debug | Status |
| `DC_STATE_EN` | 0x45504 | DC state enable | Current DC state |
| `DC_STATE_DEBUG` | 0x45520 | DC state debug | Debug info |

**Research note:** Power well registers may have different bit layouts on Raptor Lake.

### Display Clocks (DPLL)

| Register Name | TGL Offset | Description |
|---------------|------------|-------------|
| `DPLL_STATUS` | 0x6C060 | DPLL lock status |
| `DPLL1_CFGCR1` | 0x6C040 | DPLL1 config 1 |
| `DPLL1_CFGCR2` | 0x6C044 | DPLL1 config 2 |
| `DPLL_CTRL1` | 0x6C058 | DPLL control 1 |
| `DPLL_CTRL2` | 0x6C05C | DPLL control 2 |
| `CDCLK_CTL` | 0x46000 | CD clock control |
| `LCPLL1_CTL` | 0x46010 | LC PLL1 control |
| `LCPLL2_CTL` | 0x46014 | LC PLL2 control |

### Transcoder Registers (Per Pipe: A, B, C, D)

For each pipe (A=0x6xxxx, B=0x61xxx, C=0x62xxx, D=0x63xxx):

| Register Name | Pipe A Offset | Description |
|---------------|---------------|-------------|
| `TRANS_HTOTAL` | 0x60000 | Horizontal total |
| `TRANS_HBLANK` | 0x60004 | Horizontal blank |
| `TRANS_HSYNC` | 0x60008 | Horizontal sync |
| `TRANS_VTOTAL` | 0x6000C | Vertical total |
| `TRANS_VBLANK` | 0x60010 | Vertical blank |
| `TRANS_VSYNC` | 0x60014 | Vertical sync |
| `TRANS_SPACE` | 0x60024 | Space register |
| `TRANS_VSYNCSHIFT` | 0x60028 | Vsync shift |
| `TRANS_MULT` | 0x6002C | Multiplier |
| `TRANS_DATAM1` | 0x60030 | Data M value |
| `TRANS_DATAN1` | 0x60034 | Data N value |
| `TRANS_LINKM1` | 0x60040 | Link M value |
| `TRANS_LINKN1` | 0x60044 | Link N value |
| `TRANS_DDI_FUNC_CTL` | 0x60400 | DDI function control |
| `TRANS_MSA_MISC` | 0x60410 | MSA misc |
| `TRANS_CONF` | 0x70008 | Transcoder config |

### Plane Registers (Per Plane 1-7, Per Pipe A-D)

For Plane 1 on Pipe A (base 0x70xxx):

| Register Name | Offset | Description |
|---------------|--------|-------------|
| `PLANE_CTL_1_A` | 0x70180 | Plane control |
| `PLANE_STRIDE_1_A` | 0x70188 | Stride (in 64B blocks) |
| `PLANE_POS_1_A` | 0x7018C | Position (x,y) |
| `PLANE_SIZE_1_A` | 0x70190 | Size (w,h) |
| `PLANE_KEYVAL_1_A` | 0x70194 | Color key value |
| `PLANE_KEYMSK_1_A` | 0x70198 | Color key mask |
| `PLANE_SURF_1_A` | 0x7019C | Surface address (GGTT offset) |
| `PLANE_KEYMAX_1_A` | 0x701A0 | Color key max |
| `PLANE_OFFSET_1_A` | 0x701A4 | Offset |
| `PLANE_SURFLIVE_1_A` | 0x701AC | Live surface |
| `PLANE_AUX_DIST_1_A` | 0x701C0 | Aux distance |
| `PLANE_AUX_OFFSET_1_A` | 0x701C4 | Aux offset |
| `PLANE_COLOR_CTL_1_A` | 0x701CC | Color control |
| `PLANE_BUF_CFG_1_A` | 0x7027C | Buffer config |
| `PLANE_WM_1_A_0..7` | 0x70240-0x7025C | Watermarks |
| `PLANE_WM_TRANS_1_A` | 0x70268 | Transition watermark |
| `PLANE_NV12_BUF_CFG_1_A` | 0x70278 | NV12 buffer config |

### Cursor Registers

| Register Name | Pipe A Offset | Description |
|---------------|---------------|-------------|
| `CUR_CTL` | 0x70080 | Cursor control |
| `CUR_BASE` | 0x70084 | Cursor base address |
| `CUR_POS` | 0x70088 | Cursor position |
| `CUR_FBC_CTL` | 0x700A0 | Cursor FBC control |
| `CUR_SURFLIVE` | 0x700AC | Cursor live surface |
| `CUR_WM_0..7` | 0x70140-0x7015C | Cursor watermarks |
| `CUR_WM_TRANS` | 0x70168 | Cursor transition WM |
| `CUR_BUF_CFG` | 0x7017C | Cursor buffer config |

### Pipe Scaler Registers

| Register Name | Pipe A Offset | Description |
|---------------|---------------|-------------|
| `PS_PWR_GATE_1` | 0x68160 | Scaler power gate |
| `PS_WIN_POS_1` | 0x68170 | Window position |
| `PS_WIN_SZ_1` | 0x68174 | Window size |
| `PS_CTRL_1` | 0x68180 | Scaler control |
| `PS_VSCALE_1` | 0x68184 | Vertical scale |
| `PS_VPHASE_1` | 0x68188 | Vertical phase |
| `PS_HSCALE_1` | 0x68190 | Horizontal scale |
| `PS_HPHASE_1` | 0x68194 | Horizontal phase |
| `PS_ECC_STAT_1` | 0x681D0 | ECC status |

### Interrupt Registers

| Register Name | Offset | Description |
|---------------|--------|-------------|
| `GEN8_MASTER_IRQ` | 0x44200 | Master interrupt |
| `GEN8_GT_ISR0..3` | 0x44300-0x4433C | GT interrupt status/mask/enable |
| `GEN8_DE_PIPE_ISR0..2` | 0x44400-0x4442C | Display pipe interrupts |
| `GEN8_DE_PORT_ISR` | 0x44440-0x4444C | Port interrupts |
| `GEN8_DE_MISC_ISR` | 0x44460-0x4446C | Misc display interrupts |
| `GEN8_PCU_ISR` | 0x444E0-0x444EC | PCU interrupts |

### MBUS / Display Bandwidth

| Register Name | Offset | Description |
|---------------|--------|-------------|
| `MBUS_ABOX_CTL` | 0x45038 | ABOX control |
| `MBUS_DBOX_CTL_A` | 0x7003C | DBOX control (Pipe A) |
| `WM_LINETIME_A` | 0x45270 | Line time watermark |
| `WM_MISC` | 0x45260 | Watermark misc |

### PSR (Panel Self-Refresh) - For eDP

| Register Name | Offset | Description |
|---------------|--------|-------------|
| `TRANS_PSR_CTL_A` | 0x60800 | PSR control |
| `TRANS_PSR_STATUS_A` | 0x60840 | PSR status |
| `TRANS_PSR_EVENT_A` | 0x60848 | PSR events |
| `TRANS_PSR_MASK_A` | 0x60860 | PSR mask |
| `TRANS_PSR2_CTL_A` | 0x60900 | PSR2 control |
| `TRANS_PSR2_STATUS_A` | 0x60940 | PSR2 status |

---

## Key Values from Tiger Lake Reference

These are the actual values from a working Tiger Lake 1920x1080@60Hz setup:

### Working Timing Values (1920x1080@60Hz)

```
TRANS_HTOTAL_A   = 0x08a3077f  (1920 active, 2212 total)
TRANS_HBLANK_A   = 0x08a3077f  (1920 start, 2212 end)
TRANS_HSYNC_A    = 0x07cf07af  (1968 start, 2000 end)
TRANS_VTOTAL_A   = 0x04730437  (1080 active, 1140 total)
TRANS_VBLANK_A   = 0x04730437  (1080 start, 1140 end)
TRANS_VSYNC_A    = 0x0440043a  (1083 start, 1089 end)
```

### Working Plane Configuration

```
PLANE_CTL_1_A    = 0x82009000  (enabled, format, etc.)
PLANE_STRIDE_1_A = 0x0000003c  (60 blocks = 3840 bytes - see note)
PLANE_SIZE_1_A   = 0x0437077f  (1080-1 << 16 | 1920-1)
PLANE_SURF_1_A   = 0x01c40000  (GGTT offset of framebuffer)
```

**Note:** The stride value of 60 blocks (3840 bytes) from the dump doesn't match the expected 7680 bytes for 1920x32bpp linear. This suggests the system was using either tiled memory, a different pixel format, or half-resolution at the time of the dump. For linear ARGB8888 at 1920 width, use 120 blocks (7680 / 64).

### Working Power State

```
PWR_WELL_BIOS    = 0x00000001  (enabled)
PWR_WELL_DRIVER  = 0x00000003  (PW1 + PW2 enabled)
TRANS_CONF_A     = 0xc0000000  (enabled, active)
TRANS_DDI_FUNC_CTL_A = 0x8a210002  (enabled, DP SST, 6bpc, x2 lanes)
```

### Clock Configuration

```
CDCLK_CTL        = 0x00380158
LCPLL1_CTL       = 0xcc000000
TRANS_CLK_SEL_A  = 0x10000000
```

---

## Linux i915 Source Code References

Key files in the Linux kernel i915 driver to study:

| File | Content |
|------|---------|
| `drivers/gpu/drm/i915/display/intel_display.c` | Display mode setting |
| `drivers/gpu/drm/i915/display/intel_ddi.c` | DDI/port configuration |
| `drivers/gpu/drm/i915/display/intel_dpll.c` | Display PLL management |
| `drivers/gpu/drm/i915/display/intel_fbc.c` | Frame buffer compression |
| `drivers/gpu/drm/i915/display/skl_universal_plane.c` | Skylake+ planes |
| `drivers/gpu/drm/i915/i915_reg.h` | All register definitions |
| `drivers/gpu/drm/i915/gt/intel_gt_regs.h` | GT register definitions |
| `drivers/gpu/drm/i915/display/intel_display_reg_defs.h` | Display register defs |

For Raptor Lake specifically, look for:
- `IS_ALDERLAKE_P()` / `IS_ALDERLAKE_S()` macros
- `IS_ROCKETLAKE()` checks (similar Gen12 architecture)
- Any `ADL_P` / `RPL` specific code paths

---

## Research Checklist

### Phase 1: Basic Info Collection

- [ ] Run `lspci -vvv` to get BAR addresses
- [ ] Run `intel_reg dump --all` on Linux to get register values
- [ ] Document device ID and revision
- [ ] Compare BAR0/BAR1 sizes with Tiger Lake

### Phase 2: Power Management Comparison

- [ ] Compare power well register layouts
- [ ] Compare forcewake register addresses
- [ ] Document any differences in bit positions

### Phase 3: Display Pipeline Comparison

- [ ] Compare transcoder register addresses
- [ ] Compare plane register addresses
- [ ] Check for any new/removed registers
- [ ] Document timing register values for your panel

### Phase 4: Integration

- [ ] Update `xe_hw_offsets.hpp` with Raptor Lake offsets
- [ ] Create safe read-only probe for new registers
- [ ] Implement power-up sequence based on findings

---

## Tools to Use

1. **intel-gpu-tools** - `intel_reg`, `intel_gpu_top`, etc.
2. **Linux kernel source** - i915 driver documentation
3. **Intel PRM** - Programmer's Reference Manual (under NDA or leaked versions)
4. **Hackintool / IORegistryExplorer** - For macOS IOService debugging
5. **ioreg** - macOS command line IORegistry explorer

---

## Safety Recommendations

1. **Never write to unknown registers** - Only read until behavior is understood
2. **Boot flag protection** - Use `xepci=strictsafe` to prevent writes
3. **Incremental testing** - Enable one feature at a time
4. **Always have backup boot** - Keep a known-good OpenCore configuration
5. **Check power state first** - Ensure GPU is powered before MMIO access

---

## References

1. [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext) - Working Tiger Lake reference
2. [Linux i915 driver](https://github.com/torvalds/linux/tree/master/drivers/gpu/drm/i915) - Open source reference
3. Intel PRM Vol 15-17 - Display Engine, GT, etc.
4. [Intel Open Source Graphics](https://01.org/linuxgraphics) - Official Intel resources
