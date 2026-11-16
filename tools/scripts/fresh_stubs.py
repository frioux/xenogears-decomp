import json,os,re,subprocess,collections,glob
funcs=json.loads(subprocess.run(["python3","tools/scripts/select_target.py","--json","--limit","20000"],capture_output=True,text=True).stdout)
byfile=collections.defaultdict(list)
for f in funcs: byfile[f["file"]].append(f)
def atfree(unit):
    b=unit.split("/")[0]; d="asm/%s/nonmatchings/%s"%(b,"/".join(unit.split("/")[1:]))
    try: out=subprocess.run(["python3","tools/scripts/atfree.py",unit,d],capture_output=True,text=True).stdout
    except Exception: return set()
    return set(re.findall(r'\S+ (func_\w+):',out))
# Units on gcc 2.7.2-cdk with -G0: there the "atfree" pattern is what the compiler emits, not a wall.
cdk0=set()
for blk in open("gears.toml").read().split("[[build_preset]]"):
    if "gcc-2.7.2-cdk-psx" in blk and re.search(r'gp_flag\s*=\s*"-G0"',blk):
        cdk0.update(x.rstrip(".") for x in re.findall(r'"([\w./]+?\.)"',blk))
def owns_tables(unit):
    # own_rodata.py gave the unit its jump tables: a C switch there links (AGENTS.md).
    b,rest=unit.split("/",1)
    try: return re.search(r"\.rodata, %s\]"%re.escape(rest),open("config/%s.yaml"%b).read()) is not None
    except OSError: return False
res={}
for path,fs in byfile.items():
    if not os.path.exists(path): continue
    unit=path[4:-2]
    if "/psyq/" in path or unit=="slus_006.64/system/model": continue
    L=open(path).read().split("\n")
    b=unit.split("/")[0]; d="asm/%s/nonmatchings/%s"%(b,"/".join(unit.split("/")[1:]))
    jt=set(os.path.basename(p)[:-2] for p in glob.glob(d+"/*.s") if "jtbl" in open(p,errors="ignore").read())
    # A stub that stores $sp is a stack switch (alloca/coroutine) - never convertible to C.
    swsp=set(os.path.basename(p)[:-2] for p in glob.glob(d+"/*.s") if re.search(r'\b(sw|sd)\s+\$sp', open(p,errors="ignore").read()))
    af=set() if unit in cdk0 else atfree(unit)
    if owns_tables(unit): jt=set()
    for f in fs:
        n=f["function"]
        if n in jt or n in af or n in swsp: continue
        idx=[i for i,l in enumerate(L) if "INCLUDE_ASM" in l and re.search(r'[,(\s]%s\s*[,)]'%re.escape(n),l)]
        if not idx: continue
        i=idx[0]
        win=L[max(0,i-60):i]
        if any("%" in l for l in win): continue          # park note near
        if any(re.search(r'\b%s\s*\('%re.escape(n),l) for l in win): continue  # parked body above
        nxt=[j for j in range(i+1,len(L)) if "INCLUDE_ASM" in L[j]]
        below=L[i+1:(nxt[0] if nxt else len(L))]
        if any("%" in l for l in below): continue        # park note below its own stub
        if any(re.search(r'\b%s\s*\('%re.escape(n),l) for l in below): continue  # parked body below
        res.setdefault((unit,b),[]).append((f["insns"],n,path))
rows=sorted(res.items(),key=lambda kv:(-len(kv[1]),sum(x[0] for x in kv[1])/len(kv[1])))
print("total units",len(rows),"stubs",sum(len(v) for _,v in rows))
for (unit,b),st in rows[:28]:
    st=sorted(st)
    print("n=%-2d avg%6.1f %-34s %s"%(len(st),sum(x[0] for x in st)/len(st),unit," ".join("%s:%d"%(n,i) for i,n,_ in st)))
