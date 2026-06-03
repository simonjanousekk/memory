-- Prior migration was marked applied before DROP ran; PostgREST still sees two overloads.

DROP FUNCTION IF EXISTS public.insert_player(text, integer, jsonb, boolean);
DROP FUNCTION IF EXISTS public.insert_player(text, integer, boolean, jsonb);

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

NOTIFY pgrst, 'reload schema';
