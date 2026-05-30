import { Profanease } from "profanease";
import { categorized as en } from "profanease/langs/en";
import csWords from "profanease/langs/cs";

/** Match Czech terms inside nicknames (e.g. kurva123). */
const csPatterns = csWords.flatMap((word) =>
  word.includes(" ") ? [word] : [`*${word}*`],
);

const filter = new Profanease({
  languages: [en],
  list: csPatterns,
  normalize: "aggressive",
  replacement: "asterisk",
});

/** @param {string} text */
function collapseRepeatedChars(text) {
  return text.replace(/(.)\1+/gi, "$1");
}

/** @param {string | null | undefined} name */
export function sanitizeName(name) {
  if (!name) return "";

  const cleaned = filter.clean(name);
  if (cleaned !== name) return cleaned;

  const collapsed = collapseRepeatedChars(name);
  if (collapsed === name) return name;

  const collapsedCleaned = filter.clean(collapsed);
  if (collapsedCleaned === collapsed) return name;

  const { matches } = filter.analyze(collapsed);
  const spansWholeName = matches.some(
    (match) =>
      match.index === 0 && match.original.length === collapsed.length,
  );

  if (spansWholeName) {
    return "*".repeat(name.length);
  }

  return name;
}
