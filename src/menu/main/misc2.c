#include "common.h"

#include "psyq/libapi.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/menu.h"

extern u16 D_801E96A8[]; // Bit masks, LSB first
extern u16 D_801E96C8[]; // Bit masks, MSB first
extern u32 D_801E96E8[];
extern s32 D_801E9768[]; // Card result to error code
extern u8 D_801E9779;
extern u8 D_80059460; // Index of the menu overlay the menu was entered with

s32 MenuWaitForCardEvent(void);
extern void func_80039DB8(s32 arg0);
extern int Vsync(int mode);

void func_801C8A10(s32 arg0);
extern void MenuPollInput(void);
extern void func_801D1D40(void);
extern void func_801D2968(void);
extern void func_801D1CA0(void);
extern void MenuUpdateCardStatus(void);
extern void func_801C8EE8(void);
extern void func_8001BD40(s32 arg0, u8 arg1);
extern void GameCheckAndHandleSoftReset(void);
extern s32* D_8005917C; // -1 when no debugger is attached
extern int D_80059488;
extern u8 D_801E9784;
extern s32 ArchiveGetDiscNumber(void);
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern void SoundEnableAllSpuChannels(void);
extern void SoundMuteAllSpuChannels(void);
extern int ControllerIsStateStackFull(void);
void func_801D1E80(void);
void func_801D22F4(s32 arg0);
void func_801D2484(void);
s32 func_801D2F4C(u8 arg0);
s32 func_801D32B4(void);
void func_801E92CC(void);
s32 func_801E93A0(s32 arg0);
void func_801C6400(void);
void func_801C6AA0(void);
void func_801C6D4C(void);
void func_801C6D5C(void);
void func_801C6E0C(void);
void func_801C6E68(void);
void func_801C6F70(void);

void MenuSetupScreen(void) {
    g_Menu->pSelectionMenu->unk1180.x = 0x2C0;
    g_Menu->pSelectionMenu->unk1180.y = 0x100;
    g_Menu->pSelectionMenu->unk1180.w = 0x140;
    g_Menu->pSelectionMenu->unk1180.h = 0xE0;
    g_Menu->unk348->unk15B = 0x40;
    func_801C6AA0();
    func_801C6D4C();
    func_801C6E0C();
    func_801C6F70();
    func_801C6E68();
    switch (D_80059460) {
    case 2:
        g_Menu->unk348->unk15B = 0x4C;
        /* fallthrough */
    case 0:
    case 6:
        func_801C6400();
        func_801C6D5C();
        break;
    }
}
void MenuRenderFrame(void) {
    GfxEnvironment* pNextGfxEnv;
    int renderContext;

    // Debugging
    if (*D_8005917C != -1) {
        asm("break 0x400");
    }

    MenuPollInput();

    if (D_801E9784) {
        GameCheckAndHandleSoftReset();
    }

    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;

    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    func_8001BD40(0, 0xFF);
    func_801D1D40();
    g_Menu->unk2D8++;
    MenuSetPlayTimeDigits(D_80059488);
    func_801D2968();
    func_801D1CA0();
    renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->unk1180, 0, renderContext * 0xE0);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
    MenuUpdateCardStatus();
    func_801C8EE8();
}
void MenuPollInput(void) {
    u8 wasControllerUnplugged;
    u8 isLooping;
    s32 savedVolume;
    u8 input;

    isLooping = 1;
    wasControllerUnplugged = 0;
    while (isLooping) {
        // Is the main controller not plugged in?
        if (ControllerGetType(0) == CONTROLLER_TYPE_NONE) {
            if (wasControllerUnplugged == 0) {
                wasControllerUnplugged++;
                SoundMuteAllSpuChannels();
                savedVolume = D_80059488;
            }
            continue;
        }

        isLooping--;
        if (wasControllerUnplugged) {
            SoundEnableAllSpuChannels();
            D_80059488 = savedVolume;
        }
    }

    input = MENU_INPUT_IDLE;
    if (ControllerIsStateStackFull()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
                input = 0;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = 1;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = 2;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = 3;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = 4;
                MenuPlaySoundEffect(2);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = 5;
                MenuPlaySoundEffect(3);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SQUARE) {
                input = 6;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_TRIANGLE) {
                input = 7;
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_L1) {
                input = 10;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_R1) {
                input = 9;
                MenuPlaySoundEffect(1);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SELECT) {
                input = 12;
                break;
            }
        }
    }
    g_Menu->input = input;
}
void MenuSetPlayTimeDigits(u32 value) {
    ((u32*)g_Menu->unk2EC)[0] = value / 21600000;
    value %= 21600000;
    ((u32*)g_Menu->unk2EC)[1] = value / 2160000;
    value %= 2160000;
    ((u32*)g_Menu->unk2EC)[2] = value / 216000;
    value %= 216000;
    ((u32*)g_Menu->unk2EC)[3] = value / 36000;
    value %= 36000;
    ((u32*)g_Menu->unk2EC)[4] = value / 3600;
    value %= 3600;
    ((u32*)g_Menu->unk2EC)[5] = value / 600;
    value %= 600;
    ((u32*)g_Menu->unk2EC)[6] = value / 60;
}
void MenuParseNumberToDigits(u32 number) {
    s32 i;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        g_Menu->digits[i] = number / divisor;
        number %= divisor;
        divisor /= 10;
    }

    for (i = 1; i < 9; i++) {
        if (g_Menu->digits[i]) {
            if (g_Menu->digits[i - 1] == 0) {
                g_Menu->digits[i - 1] = 0xFF;
            }
            break;
        }
        g_Menu->digits[i - 1] = 0xFF;
    }
}
void MenuSetPolyG4Gradient(POLY_G4* pPoly, u_char r, u_char g, u_char b) {
    SetPolyG4(pPoly);
    pPoly->r0 = r;
    pPoly->g0 = g;
    pPoly->b0 = b;
    pPoly->r1 = r;
    pPoly->g1 = g;
    pPoly->b1 = b;
    pPoly->r2 = 0;
    pPoly->g2 = 0;
    pPoly->b2 = 0;
    pPoly->r3 = 0;
    pPoly->g3 = 0;
    pPoly->b3 = 0;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C81E0);
/* Best: 86.74% (324 bytes, same size) under XenoMenu260. Structure and
   instruction multiset match; two residuals: (1) retail loads the 5th
   parameter (`lbu t1,0x10(sp)`) after the four word stores and keeps its value
   in the `beqz` delay slot (`li v0,1`), gcc hoists the lbu to the top of the
   block and fills the slot with `sw a3,0xC(v1)`; (2) in every block after the
   first, retail puts the `index*0x24` address in v1 and the `li 1`/`li 0x100`
   constant in v0, gcc swaps them. Pointer local for the first five stores plus
   a fresh `g_Menu->unk0[...]` re-deref after it is the closest shape (plain
   re-deref everywhere is 80.41). Tried under 2.7.2 per the old note: field
   order 0/4/8/C, s32* base, pointer local throughout. Also 95.35-class dead
   zone, do not burn budget on it.
void func_801C81E0(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u8 arg4, u8 index) {
    u8* pEntry;

    pEntry = g_Menu->unk0 + index * 0x24;
    ((s32*)pEntry)[0] = arg0;
    ((s32*)pEntry)[2] = arg1;
    ((s32*)pEntry)[1] = arg2;
    ((s32*)pEntry)[3] = arg3;
    if (arg2 < arg0) {
        arg2 = arg0 - arg2;
        pEntry[0x20] = 1;
    } else {
        arg2 = arg2 - arg0;
        pEntry[0x20] = 0;
    }
    if (arg3 < arg1) {
        arg1 = arg1 - arg3;
        g_Menu->unk0[index * 0x24 + 0x21] = 1;
    } else {
        arg1 = arg3 - arg1;
        g_Menu->unk0[index * 0x24 + 0x21] = 0;
    }
    if (arg2 < arg1) {
        ((s32*)(g_Menu->unk0 + index * 0x24))[4] = (arg2 << 8) / arg1;
        ((s32*)(g_Menu->unk0 + index * 0x24))[5] = 0x100;
    } else {
        ((s32*)(g_Menu->unk0 + index * 0x24))[4] = 0x100;
        ((s32*)(g_Menu->unk0 + index * 0x24))[5] = (arg1 << 8) / arg2;
    }
    g_Menu->unk0[index * 0x24 + 0x22] = arg4;
    ((s32*)(g_Menu->unk0 + index * 0x24))[6] = 0;
    ((s32*)(g_Menu->unk0 + index * 0x24))[7] = 0;
    g_Menu->unk0[index * 0x24 + 0x23] = 0;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8324);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height) {
    pVertices[0].vx = x + 0xFF60;
    pVertices[0].vy = y + 0xFF90;
    pVertices[0].vz = 0;
    pVertices[1].vx = x + width - 0xA0;
    pVertices[1].vy = y + 0xFF90;
    pVertices[1].vz = 0;
    pVertices[2].vx = x + 0xFF60;
    pVertices[2].vy = y + height - 0x70;
    pVertices[2].vz = 0;
    pVertices[3].vx = x + width - 0xA0;
    pVertices[3].vy = y + height - 0x70;
    pVertices[3].vz = 0;
}
void MenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->unk32A) {
        func_80039DB8((g_Menu->unk2E4->sedId << 16) | soundEffectId);
    }
}
u16 MenuGetBitMaskMsbFirst(u8 index) {
    return D_801E96C8[index];
}
u16 MenuGetBitMaskLsbFirst(u8 index) {
    return D_801E96A8[index];
}
u16 MenuGetInvertedBitMaskMsbFirst(u8 index) {
    return ~D_801E96C8[index];
}
u16 MenuGetInvertedBitMaskLsbFirst(u8 index) {
    return ~D_801E96A8[index];
}
u32 MenuAndBitMaskMsbFirst(u32 mask, u8 index) {
    return D_801E96C8[index] & mask;
}
u32 MenuAndBitMaskLsbFirst(u32 mask, u8 index) {
    return D_801E96A8[index] & mask;
}
u32 MenuAndBitMaskLsbFirst32(u32 mask, u8 index) {
    return mask & D_801E96E8[index];
}
void MenuWaitForDiscChange(u8 index) {
    u_char flag;
    u_char counter;

    func_801D1E80();
    flag = 1;
    while (g_Menu->transitionEffectState) {
        MenuRenderFrame();
    }
    func_801D22F4(0);
    while (flag) {
        if (ArchiveGetDiscNumber() == index + 1) {
            flag = 0;
        } else {
            func_801E92CC();
            func_801D2F4C(index * 3 - 0x7D);
            if (func_801E93A0(index + 1)) {
                func_801D32B4();
                func_801D2F4C(0x89);
                for (counter = 0x1D; counter != 0; counter--) {
                    MenuRenderFrame();
                }
                func_801D32B4();
                MenuRenderFrame();
            } else {
                func_801D32B4();
                flag = 0;
            }
        }
    }
    func_801D2484();
}
void MenuClearCardEvents(void) {
    UnDeliverEvent(0xF4000001, 0x4);
    UnDeliverEvent(0xF4000001, 0x8000);
    UnDeliverEvent(0xF4000001, 0x100);
    UnDeliverEvent(0xF4000001, 0x2000);
}
s32 MenuWaitForCardEvent(void) {
    while (1) {
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[5]) == 1) {
            MenuClearCardEvents();
            return 3;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[3]) == 1) {
            MenuClearCardEvents();
            return 1;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[2]) == 1) {
            MenuClearCardEvents();
            return 0;
        }
        if (TestEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[4]) == 1) {
            MenuClearCardEvents();
            return 2;
        }
    }
}
s32 MenuGetCardErrorCode(long chan) {
    if (_card_info(chan)) {
        return D_801E9768[MenuWaitForCardEvent() & 0xFF];
    }
    return -1;
}
void MenuCloseCardEvents(void) {
    MenuRenderFrame();
    EnterCriticalSection();
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[2]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[3]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[4]);
    CloseEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[5]);
    ExitCriticalSection();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc2", func_801C8A10);
void MenuUpdateCardStatus(void) {
    u_char count;

    if (g_Menu->menuUnk2->unk4FE6) {
        count = ++g_Menu->unk326;
        if (D_801E9779 < count) {
            func_801C8A10(0);
            func_801C8A10(1);
            if (g_Menu->menuUnk2->unk4F74[0] == -1 && g_Menu->menuUnk2->unk4F74[1] == -1) {
                g_Menu->unk334 = 0;
            }
            g_Menu->unk326 = 0;
        }
    }
}
void MenuWaitCardOperationFrames(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->unk4F74[index] != -2) {
        g_Menu->menuUnk2->unk4FE6 = 2;
        for (i = 0x3B; i != 0; i--) {
            MenuRenderFrame();
        }
        g_Menu->menuUnk2->unk4FE6 = 0;
    }
}
void MenuWaitCardOperationVsyncs(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->unk4F74[index] != -2) {
        for (i = 0x3B; i != 0; i--) {
            Vsync(0);
        }
    }
}
