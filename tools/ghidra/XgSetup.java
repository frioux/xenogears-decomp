// Pre-script: builds the program's memory map, symbols and functions from the
// build/out/<bin>.elf files (the ELF loader is not used: the programs are
// imported as a dummy raw file and replaced here).
//
// Args: <bin> <elf path> <slus elf path | "-">
//
// For an overlay the main exe (SLUS) is mapped too (code/data, symbols and
// functions) so calls and data references into it resolve.  Overlay vs SLUS
// ownership is recorded in the program options (category "Xenogears") for
// XgExport.
//@category Xenogears

import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Program;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SourceType;
import ghidra.util.task.TaskMonitor;

public class XgSetup extends GhidraScript {
    static final long RAM_START = 0x80000000L, RAM_END = 0x80200000L;
    static final long GP = 0x80059170L;

    static class Seg { long vaddr, filesz, memsz; long off; }
    static class Sym { String name; long addr, size; int type, shndx; }
    static class Elf {
        ByteBuffer b;
        List<Seg> segs = new ArrayList<>();
        List<Sym> syms = new ArrayList<>();
        Elf(String path) throws Exception {
            b = ByteBuffer.wrap(Files.readAllBytes(Paths.get(path))).order(ByteOrder.LITTLE_ENDIAN);
            if (b.getInt(0) != 0x464c457f) throw new Exception("not an ELF: " + path);
            int phoff = b.getInt(0x1c), shoff = b.getInt(0x20);
            int phentsize = b.getShort(0x2a) & 0xffff, phnum = b.getShort(0x2c) & 0xffff;
            int shentsize = b.getShort(0x2e) & 0xffff, shnum = b.getShort(0x30) & 0xffff;
            for (int i = 0; i < phnum; i++) {
                int p = phoff + i * phentsize;
                if (b.getInt(p) != 1) continue; // PT_LOAD
                Seg s = new Seg();
                s.off = b.getInt(p + 4) & 0xffffffffL;
                s.vaddr = b.getInt(p + 8) & 0xffffffffL;
                s.filesz = b.getInt(p + 16) & 0xffffffffL;
                s.memsz = b.getInt(p + 20) & 0xffffffffL;
                if (s.vaddr >= RAM_START) segs.add(s);
            }
            for (int i = 0; i < shnum; i++) {
                int p = shoff + i * shentsize;
                if (b.getInt(p + 4) != 2) continue; // SHT_SYMTAB
                int off = b.getInt(p + 16), size = b.getInt(p + 20), link = b.getInt(p + 24);
                int strOff = b.getInt(shoff + link * shentsize + 16);
                for (int q = off + 16; q < off + size; q += 16) {
                    Sym s = new Sym();
                    int nameOff = b.getInt(q);
                    s.addr = b.getInt(q + 4) & 0xffffffffL;
                    s.size = b.getInt(q + 8) & 0xffffffffL;
                    s.type = b.get(q + 12) & 0xf;
                    s.shndx = b.getShort(q + 14) & 0xffff;
                    int e = strOff + nameOff;
                    StringBuilder sb = new StringBuilder();
                    while (b.get(e) != 0) sb.append((char) b.get(e++));
                    s.name = sb.toString();
                    syms.add(s);
                }
            }
        }
        byte[] bytes(Seg s) {
            byte[] d = new byte[(int) s.filesz];
            b.position((int) s.off);
            b.get(d);
            return d;
        }
    }

    static boolean junk(Sym s) {
        return s.name.isEmpty() || s.type == 4 || s.type == 3
            || s.name.equals("gcc2_compiled.") || s.name.equals("__gnu_compiled_c");
    }

    Memory mem;

    // Map [start, start+len) (bytes==null: uninitialized), skipping what is
    // already mapped.
    void map(String name, long start, long len, byte[] bytes) throws Exception {
        long end = start + len, cur = start;
        while (cur < end) {
            MemoryBlock hit = mem.getBlock(toAddr(cur));
            if (hit != null) { cur = hit.getEnd().getOffset() + 1; continue; }
            long next = end;
            for (MemoryBlock blk : mem.getBlocks()) {
                long bs = blk.getStart().getOffset();
                if (bs > cur && bs < next) next = bs;
            }
            MemoryBlock nb;
            String n = name + "_" + Long.toHexString(cur);
            if (bytes != null) {
                int o = (int) (cur - start), l = (int) (next - cur);
                nb = mem.createInitializedBlock(n, toAddr(cur), new ByteArrayInputStream(bytes, o, l), l, monitor, false);
            } else {
                nb = mem.createUninitializedBlock(n, toAddr(cur), next - cur, false);
            }
            nb.setRead(true); nb.setWrite(true); nb.setExecute(true);
            cur = next;
        }
    }

    void mapElf(String prefix, Elf e) throws Exception {
        for (Seg s : e.segs) {
            if (s.filesz > 0) map(prefix, s.vaddr, s.filesz, e.bytes(s));
            if (s.memsz > s.filesz) map(prefix + "_bss", s.vaddr + s.filesz, s.memsz - s.filesz, null);
        }
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        String bin = a[0];
        boolean isSlus = a[2].equals("-");
        Program p = currentProgram;
        mem = p.getMemory();
        for (MemoryBlock blk : mem.getBlocks()) mem.removeBlock(blk, monitor);

        Elf own = new Elf(a[1]);
        Elf slus = isSlus ? null : new Elf(a[2]);
        long ovStart = RAM_END, ovEnd = 0;
        for (Seg s : own.segs) { ovStart = Math.min(ovStart, s.vaddr); ovEnd = Math.max(ovEnd, s.vaddr + s.memsz); }
        if (isSlus) { ovStart = 0; }

        mapElf(isSlus ? "slus" : bin, own);
        if (!isSlus) mapElf("slus", slus);
        map("ram", RAM_START, RAM_END - RAM_START, null);
        mem.createUninitializedBlock("scratchpad", toAddr(0x1F800000L), 0x400, false);
        MemoryBlock io = mem.createUninitializedBlock("io", toAddr(0x1F801000L), 0x2000, false);
        io.setVolatile(true);
        for (MemoryBlock blk : mem.getBlocks()) { blk.setRead(true); blk.setWrite(true); blk.setExecute(true); }

        // gp program-wide
        Register gp = p.getRegister("gp");
        p.getProgramContext().setValue(gp, toAddr(RAM_START), toAddr(RAM_END - 1), BigInteger.valueOf(GP));

        p.getOptions(Program.ANALYSIS_PROPERTIES).setBoolean("Apply Data Archives", false);
        p.getOptions(Program.ANALYSIS_PROPERTIES).setBoolean("Decompiler Parameter ID", true);
        var opts = p.getOptions("Xenogears");
        opts.setString("bin", bin);
        opts.setString("ovStart", Long.toHexString(ovStart));
        opts.setString("ovEnd", Long.toHexString(ovEnd));

        int elfFuncs = 0, elfFuncsAll = 0;
        for (Sym s : own.syms) {
            if (s.type != 2) continue;
            elfFuncsAll++;
            if (s.size > 0 && s.addr >= ovStart && s.addr < ovEnd) elfFuncs++;
        }
        opts.setString("elfFuncs", elfFuncs + "/" + elfFuncsAll);

        // symbols: overlay's own first, then SLUS ones below the overlay start
        List<Sym> funcs = new ArrayList<>();
        int labels = 0;
        List<Sym> all = new ArrayList<>();
        for (Sym s : own.syms) if (!junk(s) && s.addr >= ovStart) all.add(s);
        if (!isSlus) for (Sym s : slus.syms) if (!junk(s) && s.addr < ovStart) all.add(s);
        for (Sym s : all) {
            Address ad = toAddr(s.addr);
            if (!mem.contains(ad)) continue;
            try {
                p.getSymbolTable().createLabel(ad, s.name, SourceType.IMPORTED);
                labels++;
            } catch (Exception ex) { println("label " + s.name + ": " + ex); }
            if (s.type == 2 && s.size > 0 && mem.getBlock(ad).isInitialized()) funcs.add(s);
        }

        int made = 0, failed = 0;
        for (Sym s : funcs) {
            Address ad = toAddr(s.addr);
            AddressSet body = new AddressSet(ad, toAddr(s.addr + s.size - 1));
            try {
                new DisassembleCommand(ad, body, true).applyTo(p, monitor);
                Function f = p.getFunctionManager().getFunctionAt(ad);
                if (f == null) {
                    f = p.getFunctionManager().createFunction(s.name, ad, body, SourceType.IMPORTED);
                    made++;
                }
                f.addTag("xg");
            } catch (Exception ex) {
                failed++;
                println("function " + s.name + " @" + ad + ": " + ex);
            }
        }
        println("XgSetup " + bin + ": blocks=" + mem.getBlocks().length + " labels=" + labels
            + " functions=" + made + " failed=" + failed);
    }
}
