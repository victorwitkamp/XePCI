# XePCI – Technical Overview (Current State)

This repository contains a standalone macOS kernel extension (`XePCI.kext`) and a small userspace tool (`xectl`) for bringing up and experimenting with Intel Xe‑LP iGPUs, with a primary focus on **Raptor Lake**.

This README intentionally avoids build/CI/test instructions and only documents the **current technical state** of the project.

---

## Target Hardware

- Intel Raptor Lake HX / Xe‑LP iGPU
- Primary target device: **PCI 8086:A788**
- Architecture: **Gen 12.2 (Xe‑LP)**
- Assumes a Hackintosh / OpenCore environment where the iGPU is visible as a PCI device.

Other Gen12 device IDs may be recognized in code, but only 0xA788 is considered the main supported target.

---

## Architecture

The project consists of:

- `XePCI.kext` — standalone PCI driver that:
    - Attaches to `IOPCIDevice` for the Intel iGPU.
    - Maps BAR0 (GTTMMADR) into kernel virtual memory.
    - Provides safe MMIO register access helpers.
    - Implements minimal bring‑up scaffolding (forcewake, GT config, BOs, ring/GGTT stubs).
    - Exposes an `IOUserClient` (`XeUserClient`) for userspace experiments.

- `xectl` — a thin CLI that:
    - Opens the `XeService` IOService.
    - Calls the small set of exported user‑client methods.
    - Prints device / GT info and basic register reads.

There is **no** IOAccelerator or framebuffer integration active right now; the focus is on low‑level MMIO and scaffolding.

---

## `XePCI.kext`

Main concepts:

- **PCI attach**
    - Matches vendor `0x8086` and device `0xA788` via `Info.plist` personality `XeService`.
    - Enables memory + bus mastering on the PCI function.
    - Maps BAR0 to a `volatile uint32_t *mmio` pointer.

- **Register access**
    - 32‑bit MMIO helpers, used throughout the driver:

        ```c
        inline uint32_t readReg(uint32_t off) {
            return mmio ? mmio[off >> 2] : 0;
        }

        inline void writeReg(uint32_t off, uint32_t val) {
            if (mmio) { mmio[off >> 2] = val; OSSynchronizeIO(); }
        }
        ```

- **Forcewake (GT domain)**
    - Uses Gen12 forcewake registers to keep GT powered while reading GT registers.
    - Wraps access with a `ForcewakeGuard` RAII helper so GT domains are acquired/released safely.

- **GT configuration readout**
    - Reads thread status and DSS enable registers (e.g. `GEN12_GT_THREAD_STATUS`, `GEN12_GT_GEOMETRY_DSS_ENABLE`).
    - Computes a basic “enabled DSS / EU count” for logging / sanity checks.

- **Buffer objects (BOs)**
    - Uses `IOBufferMemoryDescriptor` to allocate pinned kernel buffers.
    - Keeps a small `OSArray` of BOs and exposes them via a numeric “cookie” to userspace (no direct pointers).

- **GGTT / ring / GuC**
    - Structures and stubs exist for:
        - GGTT state
        - A single render/compute ring (head, tail, base, size)
        - GuC firmware state
    - Today these are **scaffolding only**: they allocate memory and log, but do not program the real hardware rings or submit commands.

- **`XeService` IOService**
    - Core driver class (`XeService` derives from `IOService`).
    - Responsible for:
        - PCI attach / detach.
        - BAR0 map / unmap.
        - Forcewake tests and GT config logging in `start()`.
        - Managing the BO list.
        - Creating the `XeUserClient` when userspace connects.

---

## `xectl` (userspace)

`xectl` is a single‑file C tool (`userspace/xectl.c`) that:

- Locates `XeService` via IOKit matching (bundle ID / class name).
- Opens an `IOUserClient` connection.
- Issues a small set of method calls for:
    - Device info (vendor/device/revision, basic GT info).
    - Simple BO allocation.
    - Register dumps / basic GT configuration.

Its CLI commands (e.g. `info`, `regdump`, `noop`, `mkbuf`) are thin wrappers around the kernel ABI described below.

---

## Current Feature Matrix

| Area                      | Status         | Notes                                                 |
|---------------------------|----------------|-------------------------------------------------------|
| PCI attach / BAR0 map     | ✅ working     | Matches 8086:A788, maps BAR0 into kernel space        |
| Register access helpers   | ✅ working     | 32‑bit MMIO, guarded against null `mmio`              |
| Forcewake (GT)            | ✅ working     | RAII guard used when reading GT registers             |
| GT config readout         | ✅ working     | Thread status + DSS enable, basic EU count            |
| BO allocation / tracking  | ✅ working     | `IOBufferMemoryDescriptor` + cookie‑based registry    |
| XeService / XeUserClient  | ✅ working     | User client exported, used by `xectl`                 |
| GGTT structures           | 🔄 scaffolding | Types + basic stubs, not programming HW PTEs yet      |
| Ring buffer structures    | 🔄 scaffolding | Alloc + in‑memory ring model, no HW ring programming  |
| Command submission        | 🔄 scaffolding | MI_NOOP path prepared, not actually hitting GPU ring  |
| GuC firmware              | 🔄 scaffolding | Status reads + placeholders, no real firmware load    |
| IOAccelerator / FB        | ⏳ future      | No IOAccel or IOFramebuffer subclasses in use now     |

Legend: ✅ implemented and used · 🔄 present but not completing hardware flow · ⏳ not started / only ideas.

---

## Key Data Structures (as used today)

The actual structures live in `kexts/` headers; this section only describes the intent.

- **`XeService`**
    - Holds:
        - `IOPCIDevice *pci` — the matched GPU PCI device.
        - `IOMemoryMap *bar0` / `volatile uint32_t *mmio` — BAR0 mapping.
        - `OSArray *m_boList` — small BO registry (stores `IOBufferMemoryDescriptor *`).
    - Owns the lifetime of MMIO and BOs.

- **BO representation**
    - Each BO: `IOBufferMemoryDescriptor *mem` plus an implicit index in `m_boList`.
    - Userspace sees only a `uint64_t cookie` that indexes into this array.

- **Ring / GGTT / GuC**
    - Structures exist to track a single ring’s base, size, and pointers, and to hold GGTT / GuC state.
    - At present these are only used for allocation / logging; they **do not** yet drive real hardware submission.

---

## Kernel ↔︎ User ABI (IOUserClient)

`XeUserClient` exposes a small fixed set of method selectors. The exact enum is defined in the kext sources; conceptually:

| Selector | Name             | Direction          | Description                                  |
|---------:|------------------|--------------------|----------------------------------------------|
| 0        | `createBuffer`   | in: bytes (u64)    | Allocates a BO, returns a cookie (u64)       |
| 1        | `submitNoop`     | none               | Placeholder for MI_NOOP submission           |
| 2        | `wait`           | in: timeout (u32)  | Placeholder wait API (no real fence yet)     |
| 3        | `readRegs`       | in: count (u32)    | Returns up to N dwords of MMIO register dump |

This ABI is **experimental** and only considered stable enough for the in‑tree `xectl` tool.

---

## Logging and Where to Look

The kext logs under names like `XeService` / `XePCI` using `IOLog` / os_log‑style calls.

When debugging on macOS (on the target machine):

- Use `log stream --predicate 'process == "kernel"' --style compact | grep Xe` to watch live messages.
- Use `log show --last 5m --predicate 'eventMessage CONTAINS[cd] "Xe"'` after a run to review history.
- Look for lines that confirm:
    - PCI match and BAR0 map.
    - Forcewake acquire/release.
    - GT thread status / DSS enable reads.
    - BO create/destroy activity.

---

## Short Roadmap (high‑level)

Non‑exhaustive list of next technical steps, in rough order:

1. **Real GGTT programming**
     - Program PGTBL_CTL and populate GGTT entries instead of just stubs.
2. **Single engine ring bring‑up**
     - Program ring head/tail/start/ctl registers and verify idle/active transitions.
3. **End‑to‑end MI submission**
     - Submit a minimal batch (`MI_NOOP` + `MI_BATCH_BUFFER_END`) through the real ring and observe seqno/interrupts.
4. **GuC firmware bring‑up (optional)**
     - Load GuC from a userspace‑provided blob and verify status.
5. **Optional higher‑level integration**
     - Experiment with an IOAccelerator or dumb IOFramebuffer shim once low‑level flows are reliable.

This roadmap is descriptive, not prescriptive: the code in `kexts/` is the source of truth for the current state.

---

## Reference Analysis

See [docs/FRAMEBUFFER_ANALYSIS.md](docs/FRAMEBUFFER_ANALYSIS.md) for a detailed analysis of the [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext) project, which has achieved a working framebuffer on Intel Tiger Lake. Key learnings include:

- **IOFramebuffer** subclass approach for WindowServer integration
- **Power management** sequence (power wells, forcewake)
- **GGTT mapping** for GPU-accessible framebuffer memory
- **Display pipeline** configuration (pipe, transcoder, plane, DDI)

See [docs/RESEARCH_GUIDE.md](docs/RESEARCH_GUIDE.md) for a comprehensive guide on researching Raptor Lake GPU registers, including:

- **Register dump methodology** using Linux intel-gpu-tools
- **Register categories** to collect (power, clocks, planes, transcoders)
- **Working values** from Tiger Lake for comparison
- **Linux i915 source** references for each subsystem
