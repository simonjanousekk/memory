// Anon key is public — RLS allows SELECT only; writes go through insert_player RPC.
const SUPABASE_URL =
  "https://kfigagoyhqrnlbgnaibh.supabase.co";
const SUPABASE_ANON_KEY =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtmaWdhZ295aHFybmxiZ25haWJoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Nzc5NzkxMTEsImV4cCI6MjA5MzU1NTExMX0.AmHT_Joc5XUc8Yq0Nwbdpcdm-RHT3TgWPU_taDd2O2M";

const authHeaders = {
  apikey: SUPABASE_ANON_KEY,
  Authorization: `Bearer ${SUPABASE_ANON_KEY}`,
};

async function queryPlayers(order, limit = 10) {
  const params = new URLSearchParams({
    select: "name,rank,created_at",
    order,
    limit: String(limit),
  });
  const res = await fetch(`${SUPABASE_URL}/rest/v1/players?${params}`, {
    headers: authHeaders,
  });
  if (!res.ok) {
    throw new Error(`players query failed: ${res.status}`);
  }
  return res.json();
}

/** Top N players by rank (best first). */
export function fetchTopByRank(limit = 10) {
  return queryPlayers("rank.asc", limit);
}

/** N most recently added players. */
export function fetchMostRecent(limit = 10) {
  return queryPlayers("created_at.desc", limit);
}

/** @typedef {{ name: string, rank: number, created_at: string }} PlayerRow */

/** @param {number} [limit] */
export async function fetchLeaderboard(limit = 10) {
  const [byRank, byRecent] = await Promise.all([
    fetchTopByRank(limit),
    fetchMostRecent(limit),
  ]);
  return { byRank, byRecent };
}

function leaderboardSnapshot(/** @type {{ byRank: PlayerRow[], byRecent: PlayerRow[] }} */ data) {
  return JSON.stringify(data);
}

/**
 * Poll Supabase; call onUpdate only when top-10 / recent lists actually changed.
 *
 * @param {(data: { byRank: PlayerRow[], byRecent: PlayerRow[] }) => void} onUpdate
 * @param {{ intervalMs?: number, limit?: number, onError?: (err: Error) => void }} [opts]
 * @returns {() => void} stop polling (call from onMount cleanup)
 */
export function subscribeLeaderboard(onUpdate, opts = {}) {
  const intervalMs = opts.intervalMs ?? 15_000;
  const limit = opts.limit ?? 10;
  let last = "";

  const tick = async () => {
    try {
      const data = await fetchLeaderboard(limit);
      const next = leaderboardSnapshot(data);
      if (next === last) return;
      last = next;
      onUpdate(data);
    } catch (e) {
      if (!last) {
        opts.onError?.(e instanceof Error ? e : new Error("load failed"));
      }
    }
  };

  tick();
  const timer = setInterval(tick, intervalMs);
  return () => clearInterval(timer);
}
