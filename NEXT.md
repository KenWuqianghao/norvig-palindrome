# Next ideas

Gap: 21,729 found, 22,050 bound (321 phrases, 1.5 %). The kernel loop gave +89, +39, +7, +2 per 40-minute step, so it is at a plateau.

1. Tighter bound: the full MIP root with cuts reached 21,892.2 (HiGHS log, units not confirmed). Run the full MIP to the end of its root with more memory/CPU to state a confirmed bound.
2. More cores: the full MIP with 8 threads, or on the Steam Deck (not allowed in this run).
3. A connectivity cut is not needed so far: every solution was connected to the start.

Done or rejected: middle-only DFS (Norvig, 16,111); greedy cycle splice (14,817); splice into the DFS result (+0); full MIP from scratch (root LP did not end in 30 min with dual simplex).
