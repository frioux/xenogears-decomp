// Post-script: decompiles every function XgSetup created in the program's own
// address range and writes the facts database (see the JSON schema in
// tools/scripts/ghidra_info.py, the consumer).
//
// Args: <output root dir>
//@category Xenogears

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.DataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.pcode.HighFunction;
import ghidra.program.model.pcode.HighParam;
import ghidra.program.model.pcode.HighVariable;
import ghidra.program.model.pcode.JumpTable;
import ghidra.program.model.pcode.PcodeOp;
import ghidra.program.model.pcode.PcodeOpAST;
import ghidra.program.model.pcode.Varnode;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolType;
import ghidra.program.model.lang.Register;
import ghidra.program.model.pcode.VarnodeAST;

public class XgExport extends GhidraScript {
    static final long RAM_START = 0x80000000L, RAM_END = 0x80200000L;
    static final long GP = 0x80059170L;
    static final String SLUS = "slus_006.64";

    String bin;
    long ovStart, ovEnd;
    Program p;
    TreeMap<Long, String> dataSyms = new TreeMap<>();
    long stackSpaceId;

    // ---------------------------------------------------------------- json
    static void jsonStr(StringBuilder sb, String s) {
        sb.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '"' || c == '\\') sb.append('\\').append(c);
            else if (c == '\n') sb.append("\\n");
            else if (c == '\t') sb.append("\\t");
            else if (c < 0x20 || c > 0x7e) sb.append(String.format("\\u%04x", (int) c));
            else sb.append(c);
        }
        sb.append('"');
    }

    @SuppressWarnings("unchecked")
    static void json(StringBuilder sb, Object o) {
        if (o == null) sb.append("null");
        else if (o instanceof String) jsonStr(sb, (String) o);
        else if (o instanceof Map) {
            sb.append('{');
            boolean first = true;
            for (Map.Entry<String, Object> e : ((Map<String, Object>) o).entrySet()) {
                if (!first) sb.append(',');
                first = false;
                jsonStr(sb, e.getKey());
                sb.append(':');
                json(sb, e.getValue());
            }
            sb.append('}');
        } else if (o instanceof List) {
            sb.append('[');
            boolean first = true;
            for (Object x : (List<Object>) o) {
                if (!first) sb.append(',');
                first = false;
                json(sb, x);
            }
            sb.append(']');
        } else sb.append(o.toString());
    }

    static Map<String, Object> obj(Object... kv) {
        Map<String, Object> m = new LinkedHashMap<>();
        for (int i = 0; i < kv.length; i += 2) m.put((String) kv[i], kv[i + 1]);
        return m;
    }

    static String hex8(long a) { return String.format("%08X", a & 0xffffffffL); }

    void write(File f, String s) throws Exception {
        f.getParentFile().mkdirs();
        Files.write(f.toPath(), s.getBytes(StandardCharsets.UTF_8));
    }

    String writeJson(File f, Object o) throws Exception {
        StringBuilder sb = new StringBuilder();
        json(sb, o);
        write(f, sb.toString());
        return null;
    }

    // ------------------------------------------------------------- helpers
    boolean inRam(long a) { return a >= RAM_START && a < RAM_END; }
    boolean inHw(long a) { return (a >= 0x1F800000L && a < 0x1F800400L) || (a >= 0x1F801000L && a < 0x1F803000L); }
    boolean isMem(long a) { return inRam(a) || inHw(a); }

    String nameAt(long a) {
        Address ad = toAddr(a);
        Function f = p.getFunctionManager().getFunctionAt(ad);
        if (f != null) return f.getName();
        Symbol s = p.getSymbolTable().getPrimarySymbol(ad);
        return s == null ? null : s.getName();
    }

    String progOf(long a) { return a >= ovStart && a < ovEnd ? bin : SLUS; }

    long signedConst(Varnode v) {
        long o = v.getOffset();
        int sz = v.getSize();
        if (sz == 4) return (int) o;
        if (sz == 2) return (short) o;
        if (sz == 1) return (byte) o;
        return o;
    }

    // Resolve an absolute address to (symbol base, offset): floor symbol that is
    // not a function and within 0x2000.
    long[] globalBase(long a) {
        Map.Entry<Long, String> e = dataSyms.floorEntry(a);
        if (e != null && a - e.getKey() <= 0x2000
            && p.getFunctionManager().getFunctionContaining(toAddr(e.getKey())) == null) return new long[] {e.getKey(), a - e.getKey()};
        return new long[] {a, 0};
    }

    // --------------------------------------------------------- decomposition
    static class Ptr {
        String base; long off; long stride;
        Ptr(String b, long o, long s) { base = b; off = o; stride = s; }
    }
    static final String CONST = "const", OTHER = "other", STACK = "stack";

    Set<Object> consumed;

    // a varnode in the ram space is a global variable the decompiler mapped directly
    boolean isRamVn(Varnode v) {
        return !v.isConstant() && v.getAddress().getAddressSpace() == p.getAddressFactory().getDefaultAddressSpace()
            && p.getFunctionManager().getFunctionContaining(v.getAddress()) == null;
    }

    Ptr dec(Varnode v, int depth) {
        if (depth > 10) return new Ptr(OTHER, 0, 0);
        if (isRamVn(v)) return new Ptr("deref:" + hex8(v.getOffset()), 0, 0);
        if (v.isConstant()) { consumed.add(v); return new Ptr(CONST, v.getOffset() & 0xffffffffL, 0); }
        if (v.isRegister() && v.isInput()) {
            Register r = p.getRegister(v.getAddress(), v.getSize());
            if (r != null && r.getName().equals("gp")) return new Ptr(CONST, GP, 0);
        }
        if (v.isInput()) {
            HighVariable h = v.getHigh();
            if (h instanceof HighParam) return new Ptr("param:" + ((HighParam) h).getSlot(), 0, 0);
            return new Ptr(OTHER, 0, 0);
        }
        PcodeOp d = v.getDef();
        if (d == null) return new Ptr(OTHER, 0, 0);
        consumed.add(d);
        switch (d.getOpcode()) {
        case PcodeOp.COPY:
        case PcodeOp.CAST:
            return dec(d.getInput(0), depth + 1);
        case PcodeOp.INT_ADD: {
            Ptr a = dec(d.getInput(0), depth + 1), b = dec(d.getInput(1), depth + 1);
            if (b.base.equals(CONST) && !a.base.equals(CONST)) { a.off += b.off; return a; }
            if (a.base.equals(CONST) && !b.base.equals(CONST)) { b.off += a.off; return b; }
            if (a.base.equals(CONST) && b.base.equals(CONST)) return new Ptr(CONST, (a.off + b.off) & 0xffffffffL, 0);
            // base + index: the side that is a scaled index (mult/shift) gives the stride
            long sa = scale(d.getInput(0)), sb = scale(d.getInput(1));
            if (sb != 0) { a.stride = sb; return a; }
            if (sa != 0) { b.stride = sa; return b; }
            if (!a.base.equals(OTHER)) { a.stride = -1; return a; }
            if (!b.base.equals(OTHER)) { b.stride = -1; return b; }
            return new Ptr(OTHER, 0, 0);
        }
        case PcodeOp.INT_SUB: {
            Ptr a = dec(d.getInput(0), depth + 1);
            if (d.getInput(1).isConstant() && !a.base.equals(OTHER)) { a.off -= d.getInput(1).getOffset(); return a; }
            return new Ptr(OTHER, 0, 0);
        }
        case PcodeOp.PTRADD: {
            Ptr a = dec(d.getInput(0), depth + 1);
            Varnode idx = d.getInput(1);
            long sz = d.getInput(2).getOffset();
            if (idx.isConstant()) { consumed.add(idx); a.off += signedConst(idx) * sz; }
            else if (a.stride == 0) a.stride = sz;
            return a;
        }
        case PcodeOp.PTRSUB: {
            Varnode b0 = d.getInput(0);
            if (b0.isConstant() && b0.getOffset() == stackSpaceId) return new Ptr(STACK, 0, 0);
            Ptr a = dec(b0, depth + 1);
            if (d.getInput(1).isConstant()) { consumed.add(d.getInput(1)); a.off += d.getInput(1).getOffset(); }
            return a;
        }
        case PcodeOp.LOAD: {
            Ptr a = dec(d.getInput(1), depth + 1);
            if (a.stride == 0) {
                if (a.base.equals(CONST) && isMem(a.off)) return new Ptr("deref:" + hex8(a.off), 0, 0);
                if (a.base.startsWith("param:")) return new Ptr("deref:" + a.base + "+0x" + Long.toHexString(a.off), 0, 0);
            }
            return new Ptr(OTHER, 0, 0);
        }
        case PcodeOp.CALL:
            return new Ptr("ret:" + hex8(d.getInput(0).getOffset()), 0, 0);
        default:
            return new Ptr(OTHER, 0, 0);
        }
    }

    // stride of v if v = x * c or x << k, else 0
    long scale(Varnode v) {
        PcodeOp d = v.getDef();
        if (d == null) return 0;
        if (d.getOpcode() == PcodeOp.INT_MULT && d.getInput(1).isConstant()) return d.getInput(1).getOffset();
        if (d.getOpcode() == PcodeOp.INT_LEFT && d.getInput(1).isConstant() && d.getInput(1).getOffset() < 32) return 1L << d.getInput(1).getOffset();
        if (d.getOpcode() == PcodeOp.COPY || d.getOpcode() == PcodeOp.CAST) return scale(d.getInput(0));
        return 0;
    }

    // ------------------------------------------------------------ per func
    static class CallSite {
        long target; String kind; long at; Integer nargs; Boolean retUsed; Map<String, Object> constArgs, argBases;
    }

    static class Rec {
        Function f;
        long addr;
        Map<String, Object> json = new LinkedHashMap<>();
        List<CallSite> calls = new ArrayList<>();
        // address -> {access flags, sizes}
        Map<Long, Object[]> globals = new TreeMap<>();
        List<Map<String, Object>> xrefData = new ArrayList<>();
        Map<String, long[]> fields = new LinkedHashMap<>();
        Set<String> fieldBases = new TreeSet<>();
    }

    String ascii(long a) {
        Address ad = toAddr(a);
        MemoryBlock b = p.getMemory().getBlock(ad);
        if (b == null || !b.isInitialized() || p.getFunctionManager().getFunctionContaining(ad) != null) return null;
        StringBuilder sb = new StringBuilder();
        try {
            for (int i = 0; i < 200; i++) {
                int c = p.getMemory().getByte(ad.add(i)) & 0xff;
                if (c == 0) return sb.length() >= 3 ? sb.toString() : null;
                if (c >= 0x20 && c < 0x7f || c == '\n' || c == '\t' || c == '\r') sb.append((char) c);
                else return null;
            }
        } catch (Exception e) { return null; }
        return null;
    }

    void addGlobal(Rec r, long addr, boolean rd, boolean wr, int size, boolean taken) {
        Object[] g = r.globals.get(addr);
        if (g == null) { g = new Object[] {false, false, false, new TreeSet<Integer>()}; r.globals.put(addr, g); }
        if (rd) g[0] = true;
        if (wr) g[1] = true;
        if (taken) g[2] = true;
        if (size > 0) { @SuppressWarnings("unchecked") Set<Integer> s = (Set<Integer>) g[3]; s.add(size); }
    }

    void addField(Rec r, Ptr ptr, int size, boolean write) {
        String base = ptr.base;
        long off = ptr.off;
        if (base.equals(STACK)) return;
        if (base.equals(CONST)) {
            if (!isMem(off)) { base = OTHER; off = 0; }
            else {
                addGlobal(r, off, !write, write, size, false);
                long[] gb = globalBase(off);
                base = "global:" + hex8(gb[0]);
                off = gb[1];
            }
        } else if (base.startsWith("deref:param") || base.startsWith("param:") || base.startsWith("deref:") || base.startsWith("ret:")) {
            // keep
        } else base = OTHER;
        if (base.equals(OTHER)) off = 0;
        r.fieldBases.add(base);
        String key = base + "|" + off + "|" + size + "|" + (write ? "W" : "R") + "|" + ptr.stride;
        long[] c = r.fields.get(key);
        if (c == null) { c = new long[] {0}; r.fields.put(key, c); }
        c[0]++;
    }

    static int accessSize(String mnem) {
        String m = mnem.toLowerCase();
        if (m.startsWith("lb") || m.startsWith("sb")) return 1;
        if (m.startsWith("lh") || m.startsWith("sh")) return 2;
        if (m.startsWith("lw") || m.startsWith("sw")) return 4;
        return 0;
    }

    void listingRefs(Rec r, boolean forGlobals) {
        Function f = r.f;
        for (Instruction ins : p.getListing().getInstructions(f.getBody(), true)) {
            long at = ins.getAddress().getOffset();
            for (Reference ref : ins.getReferencesFrom()) {
                RefType rt = ref.getReferenceType();
                long to = ref.getToAddress().getOffset();
                if (rt.isCall() && !rt.isComputed()) {
                    CallSite c = new CallSite();
                    c.target = to; c.kind = "call"; c.at = at;
                    r.calls.add(c);
                } else if (rt.isJump() && !rt.isComputed() && !f.getBody().contains(ref.getToAddress())
                    && p.getFunctionManager().getFunctionAt(ref.getToAddress()) != null) {
                    CallSite c = new CallSite();
                    c.target = to; c.kind = "tail"; c.at = at;
                    r.calls.add(c);
                } else if (forGlobals && isMem(to) && !rt.isFlow()) {
                    int sz = accessSize(ins.getMnemonicString());
                    boolean rd = rt.isRead(), wr = rt.isWrite();
                    addGlobal(r, to, rd, wr, sz, !rd && !wr);
                }
            }
        }
    }

    CallSite findSite(List<CallSite> cs, long at, long target) {
        for (CallSite c : cs) if (c.target == target && (c.at == at || c.at == at - 4 || c.at == at + 4)) return c;
        return null;
    }

    void analyse(Rec r, DecompInterface ifc, Map<String, Object> failures) throws Exception {
        Function f = r.f;
        listingRefs(r, false);
        DecompileResults res = ifc.decompileFunction(f, 90, monitor);
        HighFunction hf = res.decompileCompleted() ? res.getHighFunction() : null;
        List<Object> indirect = new ArrayList<>();
        List<Object> jts = new ArrayList<>();
        List<Object> funcRefs = new ArrayList<>();
        Map<String, Object> strings = new LinkedHashMap<>();
        Map<String, Object> sig = null;
        String cText = null;
        if (hf == null) {
            failures.put(hex8(r.addr), res.isTimedOut() ? "timeout" : String.valueOf(res.getErrorMessage()));
            listingRefs(r, true);
        } else {
            cText = res.getDecompiledFunction() == null ? null : res.getDecompiledFunction().getC();
            consumed = java.util.Collections.newSetFromMap(new IdentityHashMap<>());
            // signature
            ghidra.program.model.pcode.FunctionPrototype fp = hf.getFunctionPrototype();
            DataType rt = fp.getReturnType();
            boolean retVal = rt != null && !rt.isZeroLength() && !(rt instanceof ghidra.program.model.data.VoidDataType);
            int np = hf.getLocalSymbolMap().getNumParams();
            List<Object> params = new ArrayList<>();
            StringBuilder ps = new StringBuilder();
            for (int i = 0; i < np; i++) {
                ghidra.program.model.pcode.HighSymbol hp = hf.getLocalSymbolMap().getParamSymbol(i);
                ghidra.program.model.listing.VariableStorage st = hp.getStorage();
                String loc;
                if (st.isStackStorage()) loc = "stack+0x" + Integer.toHexString(st.getStackOffset());
                else if (st.getRegister() != null) loc = st.getRegister().getName();
                else loc = st.toString();
                params.add(obj("index", i, "storage", loc, "size", hp.getSize(), "type", hp.getDataType().getName()));
                if (i > 0) ps.append(", ");
                ps.append(hp.getDataType().getName()).append(' ').append(hp.getName());
            }
            String proto = (retVal ? rt.getName() : "void") + " " + f.getName() + "(" + (np == 0 ? "void" : ps) + ")";
            sig = obj("prototype", proto, "nparams", np, "params", params, "returns_value", retVal);

            Iterator<PcodeOpAST> it = hf.getPcodeOps();
            List<PcodeOpAST> ops = new ArrayList<>();
            while (it.hasNext()) ops.add(it.next());
            for (PcodeOpAST op : ops) {
                int oc = op.getOpcode();
                long at = op.getSeqnum().getTarget().getOffset();
                if (oc == PcodeOp.LOAD || oc == PcodeOp.STORE) {
                    Ptr ptr = dec(op.getInput(1), 0);
                    consumed.add(op);
                    int size = oc == PcodeOp.LOAD ? op.getOutput().getSize() : op.getInput(2).getSize();
                    addField(r, ptr, size, oc == PcodeOp.STORE);
                }
            }
            if (hex8(r.addr).equals(System.getenv("XG_DEBUG_FUNC"))) for (PcodeOpAST op : ops) println("pcode " + op);
            for (PcodeOpAST op : ops) {
                int oc = op.getOpcode();
                if (oc == PcodeOp.INDIRECT) {
                    // call side-effect modelling; but INDIRECT(ram var <- other varnode) is
                    // how the decompiler spells "the call's result is stored to this global"
                    Varnode io = op.getOutput();
                    if (io != null && isRamVn(io) && isMem(io.getOffset()) && !(isRamVn(op.getInput(0)) && op.getInput(0).getOffset() == io.getOffset()))
                        addField(r, new Ptr(CONST, io.getOffset(), 0), io.getSize(), true);
                    continue;
                }
                if (oc == PcodeOp.COPY && isRamVn(op.getInput(0)) && op.getOutput() != null && isRamVn(op.getOutput())
                    && op.getInput(0).getOffset() == op.getOutput().getOffset()) continue; // return-time global flush
                boolean mem = oc == PcodeOp.LOAD || oc == PcodeOp.STORE;
                boolean ctl = oc == PcodeOp.BRANCH || oc == PcodeOp.CBRANCH || oc == PcodeOp.CALL;
                for (int i = mem ? 2 : ctl ? 1 : 0; i < op.getNumInputs(); i++) {
                    Varnode v = op.getInput(i);
                    if (isRamVn(v) && isMem(v.getOffset()))
                        addField(r, new Ptr(CONST, v.getOffset(), 0), v.getSize(), false);
                }
                Varnode ov = op.getOutput();
                if (ov != null && isRamVn(ov) && isMem(ov.getOffset()))
                    addField(r, new Ptr(CONST, ov.getOffset(), 0), ov.getSize(), true);
            }
            for (PcodeOpAST op : ops) {
                int oc = op.getOpcode();
                long at = op.getSeqnum().getTarget().getOffset();
                if (oc == PcodeOp.CALL || oc == PcodeOp.CALLIND) {
                    int n = op.getNumInputs() - 1;
                    boolean used = op.getOutput() != null && op.getOutput().getDescendants().hasNext();
                    Map<String, Object> ca = new LinkedHashMap<>();
                    for (int i = 1; i <= n; i++) {
                        Varnode a = op.getInput(i);
                        if (a.getDef() != null && (a.getDef().getOpcode() == PcodeOp.COPY || a.getDef().getOpcode() == PcodeOp.CAST)
                            && a.getDef().getInput(0).isConstant()) a = a.getDef().getInput(0);
                        if (a.isConstant()) {
                            long v = a.getOffset() & (a.getSize() >= 8 ? -1L : ((1L << (8 * a.getSize())) - 1));
                            if (a.getSize() == 4 && v >= 0xFFFF8000L) v = (int) v;
                            ca.put(String.valueOf(i - 1), v);
                        }
                    }
                    Map<String, Object> ab = new LinkedHashMap<>();
                    Set<Object> keep = consumed;
                    consumed = java.util.Collections.newSetFromMap(new IdentityHashMap<>());
                    for (int i = 1; i <= n; i++) {
                        Varnode a = op.getInput(i);
                        if (a.isConstant()) continue;
                        Ptr x = dec(a, 0);
                        if (x.stride != 0) continue;
                        String b = null;
                        if (x.base.startsWith("param:") || x.base.startsWith("deref:") || x.base.startsWith("ret:")) b = x.base;
                        else if (x.base.equals(CONST) && isMem(x.off)) b = "global:" + hex8(x.off);
                        if (b == null) continue;
                        if (x.off != 0 && !x.base.equals(CONST)) b += "+0x" + Long.toHexString(x.off);
                        ab.put(String.valueOf(i - 1), b);
                    }
                    consumed = keep;
                    if (oc == PcodeOp.CALL) {
                        long tgt = op.getInput(0).getOffset();
                        CallSite c = findSite(r.calls, at, tgt);
                        if (c == null) {
                            c = new CallSite();
                            c.target = tgt; c.kind = "tail"; c.at = at;
                            r.calls.add(c);
                        }
                        c.nargs = n; c.retUsed = used; c.constArgs = ca; c.argBases = ab;
                    } else {
                        // CALLIND: input0 is the target pointer, args follow
                        Ptr tp = dec(op.getInput(0), 0);
                        String via = null;
                        PcodeOp d = op.getInput(0).getDef();
                        Map<String, Object> m = obj("at", hex8(at), "nargs", n, "ret_used", used, "const_args", ca, "arg_bases", ab);
                        if (d != null && d.getOpcode() == PcodeOp.LOAD) {
                            Ptr src = dec(d.getInput(1), 0);
                            m.put("via", obj("base", classify(src), "offset", srcOff(src), "stride", src.stride));
                        } else m.put("via", obj("base", classify(tp), "offset", srcOff(tp), "stride", tp.stride));
                        indirect.add(m);
                    }
                }
            }
            // jump tables
            JumpTable[] jtl = hf.getJumpTables();
            if (jtl != null) for (JumpTable jt : jtl) {
                Address[] cases = jt.getCases();
                Set<Long> uniq = new TreeSet<>();
                for (Address c : cases) uniq.add(c.getOffset());
                Map<String, Object> m = obj("at", hex8(jt.getSwitchAddress().getOffset()), "ncases", cases.length, "ntargets", uniq.size());
                jts.add(m);
            }
            // address-taken: constants in RAM nobody consumed through a LOAD/STORE chain
            Set<Long> seenConst = new HashSet<>();
            for (PcodeOpAST op : ops) {
                if (consumed.contains(op)) continue;
                int oc = op.getOpcode();
                for (int i = 0; i < op.getNumInputs(); i++) {
                    Varnode v = op.getInput(i);
                    long a = -1;
                    if (v.isConstant() && !consumed.contains(v)) a = v.getOffset() & 0xffffffffL;
                    else if (oc == PcodeOp.INT_ADD && v.isRegister() && v.isInput() && i == 0
                        && op.getInput(1).isConstant() && "gp".equals(regName(v))) a = (GP + op.getInput(1).getOffset()) & 0xffffffffL;
                    if (a < 0 || v.getSize() != 4 && !(v.isRegister())) continue;
                    if (oc == PcodeOp.CALL && i == 0) continue;
                    if (oc == PcodeOp.PTRSUB && i == 0) continue;
                    if (!isMem(a) || !seenConst.add(a * 64 + oc)) continue;
                    long at = op.getSeqnum().getTarget().getOffset();
                    Function tf = p.getFunctionManager().getFunctionAt(toAddr(a));
                    if (tf != null) {
                        funcRefs.add(obj("addr", hex8(a), "at", hex8(at)));
                        continue;
                    }
                    String s = ascii(a);
                    if (s != null) {
                        strings.put(hex8(a), s.length() > 160 ? s.substring(0, 160) : s);
                        continue;
                    }
                    if (inRam(a) && a < 0x80010000L) continue; // small numbers, not addresses
                    addGlobal(r, a, false, false, 0, true);
                }
            }
            // direct string loads through LOAD chains (array-of-chars base) are plain globals already
            for (Long a : new ArrayList<>(r.globals.keySet())) {
                String s = ascii(a);
                if (s != null && !strings.containsKey(hex8(a))) strings.put(hex8(a), s.length() > 160 ? s.substring(0, 160) : s);
            }
        }
        // record
        List<Object> callees = new ArrayList<>();
        for (CallSite c : r.calls) {
            Map<String, Object> m = obj("addr", hex8(c.target), "name", nameAt(c.target), "at", hex8(c.at),
                "bin", progOf(c.target), "kind", c.kind, "nargs", c.nargs, "ret_used", c.retUsed,
                "const_args", c.constArgs, "arg_bases", c.argBases);
            callees.add(m);
        }
        List<Object> globals = new ArrayList<>();
        for (Map.Entry<Long, Object[]> e : r.globals.entrySet()) {
            Object[] g = e.getValue();
            boolean rd = (Boolean) g[0], wr = (Boolean) g[1], tk = (Boolean) g[2];
            String acc = rd && wr ? "RW" : rd ? "R" : wr ? "W" : "addr-taken";
            @SuppressWarnings("unchecked") Set<Integer> sizes = (Set<Integer>) g[3];
            Map<String, Object> m = obj("addr", hex8(e.getKey()), "name", nameAt(e.getKey()), "access", acc,
                "sizes", new ArrayList<Object>(sizes), "addr_taken", tk);
            m.put("symbol", symBase(e.getKey()));
            globals.add(m);
        }
        List<Object> fields = new ArrayList<>();
        for (Map.Entry<String, long[]> e : r.fields.entrySet()) {
            String[] k = e.getKey().split("\\|");
            Map<String, Object> m = obj("base", k[0], "offset", Long.parseLong(k[1]), "size", Integer.parseInt(k[2]),
                "rw", k[3], "count", e.getValue()[0]);
            if (!k[4].equals("0")) m.put("stride", Long.parseLong(k[4]));
            fields.add(m);
        }
        int frame = stackFrame(f);
        if (sig != null) sig.put("stack_frame", frame);
        Map<String, Object> j = r.json;
        j.put("addr", hex8(r.addr));
        j.put("bin", bin);
        j.put("size", f.getBody().getNumAddresses());
        j.put("name", f.getName());
        j.put("decompile_ok", hf != null);
        j.put("stack_frame", frame);
        j.put("thunk", isThunk(f));
        j.put("signature", sig);
        j.put("callers", new ArrayList<Object>()); // filled in later
        j.put("callees", callees);
        j.put("indirect_calls", indirect);
        j.put("globals", globals);
        j.put("func_refs", funcRefs);
        List<Object> sl = new ArrayList<>();
        for (Map.Entry<String, Object> e : strings.entrySet()) sl.add(obj("addr", e.getKey(), "text", e.getValue()));
        j.put("strings", sl);
        j.put("jump_tables", jts);
        j.put("field_accesses", fields);
        r.json.put("_c", cText);
    }

    String regName(Varnode v) {
        Register rg = p.getRegister(v.getAddress(), v.getSize());
        return rg == null ? null : rg.getName();
    }

    String symBase(long a) {
        long[] gb = globalBase(a);
        return hex8(gb[0]) + (gb[1] != 0 ? "+0x" + Long.toHexString(gb[1]) : "");
    }

    String classify(Ptr x) {
        if (x.base.equals(CONST)) {
            if (isMem(x.off)) { long[] gb = globalBase(x.off); return "global:" + hex8(gb[0]); }
            return "other";
        }
        return x.base;
    }

    long srcOff(Ptr x) {
        if (x.base.equals(CONST) && isMem(x.off)) return globalBase(x.off)[1];
        return x.base.equals(OTHER) ? 0 : x.off;
    }

    // BIOS/syscall stub: tiny, uses `break`/`syscall` or jumps through a register other than ra, so
    // Ghidra cannot see which arguments it consumes (its nparams/nargs are unreliable)
    boolean isThunk(Function f) {
        if (f.getBody().getNumAddresses() > 0x20) return false;
        boolean jr = false;
        for (Instruction ins : p.getListing().getInstructions(f.getBody(), true)) {
            String m = ins.getMnemonicString();
            if (m.equals("jr") && !"ra".equals(String.valueOf(ins.getRegister(0))) || m.equals("break") || m.equals("syscall")) jr = true;
            if (m.startsWith("jal")) return false;
        }
        return jr;
    }

    int stackFrame(Function f) {
        int n = 0;
        for (Instruction ins : p.getListing().getInstructions(f.getBody(), true)) {
            if (++n > 8) break;
            if (ins.getMnemonicString().startsWith("addiu") && "sp".equals(String.valueOf(ins.getRegister(0)))
                && "sp".equals(String.valueOf(ins.getRegister(1)))) {
                ghidra.program.model.scalar.Scalar sc = ins.getScalar(2);
                if (sc != null && sc.getSignedValue() < 0) return (int) -sc.getSignedValue();
            }
        }
        return 0;
    }

    // ----------------------------------------------------------------- run
    @Override
    public void run() throws Exception {
        long t0 = System.currentTimeMillis();
        p = currentProgram;
        File outRoot = new File(getScriptArgs()[0]);
        var opts = p.getOptions("Xenogears");
        bin = opts.getString("bin", null);
        ovStart = Long.parseLong(opts.getString("ovStart", null), 16);
        ovEnd = Long.parseLong(opts.getString("ovEnd", null), 16);
        stackSpaceId = p.getAddressFactory().getStackSpace().getSpaceID();
        File out = new File(outRoot, bin);

        SymbolIterator si = p.getSymbolTable().getAllSymbols(true);
        while (si.hasNext()) {
            Symbol s = si.next();
            if (s.getSymbolType() == SymbolType.LABEL && isMem(s.getAddress().getOffset()) && s.getSource() == ghidra.program.model.symbol.SourceType.IMPORTED)
                dataSyms.putIfAbsent(s.getAddress().getOffset(), s.getName());
        }
        // functions' entry labels are not data bases
        for (Function f : p.getFunctionManager().getFunctions(true)) dataSyms.remove(f.getEntryPoint().getOffset());

        List<Rec> recs = new ArrayList<>();
        for (Function f : p.getFunctionManager().getFunctions(true)) {
            long a = f.getEntryPoint().getOffset();
            if (a < ovStart || a >= ovEnd) continue;
            boolean tagged = false;
            for (var t : f.getTags()) if (t.getName().equals("xg")) tagged = true;
            if (!tagged) continue;
            Rec r = new Rec();
            r.f = f; r.addr = a;
            recs.add(r);
        }

        DecompInterface ifc = new DecompInterface();
        ifc.setOptions(new DecompileOptions());
        ifc.setSimplificationStyle("decompile");
        if (!ifc.openProgram(p)) throw new RuntimeException("decompiler open failed");

        Map<String, Object> failures = new LinkedHashMap<>();
        Map<Long, Rec> byAddr = new HashMap<>();
        int done = 0;
        for (Rec r : recs) {
            byAddr.put(r.addr, r);
            try {
                analyse(r, ifc, failures);
            } catch (Exception e) {
                failures.put(hex8(r.addr), "exception " + e);
                r.json.clear();
                r.json.put("addr", hex8(r.addr)); r.json.put("bin", bin); r.json.put("size", r.f.getBody().getNumAddresses());
                r.json.put("name", r.f.getName()); r.json.put("decompile_ok", false); r.json.put("signature", null);
                for (String k : new String[] {"callers", "callees", "indirect_calls", "globals", "func_refs", "strings", "jump_tables", "field_accesses"})
                    r.json.put(k, new ArrayList<Object>());
                println("analyse " + hex8(r.addr) + ": " + e);
            }
            if (++done % 500 == 0) println("  " + bin + ": " + done + "/" + recs.size());
        }
        ifc.dispose();

        // callers (same program) and references into SLUS
        Map<Long, List<Object>> callers = new HashMap<>();
        Map<String, List<Object>> slusXrefs = new TreeMap<>();
        for (Rec r : recs) {
            String fn = r.f.getName();
            for (CallSite c : r.calls) {
                Map<String, Object> m = obj("from_func_addr", hex8(r.addr), "from_name", fn, "bin", bin, "at", hex8(c.at),
                    "kind", c.kind, "nargs", c.nargs, "ret_used", c.retUsed, "const_args", c.constArgs, "arg_bases", c.argBases);
                if (c.target >= ovStart && c.target < ovEnd) callers.computeIfAbsent(c.target, k -> new ArrayList<>()).add(m);
                else if (!bin.equals(SLUS)) slusXrefs.computeIfAbsent(hex8(c.target), k -> new ArrayList<>()).add(m);
            }
            if (!bin.equals(SLUS)) {
                @SuppressWarnings("unchecked") List<Object> gl = (List<Object>) r.json.get("globals");
                for (Object o : gl) {
                    @SuppressWarnings("unchecked") Map<String, Object> g = (Map<String, Object>) o;
                    long a = Long.parseLong((String) g.get("addr"), 16);
                    if (a >= ovStart && a < ovEnd) continue;
                    slusXrefs.computeIfAbsent((String) g.get("addr"), k -> new ArrayList<>())
                        .add(obj("from_func_addr", hex8(r.addr), "from_name", fn, "bin", bin, "kind", "data",
                            "access", g.get("access"), "sizes", g.get("sizes")));
                }
                @SuppressWarnings("unchecked") List<Object> fr = (List<Object>) r.json.get("func_refs");
                for (Object o : fr) {
                    @SuppressWarnings("unchecked") Map<String, Object> g = (Map<String, Object>) o;
                    long a = Long.parseLong((String) g.get("addr"), 16);
                    if (a >= ovStart && a < ovEnd) continue;
                    slusXrefs.computeIfAbsent((String) g.get("addr"), k -> new ArrayList<>())
                        .add(obj("from_func_addr", hex8(r.addr), "from_name", fn, "bin", bin, "kind", "funcptr", "at", g.get("at")));
                }
            }
        }

        // write
        Map<String, Object> index = new LinkedHashMap<>();
        Map<String, Object> idxFuncs = new LinkedHashMap<>();
        Map<String, List<Object>> fieldBases = new TreeMap<>();
        for (Rec r : recs) {
            @SuppressWarnings("unchecked") List<Object> cl = (List<Object>) r.json.get("callers");
            List<Object> got = callers.get(r.addr);
            if (got != null) cl.addAll(got);
            String c = (String) r.json.remove("_c");
            String a = hex8(r.addr);
            writeJson(new File(out, a + ".json"), r.json);
            write(new File(out, a + ".c"), c == null ? "// decompilation failed\n" : c);
            List<Object> ce = new ArrayList<>();
            Set<String> seen = new TreeSet<>();
            for (CallSite cs : r.calls) if (seen.add(progOf(cs.target) + ":" + hex8(cs.target))) ce.add(progOf(cs.target) + ":" + hex8(cs.target));
            @SuppressWarnings("unchecked") Map<String, Object> sg = (Map<String, Object>) r.json.get("signature");
            idxFuncs.put(a, obj("name", r.f.getName(), "size", r.json.get("size"),
                "nparams", sg == null ? null : sg.get("nparams"), "callees", ce));
            for (String b : r.fieldBases) fieldBases.computeIfAbsent(b, k -> new ArrayList<>()).add(a);
        }
        index.put("bin", bin);
        index.put("ov_start", hex8(ovStart));
        index.put("ov_end", hex8(ovEnd));
        index.put("funcs", idxFuncs);
        index.put("field_bases", new LinkedHashMap<String, Object>(fieldBases));
        writeJson(new File(out, "_index.json"), index);
        if (!bin.equals(SLUS)) writeJson(new File(out, "slus_xrefs.json"), new LinkedHashMap<String, Object>(slusXrefs));
        writeJson(new File(out, "_failures.json"), failures);

        int elfFuncs = recs.size();
        println("XGSUMMARY " + bin + " functions=" + recs.size() + " elf_FUNC_syms(sized,in-range/all)=" + opts.getString("elfFuncs", "?") + " decompile_failures=" + failures.size()
            + " export_seconds=" + (System.currentTimeMillis() - t0) / 1000);
    }
}
