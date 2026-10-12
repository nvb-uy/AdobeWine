#!/usr/bin/env python3
import sys, time
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
t_end = time.time() + secs
first = last = None
while time.time() < t_end:
    while d.pending_events():
        e = d.next_event()
        if e.type == d.extension_event.DamageNotify:
            a = e.area
            stamps[e.timestamp] = stamps.get(e.timestamp, 0) + a.width * a.height
            area += a.width * a.height
    time.sleep(0.001)
ts = sorted(stamps)
frames = []
for t in ts:
    if frames and t - frames[-1][-1] <= 25: frames[-1].append(t)
    else: frames.append([t])
starts = [f[0] for f in frames]
gaps = sorted(b - a for a, b in zip(starts, starts[1:]))
if len(starts) > 1:
    span = (starts[-1] - starts[0]) / 1000
    print(f"frames={len(starts)} span={span:.2f}s fps={(len(starts)-1)/span:.1f} frame_p50={gaps[len(gaps)//2]}ms "
          f"frame_p95={gaps[int(len(gaps)*.95)]}ms max={gaps[-1]}ms mpix={area/1e6:.1f}")
else:
    print("frames<2")
