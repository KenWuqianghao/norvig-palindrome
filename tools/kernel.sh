#!/bin/bash
# One detached kernel-search loop: perturbed LP vertex -> support union with the best -> restricted MIP.
# usage: tools/kernel.sh first_seed last_seed   (run from the repo root; best solution in runs/mip/best.sol)
set -u
cd "$(dirname "$0")/.."
M=runs/mip
for seed in $(seq "$1" "$2"); do
  t=k$seed
  OBJNOISE=0.01,$seed nice -n 19 ./build/mip ends-gen upstream/npdict.txt $M/$t.lp 2>/dev/null
  printf 'threads = 1\ntime_limit = 2400\nsolver = ipm\nsolve_relaxation = true\nwrite_solution_to_file = true\nsolution_file = %s\nwrite_solution_style = 0\n' $M/$t.lpsol > $M/$t.lpopt
  nice -n 19 highs --model_file $M/$t.lp --options_file $M/$t.lpopt > $M/$t.lplog 2>&1
  python3 -I tools/mksup.py $M/$t.sup $M/$t.lpsol:1e-6 $M/best.sol:0.5 > /dev/null
  nice -n 19 ./build/mip gen upstream/npdict.txt $M/$t.r.lp $M/$t.sup 2>/dev/null
  python3 -I tools/mkwarm.py $M/$t.r.lp $M/best.sol $M/$t.warm > /dev/null
  printf 'threads = 1\ntime_limit = 2400\nmip_rel_gap = 0\nread_solution_file = %s\nwrite_solution_to_file = true\nsolution_file = %s\nwrite_solution_style = 0\n' $M/$t.warm $M/$t.sol > $M/$t.ropt
  nice -n 19 highs --model_file $M/$t.r.lp --options_file $M/$t.ropt > $M/$t.rlog 2>&1
  nice -n 19 ./build/mip walk upstream/npdict.txt $M/$t.sol runs/$t.txt > /dev/null 2>&1
  v=$(python3 -I verify.py runs/$t.txt 2>&1 | tail -1)
  n=$(echo "$v" | sed -n 's/^OK palindrome: \([0-9,]*\) phrases.*/\1/p' | tr -d ,)
  b=$(python3 -I verify.py results/best.txt | sed -n 's/^OK palindrome: \([0-9,]*\) phrases.*/\1/p' | tr -d ,)
  gap=$(grep -E "^  Gap" $M/$t.rlog | awk '{print $2}')
  verdict=reverted
  if [ -n "$n" ] && [ "$n" -gt "$b" ]; then cp runs/$t.txt results/best.txt; cp $M/$t.sol $M/best.sol; verdict=kept; fi
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$t" "$(date +%H:%M)" "$b" "${n:-none}" "$gap" "$verdict" >> runs/kernel.tsv
  rm -f $M/$t.lp $M/$t.r.lp
done
echo done >> runs/kernel.tsv
