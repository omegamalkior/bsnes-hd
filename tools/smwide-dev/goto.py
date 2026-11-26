import sys; sys.path.insert(0,'/home/claude/tools')
from lr import Core
def boot(area,station,ar='21:9',items=True):
    c=Core('/home/claude/bsnes-hd/bsnes/out/bsnes_hd_beta_libretro.so','/mnt/user-data/uploads/Super Metroid/Super Metroid.sfc',
      {'bsnes_mode7_wsMode':'all','bsnes_mode7_widescreen':ar,'bsnes_mode7_scale':'1x'})
    c.run(600)
    def hook(c):
        if c.r16(0x0998) in (2,4,0x1f):
            c.w16(0xD914,5); c.w16(0xD918,area); c.w16(0xD916,station); c.w8(0xD820,1)
    for i in range(400):
        c.run(4,['Start'],hook); c.run(26,[],hook)
        if c.r16(0x0998)==8: break
    if items:
        c.w16(0x09A2,0xF32F); c.w16(0x09A4,0xF32F); c.w16(0x09C2,1499); c.w16(0x09C4,1499); c.w16(0x09A6,0x1007); c.w16(0x09A8,0x100F)
    c.run(300,[])
    return c
def info(c):
    bg=c.bg()
    return 'state %x room %04x area %d W %d H %d L1 %d,%d L2 %d,%d l2scroll %04x bg2size %d bg2addr %04x boss %d'%(c.r16(0x0998),c.r16(0x079B),c.r16(0x079F),c.r16(0x07A5),c.r16(0x07A7),c.r16(0x0911),c.r16(0x0915),c.r16(0x0917),c.r16(0x0919),c.r16(0x091B),bg[1]['size'],bg[1]['addr'],c.r16(0x179C))
def walk(c,dirn,frames=1500):
    room=c.r16(0x079B)
    for i in range(frames//30):
        c.run(6,[dirn,'X']); c.run(14,[dirn,'B']); c.run(10,[dirn,'A','B'])
        if c.r16(0x079B)!=room or c.r16(0x0998)!=8: break
    for i in range(400):
        if c.r16(0x0998)==8: break
        c.run(1,[dirn])
    c.run(30,[dirn])
    return c.r16(0x079B)!=room
if __name__=='__main__':
    for area in range(1,6):
        for st in range(0,2):
            try:
                c=boot(area,st)
                print(area,st,info(c))
                for d in ('Left','Right'):
                    if walk(c,d): print('   ',d,info(c)); break
            except Exception as e: print(area,st,'ERR',e)
