# Supabase — player name sanitization

Player names are sanitized **before insert** by the `insert-player` Edge Function, using the same `profanease` rules as the web poster (English + Czech).

## Flow

```
ESP32  →  POST /functions/v1/insert-player  →  sanitize name  →  insert_player RPC  →  players table
Web    →  GET  /rest/v1/players             →  (already clean; client still sanitizes as backup)
```

## Fix broken inserts (required)

If uploads fail or rows appear with empty `name` / `games`, the database likely has **two** `insert_player` overloads (PostgREST error PGRST203). Apply the migration and redeploy:

```bash
supabase db push
supabase functions deploy insert-player
```

The migration drops all overloads, recreates a single `insert_player`, deletes empty broken rows, and renumbers ranks. Profane names are censored to asterisks on insert, not rejected.

## Deploy (one-time)

1. Install the [Supabase CLI](https://supabase.com/docs/guides/cli).

2. Log in and link the project:

```bash
cd /path/to/memory
supabase login
supabase link --project-ref kfigagoyhqrnlbgnaibh
```

3. Deploy the function:

```bash
supabase functions deploy insert-player
```

`SUPABASE_URL` and `SUPABASE_SERVICE_ROLE_KEY` are injected automatically at runtime.

4. Flash the updated firmware (upload URL changed in `memory_platformio/include/supabase.h`).

## Optional: block direct RPC bypass

If `insert_player` was previously callable with the anon key, revoke it so uploads must go through the Edge Function:

```sql
REVOKE EXECUTE ON FUNCTION public.insert_player(text, integer, boolean, jsonb)
  FROM anon, authenticated;
```

Adjust the argument types to match your existing function signature (check in Supabase → Database → Functions).

## Renumber ranks (remove gaps)

If ranks look like `1, 5, 8, 9` instead of `1, 2, 3, 4`:

**SQL Editor** (Dashboard → SQL → New query), paste and run:

`supabase/scripts/renumber_ranks.sql`

**CLI** (linked project):

```bash
./supabase/scripts/renumber_ranks.sh           # renumber
./supabase/scripts/renumber_ranks.sh --dry-run # preview only
```

**Node** (service role key in env):

```bash
export SUPABASE_URL="https://kfigagoyhqrnlbgnaibh.supabase.co"
export SUPABASE_SERVICE_ROLE_KEY="your-service-role-key"
node supabase/scripts/renumber_ranks.mjs
```

Order is preserved: sort by current `rank`, then `created_at`.

## Keep filters in sync

`sanitizeName` lives in two places — update both when changing rules:

- `memory_webposter/src/lib/sanitizeName.js` (web display)
- `supabase/functions/_shared/sanitizeName.ts` (server insert)
