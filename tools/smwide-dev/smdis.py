import sys
T="""BRK s|ORA dxi|COP s|ORA sr|TSB d|ORA d|ASL d|ORA dil|PHP i|ORA m|ASL A|PHD i|TSB a|ORA a|ASL a|ORA al
BPL r|ORA diy|ORA di|ORA sriy|TRB d|ORA dx|ASL dx|ORA dily|CLC i|ORA ay|INC A|TCS i|TRB a|ORA ax|ASL ax|ORA alx
JSR a|AND dxi|JSL al|AND sr|BIT d|AND d|ROL d|AND dil|PLP i|AND m|ROL A|PLD i|BIT a|AND a|ROL a|AND al
BMI r|AND diy|AND di|AND sriy|BIT dx|AND dx|ROL dx|AND dily|SEC i|AND ay|DEC A|TSC i|BIT ax|AND ax|ROL ax|AND alx
RTI i|EOR dxi|WDM s|EOR sr|MVP mv|EOR d|LSR d|EOR dil|PHA i|EOR m|LSR A|PHK i|JMP a|EOR a|LSR a|EOR al
BVC r|EOR diy|EOR di|EOR sriy|MVN mv|EOR dx|LSR dx|EOR dily|CLI i|EOR ay|PHY i|TCD i|JML al|EOR ax|LSR ax|EOR alx
RTS i|ADC dxi|PER rl|ADC sr|STZ d|ADC d|ROR d|ADC dil|PLA i|ADC m|ROR A|RTL i|JMP ai|ADC a|ROR a|ADC al
BVS r|ADC diy|ADC di|ADC sriy|STZ dx|ADC dx|ROR dx|ADC dily|SEI i|ADC ay|PLY i|TDC i|JMP axi|ADC ax|ROR ax|ADC alx
BRA r|STA dxi|BRL rl|STA sr|STY d|STA d|STX d|STA dil|DEY i|BIT m|TXA i|PHB i|STY a|STA a|STX a|STA al
BCC r|STA diy|STA di|STA sriy|STY dx|STA dx|STX dy|STA dily|TYA i|STA ay|TXS i|TXY i|STZ a|STA ax|STZ ax|STA alx
LDY x|LDA dxi|LDX x|LDA sr|LDY d|LDA d|LDX d|LDA dil|TAY i|LDA m|TAX i|PLB i|LDY a|LDA a|LDX a|LDA al
BCS r|LDA diy|LDA di|LDA sriy|LDY dx|LDA dx|LDX dy|LDA dily|CLV i|LDA ay|TSX i|TYX i|LDY ax|LDA ax|LDX ay|LDA alx
CPY x|CMP dxi|REP s|CMP sr|CPY d|CMP d|DEC d|CMP dil|INY i|CMP m|DEX i|WAI i|CPY a|CMP a|DEC a|CMP al
BNE r|CMP diy|CMP di|CMP sriy|PEI d|CMP dx|DEC dx|CMP dily|CLD i|CMP ay|PHX i|STP i|JML ail|CMP ax|DEC ax|CMP alx
CPX x|SBC dxi|SEP s|SBC sr|CPX d|SBC d|INC d|SBC dil|INX i|SBC m|NOP i|XBA i|CPX a|SBC a|INC a|SBC al
BEQ r|SBC diy|SBC di|SBC sriy|PEA a|SBC dx|INC dx|SBC dily|SED i|SBC ay|PLX i|XCE i|JSR axi|SBC ax|INC ax|SBC alx"""
OPS=[e.split() for l in T.split('\n') for e in l.split('|')]
rom=open('/mnt/user-data/uploads/Super Metroid/Super Metroid.sfc','rb').read()
def off(a): return ((a>>16)&0x7f)*0x8000+(a&0x7fff)
def dis(addr,n=40,m=0,x=0,stop=True):
    out=[]
    for _ in range(n):
        o=off(addr); op=rom[o]; mn,md=OPS[op]
        ln={'i':1,'A':1,'s':2,'d':2,'dx':2,'dy':2,'di':2,'dxi':2,'diy':2,'dil':2,'dily':2,'sr':2,'sriy':2,'r':2,'rl':3,'a':3,'ax':3,'ay':3,'ai':3,'axi':3,'ail':3,'al':4,'alx':4,'mv':3,'m':2 if m else 3,'x':2 if x else 3}[md]
        b=rom[o+1:o+ln]; v=int.from_bytes(b,'little') if b else 0
        if md=='r': t='$%04X'%((addr+2+(v-256 if v>127 else v))&0xffff)
        elif md=='rl': t='$%04X'%((addr+3+(v-65536 if v>32767 else v))&0xffff)
        elif md in('m','x','s'): t='#$%0*X'%((ln-1)*2,v)
        elif md in('i','A'): t=''
        else: t='$%0*X'%((ln-1)*2,v)+{'dx':',x','dy':',y','ax':',x','ay':',y','alx':',x','di':' ()','dxi':' (,x)','diy':' (),y','dil':' []','dily':' [],y','sr':',s','sriy':' (,s),y','ai':' ()','axi':' (,x)','ail':' []'}.get(md,'')
        out.append('%06X  %-4s %s'%(addr,mn,t))
        if mn=='REP':
            if v&0x20:m=0
            if v&0x10:x=0
        if mn=='SEP':
            if v&0x20:m=1
            if v&0x10:x=1
        addr+=ln
        if stop and mn in('RTS','RTL','RTI','JMP','JML','BRA'): out.append('')
    return '\n'.join(out)
def find(pat,lo=0,hi=len(rom)):
    r=[];i=rom.find(pat,lo)
    while i!=-1 and i<hi:
        r.append(0x800000|((i//0x8000)<<16)|0x8000|(i&0x7fff)); i=rom.find(pat,i+1)
    return r
if __name__=='__main__':
    a=int(sys.argv[1],16); print(dis(a,int(sys.argv[2]) if len(sys.argv)>2 else 40, int(sys.argv[3]) if len(sys.argv)>3 else 0, int(sys.argv[4]) if len(sys.argv)>4 else 0))
