// The "style" half: how much ink sits on the spine at each point, and the
// offset outline that results.
//
// Width follows a broad-nib model: thickness is greatest where the direction
// of travel is perpendicular to the nib edge. stress = 0 gives vertical stress
// (Didone-like); stress near 90 gives reverse contrast, which is where a lot
// of expressive type lives.

import { add, clamp, mul, smooth, sub } from './math.js';

const MIN_HALF_WIDTH = 3.5;

export function widthProfile(spine, P) {
  const raw = spine.samples.map((s) => {
    const phi = Math.atan2(s.tan.y, s.tan.x);
    const m = Math.abs(Math.sin(phi - (P.stress * Math.PI) / 180));
    const modulation = P.contrastMin + (1 - P.contrastMin) * Math.pow(m, P.contrastGamma);
    const taper = 1 + P.taper * (s.s - 0.5);
    return P.weight * modulation * taper;
  });

  const smoothed = smooth(raw, 3);

  // A cusp appears when the offset radius exceeds the local radius of
  // curvature. Clamping here is what makes every slider position produce a
  // valid outline instead of a knotted one.
  let clampedSamples = 0;
  let clampLoss = 0;
  const half = smoothed.map((h, i) => {
    const k = Math.abs(spine.samples[i].k);
    const limit = k > 1e-6 ? 0.96 / k : Infinity;
    let v = h;
    if (v > limit) {
      clampedSamples++;
      clampLoss = Math.max(clampLoss, (v - limit) / v);
      v = limit;
    }
    return Math.max(MIN_HALF_WIDTH, v);
  });

  const final = smooth(half, 2);
  return {
    half: final,
    minHalf: Math.min(...final),
    maxHalf: Math.max(...final),
    clampedFraction: clampedSamples / smoothed.length,
    clampLoss,
  };
}

export function offsetEdges(spine, half) {
  const left = [];
  const right = [];
  for (let i = 0; i < spine.samples.length; i++) {
    const s = spine.samples[i];
    left.push(add(s.p, mul(s.nor, half[i])));
    right.push(sub(s.p, mul(s.nor, half[i])));
  }
  return { left, right };
}

// Sub-range of the spine, used by the inline so it can stop short of the
// terminals instead of poking through them.
export function trimmedRange(spine, from, to) {
  const n = spine.samples.length;
  const a = Math.round(clamp(from, 0, 1) * (n - 1));
  const b = Math.round(clamp(to, 0, 1) * (n - 1));
  return { a, b };
}
