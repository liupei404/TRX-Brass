import lv
from lv_const import *

from style import *

# # #

spectrum_style.set_geom(x=0, y=59, width=800, height=101)
spectrum_style.set_pad(all=0)

spectrum_tx_finder_style = lv.style()
spectrum_tx_finder_style.set_geom(x=0, y=0, width=800, height=50)

spectrum_rx_finder_style = lv.style()
spectrum_rx_finder_style.set_geom(x=0, y=50, width=800, height=50)

# # #

main_freqs_style.set_geom(x=0, y = 160-20, width=800, height=82)
main_freqs_style.set_pad(all=0)

main_freqs_center_style.set_geom(x=150, y=20, width=500, height=40)

main_freqs_left_style.set_geom(x=20, y=20, width=150, height=60)
main_freqs_left_style.set_pad(ver=8)

main_freqs_right_style.set_geom(x=800 - 150 - 20, y=20, width=150, height=60)

# # #

waterfall_style.set_geom(x=0, y=161 + 42 - 1, width=800, height=221)
waterfall_style.set_pad(all=0)

waterfall_rx_finder_style = lv.style()
waterfall_rx_finder_style.set_geom(x=0, y=0, width=800, height=221)

waterfall_bandinfo_style = lv.style()
waterfall_bandinfo_style.set_geom(x=0, y=2, width=800, height=24)

# # #

msg_style.set_geom(x = 101 - 15, y=65, width=616, height=89)

msg_label_normal_style.set_geom(x=0, y=20, width=586, height=48)

# # #

msg_tiny_style.set_geom(x=200 - 15, y=147 - 15, width=430, height=100)

msg_tiny_label_style = lv.style()
msg_tiny_label_style.set_geom(x=15, y=15, width=400, height=70)

# # #

top_style.set_geom(x=0, y=0, width=800, height=79)
top_style.set_pad(all=0)

tx_reflex_style = lv.style()
tx_reflex_style.set_geom(x=0, y=59, width=800, height=2)

tx_reflex_indicator_style.set_pad(left=-10, right=10)

# # #

buttons_style.set_geom(x=0, y=480 - 79, width=800, height=79)
buttons_style.set_pad(all=0)

button_style.set_geom(width=133, height=59)

# # #

info_style.set_geom(x=20, y=7, width=194, height=45)
info_style.set_pad(all=0)

info_split_style = lv.style()
info_split_style.set_geom(x=0, y=0, width=59, height=22)

info_mode_style = lv.style()
info_mode_style.set_geom(x=67, y=0, width=59, height=22)

info_agc_style = lv.style()
info_agc_style.set_geom(x=135, y=0, width=59, height=22)

info_pre_style = lv.style()
info_pre_style.set_geom(x=0, y=23, width=59, height=22)

info_att_style = lv.style()
info_att_style.set_geom(x=67, y=23, width=59, height=22)

info_atu_style = lv.style()
info_atu_style.set_geom(x=135, y=23, width=59, height=22)

# # #

smeter_style.set_geom(x=250, y=10, width=340, height=40)

# # #

clock_style.set_geom(x=629, y=17, width=153, height=26)
clock_style.set_pad(all=0)

clock_label_style = lv.style()
clock_label_style.set_geom(x=0, y=0, width=96, height=26)

bat_style.set_geom(x=116, y=0, width=37, height=26)

# # # # #

pannel_style.set_geom(x=0, y = 480 - 59 - 210 + 8, width=800, height=210)
pannel_style.set_pad(ver=20, hor=16)

# # # # #

dialog_style.set_geom(x=0, y=48, width=800, height=384)
dialog_style.set_pad(all=0)

dialog_no_buttons_style.set_geom(height=432)

# # # # #

keyboard_style.set_geom(x=0, y=480 - 59 - 210 + 8, width=800, height=210)
keyboard_style.set_pad(ver=20, hor=16)
