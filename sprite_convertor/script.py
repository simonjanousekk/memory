#!/usr/bin/env python3
"""
Convert all GIF files in sprite_convertor/sprites into C header files.

For each input GIF:
- reads all frames
- converts pixels to 1-bit monochrome
- packs bits left-to-right, top-to-bottom into bytes
- writes <gif_name>.h next to the GIF
"""

from __future__ import annotations

from pathlib import Path
from typing import List

try:
    from PIL import Image, ImageSequence
except ImportError as exc:
    raise SystemExit(
        "Missing dependency: Pillow. Install it with `python3 -m pip install pillow`."
    ) from exc


ROOT_DIR = Path(__file__).resolve().parent
SPRITES_DIR = ROOT_DIR / "sprites"


def sanitize_macro_name(name: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in name.upper())


def pack_1bit_frame(frame_rgba: Image.Image) -> List[int]:
    """Pack one frame to 1-bit bytes (MSB first)."""
    gray = frame_rgba.convert("L")
    width, height = gray.size
    bytes_per_row = (width + 7) // 8
    packed = bytearray(bytes_per_row * height)

    for y in range(height):
        for x in range(width):
            pixel = gray.getpixel((x, y))
            is_set = pixel < 128
            if is_set:
                byte_index = y * bytes_per_row + (x // 8)
                bit_index = 7 - (x % 8)
                packed[byte_index] |= 1 << bit_index

    return list(packed)


def gif_to_header(gif_path: Path) -> Path:
    with Image.open(gif_path) as gif:
        frames = [frame.convert("RGBA") for frame in ImageSequence.Iterator(gif)]
        if not frames:
            raise ValueError(f"No frames found in GIF: {gif_path}")

        width, height = frames[0].size
        for idx, frame in enumerate(frames):
            if frame.size != (width, height):
                raise ValueError(
                    f"Frame {idx} in {gif_path.name} has different size: {frame.size} "
                    f"(expected {(width, height)})"
                )

        packed_frames = [pack_1bit_frame(frame) for frame in frames]

    frame_count = len(packed_frames)
    bytes_per_frame = len(packed_frames[0])
    total_bytes = frame_count * bytes_per_frame
    total_kb = total_bytes / 1024

    stem = gif_path.stem
    data_name = f"{stem}_data"
    macro_prefix = sanitize_macro_name(stem)
    frame_count_macro = f"{macro_prefix}_FRAME_COUNT"
    frame_width_macro = f"{macro_prefix}_WIDTH"
    frame_height_macro = f"{macro_prefix}_HEIGHT"
    bytes_per_frame_macro = f"{macro_prefix}_BYTES_PER_FRAME"
    guard = f"{sanitize_macro_name(stem)}_H"

    lines: List[str] = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <Arduino.h>")
    lines.append("")
    lines.append(f"// Generated from: {gif_path.name}")
    lines.append(f"// Total Memory Usage: {total_kb:.2f} KB")
    lines.append(f"#define {frame_count_macro} {frame_count}")
    lines.append(f"#define {frame_width_macro} {width}")
    lines.append(f"#define {frame_height_macro} {height}")
    lines.append(f"#define {bytes_per_frame_macro} {bytes_per_frame}")
    lines.append("")
    lines.append(
        f"const uint8_t {data_name}[{frame_count_macro}][{bytes_per_frame_macro}] PROGMEM = {{"
    )

    for frame in packed_frames:
        hex_bytes = ", ".join(f"0x{b:02X}" for b in frame)
        lines.append(f"  {{ {hex_bytes} }},")

    lines.append("};")
    lines.append("")
    lines.append("#endif")
    lines.append("")

    out_path = gif_path.with_suffix(".h")
    out_path.write_text("\n".join(lines), encoding="utf-8")
    return out_path


def main() -> None:
    if not SPRITES_DIR.exists():
        raise FileNotFoundError(f"Sprites directory does not exist: {SPRITES_DIR}")

    gifs = sorted(SPRITES_DIR.glob("*.gif"))
    if not gifs:
        print(f"No GIFs found in: {SPRITES_DIR}")
        return

    for gif_path in gifs:
        out_path = gif_to_header(gif_path)
        print(f"Converted {gif_path.name} -> {out_path.name}")


if __name__ == "__main__":
    main()
