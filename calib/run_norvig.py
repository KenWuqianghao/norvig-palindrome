import os, sys, time
src = open(os.path.join(sys.argv[1], 'pal3.py')).read().split("if __name__ == '__main__':")[0]
os.chdir(sys.argv[1])
g = {'__name__': 'pal3'}
exec(compile(src, 'pal3.py', 'exec'), g)
p = g['Panama']()
p.search(10**8)
print(p.report(), flush=True)
open(sys.argv[2], 'w').write('\n'.join(p.dict[k] for k in p.best) + '\n')
