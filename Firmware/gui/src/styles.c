/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#include "styles.h"
#include "python/python.h"

lv_grad_dsc_t *waterfall_grad;

lv_style_t  *pannel_style;

lv_style_t  *msg_style;
lv_style_t  *msg_label_normal_style;
lv_style_t  *msg_label_select_style;

lv_style_t  *dialog_style;
lv_style_t  *dialog_no_buttons_style;
lv_style_t  *dialog_grid_style;
lv_style_t  *dialog_item_style;
lv_style_t  *dialog_title_style;
lv_style_t  *dialog_title_label_style;
lv_style_t  *dialog_label_style;
lv_style_t  *dialog_item_focus_style;
lv_style_t  *dialog_item_edited_style;
lv_style_t  *dialog_item_cursor_style;

lv_style_t  *dropdown_list_style;

lv_style_t  *slider_style;
lv_style_t  *slider_indicator_style;
lv_style_t  *slider_knob_style;
lv_style_t  *slider_indicator_edited_style;
lv_style_t  *slider_knob_edited_style;

lv_style_t  *switch_style;
lv_style_t  *switch_indicator_style;
lv_style_t  *switch_knob_style;
lv_style_t  *switch_indicator_checked_style;
lv_style_t  *switch_knob_checked_style;

lv_style_t  *keyboard_style;

lv_style_t  *ft8_waterfall_style;
lv_style_t  *ft8_finder_style;
lv_style_t  *ft8_finder_indicator_style;
lv_style_t  *ft8_table_style;
lv_style_t  *ft8_focused_style;
lv_style_t  *ft8_rx_info_style;
lv_style_t  *ft8_rx_msg_style;
lv_style_t  *ft8_rx_cq_style;
lv_style_t  *ft8_rx_to_me_style;
lv_style_t  *ft8_tx_msg_style;

lv_color_t  bg_color;

void styles_init() {
    bg_color = lv_color_hex(0x002550);

    /* * */

    waterfall_grad = python_get_grad("waterfall_grad");

    msg_style = python_get_style("msg_style");
    msg_label_normal_style = python_get_style("msg_label_normal_style");
    msg_label_select_style = python_get_style("msg_label_select_style");

    pannel_style = python_get_style("pannel_style");
    dialog_style = python_get_style("dialog_style");
    dialog_no_buttons_style = python_get_style("dialog_no_buttons_style");
    dialog_grid_style = python_get_style("dialog_grid_style");
    dialog_title_style = python_get_style("dialog_title_style");
    dialog_title_label_style = python_get_style("dialog_title_label_style");
    dialog_label_style = python_get_style("dialog_label_style");
    dialog_item_style = python_get_style("dialog_item_style");
    dialog_item_focus_style = python_get_style("dialog_item_focus_style");
    dialog_item_edited_style = python_get_style("dialog_item_edited_style");
    dialog_item_cursor_style = python_get_style("dialog_item_cursor_style");

    dropdown_list_style = python_get_style("dropdown_list_style");

    slider_style = python_get_style("slider_style");
    slider_indicator_style = python_get_style("slider_indicator_style");
    slider_knob_style = python_get_style("slider_knob_style");
    slider_indicator_edited_style = python_get_style("slider_indicator_edited_style");
    slider_knob_edited_style = python_get_style("slider_knob_edited_style");

    switch_style = python_get_style("switch_style");
    switch_indicator_style = python_get_style("switch_indicator_style");
    switch_knob_style = python_get_style("switch_knob_style");
    switch_indicator_checked_style = python_get_style("switch_indicator_checked_style");
    switch_knob_checked_style = python_get_style("switch_knob_checked_style");

    keyboard_style = python_get_style("keyboard_style");

    ft8_waterfall_style = python_get_style("ft8_waterfall_style");
    ft8_finder_style = python_get_style("ft8_finder_style");
    ft8_finder_indicator_style = python_get_style("ft8_finder_indicator_style");
    ft8_table_style = python_get_style("ft8_table_style");
    ft8_focused_style = python_get_style("ft8_focused_style");
    ft8_rx_info_style = python_get_style("ft8_rx_info_style");
    ft8_rx_msg_style = python_get_style("ft8_rx_msg_style");
    ft8_rx_cq_style = python_get_style("ft8_rx_cq_style");
    ft8_rx_to_me_style = python_get_style("ft8_rx_to_me_style");
    ft8_tx_msg_style = python_get_style("ft8_tx_msg_style");

}
