#ifndef BITMAP_SCRAMBLE_H
#define BITMAP_SCRAMBLE_H

#include <display.h>

#ifndef TUNE_TILE_SIZE
#define TUNE_TILE_SIZE 16
#endif

// Deterministic tile scramble: grid slots stay fixed; swap_count applies the
// first swap_count pairs from a seed-derived sequence to permute which source
// tile each slot shows. swap_count == 0 draws the image solved.
inline uint32_t _scramble_rng_next(uint32_t &state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

inline void _draw_bitmap_tile(int dst_x, int dst_y, const uint8_t *bitmap,
                              int bmp_w, int src_tile_col, int src_tile_row,
                              int tile_size, uint16_t color, uint16_t bg) {
  const int bpl = (bmp_w + 7) / 8;
  const int src_px0 = src_tile_col * tile_size;
  const int src_py0 = src_tile_row * tile_size;

  for (int dy = 0; dy < tile_size; dy++) {
    const uint8_t *row = bitmap + (src_py0 + dy) * bpl;
    for (int dx = 0; dx < tile_size; dx++) {
      const int px = src_px0 + dx;
      const uint8_t byte = pgm_read_byte(&row[px >> 3]);
      const bool on = byte & (0x80 >> (px & 7));
      display.writePixel(dst_x + dx, dst_y + dy, on ? color : bg);
    }
  }
}

inline void draw_bitmap_scrambled(int x, int y, const uint8_t *bitmap,
                                 int bmp_w, int bmp_h, int tile_size,
                                 int swap_count, uint32_t seed,
                                 uint16_t color = WHITE,
                                 uint16_t bg = BLACK) {
  if (tile_size <= 0 || bmp_w <= 0 || bmp_h <= 0)
    return;

  const int tiles_x = bmp_w / tile_size;
  const int tiles_y = bmp_h / tile_size;
  const int tile_count = tiles_x * tiles_y;
  if (tile_count <= 0)
    return;

  constexpr int kMaxTiles = (SCREEN_WIDTH / TUNE_TILE_SIZE) *
                            (SCREEN_HEIGHT / TUNE_TILE_SIZE);
  if (tile_count > kMaxTiles)
    return;

  uint16_t mapping[kMaxTiles];
  for (int i = 0; i < tile_count; i++)
    mapping[i] = (uint16_t)i;

  uint32_t rng = seed ? seed : 1u;
  const int swaps_to_apply =
      swap_count < 0 ? 0 : (swap_count > tile_count ? tile_count : swap_count);

  for (int s = 0; s < swaps_to_apply; s++) {
    const uint32_t r0 = _scramble_rng_next(rng);
    const uint32_t r1 = _scramble_rng_next(rng);
    const int a = (int)(r0 % (uint32_t)tile_count);
    int b = (int)(r1 % (uint32_t)tile_count);
    if (b == a)
      b = (b + 1) % tile_count;
    const uint16_t tmp = mapping[a];
    mapping[a] = mapping[b];
    mapping[b] = tmp;
  }

  for (int ty = 0; ty < tiles_y; ty++) {
    for (int tx = 0; tx < tiles_x; tx++) {
      const int slot = ty * tiles_x + tx;
      const int src = mapping[slot];
      const int src_tx = src % tiles_x;
      const int src_ty = src / tiles_x;
      _draw_bitmap_tile(x + tx * tile_size, y + ty * tile_size, bitmap, bmp_w,
                        src_tx, src_ty, tile_size, color, bg);
    }
  }
}

#endif
