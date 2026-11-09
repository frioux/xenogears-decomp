#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/script_vm.h"
#include "field/actor.h"
#include "field/text_box.h"
#include "field/main.h"

extern void func_80076AC0(s32, s32, void*, s32, s32, s32, s32);
extern void func_800A0C94();
extern int ArchiveGetDiscNumber(void);
extern void FieldScriptMemoryWriteU16(int, int);
extern s32 D_800AFD1C; // Current actor index
extern s32 D_800B2264;
extern u8 D_800B225F[];
extern s16 D_800B21DC[];
extern s32 D_800B2268;
extern int g_GameSceneMapNum;
extern void GameWaitForCdData(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern int func_8008CF3C(int);
extern void* g_PartyDataBuffers[];
extern s32 D_800AFFEC;
extern s16 D_800AFD20;

void FieldScriptVMHandlerDisableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerEnableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DA70(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DA98(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DAC4(void) {
    int nTextBoxIndex;
    ActorData* pActor;
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 0x1;
        pActor->flags |= 0x100000;
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->status |= ACTOR_STATUS_INVISIBLE;
        if (func_8009CD18(&nTextBoxIndex) == 0) {
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerEnableActorVM(void) {
    ActorData* pActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 0;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009DC4C(void) {
    ActorData* pActor;
    int nTextBoxIndex;
    short nNewRotation;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->moveModified.vx = 0;
        pActor->moveModified.vy = 0;
        pActor->moveModified.vz = 0;
        pActor->move.vx = 0;
        pActor->move.vy = 0;
        pActor->move.vz = 0;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 1;
        nNewRotation = pActor->rotation.vx | 0x8000;
        pActor->rotation.vy = nNewRotation;
        pActor->rotation.vx = nNewRotation;
        if (func_8009CD18(&nTextBoxIndex) == 0) {
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerSleep(void) {
    u_char nScriptId;

    nScriptId = g_FieldScriptVMCurActor->curScriptIndex;

    // Initialize the value with the argument as the time to sleep, else count down
    if (g_FieldScriptVMCurActor->scripts[nScriptId].waitTimer == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].waitTimer = FieldScriptVMGetArgument(1);
    } else {
        g_FieldScriptVMCurActor->scripts[nScriptId].waitTimer--;
    }
    
    // When the timer has reached 0, we move to the next instruction
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].waitTimer == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }

    D_800B00C0 = 1;
}

void FieldScriptVMHandlerShowActorById(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        if (!(pFieldActor->pActorData->flags & 0x100000)) {
            pFieldActor->status &= ~ACTOR_STATUS_INVISIBLE;
            pFieldActor->pActorData->flags &= ~0x2000000;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerHideActorById(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->status |= ACTOR_STATUS_INVISIBLE;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerShowActor(void) {
    FieldActor* pActor = &g_FieldActors[D_800AFD1C];
    pActor->status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->curAnimationId = 0xFF;
    g_FieldScriptVMCurActor->flags &= 0xFDFFFFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009DF78(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->pActorData->flags |= 0x02000000;
        pFieldActor->pActorData->flags |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E014(void) {
    g_FieldScriptVMCurActor->flags |= 0x02000800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerHideActor(void) {
    FieldActor* pActor = &g_FieldActors[D_800AFD1C];
    pActor->status |=  ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// Update sprite movement or animation speed?
void func_8009E094(void) {
    unsigned short nMoveSpeed;

    nMoveSpeed = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->moveSpeed = nMoveSpeed;
    func_80021BCC(g_FieldActors[D_800AFD1C].pSpriteData, nMoveSpeed);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009E10C(void) {
    int nValue;
    int nFlags;

    nValue = FieldScriptVMGetArgument(1);
    nFlags = (nValue & 0x1) << 7;
    if (nValue & 0x4) {
        nFlags |= 0x20;
    }
    if (nValue & 0x8) {
        nFlags |= 0x10;
    }
    if (nValue & 0x10) {
        nFlags |= 0x8;
    }
    if (nValue & 0x20) {
        nFlags |= 0x4;
    }
    if (nValue & 0x40) {
        nFlags |= 0x8000000;
    }
    g_FieldScriptVMCurActor->scriptFlags.flags &= 0xF7FFFF43;
    g_FieldScriptVMCurActor->scriptFlags.flags |= nFlags;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009E1A0(void) {
    u32 nValue;

    nValue = SCRIPT_READ_U8_REL(1) & 7;
    g_FieldScriptVMCurActor->flags &= ~7;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    nValue = (SCRIPT_READ_U8_REL(1) >> 1) & 0x38;
    g_FieldScriptVMCurActor->flags &= ~0x38;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E208(void) {
    g_FieldScriptVMCurActor->unkEC = 0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x15 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
    g_FieldScriptVMCurActor->curYPos = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
}

void func_8009E248(void) {
    func_8009E574(
        FieldScriptVMGetInstructionArgumentS16(1),
        FieldScriptVMGetInstructionArgumentS16(3)
    );
    func_8009E810(FieldScriptVMGetInstructionArgumentS16(5));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_8009E2C8(void) {
    func_8009E810(FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3)));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Read short from bytecode, absolute offset
short func_8009E330(int offset) {
    return SCRIPT_READ_U8(offset) + (SCRIPT_READ_U8(offset + 1) << 8);
}

void func_8009E35C(void) {
    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(5);
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(6)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(6))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_8009E428(void) {
    ActorData* pActor;

    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[D_800AFD1C].pActorData;
    func_8009E574(
        CONV_TO_GTE(pActor->position.vx), 
        CONV_TO_GTE(pActor->position.vz)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009E4BC(void) {
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}


typedef struct {
    /* 0x0 */ u16 f0;
    /* 0x2 */ u16 f1;
    /* 0x4 */ u16 f2;
    /* 0x6 */ u16 f3;
    /* 0x8 */ u16 f4;
    /* 0xA */ u16 f5;
    /* 0xC */ u16 f6;
    /* 0xE */ u16 f7;
} FieldScriptArgs; // Size: 0x10

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009E574);

// Writes the actor's Y position (fixed point) and the cached short copies
void func_8009E810(short nY) {
    u8 unused[0x18]; // original has an unused stack slot; reproduces the frame

    g_FieldScriptVMCurActor->position.vy = nY * 0x10000;
    g_FieldScriptVMCurActor->unkEC = nY;
    g_FieldScriptVMCurActor->curYPos = nY;
}

void func_8009E83C(void) {
    unsigned char width;
    unsigned char zWidth;
    unsigned char height;
    unsigned char solidRange;

    width = SCRIPT_READ_U8_REL(1);
    if (width) {
        g_FieldScriptVMCurActor->width = width * 2;
    }
    
    zWidth = SCRIPT_READ_U8_REL(2);
    if (zWidth) {
        g_FieldScriptVMCurActor->zWidth = zWidth * 2;
    }
    
    height = SCRIPT_READ_U8_REL(3);
    if (height) {
        g_FieldScriptVMCurActor->height = height * 2;
    }
    
    solidRange = SCRIPT_READ_U8_REL(4);
    if (solidRange) {
        g_FieldScriptVMCurActor->solidRange = SCRIPT_READ_U8_REL(4) * 2;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8009E91C(void) {
    if (!g_FieldScriptVMCurActor->flags12C_0xD) {
        g_FieldScriptVMCurActor->unk114 = HeapAlloc(0x10, 0x0);
    }
    g_FieldScriptVMCurActor->flags12C_0xD = 1;
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f0 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f1 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f2 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f3 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f4 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f5 = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f6 = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    ((FieldScriptArgs*)g_FieldScriptVMCurActor->unk114)->f7 = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

// -1: Actor has a script slot occupied w/ target ID
//  0: Actor has no script slot w/ target ID
int FieldActorGetScriptStatus(ActorData* pActor, int targetId) {
    int i;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        if (targetId == pActor->scripts[i].scriptId) {
            return -1;
        }
    }
    return 0;
}

void func_8009EB78(void) {
    ActorData* pActor;
    int actorIndex;
    int i;
    int curScriptId;

    if (FieldScriptVMGetActorIndex(1) != 0xFF) {
        actorIndex = FieldScriptVMGetActorIndex(1);
        pActor = g_FieldActors[actorIndex].pActorData;
        if (pActor->flags & 0x100000) {
            curScriptId = g_FieldScriptVMCurActor->curScriptIndex;
            g_FieldScriptVMCurActor->scripts[curScriptId].state = SCRIPT_STATE_IDLE;
            pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
        } else if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) != ACTOR_SCRIPT_EXISTS) {
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 0x5;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                return;
            }
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009ED68(void) {
    ActorData* pActor;
    int actorIndex;
    int i;

    if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    
    actorIndex = FieldScriptVMGetActorIndex(1);
    pActor = g_FieldActors[actorIndex].pActorData;
    if (pActor->flags & 0x100000) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
        pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
    } else {
        switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state) {
        case 0:
            if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) == ACTOR_SCRIPT_EXISTS) {
                break;
            }
            
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 5;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                g_FieldScriptVMCurActor->unkCF = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                break;
            }
            return;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->unkCF) || pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
            } else {
                D_800B00C0 = g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state;
            }
            return;
        default:
            return;
        }
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009F0A0(void) {
    ActorData* pActor;
    s32 actorIndex;
    int i;

    if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    
    actorIndex = FieldScriptVMGetActorIndex(1);
    pActor = g_FieldActors[actorIndex].pActorData;
    if (pActor->flags & 0x100000) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
        pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
    } else {
        switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state) {
        case 0:
            if (FieldActorGetScriptStatus(pActor, SCRIPT_READ_U8_REL(2) & 0x1F) == ACTOR_SCRIPT_EXISTS) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                break;
            }
            
            for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
                if (pActor->scripts[i].flags_0x12 != 0xF || pActor->scripts[i].isInUse) {
                    continue;
                }
                pActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(actorIndex, SCRIPT_READ_U8_REL(2) & 0x1F);
                pActor->scripts[i].flags_0x12 = SCRIPT_READ_U8_REL(2) >> 5;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 1;
                g_FieldScriptVMCurActor->unkCF = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                return;
            }
            break;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->unkCF) || pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 2;
                return;
            }
            D_800B00C0 = 1;
            break;
        case 2:
            if (pActor->scripts[g_FieldScriptVMCurActor->unkCF].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->unkCF].isInUse = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                return;
            }
            D_800B00C0 = 1;
            break;
        }
        return;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Randomize X rotation
void func_8009F424(void) {
    int nRotation;
    
    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->unk102++;
    if (!(g_FieldScriptVMCurActor->unk102 & 0xF)) {
        if (!(rand() & 0x1)) {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy + PSX_DEGREES(45));
        } else {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy - PSX_DEGREES(45));
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009F4CC(void) {
    short nSpecialRotation;
    int nRotation;
    int nRand;
    int nDeltaRotation;
    int delta;

    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->unk102++;
    if (!(g_FieldScriptVMCurActor->unk102 & 0xF)) {
        nRand = rand();
        if (nRand & 0x30) {
            nSpecialRotation = g_FieldScriptVMCurActor->rotation.vy | ~0x7FFF;
            nRotation = nSpecialRotation;
            g_FieldScriptVMCurActor->rotation.vy = nSpecialRotation;
        } else {
            delta = PSX_DEGREES(45);
            if (!(nRand & 1)) {
                nDeltaRotation = g_FieldScriptVMCurActor->rotation.vy + delta;
            } else {
                nDeltaRotation = g_FieldScriptVMCurActor->rotation.vy - delta;
            }
            nRotation = PSX_ANGLE(nDeltaRotation);
        }
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009F5A8(void) {
    unsigned short nStoredIP = g_FieldScriptVMCurActor->scriptInstructionPointer;
    func_8009F5F4(); // Do Encounter
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = nStoredIP;
}


INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009F5F4);

int FieldCharacterIdToPartyId(int characterId) {
    int i;

    if (characterId == CHARACTER_ID_NONE) {
        return -1;
    }
    
    for (i = 0; i < 3; i++) {
        if (g_GamePartyMembers[i] == CHARACTER_ID_NONE)
            return -1;
        
        if (g_GamePartyMembers[i] == characterId) {
            return i;
        }
    }

    return -1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FA54);
// 90.74%: the `if (== 0xFF) { body } return 0;` form reproduces the single
// `bne s2,v0,<epilogue>` with `move v0,zero` in the delay slot (an early
// `return 0;` instead gives beq+j).
// Remaining: (1) the original sign-extends the angle for g_Scene.unk7C with
// `sll a0,a1,16; sra v0,a0,16` reusing the unk60 shift; gcc 2.7 folds any
// `(short)`/`>>16` of this value away because both branches of the 0xFF test
// produce `(x & 7) << 9`. (2) The frame is 0x28 here vs 0x20, and the tail
// rotation stores put the actor in $v0 instead of $a0 with `move v0,zero`
// scheduled last instead of before the actor load.
/*
s32 func_8009FA54(s32 nInstructionOffset) {
    u8 nByte;
    s32 nOffset;
    s32 nAngle;
    s32 nRotation;
    s32 nX;
    s32 nY;

    if (SCRIPT_READ_U8(0) == 0xFF) {
        nOffset = nInstructionOffset * 7;
        g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8(nOffset + 5);
        nX = func_8009E330(nOffset + 1);
        nY = func_8009E330(nOffset + 3);
        func_8009E574(nX, nY);

        nByte = SCRIPT_READ_U8(nOffset + 6);
        nAngle = ((nByte + 4) & 7) << 9;
        if (nByte == 0xFF) {
            nAngle = ((FieldScriptVMGetVariableValue(8) + 4) & 7) << 9;
        }
        g_Scene.unk7C = (short)nAngle;
        g_Scene.unk60 = nAngle << 16;
        g_Scene.sceneAngle.vy = nAngle;

        nByte = SCRIPT_READ_U8(nOffset + 7);
        nRotation = ((nByte - 2) & 7) << 9 | 0x8000;
        if (nByte == 0xFF) {
            nRotation = ((FieldScriptVMGetVariableValue(6) - 2) & 7) << 9 | 0x8000;
        }
        g_FieldScriptVMCurActor->rotation.vx = nRotation;
        g_FieldScriptVMCurActor->rotation.vy = nRotation;
        g_FieldScriptVMCurActor->rotation.vz = nRotation;
    }
    return 0;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FB98);
// 66.50%: only the ordering differs - the original stores D_800B2268 before it
// reloads scriptInstructionPointer (+2) and stores it; every source order I tried
// (and a volatile store) lets gcc's scheduler hoist the ip update above the
// global store. 116-120 bytes, so it cannot be committed.
/*
void func_8009FB98(void) {
    g_GameSceneMapNum |= 0xC000;
    GameWaitForCdData();
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    D_800B2268 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
*/

extern s32 D_8006F990[];
int func_8009FC10(int actorIndex) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_8006F990[i] == actorIndex) {
            return i;
        }
    }
    
    return 0xFF;
}

void FieldScriptPartyMemberRideGear(void) {
    int index = FieldScriptVMGetArgument(1);
    if (index >= 3) {
        index = 2;
    }
    g_pGameState->gearRide[index] = 1;
    func_8009FD10(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptPartyMemberDisembarkGear(void) {
    int index = FieldScriptVMGetArgument(1);
    if (index >= 3) {
        index = 2;
    }
    g_pGameState->gearRide[index] = 0;
    func_8009FD10(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009FD10(int partyMemberIndex) {
    switch (partyMemberIndex) { 
        case 0:
            FieldScriptMemoryWriteU16(0x2A, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x2C, 0);
            FieldScriptMemoryWriteU16(0x2E, 0);
            break;
        case 1:
            FieldScriptMemoryWriteU16(0x30, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x32, 0);
            FieldScriptMemoryWriteU16(0x34, 0);
            break;
        case 2:
            FieldScriptMemoryWriteU16(0x36, g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(0x38, 0);
            FieldScriptMemoryWriteU16(0x3A, 0);
            break;
    }
}

extern void func_800AD4D4(int partyMemberIndex);

// 99.50%: gcc puts the index in $v1, the original uses $a0 (`move a0,v0`).
void func_8009FDD4(void) {
    int partyMemberIndex;

    partyMemberIndex = func_8009FC10(D_800AFD1C);
    if (partyMemberIndex != 0xFF && g_pGameState->gearRide[partyMemberIndex] == 0) {
        func_800AD4D4(partyMemberIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

extern void func_800ACFD0(void);

void func_8009FE4C(void) {
    u8 index;

    index = SCRIPT_READ_U8_REL(1);
    if (g_GamePartyMembers[index] != CHARACTER_ID_NONE && g_pGameState->gearRide[index]) {
        func_800ACFD0();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FEE4);

void func_800A0158(s32 arg0, s32* arg1, s32* arg2, s32* arg3) {
    switch (arg0) {
        case 0:
            *arg1 = FieldScriptVMGetVariableValue(0x2A);
            *arg2 = FieldScriptVMGetVariableValue(0x2C);
            *arg3 = FieldScriptVMGetVariableValue(0x2E);
            break;
        case 1:
            *arg1 = FieldScriptVMGetVariableValue(0x30);
            *arg2 = FieldScriptVMGetVariableValue(0x32);
            *arg3 = FieldScriptVMGetVariableValue(0x34);
            break;
        case 2:
            *arg1 = FieldScriptVMGetVariableValue(0x36);
            *arg2 = FieldScriptVMGetVariableValue(0x38);
            *arg3 = FieldScriptVMGetVariableValue(0x3A);
            break;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0228);

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0524);
// 74.12%: with the `volatile` removed from `g_FieldActors`
// (include/field/actor.h) every store order, frame and call argument matches; the
// remaining 26% is load sharing. The original reads the base pointer once per
// block (`lw a0,%lo(g_FieldActors)` used for both the dst and src index, and a
// single `lw a1,0x0(s2)` feeding both matrix-call arguments), which needs a
// non-volatile pointer. With the header as-is each `g_FieldActors[...]` is its own
// volatile load, so the prologue/loop registers differ. Do not remove the
// qualifier without a full make check; other units may depend on it.
/*
extern void FieldMatrixCopyTransform(MATRIX* pDest, MATRIX* pSrc);
extern void FieldMatrixCopyTranslation(MATRIX* pDest, MATRIX* pSrc);

// Copies the walkmesh state and transform of one field actor onto another.
void func_800A0524(int dstActorIndex, int srcActorIndex) {
    ActorData* pDst;
    ActorData* pSrc;
    int i;

    pDst = g_FieldActors[dstActorIndex].pActorData;
    pSrc = g_FieldActors[srcActorIndex].pActorData;
    for (i = 0; i < 4; i++) {
        pDst->walkmeshTriIds[i] = pSrc->walkmeshTriIds[i];
    }
    pDst->walkmeshId = pSrc->walkmeshId;
    pDst->unkEC = pSrc->unkEC;
    pDst->curYPos = pSrc->curYPos;
    pDst->curTriNormal.vx = pSrc->curTriNormal.vx;
    pDst->curTriNormal.vy = pSrc->curTriNormal.vy;
    pDst->curTriNormal.vz = pSrc->curTriNormal.vz;
    pDst->position.vx = pSrc->position.vx;
    pDst->position.vy = pSrc->position.vy;
    pDst->position.vz = pSrc->position.vz;
    FieldMatrixCopyTransform(&g_FieldActors[dstActorIndex].transformMatrix, &g_FieldActors[srcActorIndex].transformMatrix);
    FieldMatrixCopyTranslation(&g_FieldActors[dstActorIndex].transformMatrix, &g_FieldActors[srcActorIndex].transformMatrix);
    ((SpriteData*)g_FieldActors[dstActorIndex].pSpriteData)->position.x = g_FieldActors[srcActorIndex].pActorData->position.vx;
    ((SpriteData*)g_FieldActors[dstActorIndex].pSpriteData)->position.y = g_FieldActors[srcActorIndex].pActorData->position.vy;
    ((SpriteData*)g_FieldActors[dstActorIndex].pSpriteData)->position.z = g_FieldActors[srcActorIndex].pActorData->position.vz;
}

*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A06E8);
// 94.95%: structure right (branches, calls, frame, 464 bytes). gcc CSEs the
// `~ACTOR_STATUS_INVISIBLE` constant into a callee-saved register across the
// func_800A0C94 call (`li s0,-33; and v1,v1,s0`) where the original uses
// `andi 0xffdf` twice, which then drags the D_800AFD20 store earlier.
/*
void func_800A06E8(void) {
    FieldActor* pFieldActor;
    int partyId;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    partyId = FieldCharacterIdToPartyId(func_8008CF3C(FieldScriptVMGetArgument(1)));
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    if (partyId != -1) {
        func_80076AC0(D_800AFD1C, partyId, g_PartyDataBuffers[partyId], 2, 0, partyId, 1);
        D_800AFD20 = -0xC0;
        g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
        func_800A0C94();
        g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
        g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
        g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
    } else {
        func_80076AC0(D_800AFD1C, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);
        g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x0 = 1;
        D_800AFFEC = 1;
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->flags |= 0x100000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A08B8);

extern s32 g_PlayerActorIndex;

void func_800A0C4C(void) {
    g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags |= 0x80;
}

// FieldResetActorPosition: copies the current actor's scripted position into the
// field actor's transform/child matrices and into its sprite.
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0C94);
// 99.64%: every instruction matches except the destination register of the third
// block's `addu`: the original coalesces base+offset into the index register
// (`addu v1,v1,v0`, then the two stores off $v1); gcc 2.7.2 always coalesces it
// into the freshly loaded base register (`addu v0,v1,v0`). Same size and operand
// order; -O2/-O3 both do this. index+base operand order is required (base-first
// gives `addu v0,v0,v1`).
/*
void func_800A0C94(void) {
    FieldActor* pFieldActor;
    SpriteData* pSpriteData;
    ActorData* pActor;
    s16* pPosition;
    s32 x;
    s32 y;
    s32 z;
    s32 offset;
    u16 curY;

    pActor = g_FieldScriptVMCurActor;
    offset = D_800AFD1C * sizeof(FieldActor);
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pPosition = (s16*)&pActor->position;

    x = pPosition[1];
    pSpriteData = pFieldActor->pSpriteData;
    pFieldActor->transformMatrix.t[0] = x;
    pFieldActor->childMatrix.t[0] = x;

    y = pPosition[3];
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->transformMatrix.t[1] = y;
    pFieldActor->childMatrix.t[1] = y;

    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    z = pPosition[5];
    pFieldActor->transformMatrix.t[2] = z;
    pFieldActor->childMatrix.t[2] = z;

    pSpriteData->position.x = pActor->position.vx;
    pSpriteData->position.y = pActor->position.vy;
    pSpriteData->position.z = pActor->position.vz;
    pSpriteData->step.y = 0;

    curY = ((u16*)pPosition)[3];
    pSpriteData->field_0x84 = curY;
    pActor->curYPos = curY;
}
*/
void func_800A0D3C(void) {
    func_80076AC0(D_800AFD1C, 0, (void*)((*(s32*)(g_FieldSpriteData + 4)) + (s32)g_FieldSpriteData), 0, 0, 0x80, 1);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x8 = 0x1;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

typedef struct { s16 value; } FieldScriptGlobalS16;
extern FieldScriptGlobalS16 D_800B234A;

void func_800A0DC0(void) {
    D_800B234A.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_800A0DFC(void) {
    int nAddress;

    nAddress = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    FieldScriptMemoryWriteU16(nAddress, ArchiveGetDiscNumber());
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern int D_800ADB74;

void func_800A0E54(void) {
    if (D_800ADB74 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
    }
    D_800B00C0 = 1;
}

extern int D_800ADB84;

void func_800A0EB0(void) {
    D_800B00C0 = 1;
    D_800ADB84++;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0EE8);
// 90.55%: dispatch and case 0 match exactly. Only case 1 differs - the original
// materialises &D_800B2264 into a register (lui+addiu, then lw/sw off it), gcc
// emits %hi/%lo per access and reloads the actor first. Pointer (`s32* p =
// &D_800B2264; (*p)--;`) and array (`D_800B2264[0]--`) forms both produce the same
// %hi/%lo code. Result is one insn short (236 vs 240 bytes).
/*
extern s32 D_800B2264;
extern void* D_801E8670[];
extern void func_801E8030(int modelIndex);

void func_800A0EE8(void) {
    int nModelIndex;

    nModelIndex = g_FieldScriptVMCurActor->flags12C_0xE;
    g_FieldScriptVMCurActor->flags &= ~0x2000;
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            ((u8*)D_801E8670[nModelIndex])[0x34] = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 1:
            func_801E8030(g_FieldScriptVMCurActor->flags12C_0xE);
            D_800B2264--;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
    }
    D_800B00C0 = 1;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0FD8);

INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A1364);
// 84.19%: frame is exactly right now (u8 unused[8] reproduces the 0x30 frame with
// ra/s0 at 0x2C/0x28). Only gap left: gcc merges the last two loads of
// D_800B2264 (one load, `addiu` for the ++) and puts `sw D_800B2264` before
// `sw 0x12C(g_FieldScriptVMCurActor)`; the original reloads D_800B2264 after the
// flags12C store and keeps the ++ store last, i.e. its alias analysis treats the
// bitfield RMW as aliasing D_800B2264.
/*
void func_800A1364(void) {
    int arg;
    FieldActor* pFieldActor;
    u8 unused[8]; // original has an unused stack slot; reproduces the frame

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    func_80076AC0(D_800AFD1C, 0, (void*)((*(s32*)(g_FieldSpriteData + 4)) + (s32)g_FieldSpriteData), 0, 0, 0x80, 1);
    arg *= 2;
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->flags |= 0x2000;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    D_800B21DC[D_800B2264] = arg;
    D_800B225F[D_800B2264] = 0;
    g_FieldScriptVMCurActor->flags12C_0xE = D_800B2264 & 7;
    D_800B2264++;
}
*/

void func_800A14F0(void) {
    int arg;
    int arg3;
    FieldActor* pFieldActor;
    void* pSprite;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    pSprite = (void*)((*(s32*)(g_FieldSpriteData + arg * 4 + 4)) + (s32)g_FieldSpriteData);
    arg3 = FieldScriptVMGetArgument(3);
    func_80076AC0(D_800AFD1C, arg, pSprite, 0, arg3, arg | 0x80, 1);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
}

void func_800A1624(void) {
    int arg;
    FieldActor* pFieldActor;

    pFieldActor = &g_FieldActors[D_800AFD1C];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    func_80076AC0(D_800AFD1C, arg, (void*)((*(s32*)(g_FieldSpriteData + arg * 4 + 4)) + (s32)g_FieldSpriteData), 0, 0, arg | 0x80, 0);
    func_800A0C94();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[D_800AFD1C].status &= ~ACTOR_STATUS_INVISIBLE;
}
