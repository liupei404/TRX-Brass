# TRX Brass Firmware

**TRX Brass 收发机完整固件项目**

## 项目概述

TRX Brass Firmware 是一个完整的业余无线电收发机固件项目，基于嵌入式 Linux 和 Buildroot 构建，包含了完整的 GUI 界面、操作系统内核和 FPGA 配置。

### 组件说明

- **GUI** - 基于 LVGL 的图形用户界面，提供电台控制功能
- **System** - 基于 Buildroot 的构建系统，生成完整的 Linux 固件
- **Microblase** - FPGA 模块配置

### 版本信息
- **当前版本**: v0.6.0
- **许可证**: LGPL-2.1-or-later
- **版权**: (c) 2022-2025 Belousov Oleg aka R1CBU

---

## 项目结构

```
TRX-Brass/Firmware/
├── gui/              # GUI 应用程序
│   ├── src/          # GUI 源代码
│   ├── lvgl/         # LVGL 图形库
│   └── README.md     # GUI 详细文档
│
├── system/           # Buildroot 构建系统
│   ├── br2_external/ # 外部扩展包
│   │   ├── board/   # 板级配置
│   │   ├── configs/ # Buildroot 配置
│   │   └── package/ # 软件包
│   └── config_brass.sh # 构建初始化脚本
│
├── microblase/       # FPGA 模块
│
└── README.md         # 本文件
```

---

## 快速开始

### 前置要求

- **操作系统**: Linux (Ubuntu 20.04+ 推荐)
- **编译工具**: GCC/G++, Make, CMake
- **交叉编译器**: arm-linux-gnueabihf-gcc
- **Python**: 3.x
- **磁盘空间**: >= 20GB 可用空间

### 初次编译

#### 1. 克隆仓库

```bash
git clone https://github.com/R1CBU/TRX-Brass.git
cd TRX-Brass/Firmware
```

#### 2. 编译完整固件

```bash
cd system
bash config_brass.sh

# 等待编译完成（2-4 小时）
# 固件生成在：output_brass/images/
```

#### 3. 部署固件

```bash
# 写入 SD 卡
dd if=output_brass/images/sdcard.img of=/dev/sdX bs=4M status=progress
sync
```

---

## 重新编译指南

### 快速参考

| 场景 | 适用情况 | 预计时间 |
|------|---------|---------|
| **仅 GUI** | 修改了 GUI 源代码 | 1-3 分钟 |
| **GUI + 系统** | 修改了配置文件 | 5-15 分钟 |
| **完整固件** | 修改了内核或系统 | 20-60 分钟 |

### 详细编译说明

详细的编译步骤、场景分析和故障排除请参考：

**📖 [BUILD_GUIDE.md](BUILD_GUIDE.md)** - 完整的编译和部署指南

#### 快速命令

```bash
# 场景 1: 仅重新编译 GUI
cd gui/build && make -j$(nproc)

# 场景 2: 重新编译 GUI 包
cd system/buildroot-2024.02
make BR2_EXTERNAL=../br2_external O=../output_brass brass-gui-rebuild

# 场景 3: 完整固件重新编译
make BR2_EXTERNAL=../br2_external O=../output_brass -j$(nproc)
```

---

## 主要功能

### GUI 功能 (v0.6.0)

- ✅ 多模式支持：LSB、USB、CW、CWR、AM、NFM、RTTY、Olivia
- ✅ 频率控制：精确调节，步进可选（10Hz-5000Hz）
- ✅ 频谱显示：实时 FFT 频谱分析
- ✅ 数字模式：FT8、RTTY、Olivia
- ✅ CW 电报：完整编码/解码功能
- ✅ ATU 天调：自动天线调谐器
- ✅ SWR 扫描：驻波比扫描
- ✅ **电源管理**：背光控制和系统关机（v0.6.0 新增）

### 系统功能

- ✅ Linux 6.1 内核（Xilinx 定制）
- ✅ Buildroot 根文件系统
- ✅ Zynq-7000 FPGA 支持
- ✅ 音频处理（ALSA/PipeWire）
- ✅ 语音合成（RHVoice）
- ✅ 数字信号处理（Liquid-DSP）
- ✅ 编解码器（Codec2）

---

## 文档

### 项目文档

- **[BUILD_GUIDE.md](BUILD_GUIDE.md)** - 编译和部署完整指南
- **[gui/README.md](gui/README.md)** - GUI 项目详细文档

### 电源管理文档 (v0.6.0)

- **[gui/src/hw/POWER_QUICK_REFERENCE.md](gui/src/hw/POWER_QUICK_REFERENCE.md)** - 快速参考
- **[gui/src/hw/POWER_CONTROL.md](gui/src/hw/POWER_CONTROL.md)** - 详细功能说明
- **[gui/src/hw/POWER_USAGE_EXAMPLE.md](gui/src/hw/POWER_USAGE_EXAMPLE.md)** - 使用示例
- **[gui/src/hw/IMPLEMENTATION_SUMMARY.md](gui/src/hw/IMPLEMENTATION_SUMMARY.md)** - 实现总结
- **[gui/src/hw/POWER_CHECKLIST.md](gui/src/hw/POWER_CHECKLIST.md)** - 检查清单

---

## 硬件要求

### 开发环境
- **CPU**: x86_64，>= 4 核心推荐
- **RAM**: >= 8GB
- **存储**: >= 20GB 可用空间
- **网络**: 稳定的互联网连接（下载依赖）

### 目标硬件
- **SoC**: Xilinx Zynq-7000 (Zynq 7020/7010)
- **CPU**: ARM Cortex-A9 双核
- **RAM**: >= 512MB
- **存储**: >= 4GB SD 卡
- **显示屏**: 800x480 触摸屏
- **音频**: 支持的音频输入/输出

---

## 开发指南

### GUI 开发

```bash
# GUI 开发流程
1. 修改代码: gui/src/xxx.c
2. 快速编译: cd gui/build && make
3. 部署测试: scp brass root@device:/usr/sbin/
4. 循环测试
```

### 系统开发

```bash
# 系统开发流程
1. 修改配置: system/br2_external/xxx
2. 重新编译: make xxx-rebuild
3. 生成固件: make
4. 部署测试
```

详细开发指南请参考 [BUILD_GUIDE.md](BUILD_GUIDE.md)。

---

## 故障排除

### 编译问题

**Q: 交叉编译工具链未找到**
```bash
sudo apt install gcc-arm-linux-gnueabihf
```

**Q: Buildroot 下载失败**
```bash
# 设置代理或手动下载到 dl/ 目录
export http_proxy=http://proxy.example.com:8080
```

**Q: 磁盘空间不足**
```bash
# 清理构建目录
make BR2_EXTERNAL=../br2_external O=../output_brass clean
```

### 运行时问题

**Q: GUI 无法启动**
```bash
# 检查日志
journalctl -u brass -f

# 检查权限
ls -l /usr/sbin/brass
```

**Q: 显示无输出**
```bash
# 检查 framebuffer
ls /dev/fb*

# 检查背光
ls /sys/class/backlight/
```

更多故障排除请参考 [BUILD_GUIDE.md](BUILD_GUIDE.md)。

---

## 贡献指南

欢迎贡献代码和改进！

### 提交代码

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 推送分支
5. 创建 Pull Request

### 代码规范

- 遵循 Linux 内核代码风格
- 使用 Doxygen 风格注释
- 添加 SPDX 许可证标识
- 更新相关文档

---

## 版本历史

### v0.6.0 (2025-02-16)
- ✅ 新增完整的电源管理功能
- ✅ 实现背光开关控制
- ✅ 实现系统关机功能
- ✅ 添加电源状态检测
- ✅ 完善关机流程
- ✅ 添加电源管理文档

### v0.5.x
- 数字模式优化
- 性能改进
- Bug 修复

### v0.4.x
- FT8 支持
- RTTY 支持
- Olivia 支持

### v0.3.x
- CW 功能完善
- 频谱显示优化
- 滤波器控制

### v0.2.x
- 基础 GUI 框架
- LVGL 集成
- 硬件抽象层

### v0.1.x
- 初始版本
- 基本频率控制

---

## 许可证

本项目采用 LGPL-2.1-or-later 许可证。

详见 [gui/LICENSE](gui/LICENSE) 文件。

## 作者

- **主要作者**: Belousov Oleg aka R1CBU
- **版权**: (c) 2022-2025

## 致谢

- LVGL 图形库
- Buildroot 项目
- Xilinx Zynq 平台
- 业余无线电社区

## 相关链接

- [项目主页](https://github.com/R1CBU/TRX-Brass)
- [问题跟踪](https://github.com/R1CBU/TRX-Brass/issues)
- [文档 Wiki](https://github.com/R1CBU/TRX-Brass/wiki)

## 技术支持

- 查看 [README.md](README.md) - 项目概览
- 查看 [BUILD_GUIDE.md](BUILD_GUIDE.md) - 编译指南
- 查看 [gui/README.md](gui/README.md) - GUI 文档

---

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
- **GUI**: LVGL 8.x
- **操作系统**: Linux 6.1
- **构建系统**: Buildroot 2024.02
- **FPGA**: Xilinx Vivado
- **音频**: ALSA, PipeWire
- **数字信号**: Liquid-DSP, Codec2

---

**TRX Brass Firmware - 业余无线电收发机完整固件**

*让业余无线电更简单、更强大！*
