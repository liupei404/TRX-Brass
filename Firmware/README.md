# TRX Brass Firmware

本目录包含 TRX-Brass 的 GUI 应用、Buildroot 系统镜像构建树和 MicroBlaze FFT 固件源码。
**完整构建流程、产物路径、烧录与已知环境兼容问题以仓库根目录的
[../BUILDING.md](../BUILDING.md) 为准**，本文件只作组件索引。

## 组件说明

| 目录 | 内容 |
|------|------|
| `gui/` | GUI 应用（本仓库普通目录，`brass-gui.mk` 用 `SITE_METHOD = local` 直接构建）；`gui/lvgl/` 是唯一的 git 子模块，版本 9.5 |
| `system/` | Buildroot 2024.02 源码与 `br2_external` 外部树，`config_brass.sh` 由 defconfig 生成 `output_brass/.config` |
| `microblase/` | MicroBlaze FFT 固件源码；`mb.code` / `mb.data` 已预编译在 `gui/rootfs/usr/share/brass/fw/` |

许可证：自有 GUI 代码为 LGPL-2.1-or-later，见 [gui/LICENSE](gui/LICENSE)；
第三方组件保留各自许可证。上游作者 Belousov Oleg (R1CBU)，
上游仓库 `https://github.com/strijar/TRX-Brass`；本仓库托管在
`https://github.com/liupei404/TRX-Brass`。

## 目录结构

```
Firmware/
├── README.md                  # 本文件
├── COMPILE_SUMMARY.md         # system/ 目录与构建整理
├── gui/
│   ├── CMakeLists.txt
│   ├── src/                   # 应用源码（含 power.c 长按关机）
│   ├── lvgl/                  # LVGL 9.5，唯一子模块
│   ├── lv_drivers/            # LVGL 驱动
│   ├── rootfs/                # 打进 rootfs 的 S99gui、brass_daemon 等
│   ├── buildroot/             # 单独交叉编译 GUI 的脚本
│   ├── tests/                 # 主机端单元测试
│   ├── LICENSE
│   └── README.md
├── microblase/                # MicroBlaze FFT 固件源码
└── system/
    ├── br2_external/
    │   ├── board/             # 板级配置、7020 system.bit、U-Boot 补丁
    │   ├── configs/           # brass7020_defconfig / brass7010_defconfig
    │   └── package/           # 自定义软件包（brass-gui、rhvoice 等）
    ├── buildroot-2024.02/
    ├── buildroot-2024.02.tar.gz
    ├── config_brass.sh
    └── output_brass/          # 构建产物，被 .gitignore 忽略，不进仓库
```

## 快速开始（简版）

请在**独立 SSH 会话**中执行（不要在 IDE 内置终端跑完整编译，删除保护 shim 会拦截
Buildroot 的 `rm`），完整说明见 [../BUILDING.md](../BUILDING.md) 1.1 节：

```bash
cd system
mv output_brass output_brass.old     # 旧产物另存；本机 make clean 会被删除保护拦截
./config_brass.sh                    # 只生成 .config，不编译
cd output_brass
make -j2 2>&1 | tee /tmp/brass-build.log
```

产物为 `system/output_brass/images/sdcard.img`，首次全编译约 2–5 小时。
编译期间 `images/` 里先出现蓝牙固件子目录是正常现象，不代表镜像完成。

全编译成功后只改 `gui/`：

```bash
cd system/output_brass
make brass-gui-dirclean   # 强制重新 rsync 仓库内 GUI 源码
make -j2                  # 几分钟内重出 sdcard.img
```

只调试 GUI、不出镜像：用 `gui/buildroot/cmake.sh` 交叉编译出 `brass_gui`，
`scp` 到板子 `/usr/sbin/brass_gui`，再 `/etc/init.d/S99gui restart`。

MicroBlaze FFT 固件可选重编（需要 Xilinx `mb-gcc`）：`cd microblase && make`。
仓库已带预编译的 `mb.code` / `mb.data`，缺 `mb-gcc` 不影响出镜像。

## 重新编译场景

| 场景 | 做法 | 预计时间 |
|------|------|---------|
| 首次出镜像 | `./config_brass.sh` 后 `make -j2` 全编译 | 2–5 小时 |
| 只改 `gui/` | `make brass-gui-dirclean && make -j2` | 几分钟 |
| 只调试 GUI，不出镜像 | `gui/buildroot/cmake.sh` + `scp` 覆盖板子程序 | 1–3 分钟 |
| 改内核 / defconfig | 重新生成 `.config` 后 `make -j2` | 视改动而定 |

## 主要功能

- 多模式支持：LSB、USB、CW、CWR、AM、NFM、RTTY、Olivia、FT8
- 频率控制、实时 FFT 频谱显示、CW 编码/解码
- ATU 天调、SWR 扫描
- 电源管理：短按切背光；长按 1000 ms 保存设置、`sync()` 后关机
- 系统：Linux 6.1（Xilinx 定制）、Buildroot 2024.02、ALSA / PipeWire、RHVoice、
  Liquid-DSP、libspecbleach

## 运行与排障

- GUI 由 `/etc/init.d/S99gui` 启动 `brass_daemon`，再由它调用 `/usr/sbin/brass_gui`
- 串口调试口 `ttyPS0,115200`，默认 root 口令 `123`
- 交叉工具链由 Buildroot 自行生成在 `system/output_brass/host/`，主机不需要预装
  `arm-linux-gnueabihf-gcc`
- 镜像下载、烧录命令和各类构建报错处理见 [../BUILDING.md](../BUILDING.md)

## 文档索引

- [../BUILDING.md](../BUILDING.md) — 构建、产物位置、烧录、环境兼容处理（主文档）
- [../README.md](../README.md) — 项目总览
- [COMPILE_SUMMARY.md](COMPILE_SUMMARY.md) — `system/` 目录与构建整理
- [gui/README.md](gui/README.md) — GUI 说明
- [../Docs/TRX-Brass-CodeChanges.html](../Docs/TRX-Brass-CodeChanges.html) — 代码修改记录（含电源管理）
- [../Docs/TRX-Brass-TODO.html](../Docs/TRX-Brass-TODO.html) — 版本差异分析与待办清单

> 电源管理实现在 `gui/src/power.c` / `power.h`，主机端测试在 `gui/tests/power_test.c`。
> 本仓库没有 `BUILD_GUIDE.md`，也没有 `POWER_QUICK_REFERENCE.md` / `POWER_CONTROL.md`
> 等文档，旧版 README 里的这些链接已移除。

## 硬件要求

### 开发环境

- x86_64，>= 4 核、>= 8 GiB 内存、>= 20 GiB 可用空间
- Ubuntu（已在 25.10 / GCC 15 / uutils coreutils 上验证），或 Windows 终端 SSH 到 Ubuntu
- 主机工具：`make` / `gcc` / `g++` / `wget` / `bison` / `flex` / `perl` / `rsync` /
  `cpio` / `python3` / CMake ≥ 3.23
- 编译镜像不需要 Vivado / Vitis；仓库已带 7020 的 `system.bit`（Vivado 2019.2 生成）

### 目标硬件

- Xilinx Zynq-7000（7020 为默认目标，7010 需换 defconfig）
- ARM Cortex-A9 双核、>= 512 MB RAM、>= 4 GB SD 卡
- 800×480 触摸屏、音频输入/输出

## 附录

### 支持的频段

- **HF**: 160m, 80m, 40m, 30m, 20m, 17m, 15m, 12m, 10m, 6m
- **VHF**: 2m
- **UHF**: 70cm

### 支持的模式

- **SSB**: LSB, USB
- **CW**: CW, CWR
- **AM**: AM
- **FM**: NFM
- **数字**: RTTY, Olivia, FT8

### 技术栈

- **GUI**: LVGL 9.5
- **操作系统**: Linux 6.1 (`xlnx_rebase_v6.1_LTS_2023.1`)
- **构建系统**: Buildroot 2024.02 + 外部树 `br2_external`
- **交叉工具链**: Buildroot 自带 ARM GCC 12.3
- **FPGA**: 仓库内 `system.bit`（Vivado 2019.2）
- **音频**: ALSA, PipeWire
- **数字信号**: Liquid-DSP (ext-liquid-dsp), libspecbleach
- **语音**: RHVoice

---

**TRX Brass Firmware - 业余无线电收发机完整固件**
