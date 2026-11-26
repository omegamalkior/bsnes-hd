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
