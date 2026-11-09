# TRX-Brass

[![License](https://img.shields.io/badge/license-LGPL--2.1--or--later-blue.svg)](Firmware/gui/LICENSE)
[![Platform](https://img.shields.io/badge/platform-Zynq7020-orange.svg)](Hardware/MainUnit/)

一个基于 Xilinx Zynq7020 的业余无线电收发一体机项目，包含完整的硬件设计、FPGA 比特流和嵌入式 Linux 软件系统。

> 本仓库基于 Belousov Oleg (R1CBU) 的上游项目
> `https://github.com/strijar/TRX-Brass`。构建流程、产物路径和已知的环境兼容问题
> 一律以 [BUILDING.md](BUILDING.md) 为准，逐次代码修改见
> [Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html)。

## 项目概述

TRX-Brass 是一款业余无线电收发一体机，采用 Zynq7020 SoC 平台，支持多种调制模式、数字信号处理和图形化用户界面。

### 核心特性

- **收发模式**: LSB/USB/CW/CWR/AM/NFM/RTTY/Olivia/FT8
- **显示系统**: 800×480 TFT LCD 触摸屏，60Hz 刷新率
- **射频前端**: LTC2208 ADC (16位, 125MSPS) / DAC904E DAC (14位, 165MSPS)
- **音频系统**: NAU88C22/NAU8810 编解码器，48kHz I2S 音频
- **DSP 处理**: FFT、AGC、噪声抑制、FIR 滤波
- **数字模式**: FT8、RTTY、Olivia
- **图形界面**: LVGL 9.5 图形用户界面
- **操作系统**: Linux 6.1 (Xilinx `xlnx_rebase_v6.1_LTS_2023.1`，Buildroot 2024.02)
- **固件镜像**: `sdcard.img` 整卡镜像；仓库自带 7020 的真实 `system.bit`，编译镜像不需要本机安装 Vivado

## 硬件规格

### 主控单元 (MainUnit V1.1)

| 组件 | 规格 |
|------|------|
| **SoC** | Xilinx Zynq7020 (xc7z020-clg400-1) |
| **内存** | DDR3 1GB |
| **ADC** | LTC2208 16位 125MSPS |
| **DAC** | DAC904E 14位 165MSPS |
| **音频编解码** | NAU88C22 (主)、NAU8810 (副) |
| **显示** | 800×480 TFT LCD 触摸屏 |
| **按键** | 旋转编码器 ×2、矩阵键盘 |

### 射频单元 (RadioUnit V2.0)

| 组件 | 规格 |
|------|------|
| **功放** | GALI-74 MMIC |
| **变频器** | VHF/UHF 变频器 |
| **滤波器** | BPF/LPF 切换滤波器组 |

### 频率覆盖

- **短波**: 3.5 - 30 MHz (业余频段)
- **VHF/UHF**: 支持 (需 RadioUnit)

## 软件架构

### 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    TRX-Brass 软件架构                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  GUI 层 (LVGL 9.5)                                      │   │
│  │  - 主屏幕、频谱显示、设置对话框                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  应用层 (brass_gui)                                      │   │
│  │  - 射频控制、音频处理、数字模式                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  DSP 层                                                │   │
│  │  - AGC、FIR 滤波、FFT、噪声抑制                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  硬件抽象层 (HAL)                                       │   │
│  │  - GPIO、IIO、FPGA 控制、电源管理                        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Linux 内核 6.1 + Buildroot 2024.02                      │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  FPGA 硬件 (仓库内 system.bit / Vivado 2019.2)            │   │
│  │  - ADC/DAC、显示、音频、DSP 加速                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 目录结构

```
TRX-Brass/
├── BUILDING.md                     # 构建、产物路径、烧录、环境兼容处理（主文档）
├── README.md                       # 本文件
├── Docs/
│   ├── TRX-Brass-CodeChanges.html  # 代码修改记录
│   ├── TRX-Brass-TODO.html         # 版本差异分析与待办清单
│   ├── Datasheet/                  # 芯片数据手册 (PDF)
│   └── XME0724/                    # 显示模块资料 (PDF)
├── Firmware/
│   ├── README.md                   # 固件组件概述
│   ├── COMPILE_SUMMARY.md          # system/ 目录与构建整理
│   ├── gui/                        # GUI 应用（本仓库普通目录，不再是子模块）
│   │   ├── src/                    # 应用源码（含 power.c 长按关机）
│   │   ├── lvgl/                   # LVGL 9.5，唯一的 git 子模块
│   │   ├── lv_drivers/             # LVGL 驱动
│   │   ├── rootfs/                 # 打进 rootfs 的启动脚本与资源
│   │   ├── buildroot/              # 单独交叉编译 GUI 的脚本
│   │   ├── tests/                  # 主机端单元测试
│   │   └── CMakeLists.txt
│   ├── microblase/                 # MicroBlaze FFT 固件源码
│   └── system/
│       ├── br2_external/           # Buildroot 外部树
│       │   ├── board/              # 板级配置、7020 system.bit、U-Boot 补丁
│       │   ├── configs/            # brass7020_defconfig / brass7010_defconfig
│       │   └── package/            # 自定义软件包（含 brass-gui）
│       ├── buildroot-2024.02/      # Buildroot 2024.02 源码
│       ├── config_brass.sh         # 由 defconfig 生成 output_brass/.config
│       └── output_brass/           # 构建产物（被 .gitignore 忽略，不进仓库）
└── Hardware/
    ├── MainUnit/                   # 主控单元 V1.1（原理图 / BOM / Gerber）
    ├── RadioUnit/                  # 射频单元 V2.0（原理图 / BOM / Gerber / FixList.txt）
    ├── Case/                       # 机壳设计
    ├── MainUnit.step
    └── RadioUnit.step
```

> 固件镜像只在这台编译机器的 `Firmware/system/output_brass/images/` 生成，
> 不会出现在 GitHub 仓库里（构建产物被 `.gitignore` 忽略）。

## 快速开始

### 环境要求

- **开发主机**: Ubuntu（已在 Ubuntu 25.10 / GCC 15 / uutils coreutils 上验证），
  或 Windows 终端 SSH 到 Ubuntu。**完整编译不要在 IDE 内置终端运行**：IDE 的
  `safe-delete` shim 会拦截 Buildroot/autoconf 的 `rm`，详见
  [BUILDING.md](BUILDING.md) 1.1 节。
- **Buildroot**: 2024.02，源码与 tarball 已在仓库内，无需另外下载
- **交叉工具链**: 由 Buildroot 自行构建（ARM GCC 12.3），主机不需要预装 ARM GCC
- **子模块**: LVGL 9.5 是唯一子模块，
  `git clone --recurse-submodules <仓库地址>` 或 `git submodule update --init --recursive`
- **主机工具**: `make` / `gcc` / `g++` / `wget` / `bison` / `flex` / `perl` /
  `rsync` / `cpio` / `python3` / CMake ≥ 3.23
- **Vivado / Vitis**: 编译 SD 卡镜像**不需要**。仓库已带 7020 的真实比特流
  （`system.bit`，Vivado 2019.2 生成）；只有修改 FPGA 逻辑、重新生成比特流时才需要。
- **MicroBlaze `mb-gcc`**: 只有重编 FFT 固件才需要。`mb.code` / `mb.data` 已预编译在
  `Firmware/gui/rootfs/usr/share/brass/fw/`，缺它不影响出镜像。

### 编译固件

首次**必须完整编译一次**（交叉工具链、内核、rootfs 和打包工具都要从零生成，
约 2–5 小时）。完整命令、下载源备用方案与所有已遇到的报错处理见
[BUILDING.md](BUILDING.md) 1.1 节，简要流程如下，请在独立 SSH 会话中执行：

```bash
cd ~/TRX-Brass/Firmware/system
mv output_brass output_brass.old    # 旧产物另存；本机 make clean 会被删除保护拦截
./config_brass.sh                   # 从 brass7020_defconfig 重新生成 .config
cd output_brass
make -j2 2>&1 | tee /tmp/brass-build.log
```

`config_brass.sh` **只生成配置，不编译**，真正出镜像的是后面的 `make -j2`。
产物为 `Firmware/system/output_brass/images/sdcard.img`；编译期间 `images/`
里先出现 `linux-firmware` 的蓝牙固件子目录属正常现象，不代表镜像已完成。
7010 目标需改用 `brass7010_defconfig`。

**全编译成功之后**，只改 `Firmware/gui/` 几分钟就能重出镜像：

```bash
cd Firmware/system/output_brass
make brass-gui-dirclean   # 强制重新 rsync 仓库内 GUI 源码
make -j2
```

想更快调试（不出 IMG），可用 `Firmware/gui/buildroot/cmake.sh` 单独交叉编译出
`brass_gui`，`scp` 到已烧好系统的板子 `/usr/sbin/brass_gui`，再执行
`/etc/init.d/S99gui restart`。

**MicroBlaze FFT 固件**（可选，需要 Xilinx `mb-gcc`）：

```bash
cd Firmware/microblase
make
```

### 烧录固件

```bash
cd Firmware/system/output_brass/images
# 取文件：scp，或 python3 -m http.server 8000 后用浏览器下载（详见 BUILDING.md）
sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

**务必确认 `/dev/sdX` 是 SD 卡**，别写到系统盘。Windows 下可用 balenaEtcher /
Win32DiskImager 直接选 `sdcard.img`。串口调试口 `ttyPS0,115200`，默认 root 口令 `123`。

FPGA 比特流**不需要单独烧录**：`board/brass/post-build.sh` 会把仓库内的
`7020/system.bit` 复制进 `images/`，`genimage.cfg` 打进 boot 分区，U-Boot 启动时加载。
要换成自己的比特流，替换
`Firmware/system/br2_external/board/brass/7020/system.bit` 后重新 `make` 即可。

### 运行系统

1. 将 SD 卡插入设备
2. 上电启动
3. `/etc/init.d/S99gui` 自动启动 `brass_daemon` → `brass_gui`

## 功能说明

### 收发模式

| 模式 | 说明 | 频率范围 |
|------|------|----------|
| LSB | 下边带 | 3.5 - 30 MHz |
| USB | 上边带 | 3.5 - 30 MHz |
| CW | 电报 | 所有频段 |
| CWR | 电报接收 | 所有频段 |
| AM | 调幅 | 广播频段 |
| NFM | 窄带调频 | VHF/UHF |
| RTTY | 电传打字 | HF 频段 |
| Olivia | 数字通信 | HF 频段 |
| FT8 | 数字模式 | HF 频段 |

### DSP 功能

- **AGC (自动增益控制)**: 自适应信号放大
- **FIR 滤波**: 可配置带宽滤波器
- **FFT (快速傅里叶变换)**: 实时频谱显示
- **噪声抑制**: EMNR 算法
- **CW 解码**: 电报自动解码

### 设置系统

- **音频设置**: 增益、EQ、噪声抑制
- **射频设置**: 频率步进、AGC 参数
- **显示设置**: 亮度、背光
- **硬件设置**: GPIO、功放偏置

## 项目文档

### 构建与运行

- [BUILDING.md](BUILDING.md) — 全新编译、产物位置、下载与烧录、GCC 15 / 内核下载 /
  uutils dd 等环境兼容处理、GUI 增量重打包（**首先看这个**）
- [Firmware/README.md](Firmware/README.md) — 固件组件概述
- [Firmware/COMPILE_SUMMARY.md](Firmware/COMPILE_SUMMARY.md) — `Firmware/system/` 目录与构建整理
- [Firmware/gui/README.md](Firmware/gui/README.md) — GUI 说明

### 变更与待办

- [Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html) — 代码修改记录
- [Docs/TRX-Brass-TODO.html](Docs/TRX-Brass-TODO.html) — 版本差异分析与待办清单
- [Hardware/RadioUnit/FixList.txt](Hardware/RadioUnit/FixList.txt) — V2.0 射频板改板项

### 资料

- [Docs/Datasheet/](Docs/Datasheet/) — ADC/DAC/音频/IO 芯片数据手册
- [Docs/XME0724/](Docs/XME0724/) — 显示模块资料

### 电源管理

长按关机实现在 [Firmware/gui/src/power.c](Firmware/gui/src/power.c)：短按切背光，
长按 1000 ms 保存全部设置、`sync()` 后 `reboot(RB_POWER_OFF)`；说明见
[Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html)。
长按开机 / 电源自锁需要电源锁存电路或 FPGA 硬件完成，不是软件功能。

## 技术栈

| 类别 | 技术 |
|------|------|
| **GUI** | LVGL 9.5（`Firmware/gui/lvgl`，唯一子模块） |
| **OS** | Linux 6.1 (Xilinx `xlnx_rebase_v6.1_LTS_2023.1`) |
| **构建系统** | Buildroot 2024.02 + 外部树 `br2_external` |
| **交叉工具链** | Buildroot 自带 ARM GCC 12.3 |
| **FPGA** | 仓库内 `system.bit`（Vivado 2019.2 生成；编译镜像不需要 Vivado/Vitis） |
| **DSP** | ext-liquid-dsp (Liquid-DSP)、libspecbleach |
| **音频** | ALSA、PipeWire |
| **语音** | RHVoice |

## IP 核清单

### 公用 IP 核 (9个)

| IP 核 | 版本 | 基地址 | 功能 |
|------|------|--------|------|
| Zynq PS7 | v5.5 | - | 处理系统 |
| AXI Interconnect | v2.1 | - | 总线互连 |
| AXI FIFO MM S | v4.2 | 0x43c10000 | ADC/DAC 数据传输 |
| AXI GPIO | v2.0 | 0x41200000 | GPIO 控制 |
| DDS Compiler | v6.0 | - | 数字混频器 |
| FIR Compiler | v7.2 | - | 数字滤波器 |
| Clocking Wizard | v6.0 | - | 时钟系统 |
| Proc System Reset | v5.0 | - | 复位管理 |
| TRX Control Regs | 1.0 | 0x43c20000 | 自定义控制寄存器 |

### 显示系统 IP 核 (3个)

| IP 核 | 版本 | 基地址 | 功能 |
|------|------|--------|------|
| AXI VDMA | v6.3 | 0x43000000 | 显示 DMA |
| AXI Video Out | v4.0 | - | 视频输出 |
| Video Timing Ctrl | v6.2 | 0x43c30000 | 时序控制器 |

### 音频系统 IP 核 (2个)

| IP 核 | 版本 | 基地址 | 功能 |
|------|------|--------|------|
| AXI I2S | v1.00.a | 0x43c00000 | I2S 音频接口 |
| AXI DMA | v7.1 | 0x40410000 | 音频 DMA |

### MicroBlaze 系统 IP 核 (4个)

| IP 核 | 版本 | 基地址 | 功能 |
|------|------|--------|------|
| MicroBlaze | v11.0 | - | 软核处理器 (FFT 加速) |
| AXI BRAM Ctrl | v4.1 | 0x43d00000 | I-BRAM 控制器 |
| AXI BRAM Ctrl | v4.1 | 0x43d10000 | D-BRAM 控制器 |
| Block RAM | v8.4 | - | BRAM 存储 |

**总计**: 19 个 IP 核 (含 2 个 FIR Compiler)

> 以上为硬件设计清单，实际以 `Firmware/system/br2_external/board/brass/7020/system.bit`
> 对应的 Vivado 工程为准。

## 硬件生产

### 获取 Gerber 文件

```bash
cd Hardware/MainUnit/Gerber   # 主控单元生产文件
cd Hardware/RadioUnit/Gerber  # 射频单元生产文件
```

### BOM 表

- MainUnit BOM: [Hardware/MainUnit/BOM-V1.1.csv](Hardware/MainUnit/BOM-V1.1.csv)
- RadioUnit BOM: [Hardware/RadioUnit/BOM-V2.xlsx](Hardware/RadioUnit/BOM-V2.xlsx)

### 原理图

- MainUnit: [Hardware/MainUnit/Schematic-1.1.pdf](Hardware/MainUnit/Schematic-1.1.pdf)
- RadioUnit: [Hardware/RadioUnit/Schematic-2.0.pdf](Hardware/RadioUnit/Schematic-2.0.pdf)

## 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发流程

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 代码规范

- C 代码遵循 Linux 内核编码风格
- Python 代码遵循 PEP 8
- 添加适当的注释和文档

## 许可证

自有代码（GUI）采用 **LGPL-2.1-or-later**，见
[Firmware/gui/LICENSE](Firmware/gui/LICENSE)；第三方组件保留各自许可证
（LVGL、Buildroot、Linux 内核等）。本仓库根目录没有单独的 `LICENSE` 文件。

## 致谢

- Xilinx Vivado/Vitis 工具链
- LVGL 图形库
- Buildroot 项目
- Liquid-DSP 库
- RHVoice 语音合成
- Belousov Oleg (R1CBU) 的上游 TRX-Brass 项目

## 联系方式

- **本仓库**: https://github.com/liupei404/TRX-Brass
- **问题反馈**: https://github.com/liupei404/TRX-Brass/issues
- **上游原项目**: https://github.com/strijar/TRX-Brass

## 更新日志

本仓库不再使用 v0.5.0 / v0.6.0 这类版本号（`brass-gui.mk` 中
`BRASS_GUI_VERSION = local`）。逐次修改记录见
[Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html)，
版本差异与待办见 [Docs/TRX-Brass-TODO.html](Docs/TRX-Brass-TODO.html)。

近期主要改动：长按关机、GUI 源码改为仓库内普通目录（LVGL 9.5 为唯一子模块）、
7020 全新编译流程，以及 GCC 15 / 内核下载 / uutils dd 等宿主环境兼容性修复。

## 相关资源

- [Xilinx Zynq-7000 文档](https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html)
- [LTC2208 数据手册](Docs/Datasheet/LTC2208.pdf)
- [DAC904E 数据手册](Docs/Datasheet/DAC904.pdf)
- [NAU88C22 数据手册](Docs/Datasheet/NAU88C22.pdf)

---

**注意**: 本项目仅用于业余无线电和教育目的，使用时请遵守当地无线电法规。

Made with ❤️ by amateur radio community

---

## 本仓库改动（长按开关机与构建一致性）

- **电源键**：短按切换背光（行为不变）；长按 1000 ms 保存全部设置、`sync()` 后
  `reboot(RB_POWER_OFF)`；内核没有关机驱动时按有序退出处理，避免 `brass_daemon`
  反复重启。开机长按检测需要电源锁存 / FPGA 硬件完成，GUI 只在启动时读取
  按键状态作诊断，默认不会自行断电。
- **GUI 源码**：`Firmware/gui` 已从子模块改为本仓库的普通目录，
  `brass-gui.mk` 使用 `SITE_METHOD = local` 直接构建仓库内源码；第三方 LVGL 9.5
  位于 `Firmware/gui/lvgl`，是唯一保留的子模块，克隆请使用
  `git clone --recurse-submodules <仓库地址>`。
- **构建与验证**：三套构建链（系统镜像、GUI 单独交叉编译、MicroBlaze FFT 固件）、
  烧录步骤和主机端电源模块单元测试见 [BUILDING.md](BUILDING.md)；自有代码许可证见
  `Firmware/gui/LICENSE`（LGPL-2.1-or-later），第三方组件保留各自许可证。
- **分析与记录**：[Docs/TRX-Brass-TODO.html](Docs/TRX-Brass-TODO.html)
  为版本变更分析与待办清单，[Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html)
  为代码修改记录。
- **项目状态**：仍为原型，两套 defconfig 中默认 root 口令与 SSH 仍启用，
  无 CI 与完整测试体系，V2.0 射频板改板项见 `Hardware/RadioUnit/FixList.txt`。

本仓库基于 Belousov Oleg (R1CBU) 的原项目
`https://github.com/strijar/TRX-Brass`；默认编译目标为 7020，编译产物路径、
`sdcard.img` 下载与烧录方法、bitstream 说明见 [BUILDING.md](BUILDING.md) 第 1.1 节。
