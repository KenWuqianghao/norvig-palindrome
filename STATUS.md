# Status

Metric: distinct phrases in one palindrome under RULES.md (tie-break: words). Record: 16,111 phrases / 21,012 words (Norvig 2016).

| Item | Value |
|---|---|
| Best | 21,592 phrases / 27,572 words / 121,909 letters (`results/best.txt`, commit 6d74bc6) |
| Upper bound | 22,050 phrases (LP relaxation 22,046.41 edges + 4 seed phrases; also with a free end state) |
| Calibration | `build/dfs` (C port of pal3) gives a byte-identical copy of Norvig's text in 5 s |
| Running | full MIP (`runs/mip/m2.lp`), warm start 21,588 edges, HiGHS 1 thread, 2 h |

Check: `python3 -I verify.py results/best.txt`.

Method: the palindrome is a walk in a graph whose states are the unmatched middle letters and whose edges are phrases (`src/graph.h`). A phrase can be used once, so the walk is a set of edges, at most one per phrase, with in-degree = out-degree at each state. That is an integer program (`src/mip.c`). HiGHS solves its LP relaxation, then the integer program on the LP support; `mip walk` turns the solution into the palindrome with an Euler circuit.

Log: `decision.tsv` (not committed). Next ideas: `NEXT.md`.
