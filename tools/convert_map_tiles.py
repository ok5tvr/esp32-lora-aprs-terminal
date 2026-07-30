#!/usr/bin/env python3
"""Convert standard XYZ PNG/JPEG/WebP map tiles to raw RGB565 tiles.

Input layout:
    <input>/<zoom>/<x>/<y>.png

Output layout for the firmware:
    <output>/<zoom>/<x>/<y>.rgb

Each output file is exactly 256 x 256 x 2 bytes. Pixels are little-endian
RGB565 because the ESP32 and LVGL framebuffer use native little-endian 16-bit
color with LV_COLOR_16_SWAP=0.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - user-facing dependency check
    raise SystemExit("Pillow is required: python -m pip install Pillow") from exc

SUPPORTED_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp", ".bmp"}
TILE_SIZE = 256


def rgb888_to_rgb565(red: int, green: int, blue: int) -> int:
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3)


def parse_xyz(relative_path: Path) -> tuple[int, int, int] | None:
    if len(relative_path.parts) != 3:
        return None
    zoom_text, x_text, filename = relative_path.parts
    y_text = Path(filename).stem
    try:
        return int(zoom_text), int(x_text), int(y_text)
    except ValueError:
        return None


def convert_tile(source: Path, destination: Path) -> None:
    with Image.open(source) as image:
        image = image.convert("RGB")
        if image.size != (TILE_SIZE, TILE_SIZE):
            image = image.resize((TILE_SIZE, TILE_SIZE), Image.Resampling.LANCZOS)

        rgb = image.tobytes()
        output = bytearray(TILE_SIZE * TILE_SIZE * 2)
        offset = 0
        for index in range(0, len(rgb), 3):
            value = rgb888_to_rgb565(rgb[index], rgb[index + 1], rgb[index + 2])
            struct.pack_into("<H", output, offset, value)
            offset += 2

    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert XYZ map tiles to LoRa APRS Terminal RGB565 format.")
    parser.add_argument("--input", required=True, type=Path, help="Root of XYZ image tiles")
    parser.add_argument("--output", required=True, type=Path, help="Destination MAP directory")
    parser.add_argument("--min-zoom", type=int, default=0)
    parser.add_argument("--max-zoom", type=int, default=30)
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    if not args.input.is_dir():
        parser.error(f"Input directory does not exist: {args.input}")
    if args.min_zoom < 0 or args.max_zoom < args.min_zoom:
        parser.error("Invalid zoom range")

    converted = 0
    skipped = 0
    invalid = 0
    for source in sorted(args.input.rglob("*")):
        if not source.is_file() or source.suffix.lower() not in SUPPORTED_EXTENSIONS:
            continue
        relative = source.relative_to(args.input)
        xyz = parse_xyz(relative)
        if xyz is None:
            invalid += 1
            continue
        zoom, x, y = xyz
        if zoom < args.min_zoom or zoom > args.max_zoom:
            continue
        if x < 0 or y < 0 or x >= (1 << zoom) or y >= (1 << zoom):
            invalid += 1
            continue

        destination = args.output / str(zoom) / str(x) / f"{y}.rgb"
        if destination.exists() and not args.overwrite:
            skipped += 1
            continue
        convert_tile(source, destination)
        converted += 1
        if converted % 100 == 0:
            print(f"Converted {converted} tiles...", file=sys.stderr)

    info = args.output / "README.TXT"
    info.parent.mkdir(parents=True, exist_ok=True)
    info.write_text(
        "LoRa APRS Terminal offline map\n"
        "Format: XYZ Web Mercator, 256x256, little-endian RGB565\n"
        "Path: /MAP/<zoom>/<x>/<y>.rgb\n",
        encoding="ascii",
    )

    print(f"Converted: {converted}")
    print(f"Skipped existing: {skipped}")
    print(f"Ignored invalid paths: {invalid}")
    print(f"Output: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
