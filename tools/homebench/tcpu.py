#!/usr/bin/env python3
import os, sys, time
pid = sys.argv[1]; secs = float(sys.argv[2])
def snap():
    r = {}
    for t in os.listdir(f'/proc/{pid}/task'):
        try:
            st = open(f'/proc/{pid}/task/{t}/stat').read()
            name = open(f'/proc/{pid}/task/{t}/comm').read().strip()
        except OSError: continue
        f = st.rsplit(')', 1)[1].split()
        r[t] = (name, int(f[11]) + int(f[12]))
    return r
a = snap(); time.sleep(secs); b = snap()
hz = os.sysconf('SC_CLK_TCK')
rows = sorted(((v[0], t, (v[1] - a.get(t, (0, v[1]))[1]) * 100.0 / hz / secs) for t, v in b.items()), key=lambda r: -r[2])
print(f"total {sum(r[2] for r in rows):.1f}%")
for n, t, c in rows[:12]:
    if c > 0.5: print(f"  {c:6.1f}% {t} {n}")
