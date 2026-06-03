-- Compact players.rank to 1..N with no gaps.
-- Order: current rank ascending, then created_at (ties / same rank stay stable).

WITH ordered AS (
  SELECT id, ROW_NUMBER() OVER (ORDER BY rank ASC, created_at ASC) AS new_rank
  FROM public.players
)
UPDATE public.players AS p
SET rank = ordered.new_rank
FROM ordered
WHERE p.id = ordered.id;
