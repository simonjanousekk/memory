// The 5 sliders the client's data will eventually drive, and the internal
// parameter schema that describes one S.
//
// Sliders are NOT parameters. Sliders address a position in a 5D space;
// keyframes placed in that space carry the actual parameter sets.

import { clamp } from './math.js';

export const AXES = [
  { id: 'weight', label: 'Weight', hint: 'light \u2192 heavy' },
  { id: 'contrast', label: 'Contrast', hint: 'monolinear \u2192 modulated' },
  { id: 'width', label: 'Width', hint: 'condensed \u2192 extended' },
  { id: 'tension', label: 'Tension', hint: 'organic \u2192 tense' },
  { id: 'energy', label: 'Energy', hint: 'calm \u2192 extreme (fires events)' },
];

// [min, max] for every blendable parameter. Anything outside these gets
// clamped after blending, which is what keeps RBF overshoot from breaking a glyph.
export const RANGE = {
  weight: [10, 150],
  contrastMin: [0.03, 1],
  stress: [-45, 135],
  contrastGamma: [0.35, 2.6],
  widthScale: [0.5, 1.45],
  tension: [0.30, 0.88],
  spineAngle: [30, 80],
  aperture: [0, 1],
  spineTilt: [-0.2, 0.2],
  bowlBalance: [0.39, 0.55],
  slant: [-24, 24],
  overshoot: [0, 0.04],
  taper: [-0.65, 0.65],
  capAngleA: [-65, 65],
  capAngleB: [-65, 65],
  ballScale: [1, 2.5],
  spur: [0, 1],
  inline: [0, 0.75],
};

// Terminal shapes are a mix vector rather than a category. All four cap
// generators emit the same number of points, so a mix is always well-formed
// and "type" becomes continuous and blendable.
export const CAP_KINDS = ['cut', 'round', 'ball', 'spur'];
export const CUT_COUNT = 3;

// Per-keyframe spine edits. The seven anchors are still derived from the
// parameters above, so aperture and width keep working; these are offsets on
// top of that result. Storing offsets rather than absolute positions is what
// lets a node edit survive alongside the parameters, and keeps the edit
// blendable - it is just seven more numbers per channel.
export const NODE_COUNT = 7;
export const NODE_LABELS = [
  'upper terminal',
  'top',
  'upper left',
  'junction',
  'lower right',
  'bottom',
  'lower terminal',
];
const zeros = () => new Array(NODE_COUNT).fill(0);
const ones = () => new Array(NODE_COUNT).fill(1);

export const DEFAULTS = {
  weight: 52,
  contrastMin: 1,
  stress: 0,
  contrastGamma: 1,
  widthScale: 1,
  tension: 0.552,
  spineAngle: 52,
  aperture: 0.45,
  spineTilt: 0,
  bowlBalance: 0.468,
  slant: 0,
  overshoot: 0.012,
  taper: 0,
  capA: [1, 0, 0, 0],
  capB: [1, 0, 0, 0],
  capAngleA: 0,
  capAngleB: 0,
  ballScale: 1.5,
  spur: 0,
  inline: 0,
  cutPos: [0.2, 0.5, 0.8],
  cutAngle: [0, 0, 0],
  cutWidth: [0, 0, 0],
  nodeDX: zeros(),
  nodeDY: zeros(),
  nodeDA: zeros(),
  nodeHIn: ones(),
  nodeHOut: ones(),
};

export const VECTOR_KEYS = [
  'capA', 'capB', 'cutPos', 'cutAngle', 'cutWidth',
  'nodeDX', 'nodeDY', 'nodeDA', 'nodeHIn', 'nodeHOut',
];

export function baseParams(overrides = {}) {
  const p = {};
  for (const k of Object.keys(DEFAULTS)) {
    const v = DEFAULTS[k];
    p[k] = Array.isArray(v) ? v.slice() : v;
  }
  return applyOverrides(p, overrides);
}

export function applyOverrides(p, overrides) {
  for (const k of Object.keys(overrides)) {
    const v = overrides[k];
    p[k] = Array.isArray(v) ? v.slice() : v;
  }
  return p;
}

export function cloneParams(p) {
  const out = {};
  for (const k of Object.keys(p)) {
    out[k] = Array.isArray(p[k]) ? p[k].slice() : p[k];
  }
  return out;
}

export function clampParams(p) {
  for (const k of Object.keys(RANGE)) {
    if (typeof p[k] === 'number') p[k] = clamp(p[k], RANGE[k][0], RANGE[k][1]);
  }
  for (const key of ['capA', 'capB']) {
    p[key] = p[key].map((v) => Math.max(0, v));
    const sum = p[key].reduce((a, b) => a + b, 0);
    p[key] = sum > 1e-6 ? p[key].map((v) => v / sum) : [1, 0, 0, 0];
  }
  p.cutPos = p.cutPos.map((v) => clamp(v, 0.06, 0.94));
  p.cutAngle = p.cutAngle.map((v) => clamp(v, -80, 80));
  p.cutWidth = p.cutWidth.map((v) => clamp(v, 0, 0.6));
  p.nodeDX = p.nodeDX.map((v) => clamp(v, -0.4, 0.4));
  p.nodeDY = p.nodeDY.map((v) => clamp(v, -0.4, 0.4));
  p.nodeDA = p.nodeDA.map((v) => clamp(v, -70, 70));
  p.nodeHIn = p.nodeHIn.map((v) => clamp(v, 0.12, 2.6));
  p.nodeHOut = p.nodeHOut.map((v) => clamp(v, 0.12, 2.6));
  return p;
}
