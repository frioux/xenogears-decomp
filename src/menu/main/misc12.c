#include "common.h"

#include "psyq/pc.h"
#include "system/menu.h"

void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E71B4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E733C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E76EC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E781C);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8F60);
void func_801E91C4(P_TAG* pTag) {
    SetSemiTrans(pTag, 1);
    SetShadeTex(pTag, 0);
    setRGB0(pTag, 128, 128, 128);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E920C);
void func_801E927C(POLY_FT4* pPoly) {
    SetPolyFT4(pPoly);
    SetSemiTrans(pPoly, 0);
    SetShadeTex(pPoly, 0);
    setRGB0(pPoly, 128, 128, 128);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E92CC);
void func_801E9340(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);
