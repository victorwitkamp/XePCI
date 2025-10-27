# docs/bringup-checklist.md — Raptor Lake-P (8086:A788)

**Target PCI:** `Vendor 0x8086`, `Device 0xA788` (decimal `42888`) — RPL-P (Gen12.2).
**BAR0:** GTTMMADR (MMIO). Use Linux `i915/xe` as the register truth.

## 0) Read-only MMIO
- Map BAR0; confirm sane dword reads.
- Dump a few safe blocks (e.g., `0x0000`, `0x0100`, `0x1000`) and GTSCRATCH.

## 1) Power / Forcewake
- Program Gen12 forcewake (render/gt); enable GT power wells; ungate GT clocks.

## 2) GGTT
- Program PGTBL_CTL; set up global GTT; install scratch PTEs; verify CPU aperture reads.

## 3) Firmware
- Load GuC/HuC (DMA upload → auth → status); handle error/reset paths.

## 4) Rings / IRQ
- Init one engine (Render/Compute): ring ctx/HWSP, head/tail, doorbell; enable seqno/error/page-fault IRQs.

## 5) First Submit
- Batch: `MI_NOOP; MI_BATCH_BUFFER_END`; ring doorbell; wait seqno; timeout → GT reset.

## 6) BO / GTT manager
- Pinned pages, GGTT map, PAT, fences/refs; kAPI: create/map/destroy.

## 7) Early Ops
- BLT memfill/copy to BO (then CPU blit to FB) **or** fixed compute write-pattern.

## 8) Optional
- IOUserClient: `createBuffer`, `submit`, `wait`, `readRegister`.
- Dumb IOFramebuffer for visual debug.
