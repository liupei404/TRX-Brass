# TRX Brass

A homebrew amateur-radio SDR transceiver: hardware boards, FPGA / MicroBlaze
firmware, an embedded Linux system and an LVGL-based touch GUI.

| Item | Value |
|---|---|
| Code baseline | main repository `a2122d0`, GUI `42bafc8` |
| Target | Xilinx Zynq-7000 (7010 / 7020), Linux 6.1 Xilinx, U-Boot 2023.01 |
| GUI | LVGL 9.5.0 + embedded Python 3.11, 800x480 framebuffer |
| Bands | HF ~ 6 m / 2 m / 70 cm |
| Modes | LSB, USB, CW, CWR, AM, NFM, RTTY, Olivia, DMR, FT8/FT4, RX SSTV |
| License | Own code LGPL-2.1-or-later; vendored components keep their own licenses |

## Repository layout

| Path | Contents |
|---|---|
| `Docs/` | Datasheets, module drawings, analysis reports and change records |
| `Hardware/` | `MainUnit/` (V1.1), `RadioUnit/` (V2.0), `Case/`, schematics, BOM, Gerbers, STEP models |
| `Firmware/system/` | Buildroot 2024.02 with a custom `br2_external` tree, device tree, U-Boot, rootfs overlay |
| `Firmware/microblase/` | MicroBlaze FFT firmware (`mb-gcc`, produces `mb.code` / `mb.data`) |
| `Firmware/gui/` | LVGL application, `rootfs/` overlay, fonts, Python UI scripts, `tests/` |

`Firmware/gui` is a normal directory in this repository. The third-party LVGL
tree at `Firmware/gui/lvgl` is the only remaining submodule, so clone with:

```
git clone --recurse-submodules <your-repository-url>
```

## Power button

- **Short press**: toggle the display backlight (unchanged behaviour).
- **Long press (1000 ms)**: save all settings, `sync()`, then
  `reboot(RB_POWER_OFF)`. If the kernel has no poweroff driver, the GUI exits
  in an orderly way instead (`main_exit()` → `exit(1)`, which stops
  `brass_daemon` rather than restarting it).
- **Power on**: long-press detection while the CPU is unpowered must be done
  by the power-latch / FPGA circuitry. The GUI only reads the power key state
  at boot as a diagnostic; it never powers the device off on its own by
  default. See `Docs/TRX-Brass-CodeChanges.html` for details.

## Getting started

See `BUILDING.md` for the three build chains and flashing instructions.

Runtime configuration lives on the third SD partition, mounted at `/mnt`:
`/mnt/settings/*.yaml` (bands, modes, options, rf, hw, app/ft8). Only
bands/modes/options/rf are saved on exit.

## Project status

This is a working prototype, not a finished product. In particular:

- no project-level test suite (only a host-side unit test for the power module)
- no CI, several `FIXME` / `REWRITE` leftovers
- the V2.0 RF board still needs the rework listed in
  `Hardware/RadioUnit/FixList.txt`
- default root password and SSH are still enabled in both defconfigs

Full audits and the open item list are in:

- `Docs/TRX-Brass-TODO.html` — version change analysis and TODO / fix list
- `Docs/TRX-Brass-CodeChanges.html` — code change record (long-press power)

## Upstream

Based on the original project by Belousov Oleg (R1CBU),
`https://github.com/strijar/TRX-Brass`.
