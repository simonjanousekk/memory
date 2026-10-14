# tot–M❷ (memory)

A pocket game console built for an UMPRUM atelier project. It runs on an ESP32-S3 with a Sharp Memory LCD (400×240, 1-bit), a rotary encoder, two mechanical keyswitches and a MAX17048 fuel gauge. Players face "asynchronous opponents" (earlier players' recorded times) in short minigames, and results go to a Supabase leaderboard that an exhibition web poster displays.

Code comments and docs are in English. Some folder and file names are Czech: `korekce` = correction, `stojanek` = stand, `tisk` = print, `bezorez` = no cut-off.

## Layout

| Path | What | Editable by Claude? |
|---|---|---|
| `memory_platformio/` | Firmware (PlatformIO, Arduino, C++). See [memory_platformio/CLAUDE.md](memory_platformio/CLAUDE.md) | Yes, main code area |
| `memory_webposter/` | Exhibition web poster (Svelte 5 + Vite) showing the live leaderboard | Yes |
| `memory_webposter/s-lab/` | Separate, unrelated vanilla-JS prototype (parametric "S" logo generator). No build step | Yes |
| `supabase/` | Edge function `insert-player`, migrations, rank scripts. See `supabase/README.md` | Yes, but deploying is up to the user |
| `sprite_convertor/` | `script.py`: GIF → 1-bit C header. `enviroment/` is a local Python venv (misspelled on purpose, don't rename) | Yes |
| `memory_pcb/` | KiCad 8 PCB project, custom `new_lib` (JLC2KiCadLib), JLCPCB fab outputs | **Read-only.** Edit in KiCad |
| `case_export/` | STL versions of the 3D-printed case. Numbered folders = iterations, highest = latest | Binary, don't edit |
| `stojanek_export/` | DXF sketches for the display stand | Don't edit |
| `glass_drawings/` | Glass cover drawings and print masks (Affinity `.af` + PDFs) | Don't edit |
| `imgs/` | Reference photos | — |

## Commands

```bash
# Firmware (from memory_platformio/)
pio run                       # build
pio run -t upload             # flash (device connected over USB CDC)
pio device monitor            # serial at 115200
pio run -t compiledb          # regenerate compile_commands.json for clangd
pio run -e native && .pio/build/native/program   # simulator in an SDL window, no upload (sim/README.md)

# Web poster (from memory_webposter/)
npm install && npm run dev    # dev server
npm run build                 # production build into dist/

# S-Lab (from memory_webposter/s-lab/)
python3 -m http.server 4321

# Sprites (from repo root)
sprite_convertor/enviroment/bin/python sprite_convertor/script.py
#   converts every GIF in sprite_convertor/sprites/ to a .h next to it;
#   copy the result into memory_platformio/include/sprites/

# Supabase (from repo root; linked project kfigagoyhqrnlbgnaibh)
supabase db push
supabase functions deploy insert-player
```

`pio` is not on PATH in Claude's shell, so use `~/.platformio/penv/bin/pio`. KiCad CLI (also not on PATH): `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`.

## Cross-cutting rules

- **Name sanitizing lives in two places:** `memory_webposter/src/lib/sanitizeName.js` and `supabase/functions/_shared/sanitizeName.ts`. Change both together.
- **Leaderboard schema:** the firmware (`include/supabase.h`), the web poster (`src/lib/supabase.js`) and the edge function all depend on the `players` table and the `insert_player` RPC. A schema change has to update all three.
- The Supabase **anon key** in the firmware and web code is meant to be public (protected by RLS). The service-role key must never be committed.
- **Wi-Fi credentials** live in `memory_platformio/include/wifi/wifi_credentials.h`, which is gitignored. `wifi_credentials.example.h` is the template. Never print or commit the real values.
- Ask before running `supabase db push`, `supabase functions deploy` or `pio run -t upload`. They touch the live leaderboard or real hardware.

## Files to avoid reading in full

These are generated or binary, and huge. Grep them or read only the head:
- `memory_platformio/include/sprites/*.h` (`logo_animation_sprites.h` is ~26k lines) and `include/fonts/*.h`
- `memory_platformio/compile_commands.json`, `.pio/`, `memory_webposter/node_modules/`
- `memory_pcb/*.kicad_pcb`, `*.step`, `fp-info-cache`, `memory_pcb-backups/`
- Anything in `case_export/`, `*.stl`, `*.dxf`, `*.pdf`, `*.af`, `*.mp4`
