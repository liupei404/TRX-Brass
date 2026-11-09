#!/usr/bin/env python3
"""Convert Brass LVGL 8 ARGB8888 .bin assets to the LVGL 9 file format."""

import argparse
import struct
from pathlib import Path


LVGL8_CF_TRUE_COLOR_ALPHA = 5
LVGL9_HEADER_MAGIC = 0x19
LVGL9_CF_ARGB8888 = 0x10


def convert(source: Path, destination: Path) -> None:
    data = source.read_bytes()
    old_header, = struct.unpack_from("<I", data)
    color_format = old_header & 0x1F
    width = (old_header >> 10) & 0x7FF
    height = (old_header >> 21) & 0x7FF
    stride = width * 4
    pixels = data[4:]

    if color_format != LVGL8_CF_TRUE_COLOR_ALPHA:
        raise ValueError(f"{source}: unsupported LVGL 8 color format {color_format}")
    if len(pixels) != stride * height:
        raise ValueError(f"{source}: invalid payload size {len(pixels)}")

    new_header = struct.pack(
        "<BBHHHHH",
        LVGL9_HEADER_MAGIC,
        LVGL9_CF_ARGB8888,
        0,  # flags
        width,
        height,
        stride,
        0,  # reserved
    )
    destination.write_bytes(new_header + pixels)
    print(f"{source} -> {destination} ({width}x{height}, stride {stride})")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()

    for source in sorted(args.directory.glob("*.bin")):
        if source.name.endswith(".v9.bin"):
            continue
        convert(source, source.with_suffix(".v9.bin"))


if __name__ == "__main__":
    main()
