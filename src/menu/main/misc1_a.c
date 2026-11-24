#include "common.h"

// func_801C72BC's jump table (jtbl_801C5048) sits after the two BASLUS serial strings in
// the overlay head, so the original object that owned this function is its own unit
// (config/menu.yaml carves [0x48, .rodata, main/misc1_a] for the table).
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1_a", func_801C72BC);
