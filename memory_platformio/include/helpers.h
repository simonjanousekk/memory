#ifndef HELPERS_H
#define HELPERS_H

float dist_2(int x1, int y1, int x2, int y2) {
  return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

float map(float value, float in_min, float in_max, float out_min,
          float out_max) {
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif