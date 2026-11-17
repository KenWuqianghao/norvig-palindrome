# Next ideas

Gap: 21,729 found, 22,050 ceiling (321 phrases, 1.5 %). The kernel steps gave +89, +39, +7, +2 per 40-minute step, so the search is at a plateau.

1. Tighter bound: the full MIP root with cuts showed 21,892.2 in one HiGHS log. Not confirmed: the log was not kept and HiGHS did not state if the value is in original units. Run the full MIP to the end of its root, with more cores, to state a confirmed bound.
2. A word bound: the ceiling is for phrases only.
3. A connectivity cut is not needed so far: every solution was connected to the start.

Done or rejected: middle-only DFS (Norvig, 16,111); greedy cycle splice (14,817); splice into the DFS result (+0); full MIP from scratch (root LP did not end in 30 min with dual simplex).
