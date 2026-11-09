# Building TRX Brass

Three independent build chains. They must be run in this order on a fresh
checkout.

## 0. Prerequisites

- Clone with submodules (LVGL 9.5):

  ```
  git clone --recurse-submodules <your-repository-url>
  # or, on an existing clone:
  git submodule update --init --recursive
  ```

- Host tools: `make`, `wget`, CMake >= 3.23, a C/C++ compiler for host-side
  tests, and `mb-gcc` (Xilinx MicroBlaze toolchain) for the FFT firmware.
- The first Buildroot run downloads `buildroot-2024.02.tar.gz`, so network
  access is required once.

## 1. System image (Buildroot)

```
cd Firmware/system
./config_brass.sh          # downloads Buildroot if needed, applies brass7020_defconfig
make
```

`config_brass.sh` uses `BR2_EXTERNAL=../br2_external` and
`O=../output_brass`. For the Zynq 7010 variant, replace the defconfig with
`brass7010_defconfig`.

Outputs:

- `Firmware/system/output_brass/host/` — cross toolchain and target sysroot
- `Firmware/system/output_brass/images/sdcard.img` — flashable SD image
  (boot / rootfs / data partitions, see `br2_external/board/brass/genimage.cfg`)
- `br2_external/package/brass-gui/brass-gui.mk` builds the GUI directly from
  the in-tree `Firmware/gui` directory (`SITE_METHOD = local`)

Flash `sdcard.img` to an SD card, then boot. U-Boot loads `system.bit` before
the kernel; the GUI is started by `/etc/init.d/S99gui`.

## 2. GUI, standalone cross build (for debugging)

Requires step 1 to have produced the toolchain and sysroot.

```
cd Firmware/gui/buildroot
./cmake.sh                 # uses ../../system/output_brass
make
```

`conf.sh` points at `../../system/output_brass`; `cmake.sh` uses
`host/share/buildroot/toolchainfile.cmake`. There is no `install.sh` — see
`Firmware/gui/buildroot/README.md`.

## 3. MicroBlaze FFT firmware

```
cd Firmware/microblase
make                       # mb-gcc + mb.lds -> mb.code, mb.data, mb.dump
```

Copy `mb.code` and `mb.data` into
`Firmware/gui/rootfs/usr/share/brass/fw/` before building the image. The GUI
loads them through `/dev/uio1` and `/dev/uio2` (`src/fpga/mb.c`).

## Host-side unit test

Does not need a cross toolchain or target hardware:

```
cd Firmware/gui
gcc -std=c11 -Wall -Wextra -Werror -DPOWER_TEST_STUB -Isrc \
    tests/power_test.c src/power.c -o /tmp/power_test
/tmp/power_test
```

`POWER_TEST_STUB` prevents `power_shutdown()` from issuing a real
`reboot(RB_POWER_OFF)`.

## Runtime control

```
/etc/init.d/S99gui {start|stop|restart|reload}
```

Debug console: `ttyPS0,115200`.
