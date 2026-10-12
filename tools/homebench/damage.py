#!/usr/bin/env python3
import sys, time

if sys.argv[1] == 'stamp':
    with open(sys.argv[2], 'w') as out:
        for line in sys.stdin:
            if line.startswith('post '):
                out.write(f"{time.monotonic() * 1000:.3f}\n")
                out.flush()
            else:
                sys.stdout.write(line)
                sys.stdout.flush()
    sys.exit(0)

from Xlib import display, X
from Xlib.ext import damage

d = display.Display()
d.damage_query_version()
win = d.create_resource_object('window', int(sys.argv[1], 16))
secs = float(sys.argv[2])
dmg = win.damage_create(damage.DamageReportRawRectangles)
d.flush()
stamps = {}
area = 0
offset = None
t_end = time.time() + secs
while time.time() < t_end:
    while d.pending_events():
        e = d.next_event()
        if e.type == d.extension_event.DamageNotify:
            a = e.area
            stamps[e.timestamp] = stamps.get(e.timestamp, 0) + a.width * a.height
            area += a.width * a.height
            lag = time.monotonic() * 1000 - e.timestamp
            if offset is None or lag < offset: offset = lag
    time.sleep(0.001)
ts = sorted(stamps)
frames = []
for t in ts:
    if frames and t - frames[-1][-1] <= 3: frames[-1].append(t)
    else: frames.append([t])
starts = [f[0] for f in frames]
posts = []
if len(sys.argv) > 3 and offset is not None:
    with open(sys.argv[3]) as f:
        posts = [float(l) - offset for l in f if l.strip()]
if posts:
    starts = [s for s in starts if posts[0] <= s <= posts[-1]]
gaps = sorted(b - a for a, b in zip(starts, starts[1:]))
if len(starts) > 1:
    span = (starts[-1] - starts[0]) / 1000
    print(f"frames={len(starts)} span={span:.2f}s fps={(len(starts)-1)/span:.1f} frame_p50={gaps[len(gaps)//2]}ms "
          f"frame_p95={gaps[int(len(gaps)*.95)]}ms max={gaps[-1]}ms mpix={area/1e6:.1f}")
else:
    print("frames<2")
if posts and starts:
    delays = (next((s - p for s in starts if s >= p), None) for p in posts)
    delays = sorted(x for x in delays if x is not None)
    if delays:
        print(f"posts={len(posts)} post_to_frame_p50={delays[len(delays)//2]:.0f}ms "
              f"p95={delays[int(len(delays)*.95)]:.0f}ms max={delays[-1]:.0f}ms")
