################################################################################
#
#  Brass GUI application
#
################################################################################

# GUI 源码已作为普通目录并入本仓库（Firmware/gui）。
# 直接使用本地源码构建，避免从上游拉取时忽略本仓库中的开关机等修改。
BRASS_GUI_VERSION = local
BRASS_GUI_SITE = $(BR2_EXTERNAL_BRASS_PATH)/../../gui
BRASS_GUI_SITE_METHOD = local
BRASS_GUI_LICENSE = GPLv2

BRASS_GUI_INSTALL_STAGING = YES

BRASS_GUI_DEPENDENCIES = rhvoice libpng libcyaml ext-liquid-dsp libspecbleach

$(eval $(cmake-package))
