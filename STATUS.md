# Status (2026-11-17, run ended)

Metric: distinct phrases in one palindrome under RULES.md (tie-break: words). Record: 16,111 phrases / 21,012 words (Norvig 2016).

| Item | Value |
|---|---|
| Best | 21,729 phrases / 27,794 words / 122,679 letters (`results/best.txt`) |
| Against Norvig | +34.9 % phrases, +32.3 % words |
| Upper bound | 22,050 phrases: LP relaxation 22,046.41 edges + 4 seed phrases (also with a free end state) |
| Gap to bound | 321 phrases (1.5 %) |
| Calibration | `build/dfs` (C port of pal3) gives a byte-identical copy of Norvig's text in 5 s |

Check: `python3 -I verify.py results/best.txt`.

Method: the palindrome is a walk in a graph whose states are the unmatched middle letters and whose edges are phrases (`src/graph.h`). A phrase can be used once, so the walk is a set of edges, at most one per phrase, with in-degree = out-degree at each state. That is an integer program (`src/mip.c`). HiGHS solves the LP relaxation; restricted integer programs on LP supports (`tools/kernel.sh`) give the solutions; `mip walk` turns a solution into the palindrome with an Euler circuit.

Log: `decision.tsv` (not committed). Next ideas: `NEXT.md`.
