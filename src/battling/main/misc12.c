#include "common.h"
#include "system/memory.h"
#include "system/archive.h"

extern s32 D_8006F980;
extern s16 D_8009285C;
extern s16 D_8009286C;

extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

extern void func_800888B0(void);
extern void func_800889C8(void);
extern void func_80089330(s32, s32);
extern void func_80089534(s32, s32);
extern void func_80089B44(void* p);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088940);

/* Best attempt: objdiff 44.86%. gcc rematerialises the symbol address for the
   store (fresh `lui at` + `sw %lo(at)`), so the address never stays live in
   one register as the original's does; the pointer/value declaration order and
   a single `|=` statement all compile to the same two-address shape. */
/*
void func_80088940(void) {
    s32 value;
    s32* pFlags;

    pFlags = &D_8006F980;
    value = *pFlags;
    *pFlags = value | 0x10000;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_8008895C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800889C8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088A40);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088AF8);

void func_80088BD4(void) {
    func_800888B0();
    func_800889C8();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088BFC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088C28);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088CBC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088D1C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80088E90);

void* func_800891C0(s32 entryIndex) {
    void* pBuffer;

    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 1);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    return pBuffer;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089210);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089330);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089534);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800896C4);

void func_8008973C(void) {
    func_80089534(D_8009285C, D_8009286C);
}

void func_8008976C(s32 a0, s32 a1) {
    func_80089330(a0, a1);
    func_80089534(a0, a1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800897AC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_800898BC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089A98);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089B44);

void func_80089C54(void) {
    void* p;

    HeapSetCurrentContentType(8);
    p = HeapAlloc(0x9C, 0);
    func_80089B44(p);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089C88);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc12", func_80089CD8);
