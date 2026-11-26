import sys; sys.path.insert(0,'/home/claude/tools')
from lr import Core
from sweep import script
so=sys.argv[1]
for st in 'landing parlor a1s1 a2s0 a2s1 a4s0'.split():
    row=[]
    for ar in ('none','21:9'):
        opts={'bsnes_mode7_wsMode':'Mode 7' if ar!='none' else 'none','bsnes_mode7_widescreen':'21:9','bsnes_mode7_scale':'1x'}
        c=Core(so,'/home/claude/bench/sm.sfc',opts)
        c.run(5); c.load(f'/home/claude/shots/{st}.state'); c.run(3); c.w16(0x09C2,1499)
        a=[c.r16(x) for x in (0x05B6,0x05B8,0x05BA)]
        for f in range(1000): c.run(1,script(f))
        b=[c.r16(x) for x in (0x05B6,0x05B8,0x05BA)]
        row.append((ar,[(y-x)&0xffff for x,y in zip(a,b)]))
        del c
    print(st,row)
