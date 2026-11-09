################################################################################
#
#  Brass GUI application
#
################################################################################

# GUI 源码已作为普通目录并入本仓库（Firmware/gui），不再从任何远程仓库下载。
# 主仓库曾把下载库改为 liupei404/brass_gui；这里改为 SITE_METHOD = local，
# 确保构建使用的是仓库内源码（含长按开关机等改动）。
BRASS_GUI_VERSION = local
BRASS_GUI_SITE = $(BR2_EXTERNAL_BRASS_PATH)/../../gui
BRASS_GUI_SITE_METHOD = local
BRASS_GUI_LICENSE = LGPL-2.1-or-later
BRASS_GUI_LICENSE_FILES = LICENSE

BRASS_GUI_INSTALL_STAGING = YES

BRASS_GUI_DEPENDENCIES = rhvoice libpng libcyaml ext-liquid-dsp libspecbleach

$(eval $(cmake-package))
