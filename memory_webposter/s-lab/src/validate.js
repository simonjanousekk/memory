// Automatic rejection rules. The point of these is to let you review a whole
// lattice by looking at what got flagged, then fix the system rather than the
// individual output.

import { buildGlyph } from './glyph.js';
import { EM, SPINE_SAMPLES } from './skeleton.js';
import { pathData } from './svg.js';
import { cloneParams } from './params.js';

const RASTER = 48;
let canvas = null;

function ctx2d() {
  if (!canvas) {
    canvas = document.createElement('canvas');
    canvas.width = RASTER;
    canvas.height = RASTER;
  }
  return canvas.getContext('2d', { willReadFrequently: true });
}

// Low-res raster of a glyph, normalized to zero mean and unit length so two
// rasters can be compared by dot product.
export function rasterize(glyph) {
  const ctx = ctx2d();
  ctx.clearRect(0, 0, RASTER, RASTER);
  ctx.save();
  ctx.scale(RASTER / EM, RASTER / EM);
  ctx.fillStyle = '#000';
  ctx.fill(new Path2D(pathData(glyph, { curves: false })), 'evenodd');
  ctx.restore();

  const data = ctx.getImageData(0, 0, RASTER, RASTER).data;
  const v = new Float32Array(RASTER * RASTER);
  let ink = 0;
  for (let i = 0; i < v.length; i++) {
    v[i] = data[i * 4 + 3] / 255;
    ink += v[i];
  }
  ink /= v.length;

  let mean = 0;
  for (let i = 0; i < v.length; i++) mean += v[i];
  mean /= v.length;
  let nrm = 0;
  for (let i = 0; i < v.length; i++) {
    v[i] -= mean;
    nrm += v[i] * v[i];
  }
  nrm = Math.sqrt(nrm) || 1;
  for (let i = 0; i < v.length; i++) v[i] /= nrm;
  return { v, ink };
}

const bankCache = new WeakMap();

// Editing a keyframe changes what "reads as this skin's S" means, so the
// reference bank has to be dropped.
export function invalidateSkin(skin) {
  bankCache.delete(skin);
}

function referenceBank(skin) {
  if (!bankCache.has(skin)) {
    const bank = skin.keyframes.map((k) => rasterize(buildGlyph(cloneParams(k.params))).v);
    bankCache.set(skin, bank);
  }
  return bankCache.get(skin);
}

// "Does it still read as an S?" measured against the skin's own keyframes
// rather than against a generic alphabet. This is the useful version of the
// raster-comparison idea: analysis, not generation.
export function legibility(glyph, skin) {
  const { v, ink } = rasterize(glyph);
  const bank = referenceBank(skin);
  let best = -1;
  for (const ref of bank) {
    let d = 0;
    for (let i = 0; i < v.length; i++) d += v[i] * ref[i];
    if (d > best) best = d;
  }
  return { score: best, ink };
}

// Terminals crashing into the bowl is the classic S failure. Measure the real
// gap: centre-line distance minus the two half-widths.
function apertureGap(glyph) {
  const { samples } = glyph.spine;
  const half = glyph.profile.half;
  const skip = Math.round(SPINE_SAMPLES * 0.18);
  let worst = Infinity;
  for (const endIndex of [0, SPINE_SAMPLES - 1]) {
    for (let i = 0; i < samples.length; i++) {
      if (Math.abs(i - endIndex) < skip) continue;
      const a = samples[endIndex].p;
      const b = samples[i].p;
      const gap = Math.hypot(a.x - b.x, a.y - b.y) - half[endIndex] - half[i];
      if (gap < worst) worst = gap;
    }
  }
  return worst;
}

export function validate(glyph, skin) {
  const flags = [];
  const { profile } = glyph;

  if (profile.minHalf <= 4.2) {
    flags.push({ level: 'warn', code: 'hairline', text: 'stroke hits the hairline floor' });
  }
  if (profile.clampedFraction > 0.22) {
    flags.push({
      level: profile.clampedFraction > 0.4 ? 'fail' : 'warn',
      code: 'cusp',
      text: `weight exceeds the curve radius over ${Math.round(profile.clampedFraction * 100)}% of the spine`,
    });
  }

  const gap = apertureGap(glyph);
  if (gap < 0) {
    flags.push({ level: 'fail', code: 'collision', text: 'terminal collides with the bowl' });
  } else if (gap < 14) {
    flags.push({ level: 'warn', code: 'tight', text: 'aperture is nearly closed' });
  }

  const { score, ink } = legibility(glyph, skin);
  if (ink < 0.03) flags.push({ level: 'fail', code: 'empty', text: 'almost no ink' });
  if (ink > 0.52) flags.push({ level: 'warn', code: 'blob', text: 'counters filling in' });
  if (score < 0.55) {
    flags.push({
      level: score < 0.4 ? 'fail' : 'warn',
      code: 'unreadable',
      text: `weak match to the skin's own S shapes (${score.toFixed(2)})`,
    });
  }

  return {
    flags,
    score,
    ink,
    gap,
    ok: !flags.some((fl) => fl.level === 'fail'),
  };
}
