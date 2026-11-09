#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0xA];
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u8* unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u16 unk1E;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ s16 unk28;
    /* 0x2A */ s16 unk2A;
    /* 0x2C */ u16 unk2C;
    /* 0x2E */ u16 unk2E;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 unk32;
    /* 0x34 */ u16 unk34;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u16 unk38;
} Misc16Work; // Size: 0x3A

extern s32 D_80092834;
extern s32 D_80092838;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;

extern s32 rand(void);

void func_8008CED4(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008CEDC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008CF30);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008CF9C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008CFC4);

void func_8008D0A4(Misc16Work* pWork, s16* pOut) {
    pOut[0] = pWork->unk1C + rand() % pWork->unk24 - pWork->unk2C;
    pOut[1] = pWork->unk1E + rand() % pWork->unk26 - pWork->unk2E;
    pOut[2] = pWork->unk20 + rand() % pWork->unk28 - pWork->unk30;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D14C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D208);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D304);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D3F4);

void func_8008D580(Misc16Work* pWork) {
    s32 i;
    u8* pEntry;

    pEntry = pWork->unk10;
    for (i = 0; i < pWork->unkA; i++) {
        *(s16*)(pEntry + 6) = 0;
        pEntry += pWork->unkC;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D5C0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D680);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D980);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008D9F0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DA48);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DBC0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DC28);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DCA8);

void func_8008DCA8(s32 arg0) {
    D_80092838 = arg0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DCB8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DDFC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DE54);

void func_8008DF30(void) {
    D_80092844 = 0;
    D_8009283C = 0;
    D_80092840 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008DF50);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008E064);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc16", func_8008E0C8);
