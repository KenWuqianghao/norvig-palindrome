# Status (2026-11-17, run ended)

Headline: 27,794 words / 21,729 phrases (`results/best.txt`). Norvig 2016: 21,012 / 16,111. Best later published result: EricSpencer00 2026-09, 21,088 / 16,168.

| Item | Value |
|---|---|
| Against Norvig | +32.3 % words, +34.9 % phrases |
| Against EricSpencer00 | +31.8 % words, +34.4 % phrases |
| Ceiling | 22,050 phrases: phrases only, under the frozen rules, free-end model. LP 22,046.41 edges (exact dual certificate) + 4 seeds |
| Gap to ceiling | 321 phrases (1.5 %) |
| Calibration | `build/dfs` (C port of pal3) gives a byte-identical copy of Norvig's text in 5 s |
| Cut bound 21,892.2 | Not confirmed (full MIP root after cuts; log not kept, units not stated) |

Check: `python3 -I verify.py results/best.txt`. Rebuild and ceiling: `./reproduce.sh`. Search: `tools/pipeline.sh`. Log: `decision.tsv`.
