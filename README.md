# A longer "A man, a plan, a canal, Panama" palindrome

**27,794 words / 21,729 phrases** with Peter Norvig's phrase list `npdict.txt` and his pal3 rules (`RULES.md`).

| Result | Words | Phrases | Letters |
|---|---:|---:|---:|
| This repository (`results/best.txt`) | 27,794 | 21,729 | 122,679 |
| Norvig 2016, Version 3 ([norvig.com/palindrome.html](https://www.norvig.com/palindrome.html)) | 21,012 | 16,111 | 90,439 |
| Best later published result: EricSpencer00, 2026-09 ([palindrome-sentence-generator](https://github.com/EricSpencer00/palindrome-sentence-generator), `artifacts/norvig-v3`) | 21,088 | 16,168 | 90,937 |

Upper bound: **22,050 phrases**. This bound is for the phrase count only (there is no word bound), under the frozen rules in `RULES.md` (letters-only keys, distinct keys, the fixed 4-phrase frame), in the free-end model (the walk can stop at any palindromic state). It is the LP relaxation optimum 22,046.41 edges, proven by an exact weak-duality certificate, plus the 4 seed phrases. The best result is 321 phrases (1.5 %) below it.

## Check

```
python3 -I verify.py results/best.txt     # OK palindrome: 21,729 phrases, 27,794 words, 122,679 letters
./reproduce.sh                            # rebuild best.txt from the edge set, calibrate, check the ceiling
```

`reproduce.sh` needs a C compiler, `make` and `python3`. It does three things:
1. It runs the C port of Norvig's search and compares the output with his published text.
2. It rebuilds `results/best.txt` from `results/solutions/best.edges` with `build/mip walk`.
3. It writes the LP model and checks the dual certificate with `tools/dualbound.py`.

## Method

The palindrome is built from both ends. The state is the unmatched text in the middle. An edge is one phrase (`src/graph.h`). A palindrome is a walk from the start state to a state whose text is a palindrome. Each phrase can be used once. Thus a palindrome is a set of edges with at most one edge per phrase and equal in- and out-degree at each state: an integer program (`src/mip.c`).

- The LP relaxation gives the ceiling.
- Integer programs restricted to the support of LP solutions give the result (`tools/pipeline.sh`, HiGHS 1.16.0, one thread).
- `mip walk` turns an edge set into the palindrome with an Euler circuit.

`decision.tsv` records each attempt, kept or reverted. `tools/pipeline.sh` reruns the search (about 4 h). It stops each integer program at a time limit, so another machine can get a different file.

## Layout

| Path | Content |
|---|---|
| `RULES.md`, `verify.py` | frozen rules and the checker |
| `upstream/` | Norvig's `npdict.txt`, `pal3.py`, `pal3.ipynb`, page and MIT licence, with SHA-256 in `RULES.md` |
| `src/dfs.c` | exact C port of Norvig's pal3 search (calibration) |
| `src/graph.h`, `src/mip.c` | state graph, integer program writer, solution-to-palindrome walk |
| `src/euler.c`, `src/scc.c` | rejected greedy method, component check |
| `results/solutions/` | final edge set, intermediate edge sets, LP supports, ceiling duals |
| `tools/` | `pipeline.sh`, `dualbound.py` (from the 2026-11-17 audit), support and warm-start helpers |

## Licence

MIT (`LICENSE`). The files in `upstream/` are Peter Norvig's, under his MIT licence (`upstream/LICENSE`).
