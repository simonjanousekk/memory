import { Profanease } from "npm:profanease@2";
import { categorized as en } from "npm:profanease@2/langs/en";
import csWords from "npm:profanease@2/langs/cs";

/** Keep in sync with memory_webposter/src/lib/sanitizeName.js */

const csPatterns = csWords.flatMap((word: string) =>
  word.includes(" ") ? [word] : [`*${word}*`],
);

const filter = new Profanease({
  languages: [en],
  list: csPatterns,
  normalize: "aggressive",
  replacement: "asterisk",
});

function collapseRepeatedChars(text: string): string {
  return text.replace(/(.)\1+/gi, "$1");
}

export function sanitizeName(name: string | null | undefined): string {
  if (!name) return "";

  const trimmed = name.trim();
  if (!trimmed) return "";

  const cleaned = filter.clean(trimmed);
  if (cleaned !== trimmed) return cleaned.trim();

  const collapsed = collapseRepeatedChars(trimmed);
  if (collapsed === trimmed) return trimmed;

  const collapsedCleaned = filter.clean(collapsed);
  if (collapsedCleaned === collapsed) return trimmed;

  const { matches } = filter.analyze(collapsed);
  const spansWholeName = matches.some(
    (match) =>
      match.index === 0 && match.original.length === collapsed.length,
  );

  if (spansWholeName) {
    return "*".repeat(trimmed.length);
  }

  return trimmed;
}

/** Censored name for DB insert — never empty when the device sent non-whitespace text. */
export function censorPlayerName(name: string | null | undefined): string {
  const trimmed = (name ?? "").trim();
  if (!trimmed) return "";

  const censored = sanitizeName(trimmed).trim();
  if (censored) return censored;

  return "*".repeat(trimmed.length);
}
