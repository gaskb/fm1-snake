# FM-1 Snake

Classic Snake game firmware for the **M-VAVE FM-1** pocket synthesizer, built on [Dinghy](https://github.com/zvenson/dinghy) (Felucca / SLOOP platform).

## Features

- **240×240 incremental display**: 19×17 grid, directional eyes on the snake head, score & high-score HUD.
- **Chiptune sound effects**: synthesized square/pulse SFX in real time (`app_audio`) for start, turns, eating food, and game over.
- **Persistent high score**: saved in flash (`settings_save` with `STORE_MAGIC = "SNK1"`).
- **USB rescue**: hold **OCT−** at power-on to enter the updater if needed.

## Controls

| Input | Action |
| --- | --- |
| `PLAY` | Start / Pause / Restart |
| `KNOB1` or `SELECT` encoder | Turn left / right (90°) |
| `OCT−` / `OCT+` | Move Left / Right |
| `ENV` (or `FX`) / `SEL` (or `LFO`) | Move Up / Down |
| Keys `C` / `D` / `E` / `C#`–`D#` (both octaves) | Move Left / Down / Right / Up |

## Build

Requires Docker (on macOS/Windows) or Linux x86-64, the JieLi toolchain (`./tools/get_toolchain.sh`), and `fw-AC79_AIoT_SDK`:

```sh
JIELI_TOOLCHAIN="$HOME/.jieli/toolchain" AC79_SDK="$HOME/fw-AC79_AIoT_SDK" ./build.sh --release 0.1
```

Output package: `build/fm1-snake-0.1.fwsc` (identity `FM-1_961`).

## Credits & License

- Built on [Dinghy](https://github.com/zvenson/dinghy) by zvenson, based on [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita / Hügelton Instruments) and [SLOOP](https://github.com/isod89/sloop-fm1) (isod89).
- Licensed under **GPL-3.0-only** — see [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md).
