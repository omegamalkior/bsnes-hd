import ctypes as C, sys, numpy as np
from PIL import Image
class Var(C.Structure): _fields_=[("key",C.c_char_p),("value",C.c_char_p)]
class GI(C.Structure): _fields_=[("path",C.c_char_p),("data",C.c_void_p),("size",C.c_size_t),("meta",C.c_char_p)]
class Geo(C.Structure): _fields_=[("bw",C.c_uint),("bh",C.c_uint),("mw",C.c_uint),("mh",C.c_uint),("ar",C.c_float)]
class Tim(C.Structure): _fields_=[("fps",C.c_double),("sr",C.c_double)]
class AV(C.Structure): _fields_=[("g",Geo),("t",Tim)]
B=dict(B=0,Y=1,Select=2,Start=3,Up=4,Down=5,Left=6,Right=7,A=8,X=9,L=10,R=11)
class Core:
    def __init__(s, so, rom, opts=None):
        s.opts={k:v.encode() for k,v in (opts or {}).items()}; s.fmt=0; s.frame=None; s.pad=set(); s.geo=None; s.dirty=False
        s.l=C.CDLL(so)
        ENV=C.CFUNCTYPE(C.c_bool,C.c_uint,C.c_void_p)
        VID=C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_uint,C.c_size_t)
        AUD=C.CFUNCTYPE(None,C.c_int16,C.c_int16); AUB=C.CFUNCTYPE(C.c_size_t,C.c_void_p,C.c_size_t)
        POL=C.CFUNCTYPE(None); INP=C.CFUNCTYPE(C.c_int16,C.c_uint,C.c_uint,C.c_uint,C.c_uint)
        def env(cmd,d):
            cmd&=0xffff
            if cmd==15:
                v=C.cast(d,C.POINTER(Var)).contents
                k=v.key.decode()
                if k in s.opts: v.value=s.opts[k]; return True
                return False
            if cmd==17:
                C.cast(d,C.POINTER(C.c_bool))[0]=s.dirty; s.dirty=False; return True
            if cmd==10: s.fmt=C.cast(d,C.POINTER(C.c_int))[0]; return True
            if cmd in (9,31): C.cast(d,C.POINTER(C.c_char_p))[0]=b"/tmp/lr"; return True
            if cmd in (37,32):
                g=C.cast(d,C.POINTER(Geo)).contents; s.geo=(g.bw,g.bh,g.ar); return True
            return False
        def vid(d,w,h,p):
            if not d: return
            if s.fmt==1:
                a=np.ctypeslib.as_array(C.cast(d,C.POINTER(C.c_uint8)),(h,p))[:, :w*4].reshape(h,w,4)
                s.frame=a[:,:,2::-1].copy()
            else:
                a=np.ctypeslib.as_array(C.cast(d,C.POINTER(C.c_uint16)),(h,p//2))[:, :w].astype(np.uint32)
                s.frame=np.stack([(a>>11)<<3,((a>>5)&63)<<2,(a&31)<<3],-1).astype(np.uint8)
        def inp(port,dev,idx,i):
            if port==0 and dev==1:
                if i==256: return sum(1<<B[b] for b in s.pad)
                return 1 if any(B[b]==i for b in s.pad) else 0
            return 0
        s._cb=(ENV(env),VID(vid),AUD(lambda l,r:None),AUB(lambda d,n:n),POL(lambda:None),INP(inp))
        l=s.l
        l.retro_set_environment(s._cb[0]); l.retro_set_video_refresh(s._cb[1]); l.retro_set_audio_sample(s._cb[2])
        l.retro_set_audio_sample_batch(s._cb[3]); l.retro_set_input_poll(s._cb[4]); l.retro_set_input_state(s._cb[5])
        l.retro_init()
        gi=GI(rom.encode(),None,0,None); l.retro_load_game.restype=C.c_bool
        assert l.retro_load_game(C.byref(gi))
        l.retro_get_memory_data.restype=C.c_void_p; l.retro_get_memory_size.restype=C.c_size_t
        n=l.retro_get_memory_size(2); s.ram=(C.c_uint8*n).from_address(l.retro_get_memory_data(2))
        v=l.retro_get_memory_data(0x1000); s.vram=(C.c_uint16*65536).from_address(v) if v else None
        l.retro_serialize_size.restype=C.c_size_t; l.retro_serialize.restype=C.c_bool; l.retro_unserialize.restype=C.c_bool
    def setopt(s,k,v): s.opts[k]=v.encode(); s.dirty=True
    def run(s,n=1,pad=None,hook=None):
        if pad is not None: s.pad=set(pad)
        for _ in range(n):
            if hook: hook(s)
            s.l.retro_run()
    def r8(s,a): return s.ram[a&0x1ffff]
    def r16(s,a): a&=0x1ffff; return s.ram[a]|(s.ram[a+1]<<8)
    def w8(s,a,v): s.ram[a&0x1ffff]=v&255
    def w16(s,a,v): s.w8(a,v); s.w8(a+1,v>>8)
    def shot(s,path): Image.fromarray(s.frame).save(path); return s.frame.shape
    def save(s,path):
        n=s.l.retro_serialize_size(); b=(C.c_uint8*n)(); assert s.l.retro_serialize(b,n); open(path,'wb').write(bytes(b))
    def load(s,path):
        d=open(path,'rb').read(); b=(C.c_uint8*len(d)).from_buffer_copy(d); assert s.l.retro_unserialize(b,len(d))
    def bg(s):
        o=list((C.c_int*33).from_address(s.l.retro_get_memory_data(0x1001)))
        return [dict(zip(['addr','size','h','v','td','ts','above','below'],o[i*8:i*8+8])) for i in range(4)]+[o[32]]
    def stats(s):
        p=s.l.retro_get_memory_data(0x534d57)
        if not p: return None
        a=(C.c_uint*4).from_address(p); return (a[0],a[1],C.c_int(a[2]).value,a[3])
