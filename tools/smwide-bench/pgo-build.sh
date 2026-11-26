#!/bin/bash
# Profile-guided build of the Windows core (MSYS2 MINGW64 shell, or Git Bash with
# C:\msys64\mingw64\bin on PATH). Same source, same output frames; the compiler lays the
# code out using a profile recorded from real play, which makes it about 8-10% faster.
#
#   tools/smwide-bench/pgo-build.sh <rom> <dir with landing/parlor/a1s0/a2s1/a5s0 .state files>
#
# Result: bsnes/out/bsnes_hd_10-7_libretro.dll (stripped). Needs mingw-w64 gcc, make and libgomp.
set -e
rom="$1"; states="$2"
[ -f "$rom" ] && [ -d "$states" ] || { echo "usage: $0 <rom> <states dir>"; exit 1; }
cd "$(dirname "$0")/../.."
jobs=${NUMBER_OF_PROCESSORS:-4}
gcc -O2 -static -o bsnes/out/smwide-bench.exe tools/smwide-bench/bench.c

# 1. instrumented build
rm -f bsnes/obj/*.o bsnes/obj/*.d bsnes/obj/*.gcda
flags="-fprofile-generate -fprofile-update=atomic" options="-fprofile-generate" \
  mingw32-make -j"$jobs" -C bsnes target=libretro
cp bsnes/out/bsnes_hd_10-7_libretro.dll bsnes/out/pgo-gen.dll

# 2. training run (slow: the instrumented core runs at roughly 30 fps)
for r in landing parlor a1s0 a2s1 a5s0; do for ar in 21:9 16:9; do
  bsnes/out/smwide-bench.exe bsnes/out/pgo-gen.dll "$rom" "$states/$r.state" 1500 time $ar 2x "Mode 7"
done; done
rm -f bsnes/out/pgo-gen.dll

# 3. optimized build using the profile
rm -f bsnes/obj/*.o bsnes/obj/*.d
flags="-fprofile-use -fprofile-correction -Wno-missing-profile -DSMW_PGO" \
  mingw32-make -j"$jobs" -C bsnes target=libretro
strip --strip-unneeded bsnes/out/bsnes_hd_10-7_libretro.dll
rm -f bsnes/obj/*.gcda
bsnes/out/smwide-bench.exe bsnes/out/bsnes_hd_10-7_libretro.dll --smoke
echo "Now compare 'hash' output against the regular build before installing (must be identical)."
