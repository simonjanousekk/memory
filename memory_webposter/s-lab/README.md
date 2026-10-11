# S-Lab

A prototype for a data-driven S. Five sliders address a position in a 5D space;
the space is filled by a sparse set of art-directed keyframes, and the output is
a static SVG logo.

No dependencies, no build step. Run any static server from this folder:

```
python3 -m http.server 4321
```

then open `http://localhost:4321`.

This folder is self-contained and unrelated to the Svelte app it currently sits
inside. Move it wherever you like.

## The idea

The original problem was that interpolating between two beautiful S's gives you
a middle that is the average of two personalities and has neither. Three things
address that here, and they stack.

**1. A keyframe is a parameter set, not a drawing.** The S is built by a
generator: a spine of seven anchors, a stroke model that decides how much ink
sits on the spine at each point, and terminals. Every point in parameter space
produces a well-formed letter, so a blend of two keyframes is a valid S rather
than a smear of two outlines. You still art-direct specific slider positions,
you just art-direct them as settings.

**2. The space is sparse and scattered, not a lattice.** Keyframes sit wherever
you put them: one at the centre, one at each end of each axis, and *correctives*
at the corners where a plain blend would collide with itself. Grotesk has 15 for
5 axes. That is the pose-space deformation trick from character animation, and
it is what makes 5 dimensions authorable at all — 5 levels per axis would be
3125 cells you would never fill.

**3. Expressive moments are rules, not interpolation.** Past a threshold, a
stencil break opens, terminals become spurs, an inline appears. These fire after
the blend, so crossing the threshold is a real event rather than something the
blend smooths into mush. Each skin has its own; Brut breaks early and often,
Didone never breaks (it would ruin it) and swells its ball terminals instead.

Because the output is static, sharp behaviour is free: **Snap** mixes the result
toward the single nearest keyframe, so a slider position can land on a designed
shape rather than near one. A live-animating logo could not do this.

## Interface

- **Sliders** — the five axes the client's data will drive. `Steps` quantises
  them; at 5 per axis the whole space is 3125 positions per skin, which is
  reviewable.
- **Variation** — seed and amount. Same slider position, different S. Beyond
  wobbling the numbers this makes discrete picks: a different terminal family, a
  break that appears, an inline that opens. This is where "many outputs per
  position" comes from.
- **Blend** — `RBF` interpolates exactly through your keyframes and may
  overshoot between them, which usually reads as extra punch and is made safe by
  clamping. `Shepard` never overshoots but only reproduces a keyframe when
  sharpened hard. `Domain` switches between blending parameters and blending
  outlines; outline mode is there so you can see the mush problem directly.
- **Views** — Single, Variants (12 seeds at one position), Contact sheet (the
  lattice across two chosen axes, other axes held where the sliders are).
- **Checks** — automatic rejection rules: hairline floor, weight exceeding the
  curve radius, terminal colliding with the bowl, counters filling in, and a
  low-res raster correlation against the skin's own keyframes as a "does it
  still read as S" test. Flagged cells are outlined in the grids.

## Editing a keyframe's spine

Pick a keyframe in the **Spine** section and the stage switches to that
keyframe on its own — no seed variation, no event rules, since you are drawing
the thing the blend interpolates towards. Drag the large red dots to move
nodes and the small ones to swing handles. The sliders jump to that keyframe's
position, so leaving edit mode leaves you looking at what you just worked on.

Node edits are stored as **offsets**, not absolute positions. Aperture, width,
slant and the rest still drive the seven anchors; your edit rides on top of the
result. Move the width slider afterwards and the edited node travels with the
letter instead of being stranded. It also means a node edit blends like every
other parameter: an edit to `heavy` fades out as you move away from it.

Two things to expect:

- The angle guard still runs after your edit. If you swing a handle on the
  junction or a terminal past what the fixed anchors allow, it gets held back
  and the hint tells you which node is being held. Releasing it would put the
  wobble back.
- Edits are kept in `localStorage` so a reload does not lose an afternoon, but
  that is a safety net, not storage. **Copy whole skin** emits the entire
  `keyframes` array to paste into `src/skins.js`, which is where they become
  real. `Reset this keyframe's nodes` undoes them.

## Working loop

1. Move the sliders until you find something you want.
2. **Copy as keyframe** puts a keyframe object on the clipboard, containing the
   slider position and only the parameters that differ from the skin's base.
3. Paste it into that skin's `keyframes` array in `src/skins.js`.
4. That position now reproduces exactly, and it pulls its neighbourhood toward
   itself.
5. Select it in **Spine** and drag its nodes to draw it properly.

Fix the system, not the individual output. If one cell of the contact sheet is
wrong, add a corrective keyframe there rather than special-casing it.

## Files

| file | what it does |
| --- | --- |
| `src/skeleton.js` | the spine: seven anchors, per-axis handle lengths, angle resolution |
| `src/editor.js` | dragging nodes and handles, written back as per-keyframe offsets |
| `src/stroke.js` | broad-nib width model and offsetting |
| `src/terminals.js` | cut / round / ball / spur, all emitting the same point count |
| `src/glyph.js` | assembles contours, stencil bands, inline, fit to the em box |
| `src/skins.js` | **the design content** — base, keyframes and event rules per skin |
| `src/blend.js` | RBF and Shepard weights in 5D, snapping |
| `src/generate.js` | blend → seed variation → events → glyph |
| `src/validate.js` | the automatic checks |
| `src/fit.js` | least-squares cubic fitting for clean exported nodes |

## Geometry notes

Two constraints do most of the work in keeping arbitrary parameter
combinations from producing garbage, and both are worth knowing before you
change the skeleton:

- A cubic's chord direction is a weighted average of its tangent directions, so
  a chord falling outside the range spanned by its two end tangents forces the
  curve to overshoot and wobble. The junction and the two terminals absorb this:
  their angles are clamped to whatever the fixed anchors allow. A steeper spine
  is only geometrically available if the bowl extremes move apart vertically, so
  those are coupled.
- Stencil breaks are built from the stroke's own edge points rather than laid
  over the glyph as rectangles. A rectangle overhanging the outline turns back
  into ink under an even-odd fill — a slash across the letter instead of a break
  in it.

## Where this is not finished

- Three skins exist and none is the client's. A real skin is a couple of days of
  keyframe work once the axes are agreed.
- The five axes are typographic placeholders (weight, contrast, width, tension,
  energy). They should be chosen from the client's actual data, ideally after
  looking at what the reference S's vary along — that is the good use of the
  scraping idea: analysis, to choose axes, not retrieval to fill them.
- Batch export writes one SVG or one sheet. Exporting the full quantised lattice
  as files is a small addition once the axes are fixed.
- Outline-domain blending skips the event rules, since events act on parameters.
