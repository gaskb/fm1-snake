# FM-1 Arcade (`fm1-snake`)

8-in-1 Arcade firmware for the **M-VAVE FM-1** pocket synthesizer, built on [Dinghy](https://github.com/zvenson/dinghy) (Felucca / SLOOP platform).

## Games Included

1. **Snake** — 19×17 grid, directional eyes, progressive speed, chiptune SFX, persistent high score.
2. **Tetris** — Classic 10×20 board, all 7 tetrominoes with wall kicks, NEXT preview, **Korobeiniki 2-voice chiptune BGM** (`FX` toggles music), hard drop (`F` / `REC`).
3. **Arkanoid** — 5×9 colored bricks, smooth Q8 fixed-point physics, **rotary paddle control via `KNOB1` / `SELECT`**, 3 lives, level progression.
4. **OutRun** — Pseudo-3D raster road racer with sunset parallax, palm trees, traffic car, checkpoints, and **Magical Sound Shower / Passing Breeze / Splash Wave** 4-voice FM + drums BGM (`FX` cycles radio).
5. **Synth Hero** — Guitar Hero-style rhythm game synced to the OutRun MIDI tracks! **BASE** mode (7 white keys `C..B`) & **HIGH** mode (12 chromatic keys including `#`/`b`).
6. **Frogger** — Classic 13-row road & river crossing with 5 home lilypads.
7. **Galaga** — Scrolling starfield, swaying alien formation, dive-bombers, twin-laser starfighter.
8. **Pac-Man** — 19×19 maze, dots, blinking power pellets, 4 colored ghosts.

All high scores are saved to flash (`STORE_MAGIC = "SNK1"`). Press **`HOME`** from any game to return to the Arcade menu.

## Controls

| Context | Input | Action |
| --- | --- | --- |
| **Menu** | `KNOB1` / `SELECT` / `C#` / `D` | Select game |
| **Menu** | `PLAY` / `E` / `F` | Launch game |
| **Snake** | `C` / `D` / `E` / `C#`–`D#` | Left / Down / Right / Up |
| **Snake** | `KNOB1` / `SELECT` | Turn left / right (90°) |
| **Tetris** | `C` / `E` / `D` | Move Left / Right / Soft Drop |
| **Tetris** | `C#`–`D#` / `KNOB1` | Rotate |
| **Tetris** | `F` / `REC` | Hard Drop |
| **Tetris** | `FX` | Toggle BGM (Korobeiniki) |
| **Arkanoid** | `KNOB1` / `SELECT` (or `C`/`E`) | Move paddle |
| **Arkanoid** | `PLAY` / `C#` / `F` | Launch ball / Pause |
| **Any game** | `HOME` | Return to Arcade menu |

## Build

Requires Docker (on macOS/Windows) or Linux x86-64, the JieLi toolchain (`./tools/get_toolchain.sh`), and `fw-AC79_AIoT_SDK`:

```sh
JIELI_TOOLCHAIN="$HOME/.jieli/toolchain" AC79_SDK="$HOME/fw-AC79_AIoT_SDK" ./build.sh --release 0.3
```

Output package: `build/fm1-snake-0.3.fwsc` (identity `FM-1_963`).

## Credits & License

- Built on [Dinghy](https://github.com/zvenson/dinghy) by zvenson, based on [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita / Hügelton Instruments) and [SLOOP](https://github.com/isod89/sloop-fm1) (isod89).
- Licensed under **GPL-3.0-only** — see [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md).
