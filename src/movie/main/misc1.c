#include "common.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"

extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void func_8002A498(int mode);

// .data
extern s32 D_80076E5C;
extern s32 D_80076EC8;
extern s32 D_80076ECC;
extern s32 D_80076EF4;
extern s32 D_80076EF8;
// .bss
extern u_char D_80076F84[];

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800704E8);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80070DCC);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800712C4);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80071BA0);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80071C34);

void func_80072428(void) {
    D_80076E5C++;
    if (++D_80076ECC >= 0x3C) {
        D_80076ECC = 0;
        D_80076EC8++;
    }
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072480);

void func_8007293C(void) {
    if (ArchiveGetDebugTable() == 0) {
        func_8002A498(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        while (CdControlB(8, 0, D_80076F84) == 0) {
        }
    }
}

void func_800729A8(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072A08);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072D84);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80072F98);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_80073328);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800734B8);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800737EC);

INCLUDE_ASM("asm/movie/nonmatchings/main/misc1", func_800747AC);

s32 func_80074AF0(void) {
    s32 a;
    s32 b;
    s32 r;

    a = D_80076EF4 * 5 + 1;
    b = D_80076EF8 * 7 + 3;
    D_80076EF4 = a;
    D_80076EF8 = b;
    r = (a ^ b) + 1;
    D_80076EF4 = r;
    if (r < 0) {
        D_80076EF4 = -r;
    }
    return D_80076EF4;
}
