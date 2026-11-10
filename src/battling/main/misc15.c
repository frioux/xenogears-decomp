#include "common.h"
#include "system/memory.h"

// The block func_8008C0CC hangs off a heap node; func_8008C120 releases it.
typedef struct WorkBlock WorkBlock;
struct WorkBlock {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ void* pMem;
    /* 0xC */ u32 unkC;
    /* 0x10 */ void* pDelayedMem;
};

// A 16-byte heap node allocated by func_8008C0CC.
typedef struct HeapNode {
    /* 0x0 */ s32 type;
    /* 0x4 */ void* pOwner;
    /* 0x8 */ void* pWork;
    /* 0xC */ WorkBlock* pBlock;
} HeapNode; // Size: 0x10

extern s32 D_80092824;
extern void* D_80092828;
extern void func_8008C188();
extern s32 rand(void);
// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

// A word-aligned 16-byte chunk: assigning one copies it with four lw/four sw.
typedef struct {
    s32 unk[4];
} Words4; // Size: 0x10

typedef struct {
    s32 unk[3];
} Words3; // Size: 0xC

typedef struct {
    s32 unk[2];
} Words2; // Size: 0x8

INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BC04);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BCC8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BD70);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008BE4C);
void func_8008C0BC(void* pObj, void* pValue) {
    *(s32*)pObj = 5;
    *((void**)pObj + 1) = pValue;
}
HeapNode* func_8008C0CC(void* pOwner) {
    HeapNode* pNode;
    s32 type;
    void* pWork;

    HeapSetCurrentContentType(7);
    pNode = HeapAlloc(0x10, 2);
    type = *(s32*)pOwner;
    pWork = D_80092828;
    pNode->pOwner = pOwner;
    pNode->pBlock = NULL;
    pNode->type = type;
    pNode->pWork = pWork;
    return pNode;
}
void func_8008C120(HeapNode* pNode) {
    WorkBlock* pBlock = pNode->pBlock;

    if (pBlock != NULL) {
        if (pBlock->pMem != NULL)
            HeapFree(pBlock->pMem);
        if (pBlock->pDelayedMem != NULL)
            HeapDelayedFree(pBlock->pDelayedMem, 2);
        HeapFree(pBlock);
    }
}
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
void func_8008C7C0(u8* pObj) {
    *(Bytes8*)(pObj + 0x20) = *(Bytes8*)pObj;
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)(pObj + 0x20);
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)(pObj + 0x18);
}
void func_8008C828(u8* pObj, u8* pSrc) {
    pObj[0x2F] = 0x4C;
    pObj[0x2B] = 6;
    *(u32*)(pObj + 0x40) = 0x55555555;
    pObj[0x2F] |= 2;
    pObj[0x2C] = pSrc[0x74];
    pObj[0x2D] = pSrc[0x75];
    pObj[0x2E] = pSrc[0x76];
    *(Words4*)(pObj + 0x44) = *(Words4*)(pObj + 0x28);
    *(Words3*)(pObj + 0x54) = *(Words3*)(pObj + 0x38);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008C8B4);
void func_8008C9B8(u8* pObj) {
    *(Bytes8*)(pObj + 0x18) = *(Bytes8*)pObj;
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)(pObj + 0x18);
}
void func_8008CA00(u8* pObj, u8* pSrc) {
    pObj[0x27] = 0x48;
    pObj[0x23] = 5;
    *(u32*)(pObj + 0x34) = 0x55555555;
    pObj[0x27] |= 2;
    pObj[0x24] = pSrc[0x74];
    pObj[0x25] = pSrc[0x75];
    pObj[0x26] = pSrc[0x76];
    *(Words4*)(pObj + 0x38) = *(Words4*)(pObj + 0x20);
    *(Words2*)(pObj + 0x48) = *(Words2*)(pObj + 0x30);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CA84);
void func_8008CC2C(u8* pObj) {
    *(Bytes8*)(pObj + 0x10) = *(Bytes8*)pObj;
}
void func_8008CC54(u8* pObj, u8* pSrc) {
    pObj[0x1B] = 3;
    pObj[0x1F] = 0x42;
    pObj[0x1C] = pSrc[0x74];
    pObj[0x1D] = pSrc[0x75];
    pObj[0x1E] = pSrc[0x76];
    *(Words4*)(pObj + 0x28) = *(Words4*)(pObj + 0x18);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CCB0);
void func_8008CD54(void) {
}
void func_8008CD5C(u8* pObj, u8* pSrc) {
    s16 n;

    pObj[0x13] = 3;
    pObj[0x17] = 0x62;
    n = rand() % 2 + 2;
    *(s16*)(pObj + 0x1E) = n;
    *(s16*)(pObj + 0x1C) = n * 2;
    pObj[0x14] = pSrc[0x74];
    pObj[0x15] = pSrc[0x75];
    pObj[0x16] = pSrc[0x76];
    *(Words4*)(pObj + 0x20) = *(Words4*)(pObj + 0x10);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc15", func_8008CE0C);
