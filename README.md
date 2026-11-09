# TRX-Brass

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-v0.6.0-green.svg)](Firmware/system/br2_external/package/brass-gui/brass-gui.mk)
[![Platform](https://img.shields.io/badge/platform-Zynq7020-orange.svg)](Hardware/MainUnit/)

一个基于 Xilinx Zynq7020 的业余无线电收发一体机项目，包含完整的硬件设计、FPGA 固件和软件系统。

## 项目概述

TRX-Brass 是一款专业的业余无线电收发一体机，采用 Zynq7020 SoC 平台，支持多种调制模式、数字信号处理和图形化用户界面。

### 核心特性

- **收发模式**: LSB/USB/CW/CWR/AM/NFM/RTTY/Olivia/FT8
- **显示系统**: 800×480 TFT LCD 触摸屏，60Hz 刷新率
- **射频前端**: LTC2208 ADC (16位, 125MSPS) / DAC904E DAC (14位, 165MSPS)
- **音频系统**: NAU88C22/NAU8810 编解码器，48kHz I2S 音频
- **DSP 处理**: FFT、AGC、噪声抑制、FIR 滤波
- **数字模式**: 完整的 FT8、RTTY、Olivia 实现
- **图形界面**: LVGL 8.x 图形用户界面
- **操作系统**: Linux 6.1 (基于 Buildroot 2024.02)

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
│  │  GUI 层 (LVGL 8.x)                                      │   │
│  │  - 主屏幕、频谱显示、设置对话框                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                            ↓                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  应用层 (brass_gui v0.6.0)                              │   │
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
│  │  FPGA 硬件 (Vivado 2024.2)                              │   │
│  │  - ADC/DAC、显示、音频、DSP 加速                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 目录结构

```
TRX-Brass/
├── Firmware/                 # 固件和软件
│   ├── gui/                 # GUI 应用程序
│   │   ├── src/            # 源代码
│   │   │   ├── main.c      # 主程序
│   │   │   ├── radio.c     # 射频控制
│   │   │   ├── dsp/        # DSP 处理模块
│   │   │   ├── fpga/       # FPGA 控制接口
│   │   │   ├── hw/         # 硬件抽象层
│   │   │   ├── audio/      # 音频处理
│   │   │   ├── cw/         # CW 电报
│   │   │   ├── ft8/        # FT8 数字模式
│   │   │   ├── olivia/     # Olivia 数字模式
│   │   │   └── settings/   # 设置系统
│   │   ├── lvgl/           # LVGL 图形库
│   │   └── lv_drivers/     # LVGL 驱动
│   ├── system/             # Buildroot 构建系统
│   │   ├── br2_external/  # Buildroot 扩展
│   │   │   ├── board/     # 板级配置
│   │   │   ├── configs/   # defconfig
│   │   │   └── package/   # 自定义软件包
│   │   └── config_brass.sh # 构建脚本
│   └── microblase/        # MicroBlaze FPGA 模块
├── Hardware/              # 硬件设计
│   ├── MainUnit/          # 主控单元
│   │   ├── Schematic-1.1.pdf
│   │   ├── BOM-V1.1.csv
│   │   └── Gerber/        # 生产文件
│   ├── RadioUnit/         # 射频单元
│   │   ├── Schematic-2.0.pdf
│   │   ├── BOM-V2.xlsx
│   │   └── Gerber/
│   └── Case/              # 机壳设计
├── Docs/                  # 文档
│   ├── Buildroot_7020编译指南.md
│   └── Datasheet/         # 芯片数据手册
├── TRX_Brass.md          # 项目主文档 (IP核综合方案)
├── 设备树IP核分析.md     # 设备树配置
└── README.md            # 本文件
```

## 快速开始

### 环境要求

- **开发主机**: Ubuntu 20.04+ / Windows 10+ (WSL2)
- **Vivado**: 2024.2
- **Vitis**: 2024.2
- **Buildroot**: 2024.02
- **编译工具链**: ARM GCC 9.x+

### 编译固件

#### 1. 编译 GUI 应用

```bash
cd Firmware/gui
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 2. 编译完整固件

```bash
cd Firmware/system
bash config_brass.sh
```

**预计时间**:
- 仅修改 GUI: 1-3 分钟
- GUI + 系统: 5-15 分钟
- 完整固件: 2-4 小时

#### 3. 编译 MicroBlaze

```bash
cd Firmware/microblase
make
```

### 烧录固件

#### 1. 烧录 SD 卡

```bash
# 在 Linux 上
dd if=output_brass/images/sdcard.img of=/dev/sdX bs=4M status=progress
sync

# 在 Windows 上使用 Win32DiskImager
```

#### 2. 烧录 FPGA

```bash
# 通过 Vivado
vivado -mode batch -source program_fpga.tcl
```

### 运行系统

1. 将 SD 卡插入设备
2. 上电启动
3. GUI 自动启动

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

### 核心文档

- [TRX_Brass.md](TRX_Brass.md) - 项目主文档，IP核综合方案
- [设备树IP核分析.md](设备树IP核分析.md) - 设备树配置详解
- [I2C芯片清单与分析.md](I2C芯片清单与分析.md) - I2C 设备分析

### IP核分析

- [项目IP核清单与Vivado2024.2优化方案.md](项目IP核清单与Vivado2024.2优化方案.md)
- [显示系统IP核分析与Vitis优化方案.md](显示系统IP核分析与Vitis优化方案.md)
- [DAC904E到Zynq7020的IP核分析.md](DAC904E到Zynq7020的IP核分析.md)
- [LTC2208到Zynq7020的IP核分析.md](LTC2208到Zynq7020的IP核分析.md)

### 构建指南

- [Firmware/BUILD_GUIDE.md](Firmware/BUILD_GUIDE.md) - Buildroot 编译指南
- [Firmware/README.md](Firmware/README.md) - 固件概述
- [Firmware/gui/README.md](Firmware/gui/README.md) - GUI 详细文档

### 电源管理 (v0.6.0 新增)

- [Firmware/gui/src/hw/POWER_QUICK_REFERENCE.md](Firmware/gui/src/hw/POWER_QUICK_REFERENCE.md) - 快速参考
- [Firmware/gui/src/hw/POWER_CONTROL.md](Firmware/gui/src/hw/POWER_CONTROL.md) - 详细功能说明

## 技术栈

| 类别 | 技术 |
|------|------|
| **GUI** | LVGL 8.x |
| **OS** | Linux 6.1 (Xilinx 定制) |
| **构建系统** | Buildroot 2024.02 |
| **FPGA** | Vivado 2024.2 + Vitis 2024.2 |
| **DSP** | Liquid-DSP, Codec2 |
| **音频** | ALSA, PipeWire |
| **语音** | RHVoice |
| **噪声抑制** | libspecbleach |

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

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

## 致谢

- Xilinx Vivado/Vitis 工具链
- LVGL 图形库
- Buildroot 项目
- Liquid-DSP 库
- Codec2 语音编解码
- RHVoice 语音合成

## 联系方式

- **项目主页**: https://github.com/strijar/TRX-Brass
- **问题反馈**: [GitHub Issues](https://github.com/strijar/TRX-Brass/issues)

## 更新日志

### v0.6.0 (当前版本)

- ✨ 新增电源管理功能
- ✨ 改进 GUI 性能
- 🐛 修复音频输出问题
- 📝 更新文档

### v0.5.0

- ✨ 添加 FT8 数字模式
- ✨ 优化 FFT 性能
- 🐛 修复 CW 解码问题

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
- **构建与验证**：三套构建链、烧录步骤和主机端电源模块单元测试见
  [BUILDING.md](BUILDING.md)；自有代码许可证见 `Firmware/gui/LICENSE`
  （LGPL-2.1-or-later），第三方组件保留各自许可证。
- **分析与记录**：[Docs/TRX-Brass-TODO.html](Docs/TRX-Brass-TODO.html)
  为版本变更分析与待办清单，[Docs/TRX-Brass-CodeChanges.html](Docs/TRX-Brass-CodeChanges.html)
  为代码修改记录。
- **项目状态**：仍为原型，两套 defconfig 中默认 root 口令与 SSH 仍启用，
  无 CI 与完整测试体系，V2.0 射频板改板项见 `Hardware/RadioUnit/FixList.txt`。

本仓库基于 Belousov Oleg (R1CBU) 的原项目
`https://github.com/strijar/TRX-Brass`。

## 全新编译与固件下载（7020）

默认目标为 7020。按当前仓库文件重新配置并全新编译、产物路径、`sdcard.img`
下载与烧录方法、bitstream 说明，见 [BUILDING.md](BUILDING.md) 第 1.1 节。

- 仓库已带 7020 的真实 `system.bit`
  （`Firmware/system/br2_external/board/brass/7020/system.bit`），编译镜像时
  自动打进 boot 分区，无需 Vivado 生成比特流。
- 长按关机（1000 ms）会随 `brass-gui` 编进 rootfs；长按开机 / 电源自锁仍需
  电源锁存电路或 FPGA 硬件，不是软件编译能实现的。
