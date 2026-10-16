################################################################################
#
#  Brass GUI application
#
################################################################################

BRASS_GUI_VERSION = 42bafc864885eef05156ea8cdd1dc5c32efb338d
BRASS_GUI_SITE = https://github.com/strijar/brass_gui
BRASS_GUI_SITE_METHOD = git
BRASS_GUI_GIT_SUBMODULES = YES
BRASS_GUI_LICENSE = GPLv2

BRASS_GUI_INSTALL_STAGING = YES

BRASS_GUI_DEPENDENCIES = rhvoice libpng libcyaml ext-liquid-dsp libspecbleach

$(eval $(cmake-package))
