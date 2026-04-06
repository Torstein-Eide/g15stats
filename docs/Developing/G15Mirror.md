# G15Mirror Developer Guide

`g15mirror` is an `LD_PRELOAD` helper that intercepts g15daemon client calls and records what `g15stats` sends to the LCD.

Use this when you want to debug rendering, compare frames across modes, or inspect raw frame payloads without changing `g15stats` logic.

## What It Captures

- `new_g15_screen(...)` calls (socket + screen type)
- `g15_send(...)` frame payloads
- `g15_send_cmd(...)` command writes

For packed screen types (`G15_WBMPBUF` / `G15_G15RBUF`), it writes:

- raw `.bin` frames
- `.pbm` previews (160x43, P4)
- short ASCII preview to `stderr`

## Build

From repository root:

```bash
gcc -shared -fPIC -O2 -Wall -Wextra -ldl -pthread -o tests/libg15mirror.so tests/g15mirror.c
```

## Run `g15stats` Through the Mirror

```bash
mkdir -p .tmp/g15mirror
G15MIRROR_DIR=.tmp/g15mirror LD_PRELOAD="$PWD/tests/libg15mirror.so" ./g15stats
```

This creates files like:

- `.tmp/g15mirror/frame-YYYYMMDD-HHMMSS-ms-sockN-G15RBUF-860.bin`
- `.tmp/g15mirror/frame-... .pbm`

## Useful Environment Variables

- `G15MIRROR_DIR` - output directory for dumps (default: `/tmp/g15mirror`)

## Quick Workflow

1. Build mirror library.
2. Run `g15stats` with `LD_PRELOAD` and `G15MIRROR_DIR`.
3. Switch screens/modes with `L2/L3/L4/L5`.
4. Inspect `.pbm` files or raw `.bin` payloads for exact output.

## Notes

- This tool is intended for development/debugging, not production runtime.
- If a client does not use `g15_send_cmd`, mirror logs command support gracefully.
- For deterministic single-screen output during debugging, combine with:
  - `G15STATS_FORCE_SCREEN=<id>`
  - `G15STATS_FORCE_MODE=<0|1>`
