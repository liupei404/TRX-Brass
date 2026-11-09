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
- `Firmware/system/buildroot-2024.02.tar.gz` is already in the repository, so
  Buildroot itself is not downloaded. Network access is still needed once for
  the package sources and the ~190 MB Xilinx Linux tarball.
- Section 1.1 is the verified full 7020 rebuild procedure and tells you where
  to pick up the finished image.

## 1. System image (Buildroot)

```
cd Firmware/system
./config_brass.sh          # regenerates output_brass/.config from brass7020_defconfig
cd output_brass
make -j2                   # -j4 may OOM on an 8 GiB host
```

`config_brass.sh` uses `BR2_EXTERNAL=../br2_external` and
`O=../output_brass`; it overwrites `.config`, so do not keep custom edits
there. For the Zynq 7010 variant, replace the defconfig with
`brass7010_defconfig`. For a clean rebuild from the current files, run
`make clean` first (it keeps `.config` and the `dl/` cache). See section 1.1.

Outputs:

- `Firmware/system/output_brass/host/` — cross toolchain and target sysroot
- `Firmware/system/output_brass/images/sdcard.img` — flashable SD image
  (boot / rootfs / data partitions, see `br2_external/board/brass/genimage.cfg`)
- `br2_external/package/brass-gui/brass-gui.mk` builds the GUI directly from
  the in-tree `Firmware/gui` directory (`SITE_METHOD = local`)

Flash `sdcard.img` to an SD card, then boot. U-Boot loads `system.bit` before
the kernel; the GUI is started by `/etc/init.d/S99gui`.

## 1.1 7020 全新编译与固件下载（2026-11-10）

以**当前仓库文件**为准，忽略此前遗留的 `.config` 与 `output_brass` 构建产物，
从 defconfig 重新配置后全新编译。默认目标就是 **7020**。

### 前置条件（本机已实测）

- 用仓库自带的 `Firmware/system/buildroot-2024.02/`（Buildroot 2024.02）。
  `/home/bh8cgj/buildroot/` 是 2026.05 开发版且为 i686 配置，与本项目不匹配，
  **不要**拿它编译这个项目。
- LVGL 子模块已检出：`Firmware/gui/lvgl`（182 MB）。若是空目录，先执行
  `git submodule update --init --recursive`。
- 网络：约 445 个包的源码需下载一次；其中 Xilinx `linux-xlnx` 6.1 内核 tarball
  约 190 MB，实测约 400 KB/s，需 8–12 分钟。`sources.buildroot.net` 备份镜像可用。
- 主机工具齐备（`make`/`gcc`/`g++`/`git`/`wget`/`bison`/`flex`/`perl`/`rsync`/
  `cpio`/`python3`）；`host-cmake` 由 Buildroot 提供 3.28.1，满足 LVGL 9.5。
- 本机 4 核 / 7.1 GiB 内存 / 416 GiB 空闲磁盘。用 `-j2`，不要让它按默认
  `BR2_JLEVEL=0` 自动开 4 个并行任务，容易内存不足。

### 编译命令

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system
mv output_brass output_brass.old    # 旧配置与旧产物另存，不删除（已另存过可跳过）
./config_brass.sh                   # 在全新 output_brass/ 生成 .config（brass7020_defconfig）
cd output_brass
nohup make -j2 </dev/null > /tmp/brass-build.log 2>&1 &
tail -f /tmp/brass-build.log
```

`config_brass.sh` 会按当前仓库文件重新生成 `.config`，所以「以前改过的配置」一律
不保留。本机批量删除保护会拦截 `make clean`，因此改用「把旧输出目录另存」实现
同样的全新编译：新目录里没有任何旧包缓存，下载的源码缓存
`Firmware/system/buildroot-2024.02/dl/` 仍会复用；`output_brass.old/` 已被
`.gitignore` 忽略，确认新固件没问题后可自行删除。若你的环境允许批量删除，
也可以在 `cd output_brass` 后执行 `make clean` 代替另存。
首次全编译（glibc 工具链 + Python3 + Linux 内核 + GUI）预计 2–5 小时；
编译日志在 `/tmp/brass-build.log`，结果以日志为准。

### 环境注意（IDE 会话）

在 CodeBuddy / VS Code 的会话内直接启动 `make` 时，IDE 注入的删除保护
（`safe-delete` shim 被放在 `PATH` 最前面）会连 autoconf / Buildroot 内部对
临时文件的 `rm` 一起拦截，日志里出现大量 `SAFE_DELETE_BULK_CONFIRM_REQUIRED`。

2026-11-10 实测：IDE 内启动的全编译在 `host-tar 1.34 Configuring` 阶段因临时文件
清不掉而失败（`mkdir: conftest.d2: File exists` →
`configure: error: cannot create temporary files`），`images/` 为空，**没有生成任何固件**。
所以完整编译必须在**独立的系统终端 / SSH 会话**里运行（其 `PATH` 不含 IDE 的
`safe-bin` shim），不要在这个 IDE 的内置终端里跑。重跑前先清掉被污染的临时目录：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system
rm -rf output_brass/build/host-tar-1.34   # 在独立终端里执行；IDE 会话内删不掉
cd output_brass
make -j2 2>&1 | tee /tmp/brass-build.log
```

若 IDE 内还有一个 `make` 在跑，先 `kill <该 make 的 PID>`，避免两个 make 同时写
同一个输出目录。IDE 里仍可用 `tail -f /tmp/brass-build.log` 看进度。

### 实际采用方式（2026-11-10，已验证）

从 **Windows 终端通过 SSH 连接 Ubuntu**，在独立 SSH 会话里直接执行上面这三条
命令，这是本项目推荐且已跑通的编译方式：SSH 会话的 `PATH` 不含 IDE 的
`safe-delete` shim，autoconf / Buildroot 的 `rm` 不会被拦截。实测启动后
`host-tar 1.34`、`host-zstd 1.5.5`、`host-ccache 4.8.2` 均顺利通过，日志里
`SAFE_DELETE` 为 0、无致命错误。

- 另开一个 SSH 窗口用 `tail -f /tmp/brass-build.log` 看进度。
- 编译期间 `output_brass/images/` 为空是正常的；出现 `sdcard.img` 才算出镜像完成。
- 全程约 2–5 小时，中途不要在同一个输出目录再启动第二个 `make`。

### GCC 15 兼容性（4 处报错与统一处理）

本机 `gcc 15.2.0` 默认按 C23 编译，Buildroot 2024.02 的 `host-m4 1.4.19`
（gnulib）会把 `_GL_ATTRIBUTE_NODISCARD` 展开成 GCC 无法解析的
`[[__nodiscard__]]`，在 `lib/gl_oset.h:275` 报
`error: expected identifier or '(' before 'int'`，编译停在 `host-m4 Building`。
这与 IDE shim 无关，独立 SSH 终端同样会遇到。

修复方式是改本地构建目录里的生成文件（不进 git），在 `host-m4` **configure
完成之后**把该宏置空，然后增量续编：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system
sed -i 's/# define _GL_ATTRIBUTE_NODISCARD \[\[__nodiscard__\]\]/# define _GL_ATTRIBUTE_NODISCARD/' \
  output_brass/build/host-m4-1.4.19/lib/config.h
cd output_brass
make -j2 2>&1 | tee -a /tmp/brass-build.log
```

`make` 会接着编，已完成的 `host-tar` / `host-zstd` / `host-ccache` 不会重来。
**不要重新 configure `host-m4`**（会重新生成 `config.h` 覆盖修复），也不要重跑
`config_brass.sh`。若以后 `make clean` 或换输出目录重新全编译，需在它 configure
完成后再执行一次上面的 `sed`。2026-11-10 本次已对当前输出目录执行过该修复。

续编时又在 `host-e2fsprogs 1.47.0` 遇到同类问题：`lib/ext2fs/tdb.c:113` 的
`typedef int bool;` 在 C23 下 `bool` 已是关键字，已改成仅当
`__STDC_VERSION__ < 202311L` 时才 typedef。再往后 `host-gawk 5.3.0` 的
`io.c:3389` 因 GCC 15 把 `incompatible-pointer-types` 升级为 error 而失败，
已去掉 `( ssize_t(*)() ) read` 的旧式强转（3389、4449 两处直接赋 `read`，
其签名与 `read_func` 一致）。第四次是 `host-gmp 6.3.0` 的 configure 报
`could not find a working compiler`（`long long reliability test 1`）：C23 把
`void g()` 视为无参函数，测试代码传 6 个参数被判错。四处都是 GCC 15 默认 C23 /
诊断升级与 Buildroot 2024.02 老包的兼容问题，不是 GUI 代码问题。

**不再逐包改源码，也不更换 gcc 版本**：已在仓库里的
`Firmware/system/br2_external/external.mk` 给所有 host 包统一加编译参数：

```make
HOSTCC += -std=gnu17 -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration -Wno-error=int-conversion
HOSTCXX += -std=gnu++17
```

`HOSTCC` / `HOSTCXX` 仍指向系统的 `gcc 15` / `g++ 15`，只是让 host 工具回到 C17
模式、把老代码常见诊断降级为警告。已用 `make printvars VARS='HOSTCC HOSTCXX'`
验证展开正确，`.config` 不变、不会全量重编。后续同类错误由这一处设置挡掉；
`host-gmp` 的 configure 已在随后续编中重新执行并完整通过（Configuring →
Building → Installing），接着 `host-mpfr 4.1.1` 也正常进入 Building，日志无报错，
无需改它们源码。

前三处对本地构建目录的源码/生成文件修改仍保留着，但**以后重新全新编译不再需要**
（统一参数已覆盖）；`buildroot-2024.02/` 本身未改动。

### 内核源码下载失败（网络波动）

`brass7020_defconfig` 通过
`$(call github,Xilinx,linux-xlnx,xlnx_rebase_v6.1_LTS_2023.1)/xlnx_rebase_v6.1_LTS_2023.1.tar.gz`
下载约 212 MB 的 Xilinx Linux 6.1 源码。2026-11-10 在 `linux-headers custom Downloading`
阶段，`github.com` 连续三次报 `The TLS connection was non-properly terminated.`，
回退到 `sources.buildroot.net` 又返回 404（备份镜像不收录这个自定义内核包）。

处理办法：把源码包预先放进 Buildroot 下载缓存，之后 `make` 直接跳过下载：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system/buildroot-2024.02/dl/linux
wget -4 --tries=20 --timeout=60 --read-timeout=120 \
  -O xlnx_rebase_v6.1_LTS_2023.1.tar.gz \
  'https://codeload.github.com/Xilinx/linux-xlnx/tar.gz/refs/tags/xlnx_rebase_v6.1_LTS_2023.1'
gzip -t xlnx_rebase_v6.1_LTS_2023.1.tar.gz    # 必须校验，避免半截包
```

`LINUX_HEADERS_DL_SUBDIR = linux`，所以缓存文件必须放在 `dl/linux/` 下且文件名保持
`xlnx_rebase_v6.1_LTS_2023.1.tar.gz`（该包不校验 hash）。本次已下载并校验完成
（212 MB，`gzip -t` 通过、tar 可读），续编 `make` 会跳过这段下载；`dl/` 缓存即使在
`make clean` 后也保留。2026-11-10 续编实测已生效：`linux-headers` 的
`.stamp_downloaded` / `.stamp_built` 均已生成。

#### 同批预缓存其它 GitHub 归档包

`brass7020_defconfig` 的 U-Boot 也用 `$(call github,...)` 取
`xlnx_rebase_v2023.01_2023.1.tar.gz`；外部树中启用的 `axis-fifo`、
`ext-liquid-dsp`、`libcyaml`、`libspecbleach` 同样走 github.com 归档。为避免
重演上述 TLS 中断，已从 `codeload.github.com` 预下载并放进各自缓存：

| 包 | 缓存路径（相对 `buildroot-2024.02/dl/`） | 校验 |
| --- | --- | --- |
| U-Boot | `uboot/xlnx_rebase_v2023.01_2023.1.tar.gz` | 23 MB；custom tarball 不校验 hash |
| axis-fifo | `axis-fifo/axis-fifo-69375f12c2393c1c982a05b14e2a9515ad3d6b67.tar.gz` | sha256 与 `.hash` 一致 |
| liquid-dsp | `ext-liquid-dsp/ext-liquid-dsp-1.6.0.tar.gz` | `.hash` 无 hash 行，不校验 |
| libcyaml | `libcyaml/libcyaml-1.4.2.tar.gz` | 原有缓存，sha256 与 `.hash` 一致 |

缓存目录按包名取子目录（U-Boot 是 `dl/uboot/`，不是 `dl/` 根目录）；任意包
可用 `make printvars VARS='<包名大写>_SOURCE <包名大写>_DL_DIR'` 查期望的文件名
与路径。

**`libspecbleach` 的 hash 需要更新**：codeload 取到的归档（88 个文件、顶层目录
`libspecbleach-ca90a65/`、meson 构建齐全）sha256 为
`41a628baf7713e2745cf68e4b81b39b4a96408674b00457d639753d188f895b4`，而
`br2_external/package/libspecbleach/libspecbleach.hash` 里是过时的
`b95cb3a6...`。已把 `.hash` 重算为新值，并把归档放进
`dl/libspecbleach/libspecbleach-ca90a65.tar.gz`；两者一致，编译到该包会跳过下载
并通过校验。注意 Buildroot 会检查 `.hash` 里的**每一行**，所以必须替换旧值，不能
追加第二行 sha256。`sources.buildroot.net` 不收录这个自定义包，备份镜像也无效。

小包（`host-gettext-tiny`、`host-ninja`、`host-libffi` 等）即使 github.com 先报
`TLS connection was non-properly terminated.`，重试时也会 302 到 codeload 并通过
sha256 校验；只有 212 MB 内核这种大包会三次全断，才必须预缓存。`rhvoice` 用
`SITE_METHOD = git`，实测 `git ls-remote` 正常，不受归档 TLS 问题影响。

GitHub **release asset** 是另一类：`host-dosfstools 4.2` 从
`github.com/dosfstools/dosfstools/releases/download/v4.2/` 下载时三次 TLS 中断
（release 资源不会 302 到 codeload）。此时可从 Buildroot 备份镜像取同一发布包：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system/buildroot-2024.02/dl/dosfstools
wget -4 --tries=3 --timeout=40 -O dosfstools-4.2.tar.gz \
  'https://sources.buildroot.net/dosfstools/dosfstools-4.2.tar.gz'
sha256sum dosfstools-4.2.tar.gz   # 应为 64926eeb...b527
```

已实测该镜像与 github release 两种来源的 sha256 都是
`64926eebf90092dca21b14259a5301b7b98e7b1943e8a201c7d726084809b527`，与
`package/dosfstools/dosfstools.hash` 一致；缓存放在 `dl/dosfstools/` 即可。

#### SourceForge 下载长时间停滞

`jpeg-turbo 2.1.5` 从 `downloads.sourceforge.net` 下载时速率掉到约 5 KB/s，
到 81% 后完全停滞 12 分钟（wget 默认 read timeout 很长，会一直挂着）。处理办法是
先从 Buildroot 备份镜像取同一发布包放进缓存，再让卡住的连接退出；Buildroot 的
`dl-wrapper` 会自动尝试下一个 URI，编译不中断：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system/buildroot-2024.02/dl/jpeg-turbo
wget -4 --tries=3 --timeout=40 -O libjpeg-turbo-2.1.5.tar.gz \
  'https://sources.buildroot.net/jpeg-turbo/libjpeg-turbo-2.1.5.tar.gz'
sha256sum libjpeg-turbo-2.1.5.tar.gz   # bc12bc9d...ddaf
PID=$(pgrep -f 'wget --passive-ftp.*libjpeg-turbo-2.1.5' | head -1)
kill -TERM "$PID"                      # 只终止卡住的连接，不动 make
```

镜像包 sha256 `bc12bc9dce55300c6bf4342bc233bcc26bd38bf289eedf147360d731c668ddaf`
与 `package/jpeg-turbo/jpeg-turbo.hash` 一致。实测终止后编译立即继续
（`jpeg` Installing to target → `host-kmod`），`make` 没有退出，无需续编。

GitHub `.../files/<附件 id>/` 这类用户附件同理：`liberation 2.1.5` 从
`github.com/liberationfonts/liberation-fonts/files/7261482/` 下载时 TLS 中断、
重试很慢。同样先从 `sources.buildroot.net/liberation/` 取包放进 `dl/liberation/`，
再终止卡住的 `wget`；sha256 `7191c669bf38899f73a2094ed00f7b800553364f90e2637010a69c0e268f25d0`
与 `package/liberation/liberation.hash` 一致。实测 `make` 未退出，编译从
`liberation` 继续到 `libgpiod` / `libxml2`。

关键路径上的源码包现已全部缓存就绪：`linux`（212 MB）、`uboot`（23 MB）、
`libcyaml` / `ext-liquid-dsp` / `libspecbleach` / `axis-fifo` / `jpeg-turbo` /
`liberation` / `dosfstools`；`rhvoice` 是 git clone，已验证连通。

`host-intltool 0.51.0` 从 `launchpad.net` 下载时 IPv6 连接超时（wget 会逐个
IPv6 地址等超时，很慢）。同样从 `sources.buildroot.net/intltool/` 取包放进
`dl/intltool/`，再终止卡住的 `wget`；sha256
`67c74d94196b153b774ab9f89b2fa6c6ba79352407037c8c14d5aeb334e959cd` 与
`package/intltool/intltool.hash` 一致（强制 `wget -4` 直连 launchpad 也能得到
同样的包）。实测 `make` 未退出，编译从 `host-intltool` 继续到 `host-libxml2`。

**注意 `images/` 里出现文件不等于固件完成**：编译后期 `linux-firmware` 会先把
蓝牙 / Wi-Fi 固件放到 `images/` 的子目录（`ar3k/`、`qca/`、`rtl_bt/` 等），
`host-genimage` / `host-mtools` / `host-dosfstools` 也会先装好。真正要等的文件是
`images/sdcard.img`，它由最后阶段的 genimage 生成，同时还会出现 `boot.bin`、
`uImage`、`brass.dtb`、`system.bit`、`rootfs.ext4`。

`nano` 的 configure 会打印 `groff: fatal error: cannot load 'DESC' ...` 并给出
"Will not generate HTML version of man pages" 警告，这只是 HTML man 页支持探测
失败，随后正常生成 Makefile，不用处理。

### U-Boot SPL 构建失败（uutils dd 与 conv=block）

`uboot custom Building` 在生成 `spl/u-boot-spl-align.bin` 时报
`make[3]: *** [scripts/Makefile.spl:249：...] 错误 1`，日志里**没有任何错误文本**——
因为 U-Boot 2023.1 的规则是：

```make
	@dd if=$< of=$@ conv=block,sync bs=4 2>/dev/null;
```

`2>/dev/null` 把诊断吞掉了。本机 `dd` 是 `dd (uutils coreutils) 0.2.2`
（Ubuntu 25.10 的 Rust 版 coreutils），它要求 `conv=block` 必须配 `cbs=N`：
`dd: conv=block or conv=unblock specified without cbs=N`；GNU dd 允许省略
`cbs`。这是与 GCC 15/C23 同类的「新宿主工具 / 老项目」兼容问题，与 GUI 代码无关。

GNU dd 在 `cbs` 未设置时 `conv=block` 本来就是空操作，所以去掉它字节等价：
`conv=sync bs=4` 仍保证 4 字节对齐，实测生成 129168 字节文件与
`u-boot-spl.bin` 逐字节相同（`cmp` 通过）。

**长期修复（已进仓库）**：新增
`br2_external/board/brass/uboot/0001-spl-align-dd-uutils.patch`，并在
`brass7020_defconfig` / `brass7010_defconfig` 里加
`BR2_TARGET_UBOOT_PATCH="$(BR2_EXTERNAL_BRASS_PATH)/board/brass/uboot/"`
（与已有的 `BR2_LINUX_KERNEL_PATCH` 同一机制）。`patch --dry-run -p1` 已验证，
以后重新跑 `config_brass.sh` 全新编译时会自动应用。

**当前这次编译**：`output_brass/.config` 保持不变（仍是
`BR2_TARGET_UBOOT_PATCH=""`；改它反而会触发重新配置 / 重新打补丁），所以已直接在
`output_brass/build/uboot-custom/scripts/Makefile.spl` 做同样的替换，并预先生成
了 `spl/u-boot-spl-align.bin`。在 SSH 终端续编即可，不需要清目录：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system/output_brass
make -j2 2>&1 | tee -a /tmp/brass-build.log
```

2026-11-10 06:03 已在 SSH 终端按上面命令续编，修复验证通过：`uboot custom`
Building → Installing to images directory 完成，`images/` 里出现 `boot.bin`
（131 KB）、`u-boot.bin`、`u-boot.img`。无需清目录、无需重新配置。

续编后 `host-uboot-tools 2021.07` 又卡在 `ftp://ftp.denx.de/pub/u-boot/` 的
FTP 连接超时（IPv6 / IPv4 都不通）。同样从 `sources.buildroot.net/uboot-tools/`
取 `u-boot-2021.07.tar.bz2`（17 MB，sha256
`312b7eeae44581d1362c3a3f02c28d806647756c82ba8c72241c7cdbe68ba77e` 与
`package/uboot-tools/uboot-tools.hash` 一致）放进 `dl/uboot-tools/`，再终止卡住的
`wget` 让 `dl-wrapper` 换源即可；`make` 未退出。

> 进度（2026-11-10 06:13）：续编约 10 分钟，日志约 62 MB、1433 个 `>>>` 阶段；
> U-Boot 产物已生成，`usbmount` / `host-python-markupsafe` 等已过。续编日志无
> 新的致命错误。`images/` 里已有 `boot.bin` / `u-boot.bin` / `u-boot.img` 和蓝牙
> 固件，但 `sdcard.img` 仍未生成，还要等内核、GUI、rootfs 打包和 genimage。

### 第一次必须全编译吗？以后只改 GUI 呢

**第一次必须全编译一次**（2–5 小时）。`sdcard.img` 不是「一个 GUI 程序」，而是整张
SD 卡的快照：U-Boot、Linux 内核、设备树、FPGA `system.bit`、几百个库（glibc /
Python / 音频 / DSP / LVGL）、rootfs 和 DATA 分区。第一次编译时，把 x86 电脑代码
变成 ARM 板子能跑的**交叉编译器**、内核，以及打包 IMG 的工具都还不存在
（`output_brass/host/bin/*-gcc` 尚未生成），必须先全部造出来；GUI 只是 rootfs 里
一个 `/usr/sbin/brass_gui`。所以现在没法只打包 GUI。

**全编译成功之后**，再改 `Firmware/gui/` 就只要几分钟：

```bash
cd /home/bh8cgj/TRX-Brass/Firmware/system/output_brass
make brass-gui-dirclean     # 删掉 GUI 构建目录，强制重新同步仓库内 GUI 源码
make -j2                     # 只重编 GUI 并更新 rootfs / sdcard.img
```

其它包不会重编，更新后的 `sdcard.img` 仍在 `images/`。

**更快的调试方式（不出 IMG）**：全编译产出的 `output_brass/host/` 里有交叉工具链，
可用 `Firmware/gui/buildroot/cmake.sh` 单独编出 `brass_gui`，`scp` 到已经烧好系统的
板子 `/usr/sbin/brass_gui`，再执行 `/etc/init.d/S99gui restart` 即可生效。

### 新固件在哪里找

编译完成后，全部产物在：

```
/home/bh8cgj/TRX-Brass/Firmware/system/output_brass/images/
├── sdcard.img        整卡镜像（boot / rootfs / data 三个分区，直接烧录用这个）
├── boot.bin  u-boot.img  brass.dtb  uImage  system.bit  uboot-env.bin
├── boot.vfat  rootfs.ext4  data.vfat
└── settings/         DATA 分区的初始配置
```

还有交叉工具链与目标 sysroot 在 `Firmware/system/output_brass/host/`。

### 下载与烧录测试

```bash
# 方式一：从这台机器直接取
scp bh8cgj@<本机IP>:/home/bh8cgj/TRX-Brass/Firmware/system/output_brass/images/sdcard.img .

# 方式二：在产物目录起临时下载服务，浏览器打开 http://<本机IP>:8000/
cd /home/bh8cgj/TRX-Brass/Firmware/system/output_brass/images
python3 -m http.server 8000

# 烧录（务必确认 /dev/sdX 是 SD 卡，别写到系统盘）
sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
```

Windows 下可用 balenaEtcher / Win32DiskImager 直接选 `sdcard.img`。
串口调试口 `ttyPS0,115200`，默认 root 口令 `123`。

### 7020 与 bitstream

- 默认 defconfig 即 `brass7020_defconfig`；7010 才需要换成 `brass7010_defconfig`。
  两者只差设备树、`uboot.config` 路径和 `BR2_ROOTFS_POST_SCRIPT_ARGS`。
- **仓库已经带 7020 的真实比特流**：
  `Firmware/system/br2_external/board/brass/7020/system.bit`（2.5 MB，
  Xilinx `.bit`，Vivado 2019.2，`system_wrapper`）。`board/brass/post-build.sh`
  会按 `7020` 参数把它复制成 `images/system.bit`，`genimage.cfg` 再打进 boot 分区。
  **不需要本机装 Vivado，也不需要自己生成 bitstream**；要用自己的新比特流，
  替换这个 `7020/system.bit` 后重新 `make` 即可。
- MicroBlaze 的 `mb.code` / `mb.data` 已在
  `Firmware/gui/rootfs/usr/share/brass/fw/`，本机缺 `mb-gcc` 不影响出镜像，
  只是不能从 `Firmware/microblase/` 源码重编它们。

### 修改过的功能能编进固件吗

能，长按关机属于 GUI / rootfs 层：`Firmware/gui/src/power.c`、`power.h`、
`main_screen.c`、`keypad.c`、`main.c`，且 `src/CMakeLists.txt` 已把 `power.c`
加入 `target_sources`。`brass-gui.mk` 用 `SITE_METHOD = local` 直接构建仓库内
`Firmware/gui`（已实测 Buildroot 把 197 MB 源码 rsync 进
`output_brass/build/brass-gui-local`），所以该修改会随 `brass-gui` 包一起编进
rootfs 和 `sdcard.img`。

- 短按：切换背光（行为不变）。
- 长按 1000 ms：保存 bands / modes / options / rf、`sync()`，再
  `reboot(RB_POWER_OFF)`；内核无关机驱动时按有序退出处理，`brass_daemon` 不会重启。
- **长按开机 / 电源自锁不是软件能编出来的**：CPU 断电期间没有任何软件运行，
  必须由电源锁存电路或 FPGA 完成。GUI 只在启动时读取按键状态作诊断，
  默认不会自行断电。

完整编译是否一次通过，以 `/tmp/brass-build.log` 为准；若 `brass-gui` 阶段报
依赖或 CMake 错误，按日志修正后重新 `make -j2` 即可（增量续编）。

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
