#ifndef HELPERS_H
#define HELPERS_H

#include <display.h>

// ---------------------------------------------------------------------------
// rotate_to_screen
//
// Transforms a point (mx, my) from an object-local coordinate space into
// screen pixel coordinates. The object is centered on the screen and rotated
// by angle (ca = cos(angle), sa = sin(angle)).
//
//   hw, hh  — half-width and half-height of the object in local pixels
//             (used to center the object before rotating)
//   ca, sa  — precomputed cos/sin of the rotation angle
// ---------------------------------------------------------------------------
static inline void rotate_to_screen(float mx, float my, float hw, float hh,
                                    float ca, float sa, int &sx, int &sy) {
  float ox = mx - hw, oy = my - hh;
  sx = SCREEN_WIDTH / 2 + (int)(ox * ca - oy * sa);
  sy = SCREEN_HEIGHT / 2 + (int)(ox * sa + oy * ca);
}

float dist_2(int x1, int y1, int x2, int y2) {
  return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

#endif
