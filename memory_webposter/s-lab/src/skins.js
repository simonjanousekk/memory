// Skins are the discrete half of the system: style is a choice, not a slider.
// Each skin owns a base parameter set, a sparse scatter of keyframes in the 5D
// slider space, and its own event rules.
//
// A keyframe's `at` lists only the axes it cares about; unlisted axes sit at
// 0.5. That is what makes a 5D space practical to author - you place a handful
// of deliberate positions instead of filling a lattice.

import { AXES, baseParams } from './params.js';

function at(spec = {}) {
  return AXES.map((a) => (spec[a.id] === undefined ? 0.5 : spec[a.id]));
}

function buildSkin(def) {
  const base = baseParams(def.base);
  const keyframes = def.keyframes.map((k) => ({
    name: k.name,
    at: at(k.at),
    params: baseParams({ ...def.base, ...k.p }),
  }));
  return { ...def, base, keyframes };
}

export const SKINS = [
  buildSkin({
    id: 'grotesk',
    name: 'Grotesk',
    note: 'Monolinear, closed apertures, flat cuts. The structural baseline.',
    base: {
      weight: 56, contrastMin: 1, stress: 0, widthScale: 1, tension: 0.562,
      spineAngle: 54, aperture: 0.4, bowlBalance: 0.468, overshoot: 0.014,
      capA: [1, 0, 0, 0], capB: [1, 0, 0, 0],
    },
    keyframes: [
      { name: 'home', at: {}, p: {} },
      { name: 'light', at: { weight: 0 }, p: { weight: 21, aperture: 0.56, tension: 0.597 } },
      { name: 'heavy', at: { weight: 1 }, p: { weight: 88, widthScale: 1.1, aperture: 0.36, tension: 0.515, bowlBalance: 0.455 } },
      { name: 'mono', at: { contrast: 0 }, p: { contrastMin: 1 } },
      { name: 'modulated', at: { contrast: 1 }, p: { contrastMin: 0.24, stress: -8, contrastGamma: 1.25, capAngleA: 12, capAngleB: 12 } },
      { name: 'condensed', at: { width: 0 }, p: { widthScale: 0.62, weight: 46, tension: 0.62, spineAngle: 64 } },
      { name: 'extended', at: { width: 1 }, p: { widthScale: 1.34, tension: 0.515, spineAngle: 44 } },
      { name: 'organic', at: { tension: 0 }, p: { tension: 0.702, spineAngle: 44, aperture: 0.62, overshoot: 0.024 } },
      { name: 'tense', at: { tension: 1 }, p: { tension: 0.351, spineAngle: 71, aperture: 0.22, bowlBalance: 0.482 } },
      { name: 'calm', at: { energy: 0 }, p: { capAngleA: 0, capAngleB: 0, slant: 0 } },
      { name: 'extreme', at: { energy: 1 }, p: { capAngleA: -38, capAngleB: 34, slant: 6, spur: 0.5, capA: [0.4, 0, 0, 0.6], capB: [0.5, 0, 0, 0.5], taper: 0.22 } },
      // Correctives: positions where a plain blend of the axis extremes would
      // collide with itself.
      { name: 'heavy+tense', at: { weight: 1, tension: 1 }, p: { weight: 76, widthScale: 1.06, aperture: 0.34, tension: 0.409, spineAngle: 66, bowlBalance: 0.49 } },
      { name: 'heavy+modulated', at: { weight: 1, contrast: 1 }, p: { weight: 96, widthScale: 1.08, contrastMin: 0.17, contrastGamma: 1.5, aperture: 0.34 } },
      { name: 'light+extreme', at: { weight: 0, energy: 1 }, p: { weight: 23, spur: 0.75, capAngleA: -48, capAngleB: 44, taper: 0.35, slant: 8 } },
      { name: 'heavy+extreme', at: { weight: 1, energy: 1 }, p: { weight: 74, widthScale: 1.12, aperture: 0.42, capAngleA: -30, capAngleB: 28, spur: 0.35, taper: 0.12 } },
      { name: 'condensed+heavy', at: { width: 0, weight: 1 }, p: { weight: 60, widthScale: 0.72, aperture: 0.3, tension: 0.585, spineAngle: 68 } },
    ],
    rules: [
      {
        name: 'stencil break',
        when: (a) => a.energy > 0.58,
        apply: (a, p, r) => {
          const t = ramp(a.energy, 0.58, 1);
          p.cutWidth[1] = 0.18 * t;
          p.cutPos[1] = 0.5 + (r() - 0.5) * 0.1;
          p.cutAngle[1] = (r() - 0.5) * 40;
        },
      },
      {
        name: 'triple break',
        when: (a) => a.energy > 0.82,
        apply: (a, p, r) => {
          const t = ramp(a.energy, 0.82, 1);
          p.cutWidth[0] = 0.11 * t;
          p.cutWidth[2] = 0.11 * t;
          p.cutPos[0] = 0.21 + (r() - 0.5) * 0.12;
          p.cutPos[2] = 0.79 + (r() - 0.5) * 0.12;
          p.cutAngle[0] = (r() - 0.5) * 50;
          p.cutAngle[2] = (r() - 0.5) * 50;
        },
      },
      {
        name: 'inline opens',
        when: (a) => a.energy > 0.66 && a.weight > 0.55,
        apply: (a, p) => {
          p.inline = 0.3 * ramp(a.energy, 0.66, 1) * ramp(a.weight, 0.55, 1);
        },
      },
    ],
  }),

  buildSkin({
    id: 'didone',
    name: 'Didone',
    note: 'High contrast, vertical stress, ball terminals. Breaks if you cut it, so its events grow instead.',
    base: {
      weight: 60, contrastMin: 0.16, stress: 0, contrastGamma: 1.35,
      widthScale: 0.98, tension: 0.585, spineAngle: 50, aperture: 0.34,
      bowlBalance: 0.462, overshoot: 0.022, ballScale: 1.42,
      capA: [0, 0, 1, 0], capB: [0, 0, 1, 0],
    },
    keyframes: [
      { name: 'home', at: {}, p: {} },
      { name: 'light', at: { weight: 0 }, p: { weight: 28, contrastMin: 0.3, ballScale: 1.7, aperture: 0.46 } },
      { name: 'heavy', at: { weight: 1 }, p: { weight: 92, widthScale: 1.12, contrastMin: 0.12, ballScale: 1.18, aperture: 0.36, tension: 0.55 } },
      { name: 'flat', at: { contrast: 0 }, p: { contrastMin: 0.62, contrastGamma: 0.9, ballScale: 1.24 } },
      { name: 'hairline', at: { contrast: 1 }, p: { contrastMin: 0.06, contrastGamma: 1.7, ballScale: 1.7 } },
      { name: 'condensed', at: { width: 0 }, p: { widthScale: 0.6, weight: 44, ballScale: 1.22, spineAngle: 62, tension: 0.632 } },
      { name: 'extended', at: { width: 1 }, p: { widthScale: 1.36, spineAngle: 42, tension: 0.526 } },
      { name: 'organic', at: { tension: 0 }, p: { tension: 0.725, spineAngle: 42, aperture: 0.54, stress: -14 } },
      { name: 'tense', at: { tension: 1 }, p: { tension: 0.374, spineAngle: 68, aperture: 0.2, bowlBalance: 0.485 } },
      { name: 'calm', at: { energy: 0 }, p: { slant: 0, ballScale: 1.45, taper: 0 } },
      { name: 'extreme', at: { energy: 1 }, p: { slant: -14, ballScale: 1.85, taper: -0.4, capA: [0, 0, 0.8, 0.2], stress: 16 } },
      { name: 'heavy+hairline', at: { weight: 1, contrast: 1 }, p: { weight: 104, widthScale: 1.12, contrastMin: 0.05, contrastGamma: 1.9, ballScale: 1.38, aperture: 0.32 } },
      { name: 'light+extreme', at: { weight: 0, energy: 1 }, p: { weight: 26, ballScale: 2.0, slant: -18, contrastMin: 0.35 } },
      { name: 'tense+heavy', at: { tension: 1, weight: 1 }, p: { weight: 82, widthScale: 1.06, tension: 0.421, spineAngle: 64, aperture: 0.32, bowlBalance: 0.492 } },
    ],
    rules: [
      {
        name: 'terminals swell',
        when: (a) => a.energy > 0.7,
        apply: (a, p) => {
          p.ballScale += 0.5 * ramp(a.energy, 0.7, 1);
        },
      },
      {
        name: 'stress drifts',
        when: (a) => a.energy > 0.55,
        apply: (a, p, r) => {
          p.stress += (r() - 0.5) * 26 * ramp(a.energy, 0.55, 1);
        },
      },
    ],
  }),

  buildSkin({
    id: 'brut',
    name: 'Brut',
    note: 'Reverse contrast, angled cuts, stencil-prone. The loudest skin; events fire early.',
    base: {
      weight: 62, contrastMin: 0.4, stress: 88, contrastGamma: 0.85,
      widthScale: 1.04, tension: 0.398, spineAngle: 66, aperture: 0.3,
      bowlBalance: 0.478, overshoot: 0.006, capAngleA: -30, capAngleB: 26,
      capA: [1, 0, 0, 0], capB: [1, 0, 0, 0],
    },
    keyframes: [
      { name: 'home', at: {}, p: {} },
      { name: 'light', at: { weight: 0 }, p: { weight: 30, contrastMin: 0.6, tension: 0.468 } },
      { name: 'heavy', at: { weight: 1 }, p: { weight: 92, widthScale: 1.1, contrastMin: 0.34, aperture: 0.3, tension: 0.374, bowlBalance: 0.49 } },
      { name: 'flat', at: { contrast: 0 }, p: { contrastMin: 1, stress: 0 } },
      { name: 'reversed', at: { contrast: 1 }, p: { contrastMin: 0.2, stress: 94, contrastGamma: 0.7 } },
      { name: 'condensed', at: { width: 0 }, p: { widthScale: 0.58, weight: 46, spineAngle: 74, tension: 0.421 } },
      { name: 'extended', at: { width: 1 }, p: { widthScale: 1.42, spineAngle: 40, tension: 0.351 } },
      { name: 'organic', at: { tension: 0 }, p: { tension: 0.679, spineAngle: 46, aperture: 0.5, capAngleA: -12, capAngleB: 10 } },
      { name: 'tense', at: { tension: 1 }, p: { tension: 0.3, spineAngle: 76, aperture: 0.14, bowlBalance: 0.5 } },
      { name: 'calm', at: { energy: 0 }, p: { capAngleA: -10, capAngleB: 8, slant: 0, spur: 0 } },
      { name: 'extreme', at: { energy: 1 }, p: { capAngleA: -58, capAngleB: 54, slant: -16, spur: 0.7, capA: [0.2, 0, 0, 0.8], capB: [0.2, 0, 0, 0.8], taper: 0.55 } },
      { name: 'heavy+reversed', at: { weight: 1, contrast: 1 }, p: { weight: 96, widthScale: 1.1, contrastMin: 0.16, stress: 96, aperture: 0.28, contrastGamma: 0.65 } },
      { name: 'heavy+extreme', at: { weight: 1, energy: 1 }, p: { weight: 78, widthScale: 1.14, aperture: 0.4, contrastMin: 0.5, capAngleA: -44, capAngleB: 40, spur: 0.4, taper: 0.2 } },
      { name: 'tense+extreme', at: { tension: 1, energy: 1 }, p: { tension: 0.304, spineAngle: 78, capAngleA: -58, capAngleB: 54, aperture: 0.24, spur: 0.65 } },
      { name: 'light+heavyenergy', at: { weight: 0, energy: 1 }, p: { weight: 26, spur: 0.8, taper: 0.62, capAngleA: -60, contrastMin: 0.75 } },
    ],
    rules: [
      {
        name: 'early stencil',
        when: (a) => a.energy > 0.34,
        apply: (a, p, r) => {
          const t = ramp(a.energy, 0.34, 1);
          p.cutWidth[1] = 0.18 * t;
          p.cutPos[1] = 0.48 + (r() - 0.5) * 0.18;
          p.cutAngle[1] = (r() - 0.5) * 70;
        },
      },
      {
        name: 'shatter',
        when: (a) => a.energy > 0.62,
        apply: (a, p, r) => {
          const t = ramp(a.energy, 0.62, 1);
          p.cutWidth[0] = 0.16 * t;
          p.cutWidth[2] = 0.16 * t;
          p.cutPos[0] = 0.18 + (r() - 0.5) * 0.16;
          p.cutPos[2] = 0.82 + (r() - 0.5) * 0.16;
          p.cutAngle[0] = (r() - 0.5) * 80;
          p.cutAngle[2] = (r() - 0.5) * 80;
        },
      },
      {
        name: 'inline opens',
        when: (a) => a.contrast > 0.6 && a.weight > 0.45,
        apply: (a, p) => {
          p.inline = 0.46 * ramp(a.contrast, 0.6, 1) * ramp(a.weight, 0.45, 1);
        },
      },
    ],
  }),
];

export function ramp(v, a, b) {
  if (b === a) return v >= b ? 1 : 0;
  return Math.max(0, Math.min(1, (v - a) / (b - a)));
}

export function getSkin(id) {
  return SKINS.find((s) => s.id === id) ?? SKINS[0];
}
