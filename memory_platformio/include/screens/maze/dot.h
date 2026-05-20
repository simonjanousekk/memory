#ifndef DOT_H
#define DOT_H

#include <display.h>
#include <helpers.h>

// ---------------------------------------------------------------------------
// Dot
//   Stores maze-local position and velocity. All drawing goes through the
//   same rotate_to_screen transform used by Maze, so the dot always stays
//   locked to the maze regardless of rotation angle.
// ---------------------------------------------------------------------------
class Dot {
  float _x, _y;
  float _vx = 0, _vy = 0;

public:
  void reset(float x, float y)       { _x = x;    _y = y;    _vx = 0;   _vy = 0; }
  void set_pos(float x, float y)     { _x = x;    _y = y; }
  void set_vel(float vx, float vy)   { _vx = vx;  _vy = vy; }
  void add_vel(float dvx, float dvy) { _vx += dvx; _vy += dvy; }
  void scale_vel(float s)            { _vx *= s;   _vy *= s; }
  void step(float dt)                { _x += _vx * dt; _y += _vy * dt; }

  float x()  const { return _x; }
  float y()  const { return _y; }
  float vx() const { return _vx; }
  float vy() const { return _vy; }

  void draw(float angle, float hw, float hh, int r = 2) const {
    float ca = cosf(angle), sa = sinf(angle);
    int sx, sy;
    rotate_to_screen(_x, _y, hw, hh, ca, sa, sx, sy);
    display.fillCircle(sx, sy, r, BLACK);
  }
};

#endif
