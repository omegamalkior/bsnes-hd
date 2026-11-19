#!/bin/bash
# compares frame hashes of two builds over all scenarios; prints mismatches
A=$1; B=$2; cd /home/claude/bench; bad=0; n=0
IFS="|" read -ra CFGS <<< "${3:-21:9 2x|19.5:9 2x|16:9 2x|21:9 1x|21:9 3x|16:9 4x}"; unset IFS
for cfg in "${CFGS[@]}"; do
  set -- $cfg
  for st in landing parlor a1s0 a1s1 a2s0 a2s1 a3s0 a4s0 a4s1 a5s0; do
    ha=$(./bench $A sm.sfc /home/claude/shots/$st.state 700 hash $1 $2 "Mode 7" | sed 's/.*hash=//')
    hb=$(./bench $B sm.sfc /home/claude/shots/$st.state 700 hash $1 $2 "Mode 7" | sed 's/.*hash=//')
    n=$((n+1)); if [ "$ha" != "$hb" ]; then bad=$((bad+1)); echo "MISMATCH $cfg $st $ha $hb"; fi
  done
  ha=$(./bench $A sm.sfc - 4000 hash $1 $2 "Mode 7" boot | sed 's/.*hash=//'); hb=$(./bench $B sm.sfc - 4000 hash $1 $2 "Mode 7" boot | sed 's/.*hash=//')
  n=$((n+1)); if [ "$ha" != "$hb" ]; then bad=$((bad+1)); echo "MISMATCH $cfg boot $ha $hb"; fi
done
echo "compared $n runs, $bad mismatches"
