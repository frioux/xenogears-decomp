#!/bin/bash
mips-linux-gnu-objdump -d -r --no-show-raw-insn "$1" | awk -v f="<$2>:" '$0 ~ f {p=1; next} p && /^[0-9a-f]+ <[^.>][^>]*>:$/ {exit} p'
