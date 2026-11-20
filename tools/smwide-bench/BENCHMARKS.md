# Super Metroid widescreen: benchmarks

Measured 2026-11-18 with `bench.c` (a headless libretro frontend) on a 2-core Linux
build machine, GCC -O3, OpenMP on. Each figure is the best of 3 runs of 1000 frames
of scripted play from a save state, in milliseconds of emulation per frame (lower is
better; 16.7 ms is the real-time budget). Absolute numbers will differ on other
machines; the before/after ratios are what matter.

"Before" is commit f8c0f69, "after" is the commit that adds this file. Output frames
were compared with a hash over every pixel of every frame: 88 runs (10 save states
and a 4000-frame boot run, at 16:9 / 19.5:9 / 21:9 and HD scales 1x to 4x), 0 differences.

## Average over 5 rooms (ms per frame)

| Setting                    | Before | After | Change | Speed-up |
|----------------------------|-------:|------:|-------:|---------:|
| 21:9, HD scale 2x          |  4.50  | 2.51  |  -44%  |  1.79x   |
| 19.5:9, HD scale 2x        |  4.41  | 2.35  |  -47%  |  1.87x   |
| 16:9, HD scale 2x          |  3.85  | 2.30  |  -40%  |  1.67x   |
| 21:9, HD scale 1x          |  3.07  | 2.33  |  -24%  |  1.32x   |
| 21:9, 2x, one CPU thread*  |  6.43  | 2.91  |  -55%  |  2.21x   |
| Unmodified core, 4:3, 2x   |  3.35  |   -   |        |          |

\* one room (Parlor) only.

## Per room, 21:9 at HD scale 2x (ms per frame)

| Room                         | Before | After |
|------------------------------|-------:|------:|
| Crateria landing site        |  4.76  | 2.55  |
| Crateria Parlor              |  4.94  | 2.55  |
| Brinstar save room           |  4.13  | 2.29  |
| Norfair (room next to save)  |  4.17  | 2.79  |
| Tourian save room            |  4.46  | 2.39  |

## What changed

1. Lines without Mode 7 are rendered once at the original resolution and enlarged,
   instead of rendering every enlarged pixel (4 per pixel at 2x) separately.
2. Background tiles are decoded a whole row at a time with a lookup table and written
   directly; fully transparent tile rows are skipped.
3. Backgrounds are not drawn in the parts of a line that are masked black.
4. Two smoothing loops copied a whole scanline record (several KB) per step; they now
   use references.
5. Two small heap allocations per scanline replaced with stack arrays.

1-3 are only used when Super Metroid is loaded, because that is the only game the
frame-for-frame comparison was run on.

## Sprite position self-check (added 2026-11-20)

OAM only stores the low 9 bits of a sprite's horizontal position. At 21:9 the picture is
496 pixels wide, so a sprite just past the right edge used to reappear at the left edge
(the gunship showing up twice). The core now records the full position when the game
writes each sprite and places sprites by that.

`bench <core> <rom> <state> <frames> check` counts, per frame, sprite tiles drawn in the
side areas with and without a recorded position. Any tile without one could be in the
wrong place, so the expected count is 0.

Result on the commit that added this: 10 save states x 16:9 / 19.5:9 / 21:9 x 1200 frames
of scripted play (36,000 frames): 0 tiles without a recorded position. With widescreen
off, 21 hash-compared runs are identical to the previous version.
