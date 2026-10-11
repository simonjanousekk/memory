// Contours -> SVG path data. All subpaths live in one path with
// fill-rule="evenodd", so the inline and the stencil cuts knock out of the
// stroke without needing boolean operations.

import { fitClosed } from './fit.js';
import { EM } from './skeleton.js';

const f = (v) => (Math.round(v * 100) / 100).toString();

const MIN_AREA = 4;

function polyPath(points) {
  let d = `M${f(points[0].x)} ${f(points[0].y)}`;
  for (let i = 1; i < points.length; i++) d += `L${f(points[i].x)} ${f(points[i].y)}`;
  return d + 'Z';
}

function bezierPath(points, corners, tol) {
  const segs = fitClosed(points, corners, tol);
  if (segs.length === 0) return '';
  let d = `M${f(segs[0][0].x)} ${f(segs[0][0].y)}`;
  for (const [, c1, c2, p3] of segs) {
    d += `C${f(c1.x)} ${f(c1.y)} ${f(c2.x)} ${f(c2.y)} ${f(p3.x)} ${f(p3.y)}`;
  }
  return d + 'Z';
}

export function pathData(glyph, opts = {}) {
  const { curves = true, tol = 0.7 } = opts;
  const parts = [];
  for (const c of glyph.contours) {
    const area = c.area ?? Infinity;
    if (area < MIN_AREA) continue;
    if (c.hard || !curves) parts.push(polyPath(c.points));
    else parts.push(bezierPath(c.points, c.corners, tol));
  }
  return parts.join('');
}

export function svgDocument(d, opts = {}) {
  const { fg = '#111111', bg = 'none', size = EM, label = 'S' } = opts;
  const bgRect = bg === 'none' ? '' : `<rect width="${size}" height="${size}" fill="${bg}"/>`;
  return `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${EM} ${EM}" width="${size}" height="${size}">
<title>${label}</title>
${bgRect}<path fill="${fg}" fill-rule="evenodd" d="${d}"/>
</svg>`;
}

export function download(filename, text, mime = 'image/svg+xml') {
  const blob = new Blob([text], { type: mime });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
