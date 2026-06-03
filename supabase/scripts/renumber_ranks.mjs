#!/usr/bin/env node
/**
 * Renumber players.rank to 1..N via Supabase REST RPC (no SQL file needed).
 *
 * Env (from Dashboard → Settings → API):
 *   SUPABASE_URL=https://xxxx.supabase.co
 *   SUPABASE_SERVICE_ROLE_KEY=eyJ...   (secret — never commit)
 *
 * Usage:
 *   node supabase/scripts/renumber_ranks.mjs
 *   node supabase/scripts/renumber_ranks.mjs --dry-run
 */

const dryRun = process.argv.includes("--dry-run");

const url = process.env.SUPABASE_URL;
const key = process.env.SUPABASE_SERVICE_ROLE_KEY;

if (!url || !key) {
  console.error("Set SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY");
  process.exit(1);
}

const headers = {
  apikey: key,
  Authorization: `Bearer ${key}`,
  "Content-Type": "application/json",
  Prefer: "return=representation",
};

async function fetchPlayers() {
  const res = await fetch(
    `${url}/rest/v1/players?select=id,rank,name,created_at&order=rank.asc,created_at.asc`,
    { headers },
  );
  if (!res.ok) throw new Error(`GET players: ${res.status} ${await res.text()}`);
  return res.json();
}

async function renumberViaRpc() {
  const res = await fetch(`${url}/rest/v1/rpc/renumber_players`, {
    method: "POST",
    headers,
    body: "{}",
  });
  if (!res.ok) {
    const body = await res.text();
    throw new Error(
      `RPC renumber_players failed (${res.status}).\n${body}\n\n` +
        "Run SQL in Dashboard instead: supabase/scripts/renumber_ranks.sql\n" +
        "Or: ./supabase/scripts/renumber_ranks.sh",
    );
  }
}

async function renumberViaPatch(players) {
  const sorted = [...players].sort((a, b) => {
    if (a.rank !== b.rank) return a.rank - b.rank;
    return new Date(a.created_at) - new Date(b.created_at);
  });

  for (let i = 0; i < sorted.length; i++) {
    const want = i + 1;
    if (sorted[i].rank === want) continue;
    const res = await fetch(
      `${url}/rest/v1/players?id=eq.${sorted[i].id}`,
      {
        method: "PATCH",
        headers,
        body: JSON.stringify({ rank: want }),
      },
    );
    if (!res.ok) throw new Error(`PATCH id=${sorted[i].id}: ${res.status} ${await res.text()}`);
  }
}

function printRanks(label, players) {
  console.log(`\n=== ${label} ===`);
  for (const p of players) {
    console.log(`${String(p.rank).padStart(4)}  ${p.name}`);
  }
}

const players = await fetchPlayers();
printRanks("Ranks before", players);

if (dryRun) {
  console.log("\nDry run — no changes.");
  process.exit(0);
}

try {
  await renumberViaRpc();
} catch (e) {
  console.warn(String(e.message).split("\n")[0]);
  console.warn("Falling back to per-row PATCH…");
  await renumberViaPatch(players);
}

const after = await fetchPlayers();
printRanks("Ranks after", after);
console.log("\nDone.");
