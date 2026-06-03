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

static void _sb_timeouts(HTTPClient& http) {
  http.setConnectTimeout(10000);
  http.setTimeout(20000);
}

static bool _leaderboard_valid = false;

bool supabase_leaderboard_valid() { return _leaderboard_valid; }

// Mid-board start rank; only call after a successful fetch (or valid cached data).
int supabase_default_start_rank() {
  if (!_leaderboard_valid || leaderboard_size <= 0) return 1;
  return leaderboard_size / 2 + 1;
}

static bool _fetch_leaderboard_once() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, SUPABASE_HOST "/rest/v1/players?order=rank.asc&select=*");
  _sb_auth(http);
  _sb_timeouts(http);

  int code = http.GET();
  if (code != 200) {
    Serial.printf("[fetch] HTTP %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  if (payload.isEmpty()) {
    Serial.println("[fetch] empty body");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[fetch] JSON %s (%u bytes)\n", err.c_str(), (unsigned)payload.length());
    return false;
  }

  if (!doc.is<JsonArray>()) {
    Serial.println("[fetch] not a JSON array");
    return false;
  }

  JsonArray arr = doc.as<JsonArray>();
  int parsed = 0;

  for (JsonObject player : arr) {
    if (parsed >= MAX_PLAYERS) break;
    LeaderboardEntry& e = leaderboard[parsed];

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

    parsed++;
  }

  leaderboard_size = parsed;
  _leaderboard_valid = true;
  Serial.printf("[fetch] ok players=%d\n", leaderboard_size);
  return true;
}

// ---------------------------------------------------------------------------
// supabase_fetch_leaderboard
//
// GET /rest/v1/players?order=rank.asc&select=*
// Retries on failure; keeps the last good leaderboard if a refresh fails.
// ---------------------------------------------------------------------------
bool supabase_fetch_leaderboard() {
  static constexpr int kRetries = 3;

  for (int attempt = 1; attempt <= kRetries; attempt++) {
    if (_fetch_leaderboard_once()) return true;
    if (attempt < kRetries) {
      Serial.printf("[fetch] retry %d/%d\n", attempt, kRetries);
      delay(600);
    }
  }

  if (_leaderboard_valid) {
    Serial.printf("[fetch] refresh failed, keeping %d players\n", leaderboard_size);
    return true;
  }

  Serial.println("[fetch] failed, no cached leaderboard");
  return false;
}

// ---------------------------------------------------------------------------
// supabase_insert_player
//
// POST /functions/v1/insert-player
// Edge Function sanitizes the name, then calls insert_player RPC server-side.
// Returns true on HTTP 200/201/204.
// ---------------------------------------------------------------------------
bool supabase_insert_player(const char* name, int rank,
                            const GamePair* rounds, uint8_t rounds_count,
                            bool is_ghost = false) {
  if (!name || name[0] == '\0') {
    Serial.println("[upload] aborted: empty name");
    return false;
  }
  if (rounds_count == 0) {
    Serial.println("[upload] aborted: no rounds to save");
    return false;
  }

  // ~80 bytes per round + overhead; avoid heap JsonDocument overallocation failures.
  const size_t cap = 192 + (size_t)rounds_count * 96;
  JsonDocument doc;
  doc.to<JsonObject>();

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
  payload.reserve(cap);
  size_t len = serializeJson(doc, payload);
  if (len < 10) {
    Serial.printf("[upload] aborted: JSON too short (%u)\n", (unsigned)len);
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, SUPABASE_HOST "/functions/v1/insert-player");
  _sb_auth(http);
  _sb_timeouts(http);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);
  // 204 No Content — do not call getString(); it can block forever on ESP32.
  if (code != 200 && code != 201 && code != 204) {
    String response = http.getString();
    Serial.printf("[upload] HTTP %d: %s\n", code, response.c_str());
    http.end();
    return false;
  }
  http.end();
  Serial.printf("[upload] ok HTTP %d\n", code);
  return true;
}

#endif
