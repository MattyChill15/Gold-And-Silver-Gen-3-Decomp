import re,subprocess,sys
def load(ws, header, prefix):
    src=subprocess.run(['cpp','-P','-dD','-I',ws+'/include','-I',ws+'/include/constants','-include',ws+'/include/gba/types.h' if False else '/dev/null',ws+'/include/'+header],capture_output=True,text=True).stdout
    env={}
    def ev(e):
        e=e.strip()
        e=re.sub(r'\b([A-Z_][A-Z0-9_]*)\b',lambda m:str(env[m.group(1)]) if m.group(1) in env else m.group(0),e)
        try: return int(eval(e.replace('/','//')))
        except Exception: return None
    for m in re.finditer(r'#define\s+(\w+)\s+(.+)',src):
        v=ev(m.group(2))
        if v is not None: env[m.group(1)]=v
    for body in re.findall(r'enum[^{;]*\{(.*?)\}',src,re.S):
        cur=-1
        for item in body.split(','):
            item=item.strip()
            if not item: continue
            if '=' in item:
                n,e=item.split('=',1); n=n.strip(); v=ev(e)
                if v is None: continue
                cur=v
            else: n=item; cur+=1
            env[n]=cur
        # second pass defines may reference enums
    for m in re.finditer(r'#define\s+(\w+)\s+(.+)',src):
        if m.group(1) not in env:
            v=ev(m.group(2))
            if v is not None: env[m.group(1)]=v
    return {k:v for k,v in env.items() if k.startswith(prefix)}
