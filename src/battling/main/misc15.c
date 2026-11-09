#include "common.h"

// A 16-byte heap node allocated by func_8008C0CC.
typedef struct HeapNode {
    /* 0x0 */ s32 type;
    /* 0x4 */ void* pOwner;
    /* 0x8 */ void* pWork;
    /* 0xC */ void* pData;
} HeapNode; // Size: 0x10

extern s32 D_80092824;
extern void* D_80092828;
extern void func_8008C188();

INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BC04);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BCC8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BD70);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BE4C);
void func_8008C0BC(void* pObj, void* pValue) {
    *(s32*)pObj = 5;
    *((void**)pObj + 1) = pValue;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C0CC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C120);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C188);
void func_8008C298(void* pWork) {
    D_80092824 = 0;
    func_8008C188(pWork, NULL);
}
void func_8008C2C0(void* pWork) {
    D_80092828 = pWork;
    func_8008C298(pWork);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C2E8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C3A8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C4B0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C620);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C7C0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C828);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C8B4);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C9B8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CA00);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CA84);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CC2C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CC54);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CCB0);
void func_8008CD54(void) {
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CD5C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CE0C);
