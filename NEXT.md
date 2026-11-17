# Next ideas (one change per measurement)

1. euler v1: splice shortest cycles at any visited state (running).
2. If v1 stalls: also let the walk end move (path from the end to another terminal state).
3. Cycle choice: prefer cycles through rare phrases, or random among shortest.
4. Large-neighbourhood search: remove random segments X..X, re-insert greedily, keep if not worse.
5. Segment replacement: replace a sub-walk X..Y with a longer path X..Y.
6. Upper bound: LP of the phrase-disjoint circulation, to know the room left.
7. DFS side track: Norvig's own suggestion (subtract used phrases from prefix counts), randomized restarts.
