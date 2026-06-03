#!/usr/bin/env bash
# Renumber players.rank to 1..N (removes gaps like 1,5,8,9 → 1,2,3,4).
#
# Prerequisites: Supabase CLI logged in and project linked from repo root:
#   cd /path/to/memory
#   supabase login
#   supabase link --project-ref kfigagoyhqrnlbgnaibh
#
# Usage:
#   ./supabase/scripts/renumber_ranks.sh
#   ./supabase/scripts/renumber_ranks.sh --dry-run   # show ranks only, no update

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="$(cd "$ROOT/.." && pwd)"
SQL_FILE="$ROOT/scripts/renumber_ranks.sql"
DRY_RUN=false

for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=true ;;
    -h|--help)
      echo "Usage: $0 [--dry-run]"
      exit 0
      ;;
  esac
done

cd "$REPO"

if ! command -v supabase >/dev/null 2>&1; then
  echo "Error: supabase CLI not found. Install: https://supabase.com/docs/guides/cli" >&2
  exit 1
fi

_show_ranks() {
  supabase db execute --sql "SELECT rank, name, created_at FROM public.players ORDER BY rank ASC, created_at ASC;"
}

echo "=== Ranks before ==="
_show_ranks

if $DRY_RUN; then
  echo ""
  echo "Dry run — no changes. Run without --dry-run to renumber."
  exit 0
fi

echo ""
echo "=== Renumbering ==="
if [ -f "$SQL_FILE" ]; then
  supabase db execute --file "$SQL_FILE"
else
  supabase db execute --sql "SELECT public.renumber_players();"
fi

echo ""
echo "=== Ranks after ==="
_show_ranks

echo "Done."
