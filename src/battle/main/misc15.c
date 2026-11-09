#include "common.h"

extern u8 D_800D366C;
extern int ArchiveDataSync(void);
extern void func_8008AA40(u8 arg0);
extern void func_800716D8(void);

void func_8008AA74(u8 arg0) {
    if (D_800D366C != 0) {
        func_8008AA40(arg0);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008AAA0);
void func_8008AB4C(void) {
    ArchiveSetIndex(0x20, 0);
}
void func_8008AB70(void) {
    ArchiveSetIndex(0x20, 2);
}
void func_8008AB94(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 func_8008ABB8(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 func_8008AC00(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void func_8008AC50(void) {
    while (ArchiveDataSync() != 0) {
        func_800716D8();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008AC88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B108);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B908);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BC40);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BC98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BD50);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C360);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C3F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008CCCC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008CD28);
