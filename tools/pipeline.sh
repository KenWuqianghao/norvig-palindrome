#!/bin/bash
# The search that found results/best.txt (steps e6 to e12 in decision.tsv), as one script.
# Needs HiGHS 1.16.0 (git f3f113d; `brew install highs`). One thread, nice 19. About 4 h on an M1 Pro.
# The integer programs stop at time limits, so the result depends on machine speed and load.
# Default: use the committed LP supports (results/solutions/*.support). --lp: re-solve the LPs first.
# Output: runs/pipeline/best.txt, checked by verify.py.
set -eu
cd "$(dirname "$0")/.."
make -s tools
W=runs/pipeline; S=results/solutions; N="nice -n 19"
mkdir -p $W

highs_run() { # model options-text
  printf '%b' "$2" > "$1.opt"
  $N highs --model_file "$1.lp" --options_file "$1.opt" > "$1.log" 2>&1
}

lp_support() { # name gen-mode objnoise
  [ "${LP:-0}" = 1 ] || { cp $S/$1.support $W/$1.support; return; }
  OBJNOISE=$3 $N build/mip $2 upstream/npdict.txt $W/$1.lp 2>/dev/null
  highs_run $W/$1 "threads = 1\nsolver = ipm\nsolve_relaxation = true\nwrite_solution_to_file = true\nsolution_file = $W/$1.sol\nwrite_solution_style = 0\n"
  cp $W/$1.sol $W/$1.support
}

restricted() { # name time_limit warm-name|- support-files...
  local name=$1 tl=$2 warm=$3; shift 3
  python3 -I tools/mksup.py $W/$name.sup "$@" > /dev/null
  $N build/mip gen upstream/npdict.txt $W/$name.lp $W/$name.sup 2>/dev/null
  local opt="threads = 1\ntime_limit = $tl\nmip_rel_gap = 0\nwrite_solution_to_file = true\nsolution_file = $W/$name.sol\nwrite_solution_style = 0\n"
  if [ "$warm" != - ]; then
    python3 -I tools/mkwarm.py $W/$name.lp $W/$warm.sol $W/$name.warm > /dev/null
    opt="${opt}read_solution_file = $W/$name.warm\n"
  fi
  highs_run $W/$name "$opt"
  echo "$name: $(grep -E '^  Primal bound|^  Gap' $W/$name.log | tr -s ' ' | tr '\n' ' ')"
}

[ "${1:-}" = --lp ] && LP=1
lp_support lp1 gen 0
lp_support lp2 ends-gen 0
lp_support n1 ends-gen 0.01,1
lp_support k2lp ends-gen 0.01,2
lp_support k3lp ends-gen 0.01,3

restricted r1 1800 - $W/lp1.support:1e-6                    # e6
restricted r2 2400 r1 $W/lp2.support:1e-6 $W/r1.sol:0.5     # e8
restricted r3 2400 r2 $W/n1.support:1e-6 $W/r2.sol:0.5      # e9
restricted k2 2400 r3 $W/k2lp.support:1e-6 $W/r3.sol:0.5    # e10
restricted k3 2400 k2 $W/k3lp.support:1e-6 $W/k2.sol:0.5    # e11
count=$(python3 -I tools/mksup.py /dev/null $W/k3.sol:0.5)
MINCOUNT=$count restricted w2 1800 k3 $W/k3lp.support:1e-6 $W/k3.sol:0.5   # e12: keep the edge count, maximize words

$N build/mip walk upstream/npdict.txt $W/w2.sol $W/best.txt
python3 -I verify.py $W/best.txt
