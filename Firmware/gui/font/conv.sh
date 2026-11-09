#!/bin/sh

lv_font_conv --font $1.ttf -r 0x20-0x7F --format lvgl --no-compress --bpp 4 --size $2 -o $1_$2.c
