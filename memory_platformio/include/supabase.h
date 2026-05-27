#ifndef SUPABASE_H
#define SUPABASE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <game_state.h>

// ---------------------------------------------------------------------------
// Supabase project credentials.
// The anon key is intentionally public — it only allows SELECT and the
// single SECURITY DEFINER RPC. All writes are gated by that RPC.
// ---------------------------------------------------------------------------
#define SUPABASE_HOST    "https://kfigagoyhqrnlbgnaibh.supabase.co"
#define SUPABASE_ANON_KEY \
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9." \
  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtmaWdhZ295aHFybmxiZ25haWJoIiwicm9sZSI6" \
  "ImFub24iLCJpYXQiOjE3Nzc5NzkxMTEsImV4cCI6MjA5MzU1NTExMX0.AmHT_" \
  "Joc5XUc8Yq0Nwbdpcdm-RHT3TgWPU_taDd2O2M"

// ---------------------------------------------------------------------------
// Internal — attach the two required Supabase auth headers.
// ---------------------------------------------------------------------------
static void _sb_auth(HTTPClient& http) {
  http.addHeader("apikey",        SUPABASE_ANON_KEY);
  http.addHeader("Authorization", "Bearer " SUPABASE_ANON_KEY);
}

// ---------------------------------------------------------------------------
// supabase_fetch_leaderboard
//
// GET /rest/v1/players?order=rank.asc&select=*
// Fills the global leaderboard[] array and sets leaderboard_size.
// Returns true on success.
// ---------------------------------------------------------------------------
bool supabase_fetch_leaderboard() {
  WiFiClientSecure client;
  client.setInsecure();   // skip certificate verification (anon key is the auth)

  HTTPClient http;
  http.begin(client, SUPABASE_HOST "/rest/v1/players?order=rank.asc&select=*");
  _sb_auth(http);

  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return false;

  JsonArray arr = doc.as<JsonArray>();
  leaderboard_size = 0;

  for (JsonObject player : arr) {
    if (leaderboard_size >= MAX_PLAYERS) break;
    LeaderboardEntry& e = leaderboard[leaderboard_size];

    const char* n = player["name"] | "?";
    strncpy(e.name, n, sizeof(e.name) - 1);
    e.name[sizeof(e.name) - 1] = '\0';

    e.rank        = player["rank"]     | 0;
    e.is_ghost    = player["is_ghost"] | false;
    e.games_count = 0;

    JsonArray games = player["games"];
    for (JsonObject g : games) {
      if (e.games_count >= MAX_GAMES_PER_PLAYER) break;
      GamePair& gp    = e.games[e.games_count++];
      gp.maze_seed    = g["maze_seed"]  | (uint32_t)0;
      gp.maze_time    = g["maze_time"]  | (uint32_t)0;
      gp.words_seed   = g["words_seed"] | (uint32_t)0;
      gp.words_time   = g["words_time"] | (uint32_t)0;
      gp.count_seed   = g["count_seed"] | (uint32_t)0;
      gp.count_time   = g["count_time"] | (uint32_t)0;
    }

    leaderboard_size++;
  }

  return leaderboard_size > 0;
}

// ---------------------------------------------------------------------------
// supabase_insert_player
//
// POST /rest/v1/rpc/insert_player
// Calls the SECURITY DEFINER RPC that atomically shifts existing ranks and
// inserts the new entry.  Returns true on HTTP 200/201/204.
// ---------------------------------------------------------------------------
bool supabase_insert_player(const char* name, int rank,
                            const GamePair* rounds, uint8_t rounds_count,
                            bool is_ghost = false) {
  JsonDocument doc;
  doc["p_name"]     = name;
  doc["p_rank"]     = rank;
  doc["p_is_ghost"] = is_ghost;

  JsonArray games = doc["p_games"].to<JsonArray>();
  for (int i = 0; i < rounds_count; i++) {
    JsonObject g    = games.add<JsonObject>();
    g["maze_seed"]  = rounds[i].maze_seed;
    g["maze_time"]  = rounds[i].maze_time;
    g["words_seed"] = rounds[i].words_seed;
    g["words_time"] = rounds[i].words_time;
    g["count_seed"] = rounds[i].count_seed;
    g["count_time"] = rounds[i].count_time;
  }

  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, SUPABASE_HOST "/rest/v1/rpc/insert_player");
  _sb_auth(http);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);
  http.end();

  return code == 200 || code == 201 || code == 204;
}

#endif
