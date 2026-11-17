import sys, re
lp, sol, out = sys.argv[1:]
on=set(); cols=False
for l in open(sol):
    if l.startswith('# Columns'): cols=True; continue
    if l.startswith('# Rows'): break
    if cols:
        n,v=l.split()
        if float(v)>0.5: on.add(n)
txt=open(lp).read(); obj=txt[:txt.index('Subject To')]
names=re.findall(r'\b([xy]\d+)\b', obj)
with open(out,'w') as f:
    f.write('Model status\nUnknown\n\n# Primal solution values\nFeasible\nObjective 0\n# Columns %d\n'%len(names))
    for n in names: f.write('%s %d\n'%(n, 1 if n in on else 0))
    f.write('# Rows 0\n')
print(len(names), sum(n in on for n in names))
