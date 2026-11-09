/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2025 Belousov Oleg aka R1CBU
 */

#pragma once

#include <unistd.h>
#include "lvgl/lvgl.h"

extern lv_color_t   bg_color;

extern lv_grad_dsc_t *waterfall_grad;

extern lv_style_t   *pannel_style;

extern lv_style_t   *msg_style;
extern lv_style_t   *msg_label_normal_style;
extern lv_style_t   *msg_label_select_style;

extern lv_style_t   *dialog_style;
extern lv_style_t   *dialog_no_buttons_style;
extern lv_style_t   *dialog_grid_style;
extern lv_style_t   *dialog_title_style;
extern lv_style_t   *dialog_title_label_style;
extern lv_style_t   *dialog_label_style;
extern lv_style_t   *dialog_item_style;
extern lv_style_t   *dialog_label_style;
extern lv_style_t   *dialog_item_focus_style;
extern lv_style_t   *dialog_item_edited_style;
extern lv_style_t   *dialog_item_cursor_style;

extern lv_style_t   *dropdown_list_style;

extern lv_style_t   *slider_style;
extern lv_style_t   *slider_indicator_style;
extern lv_style_t   *slider_knob_style;
extern lv_style_t   *slider_indicator_edited_style;
extern lv_style_t   *slider_knob_edited_style;

extern lv_style_t   *switch_style;
extern lv_style_t   *switch_indicator_style;
extern lv_style_t   *switch_knob_style;
extern lv_style_t   *switch_indicator_checked_style;
extern lv_style_t   *switch_knob_checked_style;

extern lv_style_t   *keyboard_style;

extern lv_style_t   *ft8_waterfall_style;
extern lv_style_t   *ft8_finder_style;
extern lv_style_t   *ft8_finder_indicator_style;
extern lv_style_t   *ft8_table_style;
extern lv_style_t   *ft8_focused_style;
extern lv_style_t   *ft8_rx_info_style;
extern lv_style_t   *ft8_rx_msg_style;
extern lv_style_t   *ft8_rx_cq_style;
extern lv_style_t   *ft8_rx_to_me_style;
extern lv_style_t   *ft8_tx_msg_style;

void styles_init();
