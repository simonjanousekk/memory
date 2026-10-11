import { buildGlyph } from '../src/glyph.js';
import { getSkin } from '../src/skins.js';
import { baseParams } from '../src/params.js';

const skin = getSkin('grotesk');
const g = buildGlyph(baseParams(skin.base));
const { samples } = g.spine;
const p = g.profile;
console.log('weight(half):', skin.base.weight);
console.log('minHalf', p.minHalf.toFixed(1), 'maxHalf', p.maxHalf.toFixed(1));
console.log('clampedFraction', p.clampedFraction.toFixed(3), 'clampLoss', p.clampLoss.toFixed(3));
let out = [];
for (let i = 0; i < samples.length; i += 10) {
  const k = Math.abs(samples[i].k);
  out.push(`${(i/(samples.length-1)).toFixed(2)} R=${k>1e-9?(1/k).toFixed(0):'inf'} h=${p.half[i].toFixed(0)}`);
}
console.log(out.join('\n'));
