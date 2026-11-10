#include "common.h"
#include "psyq/libgpu.h"
#include "system/archive.h"
#include "system/memory.h"

extern s32 D_800D2D40;
extern s32 D_800D2D48;
extern u8 D_800C3D04[];
extern u8 D_800C3D0C[];
extern u16 D_800C3D40;
extern u16 D_800C3E30;
extern u8 D_800D3611;
extern u8 *D_800D3368[];

extern void func_800A22A8(u8 *);
extern void func_800A2D1C(u8 *);
extern void func_800A9FF0(s32);
extern void func_800AA934(void *, void *, void *, void *);
extern s32 func_800AF400(void);
extern void func_800A429C(void *);
extern void func_800A64E4(void);
extern void func_800BF85C(s32, s32);

extern u8 *D_800D33E4;
extern u16 D_800D361A;
extern u_long *D_800D3604;
extern RECT D_800D3628;
extern void *D_800C3AC4;
extern void *D_800C3AC8;
extern u8 D_800C3D68;
extern u8 D_800591AC;
extern s32 D_80059464;
extern s32 D_800C360C;
extern u8 D_800C4000[];
extern StreamDataQueueEntry *D_800C3B78;
extern u8 *D_800658C8;
extern s32 D_800C3E88;
extern u16 D_800C3CF0;
extern u8 D_800C3D6C;
extern u16 D_800C3B7C;
extern u8 D_800C3B74;
extern u8 D_800C3ACC[];
extern u8 D_800D330A[];
extern void func_800A2234(s32);
extern void func_800A2CA4(s32);
extern void func_800B00D0(void);

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void func_80029AFC(StreamDataQueueEntry *pQueue, int arg1, int arg2);

// Per-unit 6-byte state records, unit id 0..3; the original spells each byte
// as its own symbol.
extern u8 D_800C3AAC[];
extern u8 D_800C3AAD[];
extern u8 D_800C3AAE[];
extern u8 D_800C3AAF[];
extern u8 D_800C3AB0[];
extern u8 D_800C3AB1[];

// Allocation block owned by the structure passed to func_800A8A88.
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void *unk14;
    /* 0x18 */ void *unk18;
    /* 0x1C */ void *unk1C;
    /* 0x20 */ void *unk20;
} Misc23Block;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5914);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5A48);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5BE8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5D54);

void func_800A5E9C(s32 arg0, s32 arg1) {
    D_800D2D40 = arg0;
    D_800D2D48 = arg1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A5EB4);

void func_800A6444(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    if (arg0 < 4) {
        if (arg1 >= 0) {
            D_800D3611 = 1;
            D_800C3AAC[arg0 * 6] = 1;
            D_800C3AAD[arg0 * 6] = arg1;
            D_800C3AAE[arg0 * 6] = arg2;
            D_800C3AAF[arg0 * 6] = arg3;
            D_800C3AB0[arg0 * 6] = arg4;
            D_800C3AB1[arg0 * 6] = arg5;
        } else {
            D_800C3AAC[arg0 * 6] = 0;
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A64E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6884);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6AE8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A6F98);
/* 84.20%: identical instruction multiset except that the original keeps
   &D_800D3628 in s0 across LoadImage/DrawSync and gcc 2.7.2 rematerialises the
   symbol address instead (also costs an s0 slot and shuffles the i++ into the
   delay slot). Tried: RECT *pRect, u8 *pBuf, reusing the loop pointer, all
   identical code.
void func_800A6F98(void) {
    u8 *p;
    s32 i;

    if (D_800D361A != 0) {
        p = *(u8 **)(D_800D33E4 + 0x118);
        for (i = 0; i < *(u8 *)(D_800D33E4 + 0x10E); i++) {
            if (*(u16 *)(p + 0x1A) != 0) {
                *(s32 *)p = 0;
            }
            p += 0x30;
        }
        p = (u8 *)&D_800D3628;
        LoadImage((RECT *)p, D_800D3604);
        DrawSync(0);
        func_800A429C(p - 0x28);
    }
    func_800A64E4();
    HeapFree(D_800C3AC4);
    HeapFree(D_800C3AC8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7064);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A7948);

void func_800A8A88(Misc23Block *pBlock) {
    if (pBlock->unk14 != NULL) {
        HeapFree(pBlock->unk14);
        HeapFree(*(void **)pBlock->unk1C);
        HeapFree(pBlock->unk1C);
        HeapFree(pBlock->unk20);
        if (pBlock->unk18 != NULL) {
            HeapFree(pBlock->unk18);
        }
        pBlock->unk14 = NULL;
    }
}

void func_800A8B0C(void) {
    s32 i;
    s32 off;

    D_800C3E88 = 0;
    D_800C3CF0 = 0;
    D_800C3D6C = 0;
    D_800C3D68 = 0;
    D_800C3B7C = 0;
    D_800C3B74 = 1;
    func_800A2234(*(s16 *)((u8 *)D_800658C8 + 0x348));
    func_800A2CA4(*(s16 *)((u8 *)D_800658C8 + 0x34A));
    func_800B00D0();
    for (i = 0x1F; i >= 0; i--) {
        ((u8 **)&D_800D33E4)[i - 0x1F] = NULL;
    }
    for (off = 0x98; off >= 0; off -= 8) {
        *(s32 *)(D_800C3ACC + off) = 0;
    }
    for (off = 0x14; off >= 0; off -= 0x14) {
        *(u16 *)(D_800D330A + off) = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A8BF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9540);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A96B4);
/* best 87.98%: register assignment (s0=pQueue, s1=base, s2=idx) and the whole
   instruction multiset match; only the two archiveIndex stores sit in the
   previous call's delay slot instead of the ArchiveDecodeAlignedSize one, and
   the pData store lands after that call instead of before it. Also tried: a
   size local with the stores after the decode (78.71%), pData before
   archiveIndex per entry (86.10%), base+1/base+2 inline (77.98%).
void func_800A96B4(s32 arg0) {
    StreamDataQueueEntry *pQueue;
    s32 dirIndex;
    s32 entryIndex;
    s32 fileIdx;

    ArchiveGetArchiveOffsetIndices(&dirIndex, &entryIndex);
    ArchiveSetIndex(0x28, 0);
    HeapChangeCurrentUser(4, NULL);
    pQueue = HeapAlloc(0x18, 1);
    D_800C3B78 = pQueue;
    fileIdx = arg0 * 2 + 1;
    pQueue[0].archiveIndex = fileIdx;
    ArchiveGetFilePath(fileIdx);
    pQueue[0].pData = HeapAlloc(ArchiveDecodeAlignedSize(fileIdx), 1);
    pQueue[1].archiveIndex = fileIdx + 1;
    pQueue[1].pData = HeapAlloc(ArchiveDecodeAlignedSize(fileIdx + 1), 1);
    pQueue[2].archiveIndex = 0;
    pQueue[2].pData = NULL;
    func_80029AFC(D_800C3B78, 0, 0);
    ArchiveSetIndex(dirIndex, entryIndex);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A979C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9A50);

void func_800A9F94(void) {
    s32 i;

    for (i = 0; i < 0x1F; i++) {
        func_800A9FF0(i);
    }
    func_800A22A8(D_800C3D0C);
    func_800A2D1C(D_800C3D04);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc23", func_800A9FF0);

void func_800AA320(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}

void func_800AA384(u16 arg0, u16 arg1, void *arg2) {
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    D_800C3D68 = 1;
    ((u8 *)D_800D3368[arg0])[0x35] = 1;
    D_80059464 = 0;
    D_800591AC = 1;
    func_800BF85C(arg0, func_800AF400());
    D_800C360C = 1;
    D_800C4000[func_800AF400()] = 0;
    if (D_800D3368[arg0] != NULL) {
        func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
    }
}
void func_800AA454(u16 arg0, u16 arg1, void *arg2) {
    u16 savedIdx;
    u16 savedUnk;

    savedIdx = D_800C3D40;
    savedUnk = D_800C3E30;
    D_800C3D40 = arg0;
    D_800C3E30 = arg1;
    ((u8 *)D_800D3368[arg0])[0x35] = 0;
    if (D_800D3368[arg0] != NULL) {
        if ((s32)arg0 != func_800AF400()) {
            func_800AA934(D_800D3368[arg0], D_800D3368[arg0], D_800C3D0C, arg2);
        }
    }
    D_800C3D40 = savedIdx;
    D_800C3E30 = savedUnk;
}
