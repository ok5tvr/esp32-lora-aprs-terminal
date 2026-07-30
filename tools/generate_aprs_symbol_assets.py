#!/usr/bin/env python3
"""Generate LVGL RGB565 APRS symbol assets from a two-table reference chart.

The reference chart is expected to contain the primary table in the upper
half and the alternate table in the lower half, with codes ! through ~ laid
out left-to-right in rows of 16 symbols.
"""
from __future__ import annotations

import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

SYMBOL_FIRST = ord("!")
SYMBOL_LAST = ord("~")
SYMBOL_COUNT = SYMBOL_LAST - SYMBOL_FIRST + 1
ICON_SIZE = 30
CROP_SIZE = 40
X_CENTER_0 = 30.5
X_STEP = 43.05
PRIMARY_Y_CENTER_0 = 28.5
ALTERNATE_Y_CENTER_0 = 307.5
Y_STEP = 43.05


def crop_symbol(chart: Image.Image, index: int, alternate: bool) -> Image.Image:
    row = index // 16
    column = index % 16
    center_x = X_CENTER_0 + column * X_STEP
    base_y = ALTERNATE_Y_CENTER_0 if alternate else PRIMARY_Y_CENTER_0
    center_y = base_y + row * Y_STEP
    half = CROP_SIZE / 2.0
    box = (
        round(center_x - half),
        round(center_y - half),
        round(center_x + half),
        round(center_y + half),
    )
    return chart.crop(box).convert("RGB").resize(
        (ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS
    )


def rgb565_bytes(image: Image.Image) -> bytes:
    output = bytearray()
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue = pixels[x, y]
            value = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)
            # ESP32 is little-endian and LV_COLOR_16_SWAP is disabled.
            output.append(value & 0xFF)
            output.append((value >> 8) & 0xFF)
    return bytes(output)


def alpha_mask_bytes(image: Image.Image, size: int = 24) -> bytes:
    resized = image.convert("RGB").resize((size, size), Image.Resampling.LANCZOS)
    output = bytearray()
    pixels = resized.load()
    for y in range(resized.height):
        for x in range(resized.width):
            red, green, blue = pixels[x, y]
            # Convert the colored chart glyph to a reusable alpha-only indicator.
            # White chart background becomes transparent; colored/black pixels stay.
            distance = 255 - min(red, green, blue)
            output.append(max(0, min(255, distance * 4)))
    return bytes(output)


def unknown_icon() -> Image.Image:
    image = Image.new("RGB", (ICON_SIZE, ICON_SIZE), "white")
    draw = ImageDraw.Draw(image)
    draw.ellipse((3, 3, ICON_SIZE - 4, ICON_SIZE - 4), outline=(90, 100, 115), width=2)
    font = ImageFont.load_default()
    text = "?"
    bounds = draw.textbbox((0, 0), text, font=font)
    width = bounds[2] - bounds[0]
    height = bounds[3] - bounds[1]
    draw.text(
        ((ICON_SIZE - width) // 2, (ICON_SIZE - height) // 2 - 1),
        text,
        fill=(50, 60, 75),
        font=font,
    )
    return image


def byte_array(name: str, data: bytes) -> str:
    lines = [
        f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST std::uint8_t {name}[] = {{"
    ]
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def generate_table(chart: Image.Image, alternate: bool, output: Path) -> None:
    prefix = "alternate" if alternate else "primary"
    namespace_array = "alternate" if alternate else "primary"
    sections = [
        '#include "ui/icons/aprs_icon_assets.h"',
        "#include <cstdint>",
        "",
        "#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 0",
        "#error APRS symbol assets require LV_COLOR_DEPTH=16 and LV_COLOR_16_SWAP=0",
        "#endif",
        "",
        "namespace Ui::AprsIconAssets {",
        "namespace {",
        "",
        f"constexpr std::uint32_t ICON_WIDTH = {ICON_SIZE};",
        f"constexpr std::uint32_t ICON_HEIGHT = {ICON_SIZE};",
        "",
    ]
    names: list[str] = []
    for index in range(SYMBOL_COUNT):
        code = SYMBOL_FIRST + index
        name = f"{prefix}_{code:02x}_data"
        names.append(name)
        sections.append(byte_array(name, rgb565_bytes(crop_symbol(chart, index, alternate))))
        sections.append("")

    sections.extend(
        [
            "lv_img_dsc_t makeDescriptor(const std::uint8_t* data, std::uint32_t size) {",
            "    lv_img_dsc_t descriptor{};",
            "    descriptor.header.always_zero = 0;",
            "    descriptor.header.reserved = 0;",
            "    descriptor.header.cf = LV_IMG_CF_TRUE_COLOR;",
            "    descriptor.header.w = ICON_WIDTH;",
            "    descriptor.header.h = ICON_HEIGHT;",
            "    descriptor.data_size = size;",
            "    descriptor.data = data;",
            "    return descriptor;",
            "}",
            "",
            "}  // namespace",
            "",
            f"const lv_img_dsc_t {namespace_array}[SYMBOL_COUNT] = {{",
        ]
    )
    for name in names:
        sections.append(f"    makeDescriptor({name}, sizeof({name})),")
    sections.extend(["};", "", "}  // namespace Ui::AprsIconAssets", ""])
    output.write_text("\n".join(sections), encoding="utf-8")


def generate_common(chart: Image.Image, output: Path) -> None:
    unknown = rgb565_bytes(unknown_icon())
    car = alpha_mask_bytes(crop_symbol(chart, ord(">") - SYMBOL_FIRST, False))
    digipeater = alpha_mask_bytes(crop_symbol(chart, ord("#") - SYMBOL_FIRST, False))
    gateway = alpha_mask_bytes(crop_symbol(chart, ord("&") - SYMBOL_FIRST, True))
    text = f'''#include "ui/icons/aprs_icon_assets.h"
#include <cstdint>

#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 0
#error APRS symbol assets require LV_COLOR_DEPTH=16 and LV_COLOR_16_SWAP=0
#endif

namespace Ui::AprsIconAssets {{
namespace {{

constexpr std::uint32_t ICON_WIDTH = {ICON_SIZE};
constexpr std::uint32_t ICON_HEIGHT = {ICON_SIZE};
constexpr std::uint32_t INDICATOR_SIZE = 24;

{byte_array("unknown_data", unknown)}

{byte_array("car_indicator_data", car)}

{byte_array("digipeater_indicator_data", digipeater)}

{byte_array("gateway_indicator_data", gateway)}

lv_img_dsc_t makeColorDescriptor(const std::uint8_t* data, std::uint32_t size) {{
    lv_img_dsc_t descriptor{{}};
    descriptor.header.always_zero = 0;
    descriptor.header.reserved = 0;
    descriptor.header.cf = LV_IMG_CF_TRUE_COLOR;
    descriptor.header.w = ICON_WIDTH;
    descriptor.header.h = ICON_HEIGHT;
    descriptor.data_size = size;
    descriptor.data = data;
    return descriptor;
}}

lv_img_dsc_t makeAlphaDescriptor(const std::uint8_t* data, std::uint32_t size) {{
    lv_img_dsc_t descriptor{{}};
    descriptor.header.always_zero = 0;
    descriptor.header.reserved = 0;
    descriptor.header.cf = LV_IMG_CF_ALPHA_8BIT;
    descriptor.header.w = INDICATOR_SIZE;
    descriptor.header.h = INDICATOR_SIZE;
    descriptor.data_size = size;
    descriptor.data = data;
    return descriptor;
}}

}}  // namespace

const lv_img_dsc_t unknown = makeColorDescriptor(unknown_data, sizeof(unknown_data));
const lv_img_dsc_t car = makeAlphaDescriptor(car_indicator_data, sizeof(car_indicator_data));
const lv_img_dsc_t digipeater = makeAlphaDescriptor(
    digipeater_indicator_data, sizeof(digipeater_indicator_data));
const lv_img_dsc_t gateway = makeAlphaDescriptor(
    gateway_indicator_data, sizeof(gateway_indicator_data));

}}  // namespace Ui::AprsIconAssets
'''
    output.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    chart = Image.open(args.input).convert("RGBA")
    generate_table(chart, False, args.output_dir / "aprs_icon_assets_primary.cpp")
    generate_table(chart, True, args.output_dir / "aprs_icon_assets_alternate.cpp")
    generate_common(chart, args.output_dir / "aprs_icon_assets.cpp")


if __name__ == "__main__":
    main()
