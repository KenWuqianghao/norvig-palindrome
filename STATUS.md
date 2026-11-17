# Status

Metric: distinct phrases in one palindrome under RULES.md (tie-break: words). Record to beat: 16,111 phrases / 21,012 words (Norvig 2016).

| Item | Value |
|---|---|
| Calibration | `build/dfs` (C port of pal3) gives a byte-identical copy of Norvig's text, 16,111 phrases, in 5 s |
| Best so far | 16,111 (calibration) |
| Running | `build/euler` v1: splice shortest cycles into the walk at any visited state |

Log: `decision.tsv` (not committed). Next ideas: `NEXT.md`.
