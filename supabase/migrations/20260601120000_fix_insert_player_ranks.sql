-- Duplicate insert_player overloads break PostgREST RPC (PGRST203 ambiguous function).
-- Drop every known signature, then create exactly one canonical function.

DROP FUNCTION IF EXISTS public.insert_player(text, integer, jsonb, boolean);
DROP FUNCTION IF EXISTS public.insert_player(text, integer, boolean, jsonb);

CREATE OR REPLACE FUNCTION public.renumber_players()
RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = public
AS $$
BEGIN
  WITH ordered AS (
    SELECT id, ROW_NUMBER() OVER (ORDER BY rank ASC, created_at ASC) AS new_rank
    FROM players
  )
  UPDATE players AS p
  SET rank = ordered.new_rank
  FROM ordered
  WHERE p.id = ordered.id;
END;
$$;

CREATE OR REPLACE FUNCTION public.insert_player(
  p_name text,
  p_rank integer,
  p_is_ghost boolean,
  p_games jsonb
)
RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = public
AS $$
DECLARE
  v_name text;
  v_rank integer;
  v_max_rank integer;
BEGIN
  v_name := NULLIF(btrim(p_name), '');
  IF v_name IS NULL THEN
    v_name := '?';
  END IF;

  SELECT COALESCE(MAX(rank), 0) INTO v_max_rank FROM players;
  v_rank := GREATEST(1, LEAST(COALESCE(NULLIF(p_rank, 0), 1), v_max_rank + 1));

  PERFORM pg_advisory_xact_lock(hashtext('players_rank_insert'));

  UPDATE players
  SET rank = rank + 1
  WHERE rank >= v_rank;

  INSERT INTO players (name, rank, games, is_ghost)
  VALUES (
    v_name,
    v_rank,
    COALESCE(p_games, '[]'::jsonb),
    COALESCE(p_is_ghost, false)
  );
END;
$$;

GRANT EXECUTE ON FUNCTION public.insert_player(text, integer, boolean, jsonb) TO anon, authenticated, service_role;

-- Remove broken rows and compact ranks.
DELETE FROM players WHERE btrim(name) = '' AND games = '[]'::jsonb;

SELECT public.renumber_players();
