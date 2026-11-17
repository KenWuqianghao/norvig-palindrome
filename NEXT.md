# Next ideas (one change per measurement)

Gap: 21,592 found, 22,050 bound (458 phrases, 2.1%).

1. Full MIP with warm start (running): better solution, or a smaller gap.
2. Larger restricted MIP: LP support plus edges with reduced cost 0.
3. Several LP vertices (perturbed objective, for example weight by words) and the union of their supports.
4. Tie-break: with the phrase count fixed at the best, maximize words.
5. Tighter bound: add connectivity cuts if a solution has a component that does not touch the start.

Done or rejected: middle-only DFS (Norvig, 16,111); greedy cycle splice (14,817); splice into the DFS result (+0).
