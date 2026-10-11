// Least-squares cubic fitting (Schneider's method, minus reparameterization).
// The generator produces smooth sampled polylines; this turns them back into
// a small number of Bezier segments so exported SVG has editable nodes rather
// than a few hundred line-to commands.

import { add, dist, mul, norm, sub } from './math.js';

const B0 = (u) => (1 - u) ** 3;
const B1 = (u) => 3 * u * (1 - u) ** 2;
const B2 = (u) => 3 * u * u * (1 - u);
const B3 = (u) => u ** 3;

function chordParams(pts) {
  const u = [0];
  for (let i = 1; i < pts.length; i++) u.push(u[i - 1] + dist(pts[i - 1], pts[i]));
  const total = u[u.length - 1] || 1;
  return u.map((v) => v / total);
}

function evalCubic(bez, t) {
  const [p0, p1, p2, p3] = bez;
  const a = B0(t), b = B1(t), c = B2(t), d = B3(t);
  return {
    x: a * p0.x + b * p1.x + c * p2.x + d * p3.x,
    y: a * p0.y + b * p1.y + c * p2.y + d * p3.y,
  };
}

function generateBezier(pts, u, t1, t2) {
  const n = pts.length;
  const first = pts[0];
  const last = pts[n - 1];
  let c00 = 0, c01 = 0, c11 = 0, x0 = 0, x1 = 0;

  for (let i = 0; i < n; i++) {
    const a0 = mul(t1, B1(u[i]));
    const a1 = mul(t2, B2(u[i]));
    c00 += a0.x * a0.x + a0.y * a0.y;
    c01 += a0.x * a1.x + a0.y * a1.y;
    c11 += a1.x * a1.x + a1.y * a1.y;
    const tmp = sub(pts[i], {
      x: first.x * (B0(u[i]) + B1(u[i])) + last.x * (B2(u[i]) + B3(u[i])),
      y: first.y * (B0(u[i]) + B1(u[i])) + last.y * (B2(u[i]) + B3(u[i])),
    });
    x0 += a0.x * tmp.x + a0.y * tmp.y;
    x1 += a1.x * tmp.x + a1.y * tmp.y;
  }

  const det = c00 * c11 - c01 * c01;
  let alphaL = det === 0 ? 0 : (x0 * c11 - x1 * c01) / det;
  let alphaR = det === 0 ? 0 : (c00 * x1 - c01 * x0) / det;

  const segLen = dist(first, last);
  if (alphaL < 1e-6 || alphaR < 1e-6) {
    alphaL = alphaR = segLen / 3;
  }
  return [first, add(first, mul(t1, alphaL)), add(last, mul(t2, alphaR)), last];
}

function maxError(pts, u, bez) {
  let max = 0;
  let index = Math.floor(pts.length / 2);
  for (let i = 1; i < pts.length - 1; i++) {
    const d = dist(evalCubic(bez, u[i]), pts[i]);
    if (d > max) {
      max = d;
      index = i;
    }
  }
  return { max, index };
}

function fitRecursive(pts, t1, t2, tol, depth, out) {
  if (pts.length < 3) {
    const t = sub(pts[pts.length - 1], pts[0]);
    const l = Math.hypot(t.x, t.y) / 3;
    out.push([pts[0], add(pts[0], mul(t1, l)), add(pts[pts.length - 1], mul(t2, l)), pts[pts.length - 1]]);
    return;
  }
  const u = chordParams(pts);
  const bez = generateBezier(pts, u, t1, t2);
  const { max, index } = maxError(pts, u, bez);
  if (max < tol || depth > 12) {
    out.push(bez);
    return;
  }
  const center = norm(sub(pts[index + 1] ?? pts[index], pts[index - 1]));
  fitRecursive(pts.slice(0, index + 1), t1, mul(center, -1), tol, depth + 1, out);
  fitRecursive(pts.slice(index), center, t2, tol, depth + 1, out);
}

// corners are indices in a closed polyline where the tangent genuinely breaks
// (terminal joins). Splitting there stops the fitter from rounding them off.
export function fitClosed(points, corners, tol = 0.6) {
  const n = points.length;
  const breaks = [...new Set(corners.map((c) => ((c % n) + n) % n))].sort((a, b) => a - b);
  if (breaks.length === 0) breaks.push(0);

  const out = [];
  for (let b = 0; b < breaks.length; b++) {
    const start = breaks[b];
    const end = breaks[(b + 1) % breaks.length];
    const span = [];
    let i = start;
    for (;;) {
      span.push(points[i]);
      if (i === end) break;
      i = (i + 1) % n;
      if (span.length > n) break;
    }
    if (span.length < 2) continue;
    const t1 = norm(sub(span[1], span[0]));
    const t2 = norm(sub(span[span.length - 2], span[span.length - 1]));
    fitRecursive(span, t1, t2, tol, 0, out);
  }
  return out;
}
