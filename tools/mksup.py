import sys, re
def ones(path, thr):
    s=set(); cols=False
    for l in open(path):
        if l.startswith('# Columns'): cols=True; continue
        if l.startswith('# Rows'): break
        if cols:
            n,v=l.split()
            if float(v)>thr: s.add(n)
    return s
out, *srcs = sys.argv[1:]
u=set()
for s in srcs:
    p,t=s.split(':'); u|=ones(p,float(t))
with open(out,'w') as f:
    f.write('# Columns %d\n'%len(u))
    for n in sorted(u, key=lambda n:(n[0],int(n[1:]))): f.write('%s 1\n'%n)
    f.write('# Rows 0\n')
print(len(u))
