# Copied from the 2026-11-17 audit (/Users/kenwu/research/audits/norvig-palindrome.md, Check 4), file
# audits/norvig-scratch/dualbound.py. Unchanged below this header.
# usage: python3 -I tools/dualbound.py model.lp duals.sol
"""Weak-duality certificate: for row multipliers y with valid signs, max c.x s.t. Ax ? b, 0<=x<=u is at most
b.y + sum_j u_j * max(0, c_j - (A^T y)_j). Uses exact rational arithmetic on the given multipliers."""
import re, sys
from fractions import Fraction as F
from collections import defaultdict
lp, sol = sys.argv[1:]
lines = open(sol).read().split('\n')
i = lines.index('# Dual solution values')
j = next(k for k in range(i, len(lines)) if lines[k].startswith('# Rows'))
duals = {}
for l in lines[j + 1:]:
    if l.startswith('#') or not l.strip(): break
    n, v = l.split(); duals[n] = F(v)
txt = open(lp).read()
st = txt[txt.index('Subject To'):txt.index('Bounds')].split('\n', 1)[1]
rows = []
for row in re.split(r'\n (?=\w+:)', st):
    m = re.match(r'\s*(\w+):(.*?)(=|<=|>=)\s*(\S+)\s*$', row, re.S)
    if m: rows.append((m.group(1), re.findall(r'([+-])\s*([xy]\d+)', m.group(2)), m.group(3), F(m.group(4))))
cols = set(re.findall(r'\b([xy]\d+)\b', txt[txt.index('Bounds'):]))
c = {v: (F(1) if v[0] == 'x' else F(0)) for v in cols}
for sign in (1, -1):
    red = dict(c); bound = F(0); clipped = 0
    for name, terms, op, rhs in rows:
        y = sign * duals.get(name, F(0))
        if op == '<=' and y < 0: y = F(0); clipped += 1
        if op == '>=' and y > 0: y = F(0); clipped += 1
        bound += rhs * y
        for s, v in terms: red[v] -= y if s == '+' else -y
    bound += sum(max(F(0), r) for r in red.values())   # every column has upper bound 1
    print(f'sign {sign:+d}: certified upper bound {float(bound):.6f} (clipped {clipped} multipliers)')
