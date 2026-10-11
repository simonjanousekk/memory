// One slider position -> one S.
//
// Order matters: blend, then vary by seed, then fire events. Events run last
// so a threshold crossing is never smoothed away by the blend, which is the
// whole point of having them.

import { blendContours, blendParams, makeBlender } from './blend.js';
import { buildGlyph } from './glyph.js';
import { AXES, clampParams, cloneParams } from './params.js';
import { rng } from './math.js';

const blenderCache = new WeakMap();

function blenderFor(skin) {
  if (!blenderCache.has(skin)) blenderCache.set(skin, makeBlender(skin.keyframes));
  return blenderCache.get(skin);
}

// Deterministic variation at a fixed slider position. Continuous wobble alone
// produces twelve near-identical S's, so the interesting half is the discrete
// picks below: a different terminal family, a break that appears, an inline
// that opens. Those are the choices a designer would actually make when asked
// for another version of the same logo.
const CAP_CHOICES = [
  [1, 0, 0, 0],
  [0, 1, 0, 0],
  [0, 0, 1, 0],
  [0, 0, 0, 1],
  [0.5, 0, 0, 0.5],
  [0.35, 0.65, 0, 0],
];

function mixToward(current, target, t) {
  return current.map((v, i) => v * (1 - t) + target[i] * t);
}

function vary(p, r, amount) {
  if (amount <= 0) return p;
  const j = (span) => (r() - 0.5) * 2 * span * amount;
  const chance = (base) => r() < base * amount;

  p.stress += j(12);
  p.tension += j(0.06);
  p.aperture += j(0.16);
  p.spineTilt += j(0.06);
  p.bowlBalance += j(0.025);
  p.spineAngle += j(8);
  p.slant += j(6);
  p.widthScale += j(0.09);
  p.weight *= 1 + j(0.1);
  p.capAngleA += j(26);
  p.capAngleB += j(26);
  p.taper += j(0.22);
  p.ballScale += j(0.22);
  p.spur += j(0.2);
  p.overshoot += j(0.008);

  if (chance(0.75)) {
    const pick = CAP_CHOICES[Math.floor(r() * CAP_CHOICES.length)];
    p.capA = mixToward(p.capA, pick, 0.45 + 0.45 * r());
  }
  if (chance(0.75)) {
    const pick = CAP_CHOICES[Math.floor(r() * CAP_CHOICES.length)];
    p.capB = mixToward(p.capB, pick, 0.45 + 0.45 * r());
  }
  if (chance(0.5)) {
    const i = Math.floor(r() * p.cutWidth.length);
    p.cutWidth[i] = Math.max(p.cutWidth[i], 0.1 + 0.1 * r());
    p.cutPos[i] = 0.15 + 0.7 * r();
    p.cutAngle[i] = (r() - 0.5) * 90;
  }
  if (chance(0.3)) p.inline = Math.max(p.inline, 0.28 + 0.24 * r());
  if (chance(0.35)) p.slant += (r() < 0.5 ? -1 : 1) * (6 + 8 * r());

  return p;
}

export function axesFromSliders(sliders) {
  const a = {};
  for (const ax of AXES) a[ax.id] = sliders[ax.id] ?? 0.5;
  return a;
}

export function quantize(value, levels) {
  if (!levels || levels < 2) return value;
  return Math.round(value * (levels - 1)) / (levels - 1);
}

export function generate(skin, sliders, opts = {}) {
  const {
    seed = 0,
    variance = 0,
    levels = 0,
    blendMode = 'rbf',
    snap = 0,
    sharpen = 3,
    sigma = 0.45,
    domain = 'params',
  } = opts;

  const axes = axesFromSliders(sliders);
  for (const k of Object.keys(axes)) axes[k] = quantize(axes[k], levels);
  const x = AXES.map((a) => axes[a.id]);

  const blender = blenderFor(skin);
  const weights = blender.weights(x, { mode: blendMode, snap, sharpen, sigma });

  if (domain === 'outline') {
    const glyphs = skin.keyframes.map((k) => buildGlyph(cloneParams(k.params)));
    const contours = blendContours(glyphs, weights);
    const ref = glyphs[0];
    return {
      axes,
      weights,
      params: blendParams(skin.keyframes, weights),
      firedRules: [],
      glyph: { contours, spine: ref.spine, profile: ref.profile, params: null },
    };
  }

  let params = clampParams(blendParams(skin.keyframes, weights));
  const r = rng(seed * 2654435761 + 12345);
  params = vary(params, r, variance);
  params = clampParams(params);

  const firedRules = [];
  for (const rule of skin.rules ?? []) {
    if (rule.when(axes, params)) {
      rule.apply(axes, params, r);
      firedRules.push(rule.name);
    }
  }
  params = clampParams(params);

  return { axes, weights, params, firedRules, glyph: buildGlyph(params) };
}
