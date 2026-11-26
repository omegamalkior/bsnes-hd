import sys; sys.path.insert(0,'/home/claude/tools')
from lr import Core
from util import sheet
so=sys.argv[1]; out=sys.argv[2]
c=Core(so,'/home/claude/bench/sm.sfc',{'bsnes_mode7_wsMode':'Mode 7','bsnes_mode7_widescreen':'21:9','bsnes_mode7_scale':'1x'})
c.run(5); c.load('/home/claude/shots/landing.state'); c.run(3)
fr=[]; want=[840,825,810,800,790,780,770,760,740,720]
c.run(40,['Left'])
for i in range(700):
    c.run(1,['Left'] if i%2==0 else [])
    x=c.r16(0x0911)
    if want and x<=want[0]: fr.append(c.frame.copy()); print('L1X',x,'stats',c.stats() if hasattr(c,'stats') else None); want.pop(0)
sheet(fr,out,2)
