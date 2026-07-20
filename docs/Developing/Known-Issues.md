# Known issues

## Unsynchronized access to `cycle`, `mode[]`, and `submode`

**Status:** open, not yet fixed.

### The issue

`cycle`, `mode[MAX_SCREENS + 1]`, and `submode` are plain global `int`s
(declared in the "Global state" section of `g15stats.c`) that control which
screen is shown, which display mode each screen is in, and the current
sub-mode. They are read and written from two different threads with no
synchronization:

- `keyboard_watch()` runs on its own `pthread` (spawned in `main()`) and
  processes G15 key events. On L1-L5 presses it writes these globals directly,
  e.g. `cycle = find_next_visible_screen(cycle, 1);`, `mode[cycle]++;`,
  `submode++;`.
- `run_loop()` runs on the main thread and reads/writes the same globals every
  frame — e.g. indexing `mode[cycle]` to decide how to render the current
  screen, and itself changing `cycle` in response to forced-screen/auto-cycle
  logic.

Neither side takes a lock, uses an atomic type, or marks the variables
`volatile`. The only mutex in the file (`g15stats_wait()`'s `dummy_mutex`)
exists solely to drive a `pthread_cond_timedwait`-based sleep and never
touches these globals.

This is undefined behavior under the C11 memory model. In practice, on
typical x86/ARM hardware, a key press landing mid-frame is unlikely to crash
but can produce a torn read — e.g. `cycle` changing between the `mode[cycle]`
lookup and a later use of `cycle` in the same render pass, or a mode change
being silently dropped/duplicated. The realistic symptom is a rare display
glitch (wrong mode shown for one frame, a missed or double mode-cycle on a
key press), not a hard crash.

### Why it's not fixed yet

`cycle`, `mode[]`, and `submode` are read from nearly every screen renderer,
`print_info_label()`, the info-cycling logic, and both background threads.
Per AGENTS.md, these are exactly the global screen-state variables that
"input handling, info-cycling, and every screen renderer all read" — a
correct fix touches call sites throughout the ~5000-line file rather than one
localized spot, so it's being tracked here rather than patched incidentally
alongside smaller, localized bug fixes.

### Options considered

1. **Mutex around every access.**
   Wrap all reads and writes of `cycle`, `mode[]`, and `submode` in both
   `keyboard_watch()` and `run_loop()` (and any renderer that reads them) with
   a `pthread_mutex_t`. Fully correct, but the most invasive: dozens of call
   sites across nearly every `draw_*_screen()` function, `print_info_label()`,
   and the info-cycling code would need to take the lock, and holding a lock
   across a full render pass (to keep `cycle`/`mode[cycle]` consistent for
   the whole frame) risks blocking `keyboard_watch()` for the frame's
   duration.

2. **Mutex around the "critical section" only.**
   Lock only where `keyboard_watch()` writes and where `run_loop()` snapshots
   the values at the top of each frame into local variables, then have the
   rest of the frame's rendering use the local snapshot instead of the
   globals directly. Much smaller diff than (1), but requires introducing a
   snapshot/local-copy convention that every renderer would need to adopt to
   actually close the race for `mode[cycle]` (not just `cycle` itself).

3. **`_Atomic` / C11 atomics.**
   Change `cycle` and `submode` to `_Atomic int`, and `mode[]` to an array of
   `_Atomic int`. Removes the data race for individual reads/writes with very
   little code churn (mostly type changes, no explicit locking calls), but
   doesn't provide atomicity across *multiple* related reads in the same
   frame (e.g. reading `cycle` then indexing `mode[cycle]` can still observe
   `cycle` and `mode[]` from two different key-press "generations").

4. **Single-writer queue from `keyboard_watch()` to the main thread.**
   Instead of `keyboard_watch()` writing the globals directly, have it push a
   small event (key + timestamp) onto a lock-free/mutex-protected queue; only
   `run_loop()` (main thread) ever mutates `cycle`/`mode[]`/`submode`, at a
   well-defined point once per frame. This removes the race by construction
   (single writer) and is the architecturally "deepest" fix, but is the
   largest change of the five — it restructures how key events flow through
   the program.

5. **Leave as-is, documented.**
   Given the failure mode is a rare, cosmetic display glitch rather than a
   crash or data corruption, and every other option carries meaningful risk
   of introducing a regression in code explicitly flagged as sensitive,
   the current chosen approach is to leave the code unchanged and track the
   issue here so it isn't rediscovered as a surprise. Revisit if a real,
   reproducible symptom (not just the theoretical race) shows up.

**Current approach: option 5.** No code change has been made for this issue;
this document exists so the tradeoffs are visible next time it comes up.
