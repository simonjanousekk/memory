// Weighting a sparse set of keyframes scattered in the 5D slider space.
//
// Two schemes, because they fail in opposite directions:
//
//   rbf      Exact interpolation. At a keyframe you get precisely what you
//            designed. Between keyframes it can overshoot outside the hull of
//            your values, which usually reads as extra punch, and is made safe
//            by clamping parameters afterwards.
//
//   shepard  Normalized Gaussian weights, sharpened by an exponent. Never
//            overshoots, but only reproduces a keyframe exactly when sharpened
//            hard. This is the "mostly a designed shape with short
//            transitions" behaviour.
//
// snap then mixes either result toward the single nearest keyframe. Because
// the output is a static logo, snapping is free character: no one sees the
// discontinuity.

import { AXES } from './params.js';
import { clamp, solve } from './math.js';

const MQ_C = 0.6;
const kernel = (r) => Math.sqrt(r * r + MQ_C * MQ_C);

function distance(a, b) {
  let sum = 0;
  for (let i = 0; i < a.length; i++) {
    const d = a[i] - b[i];
    sum += d * d;
  }
  return Math.sqrt(sum);
}

export function axisVector(sliders) {
  return AXES.map((a) => clamp(sliders[a.id] ?? 0.5, 0, 1));
}

// Precompute the cardinal basis so weights at any point are one matrix-vector
// product. The appended linear polynomial keeps the system well conditioned
// and makes the weights sum to 1.
export function makeBlender(keyframes) {
  const pos = keyframes.map((k) => k.at);
  const n = pos.length;
  const dim = pos[0].length;
  const m = dim + 1;
  const size = n + m;

  const A = Array.from({ length: size }, () => new Array(size).fill(0));
  for (let i = 0; i < n; i++) {
    for (let j = 0; j < n; j++) A[i][j] = kernel(distance(pos[i], pos[j]));
    A[i][n] = 1;
    for (let d = 0; d < dim; d++) A[i][n + 1 + d] = pos[i][d];
  }
  for (let j = 0; j < n; j++) {
    A[n][j] = 1;
    for (let d = 0; d < dim; d++) A[n + 1 + d][j] = pos[j][d];
  }

  const rhs = Array.from({ length: size }, (_, i) =>
    Array.from({ length: n }, (_, j) => (i === j ? 1 : 0))
  );
  const basis = solve(A, rhs);

  return {
    keyframes,
    positions: pos,
    weights(x, opts = {}) {
      const mode = opts.mode ?? 'rbf';
      const snap = clamp(opts.snap ?? 0, 0, 1);
      const sigma = opts.sigma ?? 0.45;
      const sharpen = opts.sharpen ?? 3;

      let w;
      if (mode === 'rbf') {
        w = new Array(n).fill(0);
        for (let j = 0; j < n; j++) {
          let acc = 0;
          for (let i = 0; i < n; i++) acc += kernel(distance(x, pos[i])) * basis[i][j];
          acc += basis[n][j];
          for (let d = 0; d < dim; d++) acc += x[d] * basis[n + 1 + d][j];
          w[j] = acc;
        }
      } else {
        w = pos.map((p) => {
          const r = distance(x, p) / sigma;
          return Math.exp(-r * r);
        });
        const peak = Math.max(...w, 1e-9);
        w = w.map((v) => Math.pow(v / peak, sharpen));
        const sum = w.reduce((a, b) => a + b, 0) || 1;
        w = w.map((v) => v / sum);
      }

      if (snap > 0) {
        let best = 0;
        let bestD = Infinity;
        for (let i = 0; i < n; i++) {
          const d = distance(x, pos[i]);
          if (d < bestD) {
            bestD = d;
            best = i;
          }
        }
        w = w.map((v, i) => v * (1 - snap) + (i === best ? snap : 0));
      }

      return w;
    },
  };
}

export function blendValues(keyframes, weights, pick) {
  const sample = pick(keyframes[0]);
  if (Array.isArray(sample)) {
    const out = new Array(sample.length).fill(0);
    for (let i = 0; i < keyframes.length; i++) {
      const v = pick(keyframes[i]);
      for (let j = 0; j < out.length; j++) out[j] += v[j] * weights[i];
    }
    return out;
  }
  let acc = 0;
  for (let i = 0; i < keyframes.length; i++) acc += pick(keyframes[i]) * weights[i];
  return acc;
}

export function blendParams(keyframes, weights) {
  const keys = Object.keys(keyframes[0].params);
  const out = {};
  for (const k of keys) {
    out[k] = blendValues(keyframes, weights, (kf) => kf.params[k]);
  }
  return out;
}

// Outline-domain blending, kept for comparison. Requires identical topology,
// which buildGlyph guarantees.
export function blendContours(glyphs, weights) {
  const ref = glyphs[0];
  return ref.contours.map((c, ci) => {
    const points = c.points.map((_, pi) => {
      let x = 0;
      let y = 0;
      for (let g = 0; g < glyphs.length; g++) {
        const p = glyphs[g].contours[ci].points[pi];
        x += p.x * weights[g];
        y += p.y * weights[g];
      }
      return { x, y };
    });
    return { points, corners: c.corners, hard: c.hard };
  });
}
