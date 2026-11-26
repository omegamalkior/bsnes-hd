import sys; sys.path.insert(0,'/home/claude/tools')
from lr import Core
from util import sheet
so=sys.argv[1]; out=sys.argv[2]
c=Core(so,'/home/claude/bench/sm.sfc',{'bsnes_mode7_wsMode':'Mode 7','bsnes_mode7_widescreen':'21:9','bsnes_mode7_scale':'1x'})
c.run(5); c.load('/home/claude/shots/landing.state'); c.run(3)
fr=[]; want=[1230,1260,1290,1310,1330,1350,1370,1400]
for i in range(900):
    c.run(1,['Right'] if i%2==0 else [])
    x=c.r16(0x0911)
    if want and x>=want[0]:
        fr.append(c.frame.copy()); print('L1X',x,'ship centre rel. to camera',1150-x, 'stats',c.stats() if hasattr(c,'stats') else ''); want.pop(0)
    if not want: break
sheet(fr,out,2)
