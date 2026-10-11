// Direct manipulation of one keyframe's spine.
//
// Dragging writes node offsets, not absolute positions, so a node edit sits on
// top of whatever the parameters produce. Move the width slider afterwards and
// the edited node travels with the letter instead of being stranded.
//
// The overlay is updated in place rather than re-rendered, because replacing
// the markup mid-drag would destroy the element holding pointer capture.

import { EM } from './skeleton.js';
import { applyFit, undoFit } from './glyph.js';
import { NODE_LABELS } from './params.js';

const HANDLE_MIN = 0.12;
const HANDLE_MAX = 2.6;

// Keep a dragged angle on the same turn as the stored one, so crossing the
// 180 degree boundary does not spin the node around.
function nearestTurn(target, reference) {
  let d = target - reference;
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return reference + d;
}

function handlePoints(glyph) {
  const { segments } = glyph.spine;
  const out = [];
  const inn = [];
  for (let i = 0; i < segments.length; i++) {
    out[i] = segments[i][1];
    inn[i + 1] = segments[i][2];
  }
  return { out, inn };
}

function spinePath(glyph) {
  return glyph.spine.samples
    .map((s, i) => {
      const p = applyFit(glyph.fit, s.p);
      return `${i ? 'L' : 'M'}${p.x.toFixed(1)} ${p.y.toFixed(1)}`;
    })
    .join('');
}

export function overlayMarkup(glyph) {
  const { anchors } = glyph.spine;
  const { out, inn } = handlePoints(glyph);
  const at = (p) => applyFit(glyph.fit, p);

  let svg = `<path class="ed-spine" d="${spinePath(glyph)}"/>`;
  for (let i = 0; i < anchors.length; i++) {
    const a = at(anchors[i].p);
    for (const [kind, pt] of [['out', out[i]], ['in', inn[i]]]) {
      if (!pt) continue;
      const h = at(pt);
      svg += `<line class="ed-stem" data-kind="${kind}" data-index="${i}" x1="${a.x.toFixed(1)}" y1="${a.y.toFixed(1)}" x2="${h.x.toFixed(1)}" y2="${h.y.toFixed(1)}"/>`;
      svg += `<circle class="ed-handle" data-kind="${kind}" data-index="${i}" cx="${h.x.toFixed(1)}" cy="${h.y.toFixed(1)}" r="10"/>`;
    }
  }
  for (let i = 0; i < anchors.length; i++) {
    const a = at(anchors[i].p);
    svg += `<circle class="ed-anchor" data-kind="anchor" data-index="${i}" cx="${a.x.toFixed(1)}" cy="${a.y.toFixed(1)}" r="15"><title>${NODE_LABELS[i]}</title></circle>`;
  }
  return `<g class="ed">${svg}</g>`;
}

export function updateOverlay(root, glyph) {
  const { anchors } = glyph.spine;
  const { out, inn } = handlePoints(glyph);
  const at = (p) => applyFit(glyph.fit, p);

  root.querySelector('.ed-spine').setAttribute('d', spinePath(glyph));
  for (let i = 0; i < anchors.length; i++) {
    const a = at(anchors[i].p);
    const dot = root.querySelector(`.ed-anchor[data-index="${i}"]`);
    dot.setAttribute('cx', a.x.toFixed(1));
    dot.setAttribute('cy', a.y.toFixed(1));
    for (const [kind, pt] of [['out', out[i]], ['in', inn[i]]]) {
      if (!pt) continue;
      const h = at(pt);
      const stem = root.querySelector(`.ed-stem[data-kind="${kind}"][data-index="${i}"]`);
      stem.setAttribute('x1', a.x.toFixed(1));
      stem.setAttribute('y1', a.y.toFixed(1));
      stem.setAttribute('x2', h.x.toFixed(1));
      stem.setAttribute('y2', h.y.toFixed(1));
      const grip = root.querySelector(`.ed-handle[data-kind="${kind}"][data-index="${i}"]`);
      grip.setAttribute('cx', h.x.toFixed(1));
      grip.setAttribute('cy', h.y.toFixed(1));
    }
  }
}

// svg -> em coordinates, then back out through the fit, so we work in the
// space the spine actually lives in rather than the framed one.
function pointerToSpine(event, svg, fit) {
  const rect = svg.getBoundingClientRect();
  const em = {
    x: ((event.clientX - rect.left) / rect.width) * EM,
    y: ((event.clientY - rect.top) / rect.height) * EM,
  };
  return undoFit(fit, em);
}

export function attachEditor(svg, getGlyph, params, onChange) {
  let drag = null;

  const down = (event) => {
    const el = event.currentTarget;
    const index = +el.dataset.index;
    drag = {
      kind: el.dataset.kind,
      index,
      start: pointerToSpine(event, svg, getGlyph().fit),
      dx: params.nodeDX[index],
      dy: params.nodeDY[index],
    };
    el.classList.add('on');
    el.setPointerCapture(event.pointerId);
    event.preventDefault();
  };

  const move = (event) => {
    if (!drag) return;
    const glyph = getGlyph();
    const now = pointerToSpine(event, svg, glyph.fit);
    const { transform, requested, baseOut, baseIn } = glyph.spine.edit;
    const i = drag.index;

    if (drag.kind === 'anchor') {
      const a = transform.unpoint(drag.start.x, drag.start.y);
      const b = transform.unpoint(now.x, now.y);
      params.nodeDX[i] = drag.dx + (b.x - a.x);
      params.nodeDY[i] = drag.dy + (b.y - a.y);
    } else {
      const anchor = glyph.spine.anchors[i].p;
      const vx = drag.kind === 'out' ? now.x - anchor.x : anchor.x - now.x;
      const vy = drag.kind === 'out' ? now.y - anchor.y : anchor.y - now.y;
      const length = Math.hypot(vx, vy);
      if (length > 1) {
        const target = nearestTurn(transform.unangle(vx, vy), requested[i]);
        params.nodeDA[i] += target - requested[i];
        const base = drag.kind === 'out' ? baseOut[i] : baseIn[i];
        if (base > 1) {
          const key = drag.kind === 'out' ? 'nodeHOut' : 'nodeHIn';
          params[key][i] = Math.min(HANDLE_MAX, Math.max(HANDLE_MIN, length / base));
        }
      }
    }
    onChange(false);
    event.preventDefault();
  };

  const up = (event) => {
    if (!drag) return;
    drag = null;
    event.currentTarget.classList.remove('on');
    onChange(true);
  };

  for (const el of svg.querySelectorAll('.ed-anchor, .ed-handle')) {
    el.addEventListener('pointerdown', down);
    el.addEventListener('pointermove', move);
    el.addEventListener('pointerup', up);
    el.addEventListener('pointercancel', up);
  }
}

export function resetNodes(params, index = null) {
  const zero = ['nodeDX', 'nodeDY', 'nodeDA'];
  const one = ['nodeHIn', 'nodeHOut'];
  if (index === null) {
    for (const k of zero) params[k] = params[k].map(() => 0);
    for (const k of one) params[k] = params[k].map(() => 1);
  } else {
    for (const k of zero) params[k][index] = 0;
    for (const k of one) params[k][index] = 1;
  }
}

export function hasNodeEdits(params) {
  return (
    params.nodeDX.some((v) => Math.abs(v) > 1e-4) ||
    params.nodeDY.some((v) => Math.abs(v) > 1e-4) ||
    params.nodeDA.some((v) => Math.abs(v) > 1e-4) ||
    params.nodeHIn.some((v) => Math.abs(v - 1) > 1e-4) ||
    params.nodeHOut.some((v) => Math.abs(v - 1) > 1e-4)
  );
}
