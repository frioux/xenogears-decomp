import json,os,re,subprocess,collections,glob
funcs=json.loads(subprocess.run(["python3","tools/scripts/select_target.py","--json","--limit","20000"],capture_output=True,text=True).stdout)
byfile=collections.defaultdict(list)
for f in funcs: byfile[f["file"]].append(f)
def atfree(unit):
    b=unit.split("/")[0]; d="asm/%s/nonmatchings/%s"%(b,"/".join(unit.split("/")[1:]))
    try: out=subprocess.run(["python3","tools/scripts/atfree.py",unit,d],capture_output=True,text=True).stdout
    except Exception: return set()
    return set(re.findall(r'\S+ (func_\w+):',out))
res={}
for path,fs in byfile.items():
    if not os.path.exists(path): continue
    unit=path[4:-2]
    if "/psyq/" in path or unit=="slus_006.64/system/model": continue
    L=open(path).read().split("\n")
    b=unit.split("/")[0]; d="asm/%s/nonmatchings/%s"%(b,"/".join(unit.split("/")[1:]))
    jt=set(os.path.basename(p)[:-2] for p in glob.glob(d+"/*.s") if "jtbl" in open(p,errors="ignore").read())
    af=atfree(unit)
    for f in fs:
        n=f["function"]
        if n in jt or n in af: continue
        idx=[i for i,l in enumerate(L) if "INCLUDE_ASM" in l and re.search(r'[,(\s]%s\s*[,)]'%re.escape(n),l)]
        if not idx: continue
        i=idx[0]
        win=L[max(0,i-60):i]
        if any("%" in l for l in win): continue          # park note near
        if any(re.search(r'\b%s\s*\('%re.escape(n),l) for l in win): continue  # parked body above
        res.setdefault((unit,b),[]).append((f["insns"],n,path))
rows=sorted(res.items(),key=lambda kv:(-len(kv[1]),sum(x[0] for x in kv[1])/len(kv[1])))
print("total units",len(rows),"stubs",sum(len(v) for _,v in rows))
for (unit,b),st in rows[:28]:
    st=sorted(st)
    print("n=%-2d avg%6.1f %-34s %s"%(len(st),sum(x[0] for x in st)/len(st),unit," ".join("%s:%d"%(n,i) for i,n,_ in st)))
