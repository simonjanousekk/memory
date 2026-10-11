import { AXES, NODE_LABELS, clampParams, cloneParams } from './params.js';
import { SKINS, getSkin } from './skins.js';
import { generate } from './generate.js';
import { buildGlyph } from './glyph.js';
import { pathData, svgDocument, download } from './svg.js';
import { invalidateSkin, validate } from './validate.js';
import { EM } from './skeleton.js';
import {
  attachEditor, hasNodeEdits, overlayMarkup, resetNodes, updateOverlay,
} from './editor.js';

const $ = (id) => document.getElementById(id);

const state = {
  skinId: SKINS[0].id,
  sliders: Object.fromEntries(AXES.map((a) => [a.id, 0.5])),
  seed: 0,
  variance: 0.35,
  mode: 'rbf',
  sharpen: 3,
  snap: 0,
  levels: 5,
  domain: 'params',
  view: 'single',
  debug: 'off',
  sheetX: 'weight',
  sheetY: 'energy',
  editIndex: -1,
};

const STORE_KEY = 's-lab.edits.v1';

// Edits live on the skin objects so every view sees them immediately, and are
// mirrored to localStorage so a reload does not throw away an afternoon.
// Copy whole skin is still the way to make them permanent.
function loadEdits() {
  let saved;
  try {
    saved = JSON.parse(localStorage.getItem(STORE_KEY) || '{}');
  } catch {
    return;
  }
  for (const skin of SKINS) {
    const forSkin = saved[skin.id];
    if (!forSkin) continue;
    for (const kf of skin.keyframes) {
      const patch = forSkin[kf.name];
      if (patch) Object.assign(kf.params, patch);
    }
    invalidateSkin(skin);
  }
}

function saveEdits() {
  const out = {};
  for (const skin of SKINS) {
    for (const kf of skin.keyframes) {
      if (!hasNodeEdits(kf.params)) continue;
      out[skin.id] = out[skin.id] || {};
      out[skin.id][kf.name] = {
        nodeDX: kf.params.nodeDX,
        nodeDY: kf.params.nodeDY,
        nodeDA: kf.params.nodeDA,
        nodeHIn: kf.params.nodeHIn,
        nodeHOut: kf.params.nodeHOut,
      };
    }
  }
  try {
    localStorage.setItem(STORE_KEY, JSON.stringify(out));
  } catch {
    /* storage full or blocked; edits still live in memory */
  }
}

function opts(extra = {}) {
  return {
    seed: state.seed,
    variance: state.variance,
    levels: state.levels,
    blendMode: state.mode,
    snap: state.snap,
    sharpen: state.sharpen,
    domain: state.domain,
    ...extra,
  };
}

function svgMarkup(glyph, opts = {}) {
  const debug = opts.debug ?? 'off';
  const d = pathData(glyph, { curves: debug !== 'raw' });
  let overlay = '';
  if (debug === 'skeleton' && glyph.spine) {
    const spine = glyph.spine.samples.map((s, i) => `${i ? 'L' : 'M'}${s.p.x.toFixed(1)} ${s.p.y.toFixed(1)}`).join('');
    const nodes = glyph.contours[0].points
      .map((p) => `<circle cx="${p.x.toFixed(1)}" cy="${p.y.toFixed(1)}" r="2.4" fill="#e8342f"/>`)
      .join('');
    const anchors = glyph.spine.anchors
      .map((a) => `<circle cx="${a.p.x.toFixed(1)}" cy="${a.p.y.toFixed(1)}" r="8" fill="none" stroke="#1668d8" stroke-width="3"/>`)
      .join('');
    overlay = `<path d="${spine}" fill="none" stroke="#1668d8" stroke-width="3"/>${nodes}${anchors}`;
  }
  const fill = debug === 'skeleton' ? '#141416' : '#141416';
  const opacity = debug === 'skeleton' ? 0.35 : 1;
  return `<svg viewBox="0 0 ${EM} ${EM}"><path fill="${fill}" fill-opacity="${opacity}" fill-rule="evenodd" d="${d}"/>${overlay}</svg>`;
}

// ---------------------------------------------------------------- controls

function buildSkinButtons() {
  $('skins').innerHTML = SKINS.map(
    (s) => `<button data-skin="${s.id}" title="${s.note}" class="${s.id === state.skinId ? 'on' : ''}">${s.name}</button>`
  ).join('');
  $('skins').querySelectorAll('button').forEach((b) => {
    b.onclick = () => {
      state.skinId = b.dataset.skin;
      state.editIndex = -1;
      buildSkinButtons();
      render();
    };
  });
}

function buildAxisControls() {
  $('axes').innerHTML = AXES.map(
    (a) => `<div class="axis">
      <div class="head"><b>${a.label}</b><span id="out-${a.id}">0.50</span></div>
      <input type="range" id="ax-${a.id}" min="0" max="100" step="1" value="${state.sliders[a.id] * 100}">
      <div class="hint">${a.hint}</div>
    </div>`
  ).join('');
  for (const a of AXES) {
    $(`ax-${a.id}`).oninput = (e) => {
      state.sliders[a.id] = e.target.value / 100;
      render();
    };
  }
}

function bindControls() {
  $('seed').oninput = (e) => { state.seed = +e.target.value; render(); };
  $('variance').oninput = (e) => { state.variance = e.target.value / 100; render(); };
  $('reroll').onclick = () => {
    state.seed = Math.floor(Math.random() * 200);
    $('seed').value = state.seed;
    render();
  };
  $('mode').onchange = (e) => { state.mode = e.target.value; render(); };
  $('sharpen').oninput = (e) => { state.sharpen = +e.target.value; render(); };
  $('snap').oninput = (e) => { state.snap = e.target.value / 100; render(); };
  $('levels').onchange = (e) => { state.levels = +e.target.value; render(); };
  $('domain').onchange = (e) => { state.domain = e.target.value; render(); };
  $('debug').onchange = (e) => { state.debug = e.target.value; render(); };

  document.querySelectorAll('.views button').forEach((b) => {
    b.onclick = () => {
      state.view = b.dataset.view;
      document.querySelectorAll('.views button').forEach((x) => x.classList.toggle('on', x === b));
      render();
    };
  });

  $('exportSvg').onclick = () => {
    const skin = getSkin(state.skinId);
    const res = generate(skin, state.sliders, opts());
    const name = `S_${skin.id}_${AXES.map((a) => Math.round(res.axes[a.id] * 100)).join('-')}_s${state.seed}.svg`;
    download(name, svgDocument(pathData(res.glyph), { label: 'S' }));
  };

  $('exportSheet').onclick = () => download('S_contact_sheet.svg', sheetSvg());

  $('editTarget').onchange = (e) => {
    state.editIndex = +e.target.value;
    const skin = getSkin(state.skinId);
    // Jump the sliders to the keyframe you are editing, so leaving edit mode
    // leaves you looking at the position you just worked on.
    if (state.editIndex >= 0) {
      const kf = skin.keyframes[state.editIndex];
      AXES.forEach((a, i) => {
        state.sliders[a.id] = kf.at[i];
        $(`ax-${a.id}`).value = kf.at[i] * 100;
      });
      document.querySelectorAll('.views button').forEach((b) => b.classList.toggle('on', b.dataset.view === 'single'));
      state.view = 'single';
    }
    render();
  };

  $('resetNodes').onclick = () => {
    if (state.editIndex < 0) return;
    const skin = getSkin(state.skinId);
    resetNodes(skin.keyframes[state.editIndex].params);
    invalidateSkin(skin);
    saveEdits();
    render();
  };

  $('copySkin').onclick = async () => {
    const skin = getSkin(state.skinId);
    const body = skin.keyframes
      .map((k) => {
        const at = {};
        AXES.forEach((a, i) => {
          if (Math.abs(k.at[i] - 0.5) > 1e-6) at[a.id] = round(k.at[i], 3);
        });
        return `      { name: '${k.name}', at: ${JSON.stringify(at)}, p: ${JSON.stringify(diffParams(k.params, skin.base))} },`;
      })
      .join('\n');
    await copyOut(`    keyframes: [\n${body}\n    ],`);
  };

  $('copyKeyframe').onclick = async () => {
    const skin = getSkin(state.skinId);
    const res = generate(skin, state.sliders, opts());
    const at = {};
    for (const a of AXES) if (Math.abs(res.axes[a.id] - 0.5) > 1e-6) at[a.id] = round(res.axes[a.id], 3);
    const p = diffParams(res.params, skin.base);
    await copyOut(`{ name: 'new', at: ${JSON.stringify(at)}, p: ${JSON.stringify(p)} },`);
  };
}

const round = (v, n) => Math.round(v * 10 ** n) / 10 ** n;

// Only the parameters that differ from the skin's base, so a pasted keyframe
// stays readable instead of listing forty numbers.
function diffParams(params, base) {
  const out = {};
  for (const k of Object.keys(params)) {
    const v = params[k];
    const b = base[k];
    const same = Array.isArray(v)
      ? v.every((x, i) => Math.abs(x - b[i]) < 1e-3)
      : Math.abs(v - b) < 1e-3;
    if (!same) out[k] = Array.isArray(v) ? v.map((x) => round(x, 4)) : round(v, 3);
  }
  return out;
}

async function copyOut(text) {
  try {
    await navigator.clipboard.writeText(text);
    $('copyHint').textContent = 'Copied. Paste into src/skins.js.';
  } catch {
    $('copyHint').textContent = text;
  }
}

// ------------------------------------------------------------------ views

function renderSingle(skin, res) {
  if (state.editIndex >= 0) return renderEditor(skin);
  $('stage').innerHTML = `<div class="single">${svgMarkup(res.glyph, { debug: state.debug })}</div>`;
}

// Editing shows the keyframe itself: no seed variation, no event rules. You
// are drawing the thing the blend interpolates towards, so it has to be the
// unmodified article.
function keyframeGlyph(skin) {
  const kf = skin.keyframes[state.editIndex];
  return buildGlyph(clampParams(cloneParams(kf.params)));
}

function renderEditor(skin) {
  const kf = skin.keyframes[state.editIndex];
  let glyph = keyframeGlyph(skin);
  $('stage').innerHTML = `<div class="single editing">
    <svg viewBox="0 0 ${EM} ${EM}">
      <path class="ed-fill" fill="#141416" fill-rule="evenodd" d="${pathData(glyph)}"/>
      ${overlayMarkup(glyph)}
    </svg>
  </div>`;

  const svg = $('stage').querySelector('svg');
  const fill = svg.querySelector('.ed-fill');
  const overlay = svg.querySelector('.ed');

  attachEditor(svg, () => glyph, kf.params, (settled) => {
    clampParams(kf.params);
    glyph = keyframeGlyph(skin);
    fill.setAttribute('d', pathData(glyph));
    updateOverlay(overlay, glyph);
    if (settled) {
      invalidateSkin(skin);
      saveEdits();
      renderPanel(skin, generate(skin, state.sliders, opts()));
    }
    updateEditHint(skin, glyph);
  });
  updateEditHint(skin, glyph);
}

// The angle guard silently overrides a dragged handle when the node would
// force the spine to wobble. Saying so beats letting a handle appear to
// ignore you.
function guardedNodes(glyph) {
  const { requested } = glyph.spine.edit;
  return [0, 3, 6]
    .filter((i) => Math.abs(requested[i] - glyph.spine.anchors[i].angle) > 1)
    .map((i) => NODE_LABELS[i]);
}

function updateEditHint(skin, glyph = null) {
  if (state.editIndex < 0) {
    $('editHint').textContent = 'Pick a keyframe to drag its nodes and handles.';
    return;
  }
  const kf = skin.keyframes[state.editIndex];
  const parts = [`Editing "${kf.name}".`];
  parts.push(
    hasNodeEdits(kf.params)
      ? 'Nodes moved \u2014 Copy whole skin to keep them.'
      : 'Large dots move nodes, small ones move handles.'
  );
  const guarded = glyph ? guardedNodes(glyph) : [];
  if (guarded.length) parts.push(`Angle guard is holding: ${guarded.join(', ')}.`);
  $('editHint').textContent = parts.join(' ');
}

function buildEditTarget(skin) {
  const options = ['<option value="-1">off \u2014 show the blend</option>'];
  skin.keyframes.forEach((k, i) => {
    const mark = hasNodeEdits(k.params) ? ' \u2022' : '';
    options.push(`<option value="${i}" ${i === state.editIndex ? 'selected' : ''}>${k.name}${mark}</option>`);
  });
  $('editTarget').innerHTML = options.join('');
}

function renderVariants(skin) {
  const cells = [];
  for (let i = 0; i < 12; i++) {
    const res = generate(skin, state.sliders, opts({ seed: state.seed + i, variance: Math.max(0.25, state.variance) }));
    const check = validate(res.glyph, skin);
    const level = check.flags.some((f) => f.level === 'fail') ? 'fail' : check.flags.length ? 'warn' : '';
    cells.push(`<div class="cell ${level}">${svgMarkup(res.glyph)}<div class="tag">seed ${state.seed + i}</div></div>`);
  }
  $('stage').innerHTML = `<div class="grid" style="grid-template-columns:repeat(4,1fr);max-width:900px">${cells.join('')}</div>`;
}

function sheetPositions() {
  const n = state.levels >= 3 ? state.levels : 5;
  return { n, xs: Array.from({ length: n }, (_, i) => i / (n - 1)) };
}

function renderSheet(skin) {
  const { n, xs } = sheetPositions();
  const cells = [];
  for (let yi = 0; yi < n; yi++) {
    for (let xi = 0; xi < n; xi++) {
      const sliders = { ...state.sliders, [state.sheetX]: xs[xi], [state.sheetY]: xs[yi] };
      const res = generate(skin, sliders, opts());
      const check = validate(res.glyph, skin);
      const level = check.flags.some((f) => f.level === 'fail') ? 'fail' : check.flags.length ? 'warn' : '';
      cells.push(`<div class="cell ${level}">${svgMarkup(res.glyph)}<div class="tag">${xs[xi].toFixed(2)} / ${xs[yi].toFixed(2)}</div></div>`);
    }
  }
  const sel = (id, val) =>
    `<select id="${id}">${AXES.map((a) => `<option value="${a.id}" ${a.id === val ? 'selected' : ''}>${a.label}</option>`).join('')}</select>`;
  $('stage').innerHTML = `<div class="sheetwrap">
    <div class="sheethead">across ${sel('sheetX', state.sheetX)} &nbsp; down ${sel('sheetY', state.sheetY)} &nbsp; other axes held at their slider values</div>
    <div class="grid" style="grid-template-columns:repeat(${n},1fr)">${cells.join('')}</div>
  </div>`;
  $('sheetX').onchange = (e) => { state.sheetX = e.target.value; render(); };
  $('sheetY').onchange = (e) => { state.sheetY = e.target.value; render(); };
}

function sheetSvg() {
  const skin = getSkin(state.skinId);
  const { n, xs } = sheetPositions();
  const cell = 240;
  const gap = 12;
  const size = n * cell + (n + 1) * gap;
  let body = `<rect width="${size}" height="${size}" fill="#f2f1ec"/>`;
  for (let yi = 0; yi < n; yi++) {
    for (let xi = 0; xi < n; xi++) {
      const sliders = { ...state.sliders, [state.sheetX]: xs[xi], [state.sheetY]: xs[yi] };
      const res = generate(skin, sliders, opts());
      const tx = gap + xi * (cell + gap);
      const ty = gap + yi * (cell + gap);
      body += `<g transform="translate(${tx} ${ty}) scale(${cell / EM})"><path fill="#141416" fill-rule="evenodd" d="${pathData(res.glyph)}"/></g>`;
    }
  }
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${size} ${size}" width="${size}" height="${size}">${body}</svg>`;
}

// ------------------------------------------------------------- diagnostics

function renderPanel(skin, res) {
  const check = validate(res.glyph, skin);
  $('flags').innerHTML = check.flags.length
    ? check.flags.map((f) => `<div class="flag ${f.level}"><i></i><span>${f.text}</span></div>`).join('')
    : '<div class="flag ok"><i></i><span>clean</span></div>';

  $('rules').innerHTML = res.firedRules.length
    ? res.firedRules.map((r) => `<span class="chip">${r}</span>`).join('')
    : '<span class="chip" style="opacity:.45">none</span>';

  const maxW = Math.max(...res.weights.map((w) => Math.abs(w)), 1e-6);
  $('weights').innerHTML = skin.keyframes
    .map((k, i) => {
      const w = res.weights[i];
      const pct = (Math.abs(w) / maxW) * 100;
      const style = w < 0 ? `right:50%;width:${pct / 2}%` : `left:50%;width:${pct / 2}%`;
      return `<div class="wrow"><b>${k.name}</b><div class="bar"><i class="${w < 0 ? 'neg' : ''}" style="${style}"></i></div><span>${w.toFixed(2)}</span></div>`;
    })
    .join('');

  const show = ['weight', 'contrastMin', 'stress', 'widthScale', 'tension', 'spineAngle', 'aperture', 'bowlBalance', 'slant', 'taper', 'inline', 'spur'];
  const rows = show.map((k) => `<div><span>${k}</span><span>${round(res.params[k] ?? 0, 3)}</span></div>`);
  rows.push(`<div><span>terminals</span><span>${res.params.capA.map((v) => v.toFixed(2)).join(' ')}</span></div>`);
  rows.push(`<div><span>cuts</span><span>${res.params.cutWidth.map((v) => v.toFixed(2)).join(' ')}</span></div>`);
  rows.push(`<div><span>legibility</span><span>${check.score.toFixed(2)}</span></div>`);
  rows.push(`<div><span>ink</span><span>${(check.ink * 100).toFixed(1)}%</span></div>`);
  $('params').innerHTML = rows.join('');
}

// ------------------------------------------------------------------- loop

function render() {
  const skin = getSkin(state.skinId);
  const res = generate(skin, state.sliders, opts());

  buildEditTarget(skin);
  updateEditHint(skin);
  for (const a of AXES) $(`out-${a.id}`).textContent = res.axes[a.id].toFixed(2);
  $('seedOut').textContent = state.seed;
  $('varianceOut').textContent = state.variance.toFixed(2);
  $('sharpenOut').textContent = state.sharpen;
  $('snapOut').textContent = state.snap.toFixed(2);

  if (state.view === 'single') renderSingle(skin, res);
  else if (state.view === 'variants') renderVariants(skin);
  else renderSheet(skin);

  renderPanel(skin, res);
}

loadEdits();
buildSkinButtons();
buildAxisControls();
bindControls();
render();
