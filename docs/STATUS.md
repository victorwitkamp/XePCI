# XePCI Status

## What the kext does currently

- Attaches to the Intel Xe iGPU PCI device (8086:A788) as an `IOService` named `XeService`.
- Maps BAR0 and reads a very small, fixed set of MMIO registers: `0x0`, `0x4`, `0x10`, `0x100`.
- Logs vendor ID, device ID, revision, BAR0 virtual address, and the values of those registers via `XeLog(...)`.
- Exposes an `IOUserClient` with methods for:
  - Creating a small shared buffer (`kMethodCreateBuffer`).
  - Requesting a NOOP submission (`kMethodSubmit`), which is currently a stub returning `kIOReturnNotReady` from the command stream.
  - Waiting for completion (`kMethodWait`), currently a simple success stub.
  - Reading back a fixed allow-list of safe MMIO dwords (`kMethodReadRegs`), matching the offsets above.
- The command-stream helper (`XeCommandStream`) and GGTT probing helper (`XeGGTT`) are effectively stubs in the current strict-safe configuration and only log when invoked.

## Boot flags (xepci=)

The kext parses a boot-arg of the form:

- `xepci=<comma-separated-flags>`

Supported flags:

- `verbose`
  - Enables extra logging during `XeService::init`/`start`.
  - Logs a one-line summary of the parsed flags and attach information.
- `noforcewake`
  - Requests that any future forcewake logic be disabled. In the current code, the `ForcewakeGuard` is implemented as a no-op stub, so this is effectively redundant but kept for future expansion.
- `nocs`
  - Disables use of the command stream. Currently `XeCommandStream` is already a stub that only logs and returns `kIOReturnNotReady`, so this flag primarily serves as an explicit safety guard for future work.
- `strictsafe`
  - Forces a strict safe mode.
  - Implies `noforcewake` and `nocs` internally.
  - Ensures that:
    - Only BAR0 is mapped and only the small set of sampler-style registers is read.
    - No GGTT probing is performed.
    - No ring or engine registers are read or written.
    - Command-stream submission is reduced to a stub that logs and returns `kIOReturnNotReady`.

## Safety vs. experimental behavior

Safe and enabled right now:

- Attaching to the PCI device and mapping BAR0.
- Reading the small, fixed set of MMIO offsets (0x0, 0x4, 0x10, 0x100).
- Creating kernel/user shared buffers via `ucCreateBuffer`.
- Invoking `ucSubmitNoop`, which currently only calls into a stub that does not touch engine registers.
- Invoking `ucReadRegs`, which returns only the allow-listed sampler offsets.

Experimental / currently disabled paths:

- Any GGTT register probing (e.g., `PGTBL_CTL`) is stubbed out in `XeGGTT::probe()`.
- Any RCS0 ring state logging or tail updates are disabled; `XeCommandStream` methods are stubs.
- Any forcewake-based access patterns are disabled; `ForcewakeGuard` is implemented as a no-op.

As BAR0 layout and register behavior become better understood, individual reads and eventually carefully-scoped writes can be re-enabled behind additional boot flags.

## Future Directions

See [FRAMEBUFFER_ANALYSIS.md](FRAMEBUFFER_ANALYSIS.md) for detailed analysis of the [pawan295/Appleinteltgldriver.kext](https://github.com/pawan295/Appleinteltgldriver.kext) reference implementation, which has achieved a working framebuffer on Tiger Lake.

See [RESEARCH_GUIDE.md](RESEARCH_GUIDE.md) for a comprehensive list of registers to research and the methodology to collect similar data for Raptor Lake.

### Potential Next Steps

1. **Collect Raptor Lake Register Dump**
   - Run `intel_reg dump --all` on Linux with the Raptor Lake GPU
   - Compare offsets with Tiger Lake reference
   - Document any differences in register layouts

2. **IOFramebuffer Integration**
   - The reference implementation uses an `IOFramebuffer` subclass approach
   - This allows WindowServer to recognize the display device
   - Would require porting power management and display pipeline code

3. **Power Well / Forcewake Sequence**
   - Reference shows detailed power-up sequence for Tiger Lake
   - Raptor Lake should have similar but potentially different register offsets
   - Key registers: `PWR_WELL_CTL_1/2`, `FORCEWAKE_RENDER_CTL`

4. **GGTT Implementation**
   - Reference maps BAR1 (GTTMMADR) for Graphics Translation Table
   - Each 4KB page needs a PTE entry
   - Required for GPU to access framebuffer memory

5. **Display Pipeline**
   - Pipe/Transcoder timing configuration
   - Plane control and surface address
   - DDI buffer enable

### Hardware Considerations

The reference targets Tiger Lake (8086:9A49), while XePCI targets Raptor Lake (8086:A788). Both are Gen12 architecture but register offsets may differ. Intel PRMs (Programmer's Reference Manuals) should be consulted for Raptor Lake specifics.
