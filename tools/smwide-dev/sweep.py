import sys; sys.path.insert(0,'/home/claude/tools')
from lr import Core
from util import sheet
import numpy as np
so=sys.argv[1]; ars=sys.argv[2].split(','); frames=int(sys.argv[3])
states='landing parlor a1s0 a1s1 a2s0 a2s1 a3s0 a4s0 a4s1 a5s0'.split()
def script(f):
    c=f%600; p=[]
    if c<200: p=['Left','B']
    elif c<400: p=['Right','B']
    elif c<500: p=['Left']
    else: p=['Right']
    if c%40<12: p.append('A')
    if c%25<4: p.append('X')
    return p
if __name__!="__main__": ars=[]
bad=[]; tot=0
for ar in ars:
    for st in states:
        c=Core(so,'/home/claude/bench/sm.sfc',{'bsnes_mode7_wsMode':'Mode 7','bsnes_mode7_widescreen':ar,'bsnes_mode7_scale':'1x'})
        c.run(5); c.load(f'/home/claude/shots/{st}.state'); c.run(3)
        c.w16(0x09C2,1499)
        mx=0; side=0; true=0; rooms=set(); lvl=0
        for f in range(frames):
            c.run(1,script(f)); s=c.stats(); tot+=1
            rooms.add(c.r16(0x079B)); lvl+=s[3]
            true+=s[1]
            if s[0]:
                side+=1
                if s[0]>mx: mx=s[0]; worst=c.frame.copy(); wf=f
        print(f'{ar:7s} {st:8s} rooms={len(rooms)} level_frames={lvl} frames_with_unrecorded_side_sprites={side} max_tiles={mx} recorded_tiles={true}')
        if side: bad.append((ar,st,wf,worst))
        del c
if bad: sheet([b[3] for b in bad if b[0]==ars[0]][:8],'/home/claude/shots/sweep_bad.png',2)
print('total frames',tot,'scenarios with unrecorded side sprites',len(bad))
