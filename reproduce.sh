#!/bin/bash
# Rebuild results/best.txt from the committed final edge set, verify it, and check the 22,050-phrase ceiling.
# Needs a C compiler, make and python3. It does not need HiGHS. tools/pipeline.sh reruns the search itself.
set -eu
cd "$(dirname "$0")"
T=$(mktemp -d)
N="nice -n 19"
$N make -s tools

echo "== 1. Calibration: C port of Norvig's pal3 search, 1e8 steps"
$N build/dfs upstream/npdict.txt 100000000 "$T/norvig.txt"
cmp "$T/norvig.txt" results/norvig_16111.txt && echo "byte-identical to Norvig's published text"
python3 -I verify.py "$T/norvig.txt"

echo "== 2. Rebuild best.txt from results/solutions/best.edges"
$N build/mip walk upstream/npdict.txt results/solutions/best.edges "$T/best.txt"
cmp "$T/best.txt" results/best.txt && echo "byte-identical to results/best.txt"
python3 -I verify.py "$T/best.txt"

echo "== 3. Ceiling: exact weak-duality certificate for the LP of the free-end model"
$N build/mip ends-gen upstream/npdict.txt "$T/m2.lp"
$N python3 -I tools/dualbound.py "$T/m2.lp" results/solutions/ceiling_duals.sol | head -1
echo "=> at most 22,046 edges + 4 seed phrases = 22,050 phrases"
rm -rf "$T"
