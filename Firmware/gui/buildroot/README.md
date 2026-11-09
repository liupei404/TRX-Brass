# Helpers for Buildroot

These scripts build the GUI outside of the main Buildroot tree, which makes
debugging easier. They do **not** work on a fresh clone by themselves.

# Prerequisites

1. Build the system first so that the cross toolchain and target sysroot exist:

   ```
   cd Firmware/system
   ./config_brass.sh
   make
   ```

   `conf.sh` points at `../../system/output_brass`; `cmake.sh` uses
   `host/share/buildroot/toolchainfile.cmake` from that directory.

2. Initialise the LVGL submodule (LVGL 9.5) from the repository root:

   ```
   git submodule update --init --recursive
   ```

3. Host tools: CMake >= 3.23, make, Freetype development files in the target
   sysroot, and the Buildroot cross compiler in `output_brass/host/bin`.

# Usage

```
./cmake.sh
make
```

There is no `install.sh`. Installing into a running target is done either by
building the full image (`Firmware/system/output_brass/images/sdcard.img`) or
by copying `brass_gui` to `/usr/sbin/` on the target yourself.

The `CONFIG_LV_BUILD_*` options in `cmake.sh` match the LVGL 9.5 options used
by `Firmware/gui/CMakeLists.txt`.
