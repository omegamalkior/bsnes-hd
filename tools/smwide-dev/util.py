from PIL import Image
import numpy as np
def sheet(fr,path,cols=3):
    w=fr[0].shape[1]; h=fr[0].shape[0]; out=Image.new('RGB',((w+4)*cols,(h+4)*((len(fr)+cols-1)//cols)),(60,0,0))
    for k,f in enumerate(fr): out.paste(Image.fromarray(f),((k%cols)*(w+4),(k//cols)*(h+4)))
    out.save(path)
def bounds(f):
    m=f[40:].max(axis=(0,2))>12; idx=np.nonzero(m)[0]
    return (int(idx[0]),int(idx[-1])) if len(idx) else None
