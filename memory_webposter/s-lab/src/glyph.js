// Assembles one S from a parameter set.
//
// Every glyph emits the same contours with the same point counts, even when a
// component is switched off (it collapses to zero area instead of
// disappearing). That fixed topology is what lets outline-domain blending work
// at all, and it costs nothing when rendering.

import { buildSpine, EM, SPINE_SAMPLES } from './skeleton.js';
import { trimmedRange, widthProfile } from './stroke.js';
import { CAP_POINTS, makeCap } from './terminals.js';
import { CUT_COUNT } from './params.js';
import { polygonArea } from './math.js';

const INLINE_FROM = 0.06;
const INLINE_TO = 0.94;

function strokeContour(spine, half, from, to, capMixA, capMixB, optsA, optsB) {
  const left = [];
  const right = [];
  for (let i = from; i <= to; i++) {
    const s = spine.samples[i];
    left.push({ x: s.p.x + s.nor.x * half[i], y: s.p.y + s.nor.y * half[i] });
    right.push({ x: s.p.x - s.nor.x * half[i], y: s.p.y - s.nor.y * half[i] });
  }
  const last = spine.samples[to];
  const first = spine.samples[from];
  const capEnd = makeCap(last.p, last.tan, last.nor, half[to], capMixB, optsB);
  const capStart = makeCap(
    first.p,
    { x: -first.tan.x, y: -first.tan.y },
    { x: -first.nor.x, y: -first.nor.y },
    half[from],
    capMixA,
    optsA
  );

  const points = [...left, ...capEnd, ...right.slice().reverse(), ...capStart];
  const n = left.length;
  const corners = [n - 1, n + CAP_POINTS - 1, 2 * n + CAP_POINTS - 1, points.length - 1];
  return { points, corners, left, right, from };
}

// A stencil break is built from the stroke's own edge points, so the knockout
// ends exactly on the outline. A rectangle laid over the glyph would stick out
// past the edges, and under an even-odd fill those overhangs turn back into
// ink - a slash across the letter rather than a break in it.
const BAND_POINTS = 7;

function cutBand(main, spine, half, pos, angleDeg, width, weight) {
  const n = main.left.length;
  const arcStep = spine.length / (n - 1);
  const center = pos * (n - 1);
  const thickness = (width * weight * 2) / arcStep / 2;
  const hAt = half[Math.round(Math.min(n - 1, Math.max(0, center)))];
  const skew = (Math.tan((angleDeg * Math.PI) / 180) * hAt) / arcStep;

  const sample = (edge, at) => {
    const t = Math.max(0, Math.min(n - 1, at));
    const i = Math.min(n - 2, Math.floor(t));
    const f = t - i;
    const a = edge[i];
    const b = edge[i + 1];
    return { x: a.x + (b.x - a.x) * f, y: a.y + (b.y - a.y) * f };
  };

  const leftPts = [];
  const rightPts = [];
  for (let i = 0; i < BAND_POINTS; i++) {
    const u = i / (BAND_POINTS - 1);
    const off = -thickness + 2 * thickness * u;
    leftPts.push(sample(main.left, center + off + skew));
    rightPts.push(sample(main.right, center + off - skew));
  }
  return {
    points: [...leftPts, ...rightPts.reverse()],
    corners: [BAND_POINTS - 1, BAND_POINTS * 2 - 1],
    hard: false,
  };
}

function collapse(points, at) {
  return points.map(() => ({ x: at.x, y: at.y }));
}

export function buildGlyph(P) {
  const spine = buildSpine(P);
  const profile = widthProfile(spine, P);
  const half = profile.half;

  const optsA = { angle: P.capAngleA, ballScale: P.ballScale, spur: P.spur };
  const optsB = { angle: P.capAngleB, ballScale: P.ballScale, spur: P.spur };

  const main = strokeContour(
    spine, half, 0, SPINE_SAMPLES - 1, P.capA, P.capB, optsA, optsB
  );

  // Inline: a thinner S knocked out of the middle, stopping short of the
  // terminals. At inline = 0 it collapses so the point count never changes.
  const { a: ia, b: ib } = trimmedRange(spine, INLINE_FROM, INLINE_TO);
  const innerHalf = half.map((h) => h * P.inline);
  const round = [0, 1, 0, 0];
  const inline = strokeContour(
    spine, innerHalf, ia, ib, round, round,
    { angle: 0, ballScale: 1, spur: 0 },
    { angle: 0, ballScale: 1, spur: 0 }
  );
  if (P.inline < 0.02) {
    inline.points = collapse(inline.points, spine.samples[ia].p);
  }

  const cuts = [];
  for (let i = 0; i < CUT_COUNT; i++) {
    cuts.push(cutBand(main, spine, half, P.cutPos[i], P.cutAngle[i], P.cutWidth[i], P.weight));
  }

  const contours = [main, inline, ...cuts];
  const fit = fitToEm(contours, main.points);
  for (const c of contours) c.area = Math.abs(polygonArea(c.points));

  return { contours, spine, profile, params: P, fit };
}

// The spine is left in its own coordinates, so anything drawn over the glyph
// has to go through the same framing the contours did.
export function applyFit(fit, p) {
  return {
    x: EM / 2 + (p.x - fit.cx) * fit.scale,
    y: EM / 2 + (p.y - fit.cy) * fit.scale,
  };
}

export function undoFit(fit, p) {
  return {
    x: fit.cx + (p.x - EM / 2) / fit.scale,
    y: fit.cy + (p.y - EM / 2) / fit.scale,
  };
}

// Slant, taper and heavy terminals can push a glyph past the em box. Rather
// than let outputs clip, every glyph is centred and scaled down only if it
// overflows. Proportions are untouched; this is framing, not a compromise.
const MARGIN = 34;

function fitToEm(contours, reference) {
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
  for (const p of reference) {
    if (p.x < x0) x0 = p.x;
    if (p.y < y0) y0 = p.y;
    if (p.x > x1) x1 = p.x;
    if (p.y > y1) y1 = p.y;
  }
  const avail = EM - MARGIN * 2;
  const scale = Math.min(1, avail / (x1 - x0 || 1), avail / (y1 - y0 || 1));
  const cx = (x0 + x1) / 2;
  const cy = (y0 + y1) / 2;
  for (const c of contours) {
    for (const p of c.points) {
      p.x = EM / 2 + (p.x - cx) * scale;
      p.y = EM / 2 + (p.y - cy) * scale;
    }
  }
  return { scale, cx, cy };
}

export function glyphBBox(glyph) {
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
  for (const p of glyph.contours[0].points) {
    if (p.x < x0) x0 = p.x;
    if (p.y < y0) y0 = p.y;
    if (p.x > x1) x1 = p.x;
    if (p.y > y1) y1 = p.y;
  }
  return { x0, y0, x1, y1, w: x1 - x0, h: y1 - y0 };
}
