for f in expected/asm/battling/main/misc9.s.o build/src/battling/main/misc9.c.o; do
  mips-linux-gnu-objdump -d -r --no-show-raw-insn "$f" | sed -n '/<func_800846A0>:/,/^$/p' \
    | sed -E 's/^ +[0-9a-f]+:\s*//; s/^\s+[0-9a-f]+: (R_MIPS)/\1/' > /tmp/o_$(basename $f).txt
done
diff /tmp/o_misc9.s.o.txt /tmp/o_misc9.c.o.txt
