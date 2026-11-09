#include "common.h"

#include "psyq/libcd.h"
#include "psyq/pc.h"
#include "system/menu.h"

extern u_char D_801EA8F4;
extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void func_8002A498(int mode);

void func_801E64E0(void);
void func_801E6668(s32 arg0);
void func_801E76EC(s32 arg0);
void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E71B4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E733C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E76EC);
void func_801E781C(s32 arg0, u_char arg1) {
    func_801E64E0();
    if (arg0 != 0xFF) {
        if (arg1 != 0) {
            func_801E76EC(arg0);
            func_801E6668(arg0);
            ((u8*)g_Menu->unk34C)[0x2DBC] = 1;
        } else {
            func_801E6668(arg0);
            ((u8*)g_Menu->unk34C)[0x2DBC] = 0;
        }
        g_Menu->pManager->unkB = 1;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E78C8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E7C50);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E7E68);
void func_801E8018(u_char count, MenuString* pStringList, u_char* pStringIdList, void* pUnused) {
    func_801E7E68(pStringList, pStringIdList, 4, count);
}

void func_801E8044(u_char count, u_char* pBuffer) {
    s32 end;

    if (count != 0) {
        end = count + (s32)pBuffer;
        do {
            *pBuffer++ = 0;
        } while ((s32)pBuffer < end);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8070);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8474);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E86C8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8978);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8B4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8DA8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8EAC);
/* Best: 92.44%. Switch tree and every store match; gcc emits three copies of
   `a0 = pPoly` where retail keeps exactly one, stolen into the `beq s0,1` delay
   slot, and reuses a0 for the case-1 tpage load. One extra insn in the frame.
void func_801E8EAC(POLY_FT4* pPoly, u_char mode) {
    SetShadeTex(pPoly, 0);
    switch (mode) {
    case 1:
        pPoly->tpage |= 0x20;
        SetSemiTrans(pPoly, 1);
        break;
    case 0:
        SetSemiTrans(pPoly, 0);
        // fallthrough
    case 2:
        setRGB0(pPoly, 128, 128, 128);
        break;
    case 3:
        setRGB0(pPoly, 33, 33, 33);
        break;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8F60);
void func_801E91C4(P_TAG* pTag) {
    SetSemiTrans(pTag, 1);
    SetShadeTex(pTag, 0);
    setRGB0(pTag, 128, 128, 128);
}
void func_801E920C(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, s32 width, s32 height) {
    pPoly->x0 = x;
    pPoly->y0 = y;
    pPoly->y1 = y;
    pPoly->x2 = x;
    pPoly->u0 = u;
    pPoly->u2 = u;
    pPoly->x1 = x + width;
    pPoly->y2 = y + height;
    pPoly->x3 = x + width;
    pPoly->y3 = y + height;
    pPoly->v0 = v;
    pPoly->u1 = u + width;
    pPoly->v1 = v;
    pPoly->v2 = v + height;
    pPoly->u3 = u + width;
    pPoly->v3 = v + height;
}
void func_801E927C(POLY_FT4* pPoly) {
    SetPolyFT4(pPoly);
    SetSemiTrans(pPoly, 0);
    SetShadeTex(pPoly, 0);
    setRGB0(pPoly, 128, 128, 128);
}
void func_801E92CC(void) {
    if (ArchiveGetDebugTable() == 0) {
        func_8002A498(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        for (;;) {
            Vsync(3);
            if (CdControlB(8, 0, &D_801EA8F4) != 0) {
                break;
            }
        }
    }
}
void func_801E9340(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);
