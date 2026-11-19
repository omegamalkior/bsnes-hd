#!/bin/bash
# usage: run.sh <label> <so> <ar> <scale> <wsmode> [frames] [reps]
label=$1; so=$2; ar=$3; scale=$4; ws=$5; frames=${6:-1500}; reps=${7:-3}
cd /home/claude/bench
for st in landing parlor a1s0 a2s1 a5s0; do
  best=999
  for r in $(seq $reps); do
    ms=$(./bench "$so" sm.sfc /home/claude/shots/$st.state $frames time "$ar" "$scale" "$ws" | sed -E 's/.*ms\/frame=([0-9.]+).*/\1/')
    best=$(python3 -I -c "print(min($best,$ms))")
  done
  echo "$label,$ar,$scale,$st,$best"
done
