<script>
  import { onMount } from "svelte";
  import logo from "./assets/tot-m2_logo.svg";
  import blueprint_1 from "./assets/blueprint_1.png";
  import blueprint_2 from "./assets/blueprint_2.png";
  import htp_maze from "./assets/htp_maze.svg";
  import htp_words from "./assets/htp_word.svg";
  import htp_shape from "./assets/htp_shape.svg";
  import { subscribeLeaderboard } from "./lib/supabase.js";

  /** @type {{ name: string, rank: number, created_at: string }[]} */
  let topByRank = $state([]);
  /** @type {{ name: string, rank: number, created_at: string }[]} */
  let mostRecent = $state([]);
  /** @type {string | null} */
  let leaderboardError = $state(null);

  /** @param {string} iso */
  function formatDate(iso) {
    return new Date(iso).toLocaleString("cs-CZ", {
      // day: "numeric",
      // month: "numeric",
      hour: "2-digit",
      minute: "2-digit",
    });
  }

  onMount(() => {
    return subscribeLeaderboard(
      ({ byRank, byRecent }) => {
        topByRank = byRank;
        mostRecent = byRecent;
        leaderboardError = null;
      },
      {
        intervalMs: 15_000,
        onError: (e) => {
          leaderboardError = e.message;
        },
      },
    );
  });
</script>

<div class="logo">
  <img src={logo} alt="logo" />
</div>

<div class="anotace">
  <p>
    Pocket video-game console tot–M❷ [totem dva] is a new custom platform for
    developing tactile “mobile” games.Main feature is asynchronous multiplayer,
    allowing users to play against real players, without the need to meet up or
    play at the same time. Equipped with rotary encoder, 2 mechanical
    key-switches, low power Sharp Memory LCD display, ESP-32, usb-c, wifi,
    bluetooth, can last over 12 hours on a single battery charge.
  </p>
</div>

<div class="blueprint">
  <img src={blueprint_1} alt="blueprint_1" />
  <img src={blueprint_2} alt="blueprint_2" />
</div>

<div class="howtoplay">
  <div class="text">
    <h4>how to play</h4>
    <p>
      Compete with previous players in 3 simple minigames. When you win, you
      move up on the leaderboard and vice versa. You can play as many times as
      you want, with the risk of losing your ranking!
    </p>
  </div>
  <img src={htp_maze} alt="htp_maze" />
  <img src={htp_words} alt="htp_words" />
  <img src={htp_shape} alt="htp_shape" />
</div>

<div class="leaderboard">
  <div class="text">
    <h4>leaderboard</h4>
    <p>Tady by asi mělo být něco napsaný...</p>
  </div>
  {#if leaderboardError}
    <p class="leaderboard-error">{leaderboardError}</p>
  {:else}
    <div class="leaderboard-columns">
      <section>
        <h5>top 10</h5>
        <ol>
          {#each topByRank as player}
            <li>
              <span class="rank"
                >#{player.rank.toString().padStart(2, "0")}</span
              >
              <span class="name">{player.name}</span>
            </li>
          {/each}
        </ol>
      </section>

      <section>
        <h5>recent</h5>
        <ol>
          {#each mostRecent as player}
            <li>
              <span class="name">{player.name}</span>
              <span class="meta"
                >#{player.rank.toString().padStart(2, "0")}</span
              >
              <span class="meta">{formatDate(player.created_at)}</span>
            </li>
          {/each}
        </ol>
      </section>
    </div>
  {/if}
</div>
