// Terminals carry most of the character of an expressive S, so they are
// treated as a continuous mix rather than a category. Every generator emits
// CAP_POINTS points walking the same direction, which means any weighted
// blend of them is still a valid terminal.

import { CAP_KINDS } from './params.js';

export const CAP_POINTS = 14;

// u runs strictly inside (0,1); the endpoints are already supplied by the
// offset edges, so we avoid duplicated nodes.
const US = Array.from({ length: CAP_POINTS }, (_, i) => (i + 1) / (CAP_POINTS + 1));

function pt(P, nor, tan, an, at) {
  return { x: P.x + nor.x * an + tan.x * at, y: P.y + nor.y * an + tan.y * at };
}

// The cut angle is absolute - degrees from horizontal in the em box, not
// relative to the direction of travel. A cut that simply sits perpendicular to
// the stroke lands wherever the curve happens to be pointing, which is how you
// get terminals that look accidental. Here you say "cut this at 12 degrees"
// and it stays at 12 degrees whatever the aperture does.
function shearFor(tan, nor, angleDeg) {
  const a = (angleDeg * Math.PI) / 180;
  const c = Math.cos(a);
  const s = Math.sin(a);
  const denom = tan.x * s - tan.y * c;
  if (Math.abs(denom) < 1e-4) return 0;
  const k = (nor.y * c - nor.x * s) / denom;
  // Past roughly 40 degrees off perpendicular the cut stops reading as a cut
  // and starts reading as a splinter, so obliqueness is capped.
  return Math.max(-0.85, Math.min(0.85, k));
}

// An oblique cut has one long side and one short side. Left centred on the
// spine end it would bite backwards into the stroke and leave a nick where it
// crosses the edge, so the whole cut is pushed forward until its short side
// just touches the end. The terminal grows instead of self-intersecting.
function capCut(P, tan, nor, h, o) {
  const lead = Math.abs(o.shear) * h;
  return US.map((u) => {
    const across = h * (1 - 2 * u);
    return pt(P, nor, tan, across, across * o.shear + lead);
  });
}

function capRound(P, tan, nor, h) {
  return US.map((u) => {
    const a = Math.PI * u;
    return pt(P, nor, tan, h * Math.cos(a), h * Math.sin(a));
  });
}

function capBall(P, tan, nor, h, o) {
  const r = Math.max(h * 1.0001, h * o.ballScale);
  const d = Math.sqrt(Math.max(0, r * r - h * h));
  const C = { x: P.x + tan.x * d, y: P.y + tan.y * d };
  let a0 = Math.atan2(-d, h);
  let a1 = Math.atan2(-d, -h);
  while (a1 < a0) a1 += Math.PI * 2;
  return US.map((u) => {
    const a = a0 + (a1 - a0) * u;
    return pt(C, nor, tan, r * Math.cos(a), r * Math.sin(a));
  });
}

function capSpur(P, tan, nor, h, o) {
  const reach = h * 0.9 * o.spur;
  const lead = Math.abs(o.shear) * h;
  return US.map((u) => {
    const across = h * (1 - 2 * u);
    const bump = Math.pow(Math.max(0, 1 - 2.6 * u), 1.6);
    return pt(P, nor, tan, across, across * o.shear + lead + reach * bump);
  });
}

const GENERATORS = { cut: capCut, round: capRound, ball: capBall, spur: capSpur };

// mix is a weight per CAP_KINDS entry, already normalized.
export function makeCap(P, tan, nor, h, mix, options) {
  const opts = { ...options, shear: shearFor(tan, nor, options.angle) };
  const out = US.map(() => ({ x: 0, y: 0 }));
  for (let k = 0; k < CAP_KINDS.length; k++) {
    const w = mix[k];
    if (w <= 1e-4) continue;
    const pts = GENERATORS[CAP_KINDS[k]](P, tan, nor, h, opts);
    for (let i = 0; i < out.length; i++) {
      out[i].x += pts[i].x * w;
      out[i].y += pts[i].y * w;
    }
  }
  return out;
}
