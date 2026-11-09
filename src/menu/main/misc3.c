#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

extern u8 D_801EA6D0[][0x10];
extern u16 D_8005A3A0[0x10];
extern u16 D_8006F958[0x10];
extern int D_80059488;
extern int func_801E4D10(u32*, void*);
extern int func_801D2F4C(u_char);
extern u_char func_801CAA38(u_char);
extern int func_801D32B4(void);
extern void func_801CB184(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8D78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8EE8);

int func_801C9038(u8* pPath, u8* pBuffer) {
    int fd;

    fd = open(pPath, 3);
    if (fd == -1) {
        return -1;
    }
    if (read(fd, pBuffer, 0x200) != 0x200) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C90B0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9270);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C93A8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9BCC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9D34);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9EF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA1D4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA480);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA5F0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA750);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA8C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAA38);
u_char func_801CACF8(u_char a0, u_char a1, u_char a2) {
    u_char result;

    func_801D2F4C(a0);
    g_Menu->pCursors->shouldRender[3] = 1;
    result = func_801CAA38(a2);
    func_801D32B4();
    if (a1 != 0xFF && result != 0) {
        func_801D2F4C(a1);
        g_Menu->pCursors->shouldRender[3] = 1;
        result = func_801CAA38(a2);
        func_801D32B4();
    }
    return result;
}

void func_801CADB0(void) {
    g_Menu->menuUnk2->unk4F80 = 0xFF;
    g_Menu->menuUnk2->unk4F7C = 0;
    if (g_Menu->menuUnk2->unk4FE4 == 0 && g_Menu->menuUnk2->unk4FE5 != 0) {
        g_Menu->menuUnk2->unk4F7C = 0xF;
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAE08);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB184);
void func_801CB28C(u32* pSrc) {
    int i;

    func_801E4D10(pSrc, g_Menu->pDressingRoom);
    D_80059488 = *pSrc;
    for (i = 0; i < 0x10; i++) {
        D_8005A3A0[i] = D_8006F958[i];
    }
    func_801CB184();
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB304);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB8AC);
u_char func_801CB9E8(u_char a0, u_char a1) {
    u_char found;
    int i;

    found = 0;
    if (a1 == 0xFF) {
        for (i = 0; i < 0xF; i++) {
            if (D_801EA6D0[a0][i] == 0) {
                found = i;
                break;
            }
        }
        return found;
    }
    return a1;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBA4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBD90);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CC6D8);
