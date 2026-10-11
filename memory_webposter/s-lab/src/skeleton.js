// The S spine: seven anchors, each with a point and a direction of travel.
// Handles are derived from chord length, so the curve stays well-behaved for
// any parameter set. This is the "structure" half of the system.

import {
  add, cubicAt, cubicD1, cubicD2, curvatureOf, dist, fromAngle, lerp, mul,
  norm, perp, sub,
} from './math.js';

export const EM = 1000;
export const CAP_TOP = 140;
export const BASELINE = 860;
export const CAP_HEIGHT = BASELINE - CAP_TOP;
export const X_SPAN = 560;
export const CENTER_X = 500;
export const SPINE_SAMPLES = 220;

// 0.552 reproduces a circular arc when handles are measured against the
// chord's projection onto each tangent. tension scales around that: below is
// flatter and tenser, above is rounder and fuller.
const CIRCULAR = 0.552;

function anchorsNormalized(P) {
  const ap = P.aperture;
  const tilt = P.spineTilt;
  const os = P.overshoot;
  // A steeper spine is only possible if the bowl extremes move apart
  // vertically, so the two are coupled rather than left to contradict.
  const steep = (P.spineAngle - 45) / 50;
  const shift = steep * 0.09;
  return [
    // upper terminal
    { x: lerp(0.78, 0.875, ap), y: lerp(0.26, 0.105, ap), a: lerp(295, 250, ap) },
    // top of the upper bowl
    { x: 0.5 + tilt * 0.45, y: -os, a: 180 },
    // left of the upper bowl. Deliberately narrower than the lower bowl is
    // wide: an S with two equal bowls reads top-heavy.
    { x: lerp(0.175, 0.135, ap), y: Math.max(0.05, lerp(0.21, 0.235, ap) - shift), a: 90 },
    // junction, where the spine crosses the middle
    { x: 0.5 + tilt, y: P.bowlBalance, a: P.spineAngle },
    // right of the lower bowl
    { x: lerp(0.855, 0.895, ap), y: Math.min(0.95, lerp(0.70, 0.725, ap) + shift), a: 90 },
    // bottom of the lower bowl
    { x: 0.5 - tilt * 0.45, y: 1 + os, a: 180 },
    // lower terminal
    { x: lerp(0.24, 0.13, ap), y: lerp(0.79, 0.885, ap), a: lerp(228, 192, ap) },
  ];
}

// A cubic's chord direction is a weighted average of its tangent directions,
// so a chord that falls outside the range spanned by the two end tangents
// forces the curve to overshoot and wobble. That wobble was the single worst
// artefact in the first version: a lumpy spine no amount of width tuning could
// hide. Three anchors are free to absorb it - the junction and the two
// terminals - so their angles are clamped to whatever the fixed anchors allow.
const bearing = (a, b) => {
  const d = (Math.atan2(b.y - a.y, b.x - a.x) * 180) / Math.PI;
  return ((d % 360) + 360) % 360;
};

function resolveAngles(pts, spec) {
  spec[3].a = Math.max(10, Math.min(spec[3].a, bearing(pts[2], pts[3]) - 6, bearing(pts[3], pts[4]) - 6));
  spec[0].a = Math.max(spec[0].a, bearing(pts[0], pts[1]) + 4);
  spec[6].a = Math.max(spec[6].a, bearing(pts[5], pts[6]) + 4);
}

// Normalized space -> em space, including the slant shear. Directions use the
// same linear map so an anchor's intent survives anisotropic scaling. The
// inverses exist so a pointer drag in the em box can be turned back into a
// node offset.
export function makeTransform(P) {
  const sx = X_SPAN * P.widthScale;
  const sy = CAP_HEIGHT;
  const shear = Math.tan((P.slant * Math.PI) / 180);
  const pivotY = (CAP_TOP + BASELINE) / 2;
  return {
    point(x, y) {
      const py = CAP_TOP + y * sy;
      const px = CENTER_X + (x - 0.5) * sx;
      return { x: px + (pivotY - py) * shear, y: py };
    },
    dir(dx, dy) {
      const vy = dy * sy;
      const vx = dx * sx - vy * shear;
      return norm({ x: vx, y: vy });
    },
    unpoint(px, py) {
      const y = (py - CAP_TOP) / sy;
      const x = 0.5 + (px - CENTER_X - (pivotY - py) * shear) / sx;
      return { x, y };
    },
    // Em-space direction -> the angle you would have written in the anchor
    // table to produce it.
    unangle(vx, vy) {
      const dy = vy / sy;
      const dx = (vx + vy * shear) / sx;
      return (Math.atan2(dy, dx) * 180) / Math.PI;
    },
  };
}

// Node edits are applied before the angle guard, so a dragged node is still
// prevented from forcing the spine into a wobble.
function applyNodeEdits(spec, P) {
  for (let i = 0; i < spec.length; i++) {
    spec[i].x += P.nodeDX[i];
    spec[i].y += P.nodeDY[i];
    spec[i].a += P.nodeDA[i];
  }
}

export function buildSpine(P) {
  const T = makeTransform(P);
  const spec = anchorsNormalized(P);
  applyNodeEdits(spec, P);
  const pts = spec.map((a) => T.point(a.x, a.y));
  const requested = spec.map((a) => a.a);
  resolveAngles(pts, spec);

  const anchors = spec.map((a, i) => {
    const d = fromAngle(a.a);
    return { p: pts[i], d: T.dir(d.x, d.y), angle: a.a };
  });

  // Handle length per anchor comes from the chord's projection onto that
  // anchor's tangent, not from the chord length. A quarter ellipse needs a
  // long handle at the horizontal end and a short one at the vertical end;
  // using one length for both is what squares off the bowls.
  const segments = [];
  const baseOut = new Array(anchors.length).fill(0);
  const baseIn = new Array(anchors.length).fill(0);
  for (let i = 0; i < anchors.length - 1; i++) {
    const A = anchors[i];
    const B = anchors[i + 1];
    const chord = sub(B.p, A.p);
    const span = Math.hypot(chord.x, chord.y);
    const k = P.tension;
    baseOut[i] = Math.abs(chord.x * A.d.x + chord.y * A.d.y) * k;
    baseIn[i + 1] = Math.abs(chord.x * B.d.x + chord.y * B.d.y) * k;
    const la = Math.min(baseOut[i] * P.nodeHOut[i], span * 0.92);
    const lb = Math.min(baseIn[i + 1] * P.nodeHIn[i + 1], span * 0.92);
    segments.push([A.p, add(A.p, mul(A.d, la)), sub(B.p, mul(B.d, lb)), B.p]);
  }

  // Dense pass first, so arc length and curvature come from the real curve.
  const fine = [];
  const STEPS = 140;
  for (let si = 0; si < segments.length; si++) {
    const [p0, p1, p2, p3] = segments[si];
    const start = si === 0 ? 0 : 1;
    for (let i = start; i <= STEPS; i++) {
      const t = i / STEPS;
      const d1 = cubicD1(p0, p1, p2, p3, t);
      const d2 = cubicD2(p0, p1, p2, p3, t);
      fine.push({
        p: cubicAt(p0, p1, p2, p3, t),
        tan: norm(d1),
        k: curvatureOf(d1, d2),
      });
    }
  }

  const cum = [0];
  for (let i = 1; i < fine.length; i++) {
    cum.push(cum[i - 1] + dist(fine[i - 1].p, fine[i].p));
  }
  const total = cum[cum.length - 1];

  // Resample at even arc length so the width profile and ornaments can be
  // positioned in a way that reads as evenly distributed.
  const samples = [];
  let cursor = 0;
  for (let i = 0; i < SPINE_SAMPLES; i++) {
    const target = (i / (SPINE_SAMPLES - 1)) * total;
    while (cursor < cum.length - 2 && cum[cursor + 1] < target) cursor++;
    const span = cum[cursor + 1] - cum[cursor] || 1;
    const f = (target - cum[cursor]) / span;
    const a = fine[cursor];
    const b = fine[cursor + 1];
    const tan = norm({
      x: lerp(a.tan.x, b.tan.x, f),
      y: lerp(a.tan.y, b.tan.y, f),
    });
    samples.push({
      p: { x: lerp(a.p.x, b.p.x, f), y: lerp(a.p.y, b.p.y, f) },
      tan,
      nor: perp(tan),
      k: lerp(a.k, b.k, f),
      s: i / (SPINE_SAMPLES - 1),
    });
  }

  return {
    samples,
    length: total,
    segments,
    anchors,
    // Everything the node editor needs to turn a drag back into an offset.
    // `requested` is the angle before the guard clamped it, so dragging a
    // terminal handle stays proportional even where the clamp is active.
    edit: { transform: T, requested, baseOut, baseIn },
  };
}

// Point + frame at an arbitrary arc-length position, used to place ornaments
// on the spine rather than over the bounding box.
export function atPosition(spine, s) {
  const n = spine.samples.length;
  const f = Math.max(0, Math.min(1, s)) * (n - 1);
  const i = Math.min(n - 2, Math.floor(f));
  const t = f - i;
  const a = spine.samples[i];
  const b = spine.samples[i + 1];
  const tan = norm({ x: lerp(a.tan.x, b.tan.x, t), y: lerp(a.tan.y, b.tan.y, t) });
  return {
    p: { x: lerp(a.p.x, b.p.x, t), y: lerp(a.p.y, b.p.y, t) },
    tan,
    nor: perp(tan),
    index: i,
  };
}
