import lv
from lv_const import *

# # #

bandinfo_indicator_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Medium-Awesome.ttf", 20)
bandinfo_marker_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Bold-Awesome.ttf", 20)

freqs_center_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Bold-Awesome.ttf", 30)
freqs_range_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 20)

msg_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 36)
msg_tiny_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Bold-Awesome.ttf", 50)

button_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Medium-Awesome.ttf", 15)
pannel_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 18)
info_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Medium-Awesome.ttf", 15)
smeter_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Medium-Awesome.ttf", 20)
clock_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 20)
bat_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 22)

dialog_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 22)
dialog_title_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 16)

keyboard_font = lv.load_font("/usr/share/brass/font/JetBrainsMono-Regular-Awesome.ttf", 22)

# # #

spectrum_style = lv.style()

spectrum_style.set_radius(0)
spectrum_style.set_bg(color=0x040A1B, opa=LV_OPA_COVER)

# # #

spectrum_lines_style = lv.style()
spectrum_lines_style.set_line(width=2, color=0xFFC557)

# # #

spectrum_peaks_style = lv.style()

spectrum_peaks_style.set_line(width=1, color=0x845000)

# # #

finder_style = lv.style()

finder_style.set_bg(opa = LV_OPA_0)
finder_style.set_radius(0)

# # #

tx_finder_indicator_style = lv.style()

tx_finder_indicator_style.set_bg(color=0xDF1B73, opa=LV_OPA_70)
tx_finder_indicator_style.set_line(width=1)

# # #

rx_finder_indicator_style = lv.style()

rx_finder_indicator_style.set_bg(color=0x0F2946, opa=LV_OPA_70)
rx_finder_indicator_style.set_line(width=1)

# # #

rec_off_style = lv.style()
rec_off_style.set_line(color=0xFFFFFF, opa=LV_OPA_50)

# # #

rec_on_style = lv.style()
rec_on_style.set_line(color=0xFF0000, opa=LV_OPA_COVER)

# # #

waterfall_style = lv.style()

waterfall_style.set_bg(color=0x000000, opa=LV_OPA_COVER)
waterfall_style.set_radius(0)

#

waterfall_grad = lv.grad()

waterfall_grad.set_dir(LV_GRAD_DIR_HOR)
waterfall_grad.set_stops_count(5)
waterfall_grad.set_stop(0, 0,   0x040A1B)
waterfall_grad.set_stop(1, 64,  0x040AFF)
waterfall_grad.set_stop(2, 128, 0xFF0A1B)
waterfall_grad.set_stop(3, 192, 0xFFFF1B)
waterfall_grad.set_stop(4, 255, 0xFFFFFF)

# # #

bandinfo_style = lv.style()

bandinfo_style.set_radius(0)
bandinfo_style.set_border(width=0)
bandinfo_style.set_bg(opa=LV_OPA_0)

# # #

bandinfo_indicator_style = lv.style()

bandinfo_indicator_style.set_bg(color=0x141F36, opa=LV_OPA_90)
bandinfo_indicator_style.set_border(color=0xFFCF85, opa=LV_OPA_50, width=1)
bandinfo_indicator_style.set_text(color=0xFFCF85, font=bandinfo_indicator_font)

# # #

bandinfo_marker_style = lv.style()

bandinfo_marker_style.set_bg(color=0xFFCF85, opa=LV_OPA_COVER)
bandinfo_marker_style.set_text(color=0x000000, font=bandinfo_marker_font)

# # #

main_freqs_style = lv.style()

main_freqs_style.set_radius(0)
main_freqs_style.set_border(width=0)
main_freqs_style.set_bg(opa=LV_OPA_TRANSP)
main_freqs_style.set_bg(img_src="A:/usr/share/brass/images/middle.bin", img_opa=LV_OPA_COVER)

# # #

main_freqs_center_style = lv.style()
main_freqs_center_style.set_text(color=0xFFC557, font=freqs_center_font, align=LV_TEXT_ALIGN_CENTER)

# # #

main_freqs_left_style = lv.style()
main_freqs_left_style.set_text(color=0xFFCF85, font=freqs_range_font, align=LV_TEXT_ALIGN_LEFT)

# # #

main_freqs_right_style = lv.style()
main_freqs_right_style.set_text(color=0xFFCF85, font=freqs_range_font, align=LV_TEXT_ALIGN_RIGHT)

# # #

msg_style = lv.style()

msg_style.set_bg(img_src="A:/usr/share/brass/images/msg.bin", img_opa=LV_OPA_COVER)
msg_style.set_text(font=msg_font, align=LV_TEXT_ALIGN_CENTER)

# # #

msg_label_normal_style = lv.style()
msg_label_normal_style.set_text(color=0xFFCF85)

# # #

msg_label_select_style = lv.style()
msg_label_select_style.set_text(color=0x845000)

# # #

msg_tiny_style = lv.style()

msg_tiny_style.set_text(color=0xFFCF85, font=msg_tiny_font, align=LV_TEXT_ALIGN_CENTER)
msg_tiny_style.set_bg(img_src="A:/usr/share/brass/images/msg_tiny.bin", img_opa=LV_OPA_COVER)

# # #

top_style = lv.style()

top_style.set_radius(0)
top_style.set_border(width=0)
top_style.set_bg(opa=LV_OPA_TRANSP, img_src="A:/usr/share/brass/images/top.bin", img_opa=LV_OPA_COVER)

tx_reflex_grad = lv.grad()

tx_reflex_grad.set_dir(LV_GRAD_DIR_HOR)
tx_reflex_grad.set_stops_count(4)
tx_reflex_grad.set_stop(0, 0, 0x131E35)
tx_reflex_grad.set_stop(1, 128 - 16, 0xDF1B73)
tx_reflex_grad.set_stop(2, 128 + 16, 0xDF1B73)
tx_reflex_grad.set_stop(3, 255, 0x131E35)

tx_reflex_indicator_style = lv.style()

tx_reflex_indicator_style.set_bg(grad=tx_reflex_grad, opa=LV_OPA_70)

# # #

buttons_style = lv.style()

buttons_style.set_radius(0)
buttons_style.set_border(width=0)
buttons_style.set_bg(opa=LV_OPA_TRANSP, img_src="A:/usr/share/brass/images/buttons.bin", img_opa=LV_OPA_COVER)

# # #

button_style = lv.style()

button_style.set_radius(0)
button_style.set_border(width=0)
button_style.set_text(color=0xFFCF85, font=button_font)
button_style.set_bg(opa=LV_OPA_TRANSP)

# # #

info_style = lv.style()

info_style.set_radius(0)
info_style.set_border(width=0)
info_style.set_text(color=0xFFCF85, font=info_font, align=LV_TEXT_ALIGN_CENTER)
info_style.set_bg(opa=LV_OPA_TRANSP)

info_off_style = lv.style()
info_off_style.set_text(color=0x845000)

# # #

smeter_style = lv.style()

smeter_style.set_radius(0)
smeter_style.set_border(width=0)
smeter_style.set_bg(opa=LV_OPA_TRANSP)
smeter_style.set_text(color=0xFFCF85, font=smeter_font)

smeter_slice_style = lv.style()

smeter_slice_style.set_geom(width=10)
smeter_slice_style.set_pad(column=3)

smeter_part1_style = lv.style()
smeter_part1_style.set_bg(color=0x2E3442, opa=LV_OPA_COVER)

smeter_part2_style = lv.style()
smeter_part2_style.set_bg(color=0x3F526C, opa=LV_OPA_COVER)

smeter_part3_style = lv.style()
smeter_part3_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)

smeter_part4_style = lv.style()
smeter_part4_style.set_bg(color=0xFFC940, opa=LV_OPA_COVER)

# # #

clock_style = lv.style()

clock_style.set_radius(0)
clock_style.set_border(width=0)
clock_style.set_text(color=0xFFCF85, font=clock_font, align=LV_TEXT_ALIGN_LEFT)
clock_style.set_bg(opa=LV_OPA_TRANSP)

bat_style = lv.style()

bat_style.set_radius(0)
bat_style.set_border(width=0)
bat_style.set_text(color=0xFFCF85, font=bat_font)
bat_style.set_bg(opa=LV_OPA_TRANSP)

# # # # #

pannel_style = lv.style()

pannel_style.set_radius(0)
pannel_style.set_text(color=0xFFCF85, font=pannel_font)
pannel_style.set_bg(img_src="A:/usr/share/brass/images/pannel.bin", img_opa=LV_OPA_COVER)

# # # # #

dialog_style = lv.style()

dialog_style.set_radius(0)
dialog_style.set_text(color=0xFFCF85, font=dialog_font)
dialog_style.set_bg(img_src="A:/usr/share/brass/images/dialog.bin", img_opa=LV_OPA_COVER)

# # # # #

dialog_no_buttons_style = lv.style()
dialog_no_buttons_style.set_bg(img_src="A:/usr/share/brass/images/dialog_no_buttons.bin")

# # # # #

dialog_grid_style = lv.style()

dialog_grid_style.set_geom(x=0, y=20, width=780, height=330 + 68)
dialog_grid_style.set_bg(opa=LV_OPA_TRANSP)
dialog_grid_style.set_border(width=0)
dialog_grid_style.set_pad(column=5, row=5)

# # # # #

dialog_title_style = lv.style()

dialog_title_style.set_max(height=21)
dialog_title_style.set_text(color=0xFFA600, font=dialog_title_font)
dialog_title_style.set_bg(img_opa=LV_OPA_TRANSP)
dialog_title_style.set_pad(all=0)

# # # # #

dialog_title_label_style = lv.style()

dialog_title_label_style.set_max(height=29)
dialog_title_label_style.set_text(color=0xFFCF85)
dialog_title_label_style.set_bg(img_opa=LV_OPA_TRANSP)
dialog_title_label_style.set_pad(hor=20)

# # # # #

dialog_label_style = lv.style()

dialog_label_style.set_max(height=29)
dialog_label_style.set_text(color=0xFFCF85)
dialog_label_style.set_bg(img_opa=LV_OPA_TRANSP)
dialog_label_style.set_pad(hor=0)

# # # # #

dialog_item_style = lv.style()

dialog_item_style.set_radius(5)
dialog_item_style.set_max(height=38)
dialog_item_style.set_text(color=0x131E35)
dialog_item_style.set_bg(color=0xFFFFFF, opa=LV_OPA_COVER)
dialog_item_style.set_pad(ver=4, hor=8)

# # # # #

dialog_item_focus_style = lv.style()

dialog_item_focus_style.set_text(color=0x131E35)
dialog_item_focus_style.set_bg(color=0xFFD18B, opa=LV_OPA_COVER)

# # # # #

dialog_item_edited_style = lv.style()

dialog_item_edited_style.set_text(color=0xFFFFFF)
dialog_item_edited_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)

# # # # #

dialog_item_cursor_style = lv.style()

dialog_item_cursor_style.set_text(color=0x5D769A)
dialog_item_cursor_style.set_bg(color=0xFFFFFF, opa=LV_OPA_COVER)

# # # # #

dropdown_list_style = lv.style()

dropdown_list_style.set_radius(5)
dropdown_list_style.set_text(font=dialog_font)
dropdown_list_style.set_bg(color=0xFFFFFF, opa=LV_OPA_COVER)

# # # # #

slider_style = lv.style()
slider_style.set_pad(ver=12, hor=20)

slider_indicator_style = lv.style()

slider_indicator_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)
slider_indicator_style.set_radius(0)

slider_knob_style = lv.style()

slider_knob_style.set_border(color=0xFFFFFF, width=1)
slider_knob_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)
slider_knob_style.set_radius(LV_RADIUS_CIRCLE)
slider_knob_style.set_pad(ver=-7, hor=-7)

slider_indicator_edited_style = lv.style()
slider_indicator_edited_style.set_bg(color=0xFFFFFF, opa=LV_OPA_COVER)

slider_knob_edited_style = lv.style()
slider_knob_edited_style.set_bg(color=0xFFFFFF, opa=LV_OPA_COVER)

# # # # #

switch_style = lv.style()
switch_style.set_pad(ver=12, hor=10)

#

switch_indicator_style = lv.style()

switch_indicator_style.set_bg(opa=LV_OPA_TRANSP)
switch_indicator_style.set_radius(LV_RADIUS_CIRCLE)

switch_indicator_checked_style = lv.style()
switch_indicator_checked_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)

#

switch_knob_style = lv.style()

switch_knob_style.set_border(color=0xFFFFFF, width=1)
switch_knob_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)
switch_knob_style.set_radius(LV_RADIUS_CIRCLE)
switch_knob_style.set_pad(ver=-7, hor=-7)

switch_knob_checked_style = lv.style()
switch_knob_checked_style.set_bg(color=0x5D769A, opa=LV_OPA_COVER)

# # # # #

keyboard_style = lv.style()

keyboard_style.set_radius(0)
keyboard_style.set_pad(column=5, row=5)
keyboard_style.set_bg(img_src="A:/usr/share/brass/images/pannel.bin", img_opa=LV_OPA_COVER)
keyboard_style.set_text(font=keyboard_font)

# # # # #

ft8_waterfall_style = lv.style()
ft8_waterfall_style.set_geom(x=10, y=20, width=780, height=384 - 40)

ft8_finder_style = lv.style()
ft8_finder_style.set_geom(x=0, y=0, width=780, height=384 - 40)

ft8_finder_indicator_style = lv.style()
ft8_finder_indicator_style.set_bg(color=0xDF1B73, opa=LV_OPA_70)
ft8_finder_indicator_style.set_border(width=1, color=0xFFFFFF, opa=LV_OPA_50)

ft8_table_style = lv.style()

ft8_table_style.set_geom(x=10, y=20 + 40, width=780, height=384 - 40 - 40)
ft8_table_style.set_bg(color=0x141F36, opa=LV_OPA_COVER)
ft8_table_style.set_border(width=0)
ft8_table_style.set_radius(0)

#

ft8_focused_style = lv.style()
ft8_focused_style.set_bg(color=0x000000, opa=LV_OPA_50)

ft8_rx_info_style = lv.style()
ft8_rx_info_style.set_text(color=0xFFA600, align=LV_TEXT_ALIGN_CENTER)

ft8_rx_msg_style = lv.style()
ft8_rx_msg_style.set_text(color=0xFFCF85)

ft8_rx_cq_style = lv.style()
ft8_rx_cq_style.set_text(color=0xFFFFFF)

ft8_rx_to_me_style = lv.style()
ft8_rx_to_me_style.set_text(color=0xFFC940)

ft8_tx_msg_style = lv.style()
ft8_tx_msg_style.set_text(color=0x5D769A)
