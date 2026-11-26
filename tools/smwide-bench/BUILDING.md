# Building the SM-wide core

Branch: `super-metroid-widescreen`. The version string is set in
`bsnes/target-libretro/libretro.cpp` (`10.6 SM-wide r2`).

## Windows DLL (what was actually used)

The Windows core was never cross-compiled with MXE. It is built by GitHub
Actions (`.github/workflows/CI.yml`, job `Windows-libretro`) on
`windows-latest` with the MinGW-w64 GCC that ships on the runner:

    mingw32-make -j %NUMBER_OF_PROCESSORS% -C bsnes target=libretro
    strip --strip-unneeded bsnes/out/bsnes_hd_beta_libretro.dll

The link is static (see `bsnes/target-libretro/GNUmakefile`), so the DLL
needs no extra runtime DLLs. Every push to this branch publishes the DLL
and `smwide-bench.exe` to the `builds` branch of the fork.

To build on a Windows PC yourself: install MSYS2, then in the
"MSYS2 MINGW64" shell:

    pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make git
    git clone -b super-metroid-widescreen https://github.com/omegamalkior/bsnes-hd
    cd bsnes-hd
    mingw32-make -j8 -C bsnes target=libretro
    # result: bsnes/out/bsnes_hd_beta_libretro.dll

Do not use LTO (it broke the core).

## Linux .so

    cd bsnes && make -j2 target=libretro platform=linux binary=library local=false
    # result: bsnes/out/bsnes_hd_beta_libretro.so

## Test tool

    gcc -O2 -static -o smwide-bench.exe tools/smwide-bench/bench.c   (Windows)
    gcc -O2 -o bench tools/smwide-bench/bench.c -ldl                  (Linux)

    smwide-bench <core> --smoke
    smwide-bench <core> <rom> <state> <frames> time|hash|check|dump <aspect> <scale> <wsmode>

`check` reports sprites drawn in the side areas without a recorded true
position (must be 0). `hash` must match between Windows and Linux.
`run.sh` / `verify.sh` are the Linux batch scripts (paths are those of the
original dev machine; adjust). `tools/smwide-dev/` holds the Python
scripts used during development (libretro harness `lr.py`, disassembler
`smdis.py`, room sweep, repro scripts); same caveat about paths.

The ROM and save states are not in the repository. Unmodified Super
Metroid (JU), sha256 starting 12b77c4b.

## Profile-guided Windows build (2026-11-26)

`tools/smwide-bench/pgo-build.sh <rom> <states dir>` builds the same source with GCC's
profile-guided optimization: an instrumented build plays five rooms for 1500 frames each
at 21:9 and 16:9, then the core is rebuilt using that profile. The version string gets a
`pgo` suffix (`-DSMW_PGO`). It cannot run in CI because it needs the ROM.

Measured on a Zenbook S14 (MSYS2 GCC 16.2, best of 4, 3 rooms, 21:9 at HD scale 2x):

| Build                    | 1 thread | all threads |
|--------------------------|---------:|------------:|
| CI build (r2)            | 2.43 ms  | 1.56 ms     |
| profile-guided (r2 pgo)  | 2.18 ms  | 1.44 ms     |

Output: 64 hash-compared runs (5 rooms x 16:9 / 19.5:9 / 21:9 x HD scale 1x-4x, plus
widescreen mode none/all), 1200 frames each: 0 differences from the CI build.

Where the time goes (one thread, 21:9, 2x): about 1.65 ms of a 2.45 ms frame is CPU and
sound emulation and 0.85 ms is rendering. With OpenMP the rendering adds almost nothing to
the frame time, and the Super Metroid CPU hooks cost about 0.01 ms, so further work on the
widescreen renderer would not make frames noticeably faster.
