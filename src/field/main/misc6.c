#include "common.h"
#include "main/game.h"
#include "system/archive.h"
#include "system/math.h"
#include "system/memory.h"
#include "field/script_vm.h"
#include "field/actor.h"
#include "field/text_box.h"
#include "field/main.h"
#include "field/camera.h"

extern void func_80076AC0(s32, s32, void*, s32, s32, s32, s32);
extern void FieldActorApplyPositionTransform();
extern int ArchiveGetDiscNumber(void);
extern void FieldScriptMemoryWriteU16(int, int);
extern s32 g_FieldCurActorIndex; // Current actor index
// Unbounded array so cc1 materialises the base in a register for a read-modify-write.
extern s32 g_FieldActorModelSlotCount[];
extern u8 g_FieldActorModelSpriteRow[];
extern s16 g_FieldActorModelSlotArchiveBase[];
extern s32 g_FieldSceneReloadFlags;
extern int g_GameSceneMapNum;
extern void GameWaitForCdData(void);
extern void GamePartySyncSkinData(void);
extern void GamePartySyncStreamedData(void);
extern int FieldPartySlotToCharacter(int);
extern void* g_PartyDataBuffers[];
extern s32 g_FieldScriptVMScriptEndFlag;
extern s16 D_800AFD20;
extern u16 g_FieldP1ButtonsPressed;
extern u16 g_FieldP1ButtonsReleased;
extern s32 g_FieldRequestCode;
extern s16 g_FieldTalkIdleFrames; // Frames the player has gone without moving while interacting
extern s32 D_800ADB28;
extern s32 D_800ADB68;
extern u16 D_800ADF68[]; // Camera-relative facing angle per d-pad direction
extern u16 D_800ADF88[];
// The four shorts at 0x800B2340 are one object (see field/party/stats.c).
extern u16 g_FieldTalkShorts[];
extern s16 D_800B2342;
extern u8 g_FieldAlternatePadAngles;
extern u8 g_FieldPartyMoveStepRan;
extern s32 g_FieldSpriteUpdateIndex;
extern int FieldActorTestMaterialScriptMask(ActorData* pActor);
extern void FieldUpdateRandomSkinSelection(void);
extern s16 g_Unknown_800B2344;

#define FIELD_ENCOUNTER_HOLD_FRAMES 0x20 // Frames a held confirm repeats as a press
#define FIELD_TEXTBOX_COUNT 4
// Script variables holding a party member's stored map id and position: three per
// member (map, x, z), six apart, the first member's map at 0x2A.
#define FIELD_SCRIPT_VAR_PARTY_MAP(member) (0x2A + (member) * 6)
#define FIELD_SCRIPT_VAR_PARTY_X(member) (FIELD_SCRIPT_VAR_PARTY_MAP(member) + 2)
#define FIELD_SCRIPT_VAR_PARTY_Z(member) (FIELD_SCRIPT_VAR_PARTY_MAP(member) + 4)
// The two consecutive archive entries a model slot streams.
#define FIELD_ARCHIVE_MODEL_ENTRY 0x6BA

void FieldScriptVMHandlerDisableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerEnableDialogActivation(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xA = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerSetScriptFlag16(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerClearScriptFlag16(void) {
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x16 = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerDisableAndHideActor(void) {
    int nTextBoxIndex;
    ActorData* pActor;
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        pActor->scriptFlags.fields.scriptFlags_0x0 = 0x1;
        pActor->flags |= 0x100000;
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->status |= ACTOR_STATUS_INVISIBLE;
        if (FieldTextBoxFindCurrentActorTextBox(&nTextBoxIndex) == 0) {
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

void FieldScriptVMHandlerDisableActorAndResetMotion(void) {
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
        if (FieldTextBoxFindCurrentActorTextBox(&nTextBoxIndex) == 0) {
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

    g_FieldScriptVMStopFlag = 1;
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
    FieldActor* pActor = &g_FieldActors[g_FieldCurActorIndex];
    pActor->status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->curAnimationId = 0xFF;
    g_FieldScriptVMCurActor->flags &= 0xFDFFFFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerSetActorFlags2000800ById(void) {
    FieldActor* pFieldActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        pFieldActor->pActorData->flags |= 0x02000000;
        pFieldActor->pActorData->flags |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerSetActorFlags2000800(void) {
    g_FieldScriptVMCurActor->flags |= 0x02000800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerHideActor(void) {
    FieldActor* pActor = &g_FieldActors[g_FieldCurActorIndex];
    pActor->status |=  ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// Update sprite movement or animation speed?
void FieldScriptVMHandlerSetActorMoveSpeed(void) {
    unsigned short nMoveSpeed;

    nMoveSpeed = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->moveSpeed = nMoveSpeed;
    SpriteSetSpeedScale(g_FieldActors[g_FieldCurActorIndex].pSpriteData, nMoveSpeed);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetScriptFlagsFromArgument(void) {
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

void FieldScriptVMHandlerSetActorFlagsFromArgument(void) {
    u32 nValue;

    nValue = SCRIPT_READ_U8_REL(1) & 7;
    g_FieldScriptVMCurActor->flags &= ~7;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    nValue = (SCRIPT_READ_U8_REL(1) >> 1) & 0x38;
    g_FieldScriptVMCurActor->flags &= ~0x38;
    g_FieldScriptVMCurActor->flags |= nValue;
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerCommitActorYPosition(void) {
    g_FieldScriptVMCurActor->scriptYPos = 0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x0;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x15 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
    g_FieldScriptVMCurActor->curYPos = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
}

void FieldScriptVMHandlerMoveActorToPosition(void) {
    func_8009E574(
        FieldScriptVMGetInstructionArgumentS16(1),
        FieldScriptVMGetInstructionArgumentS16(3)
    );
    FieldScriptSetActorYPosition(FieldScriptVMGetInstructionArgumentS16(5));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerSetActorYFromArgument(void) {
    FieldScriptSetActorYPosition(FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3)));
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x10 = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Read short from bytecode, absolute offset
short FieldScriptReadInstructionS16(int offset) {
    return SCRIPT_READ_U8(offset) + (SCRIPT_READ_U8(offset + 1) << 8);
}

void FieldScriptVMHandlerMoveActorToIndirectPositionWithWalkmesh(void) {
    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(5);
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(6)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(6))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerSnapActorToWalkmesh(void) {
    ActorData* pActor;

    g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[g_FieldCurActorIndex].pActorData;
    func_8009E574(
        CONV_TO_GTE(pActor->position.vx), 
        CONV_TO_GTE(pActor->position.vz)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerMoveActorToIndirectPosition(void) {
    func_8009E574(
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)), 
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5))
    );
    g_FieldScriptVMCurActor->flags &= ~0x200000;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}


// PARKED 80.25% (656 bytes; retail 664 / 167 insns). No twin, m2c draft agrees
// with the body, so the structure is settled; the instruction MULTISET is identical
// and the only size gap is the three load-delay nops retail carries because its
// schedule is tighter, so this body cannot link as it stands.
// What this redo added (79.89 -> 80.25): the transform/child matrix t[1] pair needs
// points[walkmeshId].vy in ONE s16 local. Retail reads it once and stores it twice
// and so must the C: an inline second read costs 7 instructions, because the store
// through the pFieldActor base invalidates the stack load.
// Presets ruled out on this body: 2.6.0 77.34, 2.7.2-cdk 79.80, Default 80.25.
// Residual (a) schedule: retail puts sh v0,8(a0) where the address is computed and
// fills the bnez slot with addiu s1,s1,8; gcc hoists the points bump to the top of
// the body and sinks sh into the delay slot, and it hoists the two g_FieldActors
// loads of the t[1]/t[2] pairs together where retail interleaves them. Statement
// order is already retail's.
// Residual (b) allocator: retail keeps pActor in $a1 and &normals[0] in $a0, this
// build gives $a0 and $a2 and an extra $t0 for the pair bases. Two declaration
// orders measured earlier were byte-identical; '&g_FieldActors[idx]' instead of the
// offset local scores 80.22 and loses retail's index-first addu v0,a0,v0.
// Tried, all worse or equal: pointer-local do-while loop with &points[i] as a giv
// (77.55), '&g_FieldActors[idx]' per pair (80.22), m2c's per-pair temp pointers
// (unchanged).
/*
extern s16 g_FieldWalkmeshTriCount;
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern u_int FieldActorGetWalkmeshTriMaterial(ActorData* pActor);

void func_8009E574(s32 nX, s32 nY) {
    VECTOR normals[4];
    SVECTOR points[4];
    ActorData* pActor;
    SpriteData* pSprite;
    int i;
    FieldActor* pFieldActor;
    s16 nWalkY;
    s32 offset;

    pSprite = (SpriteData*)g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    for (i = 0; i < g_FieldWalkmeshTriCount - 1; i++) {
        g_FieldScriptVMCurActor->walkmeshTriIds[i] = func_8007B1C4(nX, nY, i, &points[i], &normals[i]);
    }
    g_FieldScriptVMCurActor->curWalkmeshTriMaterial = FieldActorGetWalkmeshTriMaterial(g_FieldScriptVMCurActor);
    pActor = g_FieldScriptVMCurActor;
    pActor->curTriNormal.vx = normals[pActor->walkmeshId].vx;
    pActor->curTriNormal.vy = normals[pActor->walkmeshId].vy;
    pActor->curTriNormal.vz = normals[pActor->walkmeshId].vz;
    offset = g_FieldCurActorIndex * sizeof(FieldActor);
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->transformMatrix.t[0] = nX;
    pFieldActor->childMatrix.t[0] = nX;
    nWalkY = points[pActor->walkmeshId].vy;
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->transformMatrix.t[1] = nWalkY;
    pFieldActor->childMatrix.t[1] = nWalkY;
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->transformMatrix.t[2] = nY;
    pFieldActor->childMatrix.t[2] = nY;
    pSprite->field_0x84 = points[pActor->walkmeshId].vy;
    pActor->position.vx = nX << 16;
    pActor->position.vz = nY << 16;
    pActor->position.vy = points[pActor->walkmeshId].vy << 16;
    pActor->curYPos = points[pActor->walkmeshId].vy;
    pSprite->position.x = pActor->position.vx;
    pSprite->position.y = pActor->position.vy;
    pSprite->position.z = pActor->position.vz;
    pActor->move.vx = 0;
    pActor->move.vy = 0;
    pActor->move.vz = 0;
    pActor->moveModified.vx = 0;
    pActor->moveModified.vy = 0;
    pActor->moveModified.vz = 0;
    pActor->moveStep.vx = 0;
    pActor->moveStep.vy = 0;
    pActor->moveStep.vz = 0;
    pActor->positionOffset.vy = 0;
    pActor->positionOffset.vx = 0;
    pActor->positionOffset.vz = 0;
    pSprite->step.x = 0;
    pSprite->step.y = 0;
    pSprite->step.z = 0;
    pActor->fallSpeed = 0;
    pActor->scriptYPos = 0;
    pActor->curYPos = ((s16*)&pActor->position)[3];
    pActor->scriptFlags.flags = (pActor->scriptFlags.flags & ~0x40000) | 0x400000;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009E574);


// Writes the actor's Y position (fixed point) and the cached short copies
void FieldScriptSetActorYPosition(short nY) {
    u8 unused[0x18]; // original has an unused stack slot; reproduces the frame

    g_FieldScriptVMCurActor->position.vy = nY * 0x10000;
    g_FieldScriptVMCurActor->scriptYPos = nY;
    g_FieldScriptVMCurActor->curYPos = nY;
}

void FieldScriptVMHandlerSetActorCollisionSize(void) {
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

void FieldScriptVMHandlerSnapshotIndirectArguments(void) {
    if (!g_FieldScriptVMCurActor->flags12C_0xD) {
        g_FieldScriptVMCurActor->pScriptArgs = HeapAlloc(0x10, 0x0);
    }
    g_FieldScriptVMCurActor->flags12C_0xD = 1;
    g_FieldScriptVMCurActor->pScriptArgs->arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg2 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg3 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg4 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg5 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg6 = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg7 = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->pScriptArgs->arg8 = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
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

void FieldScriptVMHandlerStartActorScript(void) {
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
            pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 0;
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

void FieldScriptVMHandlerStartActorScriptAndWait(void) {
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
        pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 0;
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
                pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                g_FieldScriptVMCurActor->waitScriptIndex = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                break;
            }
            return;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->waitScriptIndex) || pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 0;
            } else {
                g_FieldScriptVMStopFlag = g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state;
            }
            return;
        default:
            return;
        }
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerStartActorScriptAndWait2(void) {
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
        pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 0;
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
                pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 1;
                g_FieldScriptVMCurActor->waitScriptIndex = i;
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 1;
                pActor->scripts[i].scriptId = SCRIPT_READ_U8_REL(2) & 0x1F;
                return;
            }
            break;
        case 1:
            if ((pActor->curScriptIndex == g_FieldScriptVMCurActor->waitScriptIndex) || pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = 2;
                return;
            }
            g_FieldScriptVMStopFlag = 1;
            break;
        case 2:
            if (pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].flags_0x12 == 0xF) {
                g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].state = SCRIPT_STATE_IDLE;
                pActor->scripts[g_FieldScriptVMCurActor->waitScriptIndex].isInUse = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
                return;
            }
            g_FieldScriptVMStopFlag = 1;
            break;
        }
        return;
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Randomize X rotation
void FieldScriptVMHandlerRandomizeActorXRotation(void) {
    int nRotation;
    
    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->frameCounter++;
    if (!(g_FieldScriptVMCurActor->frameCounter & 0xF)) {
        if (!(rand() & 0x1)) {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy + PSX_DEGREES(45));
        } else {
            nRotation = PSX_ANGLE(g_FieldScriptVMCurActor->rotation.vy - PSX_DEGREES(45));
        }
    }
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerRandomizeActorXRotation2(void) {
    short nSpecialRotation;
    int nRotation;
    int nRand;
    int nDeltaRotation;
    int delta;

    nRotation = g_FieldScriptVMCurActor->rotation.vy;
    g_FieldScriptVMCurActor->frameCounter++;
    if (!(g_FieldScriptVMCurActor->frameCounter & 0xF)) {
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
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->rotation.vx = nRotation;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerStartEncounter(void) {
    unsigned short nStoredIP = g_FieldScriptVMCurActor->scriptInstructionPointer;
    FieldScriptVMHandlerCheckActorInteraction(); // Do Encounter
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = nStoredIP;
}


// Per-frame confirm/release check for the actor the party is standing at: finds a
// free textbox, bumps the encounter hold counter, arms the message state and turns
// the actor to the d-pad direction (relative to the camera). Called by
// FieldScriptVMHandlerStartEncounter, which undoes the trailer's IP advance.
// The talk repeat timer is g_FieldTalkShorts[1]: the tests read it signed through
// D_800B2342, the decrement goes through the array so the address is formed once.
void FieldScriptVMHandlerCheckActorInteraction(void) {
    // Retail's frame is 0x60 and stores nothing but $ra.
    u8 pad[0x48];
    u16 nAngle;
    s32 nTextBoxIndex;
    s32 nInputFree;
    s32 nSavedMessageState;
    ActorData* pActor;

    nAngle = 0;
    if (g_FieldScriptVMCurActor->scriptFlags.flags & 0x4000) {
        for (nTextBoxIndex = 0; nTextBoxIndex < FIELD_TEXTBOX_COUNT; nTextBoxIndex++) {
            if (g_FieldTextBoxes[nTextBoxIndex].cursor.visibility == 0) {
                break;
            }
        }
        nInputFree = -(nTextBoxIndex == FIELD_TEXTBOX_COUNT);
        if (nInputFree == -1 && g_FieldControl.isRandomEncountersEnabled == 0) {
            if ((g_FieldP1ButtonsPressed >> 12) != 0) {
                FieldUpdateRandomSkinSelection();
            }
            D_800ADB68 = 1;
            if (g_FieldScriptVMCurActor->curWalkmeshTriMaterial & 0x400000) {
                if (g_FieldScriptVMCurActor->prevPosition.x == ((s16*)&g_FieldScriptVMCurActor->position.vx)[1]
                    && g_FieldScriptVMCurActor->prevPosition.y == ((s16*)&g_FieldScriptVMCurActor->position.vy)[1]
                    && g_FieldScriptVMCurActor->prevPosition.z == ((s16*)&g_FieldScriptVMCurActor->position.vz)[1]) {
                    g_FieldTalkIdleFrames++;
                }
            } else {
                g_FieldTalkIdleFrames = 0;
            }
            if (g_FieldTalkIdleFrames > FIELD_ENCOUNTER_HOLD_FRAMES) {
                g_FieldTalkIdleFrames = FIELD_ENCOUNTER_HOLD_FRAMES;
                if ((g_FieldP1ButtonsPressed & 0x80)
                    && !(g_FieldScriptVMCurActor->scriptFlags.flags & 0x1800)
                    && g_FieldRequestCode == 0xFF) {
                    goto confirm;
                }
            }
            if (g_Unknown_800B2344 != 0) {
                goto talk;
            }
            if (!(g_FieldP1ButtonsReleased & 0x80)
                || (g_FieldScriptVMCurActor->scriptFlags.flags & 0x1800)
                || (g_FieldScriptVMCurActor->curWalkmeshTriMaterial & 0x400000)
                || g_FieldRequestCode != 0xFF) {
                goto buttons;
            }
        confirm:
            if (!FieldActorTestMaterialScriptMask(g_FieldScriptVMCurActor)) {
                *(volatile u32*)&g_FieldScriptVMCurActor->scriptFlags.flags |= 0x800;
                D_800ADB28 = g_FieldSpriteUpdateIndex;
            }
            goto buttons;
        talk:
            if ((g_FieldP1ButtonsReleased & 0x80) && D_800B2342 == 0
                && g_FieldRequestCode == 0xFF
                && !FieldActorTestMaterialScriptMask(g_FieldScriptVMCurActor)) {
                pActor = g_FieldScriptVMCurActor;
                *(volatile u32*)&pActor->scriptFlags.flags |= 0x800;
                nSavedMessageState = g_FieldSpriteUpdateIndex;
                *(volatile s16*)&pActor->curAnimationId = 0xFF;
                D_800ADB28 = nSavedMessageState;
                D_800B2342 = g_FieldTalkShorts[0];
            }
            if (D_800B2342 != 0) {
                g_FieldTalkShorts[1]--;
            }
        buttons:
            if (g_FieldAlternatePadAngles == 0) {
                nAngle = D_800ADF68[(g_FieldP1ButtonsPressed >> 12) ^ 0xF];
            } else {
                nAngle = D_800ADF88[(g_FieldP1ButtonsPressed >> 12) ^ 0xF];
            }
            if (!(nAngle & 0x8000)) {
                s32 nCameraAngle = g_CamInterpolation.targetAngleY;

                nAngle = (nAngle - nCameraAngle) & 0xFFF;
            }
            g_FieldScriptVMCurActor->rotation.vx = nAngle;
        } else {
            g_FieldScriptVMCurActor->rotation.vx = nAngle | 0x8000;
        }
    } else if (g_FieldPartyMoveStepRan == 0) {
        g_FieldScriptVMCurActor->scriptFlags.flags |= 0x1000000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

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

// Screened 36 variants (2.7.2 / cdk / 2.6.0 x -O1/-O2/-O3 x -G4/-G8): best is
// 90.86% here, cdk 87.35%, 2.6.0 90.86% - the 0x7C store's sll/sra pair is left.
// Also tried: the two-return shape `if (SCRIPT_READ_U8(0) == 0xFF) { ...body...;
// return 0; } return 0;` reproduces retail's `bne`-to-shared-epilogue and the exact
// 0x20 frame (90.25%, 320/324 bytes). Only the 0x7C store's sll/sra pair is left;
// gcc folds it because it can prove `((x + 4) & 7) << 9` fits in 13 bits (tried
// s32 nAngle, an `nAngle << 16 >> 16` temp, and reading 0x56 back).
// 90.86% (316 of 324 bytes): frame (0x20 with ra/s2/s1/s0), the `sll s0; sra s0`
// of nX in the 2nd call's delay slot, both call orders and the rotation value are
// right. `s16 nAngle` and `s32 nX` with an explicit (s16) cast on the call are
// what fix those; `s16 nRotation` instead CSEs the 0x8000 constant into $s0, so
// nRotation stays s32. Remaining 8 bytes: (a) the original emits
// `sll a0,a1,16; sra v0,a0,16` for the 0x7C store and shares the sll with the
// 0x60 store, i.e. nAngle is sign-extended because it is a narrow variable;
// gcc folds the conversion away here (and then re-emits a lone `sll` for 0x60),
// it only keeps the pair when the 0x60 store comes first, which then puts the
// three stores in the wrong order (88.83%, 320 bytes); (b) the original keeps
// `addu v0,zero,zero` in the bne delay slot, i.e. it has two `return 0` sites
// that gcc merged into a shared epilogue; a single trailing return puts it after
// the rotation stores and an if/else doubles it into a `j` (90.25%, 320 bytes).
// Also tried: reading 0x7C back through g_Scene.sceneAngle.vy (folded), a named
// s32 fixed-point temp (folded), `unk7C = nAngle` with s16 nAngle (folded).
// Re-measured this round (do not repeat):
// - `(s16)nAngle` on the vy/0x7C stores and `nAngle << 16 >> 16` inline both fold
//   to a lone `sll` (same 316 bytes); a named s32 fixed-point temp folds too AND
//   grows the frame to 0x28 (320 bytes, 90.12).
// - cc1_probe on the store group alone: the `sll a0; sra v0` pair DOES appear when
//   the angle has a single assignment, but every spelling that carries it through
//   the `if (nAngleByte == 0xFF)` join folds `(a << 16) >> 16` back to `a` (three
//   variants). So the missing sra is the phi letting cc1 bound the value, not the
//   store spelling.
// - the two-return shape measured: 320 bytes at 90.25 and it adds a `j` retail does
//   not have (retail's last store falls through into the epilogue), so the
//   one-return body below is the better park.
// - residual (b) `addu v0,zero,zero` in the bne delay slot: reorg fills that slot
//   with the `sll` here; the constant lands at the end of the rotation stores.
// BEST 90.86%, 316 of retail's 324 bytes (79 vs 81 insns). Both missing insns are one
// cause: retail keeps the return value 0 live in $v0 from the top of the function, so
// (a) the 'bne' delay slot holds 'addu v0,zero,zero' and (b) the angle had to be
// allocated to $a1, which is what makes the 0x7C store's 'sll a0,a1,16; sra v0,a0,16'
// possible at all. cc1 folds the constant 0 into the epilogue, so $v0 is free for the
// phi and the angle lands in it instead ('sll v0,v0,9'), which also frees the sra.
// Measured this round (do not repeat):
// - 'if (READ_U8(0) != 0xFF) return 0; body; return 0;' (early return first): 89.57%,
//   320 bytes / 80 insns - cc1 inverts it to 'beq ... ; j epilogue; move v0,zero'.
// - 's32 nResult = 0; if (...) {body} return nResult;': identical to the park (316) -
//   cc1 folds nResult to the constant, so the move is still not hoisted.
/*
s32 func_8009FA54(s32 nInstructionOffset) {
    s32 nOffset;
    s32 nX;
    s32 nY;
    u8 nAngleByte;
    s16 nAngle;
    u8 nRotationByte;
    s32 nRotation;
    ActorData* pActor;

    if (SCRIPT_READ_U8(0) == 0xFF) {
        nOffset = nInstructionOffset * 7;
        g_FieldScriptVMCurActor->walkmeshId = SCRIPT_READ_U8(nOffset + 5);
        nX = (s16)FieldScriptReadInstructionS16(nOffset + 1);
        nY = (s16)FieldScriptReadInstructionS16(nOffset + 3);
        func_8009E574(nX, nY);

        nAngleByte = SCRIPT_READ_U8(nOffset + 6);
        nAngle = ((nAngleByte + 4) & 7) << 9;
        if (nAngleByte == 0xFF) {
            nAngle = ((FieldScriptVMGetVariableValue(8) + 4) & 7) << 9;
        }
        g_Scene.sceneAngle.vy = nAngle;
        g_Scene.sceneAngleYTarget = nAngle;
        g_Scene.sceneAngleCurrent = nAngle << 16;

        nRotationByte = SCRIPT_READ_U8(nOffset + 7);
        nRotation = ((nRotationByte - 2) & 7) << 9 | 0x8000;
        if (nRotationByte == 0xFF) {
            nRotation = ((FieldScriptVMGetVariableValue(6) - 2) & 7) << 9 | 0x8000;
        }
        pActor = g_FieldScriptVMCurActor;
        pActor->rotation.vx = nRotation;
        pActor->rotation.vy = nRotation;
        pActor->rotation.vz = nRotation;
    }
    return 0;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_8009FA54);

void FieldScriptVMHandlerReloadSceneMapData(void) {
    ActorData* pActor;

    g_GameSceneMapNum |= 0xC000;
    GameWaitForCdData();
    GamePartySyncSkinData();
    GamePartySyncStreamedData();
    pActor = g_FieldScriptVMCurActor;
    g_FieldSceneReloadFlags = SCRIPT_READ_U8_REL(1);
    // gcc schedules the store of the new IP after the script read only inside a block.
    do {
        pActor->scriptInstructionPointer += 2;
    } while (0);
}

extern s32 g_FieldPartyActorIndices[];
int FieldGetPartySlotByActorIndex(int actorIndex) {
    int i;

    for (i = 0; i < 3; i++) {
        if (g_FieldPartyActorIndices[i] == actorIndex) {
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
    FieldScriptResetPartyMemberPosition(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptPartyMemberDisembarkGear(void) {
    int index = FieldScriptVMGetArgument(1);
    if (index >= 3) {
        index = 2;
    }
    g_pGameState->gearRide[index] = 0;
    FieldScriptResetPartyMemberPosition(index);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptResetPartyMemberPosition(int partyMemberIndex) {
    switch (partyMemberIndex) { 
        case 0:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(0), g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(0), 0);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(0), 0);
            break;
        case 1:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(1), g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(1), 0);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(1), 0);
            break;
        case 2:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(2), g_GameSceneMapNum & 0xFFF);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(2), 0);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(2), 0);
            break;
    }
}

extern void func_800AD4D4(int partyMemberIndex);

// 99.50%: gcc puts the index in $v1, the original uses $a0 (`move a0,v0`).
void FieldScriptVMHandlerPartyMemberOnFoot(void) {
    int partyMemberIndex;

    partyMemberIndex = FieldGetPartySlotByActorIndex(g_FieldCurActorIndex);
    if (partyMemberIndex != 0xFF && g_pGameState->gearRide[partyMemberIndex] == 0) {
        func_800AD4D4(partyMemberIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

extern void func_800ACFD0(void);

void FieldScriptVMHandlerPartyMemberInGear(void) {
    u8 index;

    index = SCRIPT_READ_U8_REL(1);
    if (g_GamePartyMembers[index] != CHARACTER_ID_NONE && g_pGameState->gearRide[index]) {
        func_800ACFD0();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

// Sprite data blob (g_FieldSpriteData). This unit only uses the offset table at
// +4; each entry is a sprite offset relative to the blob itself.
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 offsets[1];
} FieldSpriteDataBlob;

extern s32 g_FieldPartyActorSlots[];

// The non-void return type is required: declared void, cc1 (reorg) moves `sll
// v0,v1,1` into the `beq nActorIndex,0xFF` delay slot instead of a `nop`.
// position is 16.16 fixed point, so halves 1 and 5 are the integer vx and vz
// that the script variables hold; no ActorData field names those halfwords.
// The slot-table pointer local is load-bearing: reading g_FieldPartyActorSlots
// inline instead is 95.63% at 620 bytes.
int FieldScriptWritePartyMemberPosition(int partyMemberIndex) {
    s32* pActorSlots;
    s32 nActorIndex;
    s32 nWalkmeshFlags;
    ActorData* pActorData;

    pActorSlots = g_FieldPartyActorSlots;
    nActorIndex = pActorSlots[partyMemberIndex];
    if (nActorIndex == ACTOR_ID_INVALID) {
        return;
    }
    pActorData = g_FieldActors[nActorIndex].pActorData;
    nWalkmeshFlags = pActorData->walkmeshId << 14;
    switch (partyMemberIndex) {
        case 0:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(0), (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(0), ((s16*)&g_FieldActors[pActorSlots[0]].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(0), ((s16*)&g_FieldActors[pActorSlots[0]].pActorData->position)[5]);
            break;
        case 1:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(1), (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(1), ((s16*)&g_FieldActors[pActorSlots[1]].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(1), ((s16*)&g_FieldActors[pActorSlots[1]].pActorData->position)[5]);
            break;
        case 2:
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_MAP(2), (g_GameSceneMapNum & 0xFFF) | nWalkmeshFlags);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_X(2), ((s16*)&g_FieldActors[pActorSlots[2]].pActorData->position)[1]);
            FieldScriptMemoryWriteU16(FIELD_SCRIPT_VAR_PARTY_Z(2), ((s16*)&g_FieldActors[pActorSlots[2]].pActorData->position)[5]);
            break;
    }
}

void FieldScriptReadPartyMemberPosition(s32 arg0, s32* arg1, s32* arg2, s32* arg3) {
    switch (arg0) {
        case 0:
            *arg1 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_MAP(0));
            *arg2 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_X(0));
            *arg3 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_Z(0));
            break;
        case 1:
            *arg1 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_MAP(1));
            *arg2 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_X(1));
            *arg3 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_Z(1));
            break;
        case 2:
            *arg1 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_MAP(2));
            *arg2 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_X(2));
            *arg3 = FieldScriptVMGetVariableValue(FIELD_SCRIPT_VAR_PARTY_Z(2));
            break;
    }
}

extern int GameCharacterGetGearID(int characterId);
extern void FieldScriptVMHandlerSetActorModelFromSpriteIndex0(void);
extern s32 g_FieldPartyActorIndices[3];
extern s16 g_FieldWalkmeshTriCount;

void FieldScriptVMHandlerSwitchPartyLeader(void) {
    int partyIndex;
    int nFlag;
    s32 nMapData;
    s32 nX;
    s32 nY;
    FieldActor* pFieldActor;
    ActorData* pCurActor;

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    partyIndex = FieldScriptVMGetArgument(1);
    if (partyIndex >= 3) {
        partyIndex = 2;
    }
    g_FieldPartyActorIndices[partyIndex] = g_FieldCurActorIndex;
    nFlag = 1;
    if (g_GamePartyMembers[partyIndex] != CHARACTER_ID_NONE &&
        GameCharacterGetGearID(g_GamePartyMembers[partyIndex]) != CHARACTER_ID_NONE) {
        FieldScriptReadPartyMemberPosition(partyIndex, &nMapData, &nX, &nY);
        pCurActor = g_FieldScriptVMCurActor;
        pCurActor->walkmeshId = (nMapData >> 14) & 3;
        if ((g_GameSceneMapNum & 0xFFF) != (nMapData & 0x3FFF)) {
            nFlag = 0;
            nX = 0;
            nY = 0;
            pCurActor->walkmeshId = 0;
        }
        if (g_pGameState->gearRide[partyIndex] != 0 || g_GamePartyMembers[partyIndex] == 7) {
            // gcc's register allocator needs this in its own block to keep nFlag in $s2
            do {
                nFlag = 0;
            } while (0);
        }
        pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
        func_80076AC0(g_FieldCurActorIndex, partyIndex, g_PartyDataBuffers[partyIndex], 1, 0, partyIndex, 1);
        g_FieldActors[g_FieldCurActorIndex].status &= 0xFFDF;
        if ((g_GameSceneMapNum & 0xFFF) != (nMapData & 0x3FFF)) {
            g_FieldScriptVMCurActor->walkmeshId = 0;
        }
        func_8009E574(nX, nY);
        FieldActorApplyPositionTransform();
        g_FieldScriptVMCurActor->scriptFlags.flags = (g_FieldScriptVMCurActor->scriptFlags.flags | 0x400) & ~0x300;
        if (nFlag == 0) {
            g_FieldActors[g_FieldCurActorIndex].status |= 0x20;
        }
    } else {
        FieldScriptVMHandlerSetActorModelFromSpriteIndex0();
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        g_FieldScriptVMCurActor->flags |= 0x800;
        return;
    }
    if (g_FieldWalkmeshTriCount - 1 < g_FieldScriptVMCurActor->walkmeshId) {
        g_FieldScriptVMCurActor->walkmeshId = 0;
    }
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x20000;
    g_FieldScriptVMCurActor->flags |= 0xC00;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void FieldMatrixCopyTransform(MATRIX* pDest, MATRIX* pSrc);
extern void FieldMatrixCopyTranslation(MATRIX* pDest, MATRIX* pSrc);

// Screened 36 variants: best 91.72% here, cdk 73.20%, 2.6.0 91.36% - 
// the &g_FieldActors materialisation ($s2 hoist) is the gap.
// 91.72% (440 of 452 bytes): every load, store, call and the copy loop match.
// g_FieldActors is declared volatile in include/field/actor.h, but this unit's
// original object reads it through a plain pointer, so the pointer load is
// shared between uses. Reading it through a local `FieldActor**` (below)
// gets that: `(*(FieldActor**)&g_FieldActors)` does NOT, gcc keeps MEM_VOLATILE
// (74.12%). Remaining 12 bytes: the original materialises `&g_FieldActors` into
// $s2 at the first matrix call (after the copy loop, `lui s2; addiu s2; lw
// a1,0(s2)`), gcc hoists the two address insns to the top of the function where
// the local pointer is assigned and leaves `lw a1,0(s2)` where the original has
// it; the saved-$s2 slot then also moves. Tried: macro/inline cast per use
// (volatile, 74.12%), plain `g_FieldActors[...]` (74.12%).
// Also tried: `pActors = g_FieldActors;` for the head plus a separate
// `pActorAddr = &g_FieldActors;` assigned AFTER the copy loop (74.12%, 404 bytes):
// gcc then rematerialises the constant address per use and drops $s2 entirely.
//
// ROUND 474: the sole residual is the TAIL's three base loads. Retail re-forms the
// address symbolically there (`lui v0,%hi(g_FieldActors)` / `lw v0,%lo(v0)`, 2 insns,
// one load shared by both indexes of that statement), i.e. its compile had no register
// copy of &g_FieldActors live at the tail; the body below emits `lw v0,0(s2)` (1 insn,
// 3 short in total). A cc1_probe of the tail statement disproved this note's old
// "gcc keeps MEM_VOLATILE" claim: `(*(FieldActor**)&g_FieldActors)` does emit the bare
// symbolic `lw $2,g_FieldActors`. Measured this round on the body below: cast at every
// site 74.12% (404 - the aliasing-visible stores make gcc interleave the eleven field
// stores with the copy loop, which is where that number is lost), cast at the tail only
// 85.52% (412 - the three tail reads cse into ONE load), pointer local for head/middle
// plus `pCopyActors = g_FieldActors;` (volatile value read, one per tail statement)
// 83.28% (436 - the three symbolic loads and the offset+base addu order come out
// right, the tail's addus/lws reorder). Unreached: a per-statement symbolic reload
// that leaves the rest of the body intact.
/*
void func_800A0524(int dstActorIndex, int srcActorIndex) {
    FieldActor** pFieldActors;
    ActorData* pDst;
    ActorData* pSrc;
    int i;

    pFieldActors = (FieldActor**)&g_FieldActors;
    pDst = (*pFieldActors)[dstActorIndex].pActorData;
    pSrc = (*pFieldActors)[srcActorIndex].pActorData;
    for (i = 0; i < 4; i++) {
        pDst->walkmeshTriIds[i] = pSrc->walkmeshTriIds[i];
    }
    pDst->walkmeshId = pSrc->walkmeshId;
    pDst->scriptYPos = pSrc->scriptYPos;
    pDst->curYPos = pSrc->curYPos;
    pDst->curTriNormal.vx = pSrc->curTriNormal.vx;
    pDst->curTriNormal.vy = pSrc->curTriNormal.vy;
    pDst->curTriNormal.vz = pSrc->curTriNormal.vz;
    pDst->position.vx = pSrc->position.vx;
    pDst->position.vy = pSrc->position.vy;
    pDst->position.vz = pSrc->position.vz;
    FieldMatrixCopyTransform(&(*pFieldActors)[dstActorIndex].transformMatrix, &(*pFieldActors)[srcActorIndex].transformMatrix);
    FieldMatrixCopyTranslation(&(*pFieldActors)[dstActorIndex].transformMatrix, &(*pFieldActors)[srcActorIndex].transformMatrix);
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.x = (*pFieldActors)[srcActorIndex].pActorData->position.vx;
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.y = (*pFieldActors)[srcActorIndex].pActorData->position.vy;
    ((SpriteData*)(*pFieldActors)[dstActorIndex].pSpriteData)->position.z = (*pFieldActors)[srcActorIndex].pActorData->position.vz;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A0524);

void FieldScriptVMHandlerSetActorModelFromPartyMember(void) {
    FieldActor* pFieldActor;
    u_short nStatus;
    FieldActor* pStatusActor;
    s32 nOffset;
    int partyId;

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    partyId = FieldCharacterIdToPartyId(FieldPartySlotToCharacter(FieldScriptVMGetArgument(1)));
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    if (partyId != -1) {
        func_80076AC0(g_FieldCurActorIndex, partyId, g_PartyDataBuffers[partyId], 2, 0, partyId, 1);
        // g_FieldActors is volatile: the offset plus a fresh base load is retail's
        // shape, and &g_FieldActors[i] hoists the base above the call (96.42%).
        nOffset = g_FieldCurActorIndex * sizeof(FieldActor);
        pStatusActor = (FieldActor*)(nOffset + (s32)g_FieldActors);
        nStatus = pStatusActor->status;
        D_800AFD20 = -0xC0;
        pStatusActor->status = nStatus & 0xFFDF;
        FieldActorApplyPositionTransform();
        g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
        g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
        g_FieldActors[g_FieldCurActorIndex].status &= 0xFFDF;
    } else {
        func_80076AC0(g_FieldCurActorIndex, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);
        g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x0 = 1;
        g_FieldScriptVMScriptEndFlag = 1;
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->flags |= 0x100000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldPartyActorSlots[];
extern s32 g_FieldPartyActorIndices[];
extern u8 D_800AE294[];
extern s16 g_FieldPlayerActorIndex16;
extern s32 g_PlayerActorIndex;
extern s32 func_8009FA54(s32 nInstructionOffset);

// 92.94% (896 vs 916 bytes - the body is 5 insns short of retail): the structure, the
// three func_80076AC0 calls, the pSpriteData swap and the frame match. What is left:
// (a) retail materialises &g_FieldSceneReloadFlags in one register (lui t2/addiu) and
// loads through it twice (`lw t1,0(t2)`), the C value-load gives `lui/lw` twice;
// (b) the arg-6 word (D_800AE294[characterId] + g_FieldSceneReloadFlags) | 0x80 is
// re-derived from two fresh loads there and CSE'd here; (c) two addu operand orders
// (`addu v1,v1,s0` vs `addu v1,s0,v1` for g_pGameState + partyId, and the swap's
// `addu v1,v1,a2`); (d) the partyId*4 scale is hoisted into retail's beq delay slot.
// Tried: nModel/nReloadFlags locals (one load each, worse), pFieldActor reuse,
// pointer locals for the two swapped FieldActors (this body), inline table reads.
// Round 474: a `volatile s32* pReloadFlags = &g_FieldSceneReloadFlags;` local assigned
// before the party-slot store and read at the three sites gives retail's reload count
// but not its form - gcc propagates the constant address into every MEM (three fresh
// `lui`/`lw` pairs, 924 bytes / 231 insns against retail's 916 / 229) instead of
// keeping `la t2` in a register as retail does, so the (a) 3-insn form is still open.
/*
void func_800A08B8(void) {
    FieldActor* pFieldActor;
    FieldSpriteDataBlob* pSpriteData;
    void* pSprite;
    void* pSwapSprite;
    void* pOldSprite;
    FieldActor* pActors;
    FieldActor* pSwapActor;
    FieldActor* pCurActor;
    int characterId;
    int partyId;

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    characterId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));
    partyId = FieldCharacterIdToPartyId(characterId);
    g_FieldScriptVMCurActor->characterId = characterId;
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    if (partyId != -1) {
        if (partyId == 0) {
            g_PlayerActorIndex = g_FieldCurActorIndex;
            g_FieldPlayerActorIndex16 = g_FieldCurActorIndex;
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x4400;
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
        }
        g_FieldPartyActorSlots[partyId] = g_FieldCurActorIndex;
        if (g_FieldSceneReloadFlags != 0) {
            pSpriteData = g_FieldSpriteData;
            pSprite = (void*)(pSpriteData->offsets[D_800AE294[characterId] + g_FieldSceneReloadFlags] + (s32)pSpriteData);
            func_80076AC0(g_FieldCurActorIndex, D_800AE294[characterId] + g_FieldSceneReloadFlags, pSprite, 0, 0,
                (D_800AE294[characterId] + g_FieldSceneReloadFlags) | 0x80, 1);
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x400;
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x300;
            if (((u8*)g_pGameState)[0x22B1 + partyId] != 0) {
                pActors = (FieldActor*)g_FieldActors;
                pCurActor = (FieldActor*)(g_FieldCurActorIndex * sizeof(FieldActor) + (s32)pActors);
                pSwapActor = &pActors[g_FieldPartyActorIndices[partyId]];
                pSwapSprite = pSwapActor->pSpriteData;
                pOldSprite = pCurActor->pSpriteData;
                pCurActor->pSpriteData = pSwapSprite;
                ((FieldActor*)(g_FieldPartyActorIndices[partyId] * sizeof(FieldActor) + (s32)g_FieldActors))->pSpriteData = pOldSprite;
                g_FieldScriptVMCurActor->scriptFlags.flags |= 0x200;
                g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x500;
            }
        } else {
            func_80076AC0(g_FieldCurActorIndex, partyId, g_PartyDataBuffers[partyId], 1, 0, partyId, 1);
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x400;
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x300;
        }
        g_FieldActors[g_FieldCurActorIndex].status &= 0xFFDF;
        D_800AFD20 = -0xC0;
        func_8009FA54(FieldScriptVMGetVariableValue(2));
        FieldActorApplyPositionTransform();
        g_FieldScriptVMCurActor->flags &= ~0x800;
    } else {
        func_80076AC0(g_FieldCurActorIndex, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);
        g_FieldScriptVMCurActor->scriptFlags.flags |= 0x1;
        g_FieldScriptVMScriptEndFlag = 1;
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->flags |= 0x100000;
    }
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x20000;
    g_FieldScriptVMCurActor->flags |= 0x400;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A08B8);


extern s32 g_PlayerActorIndex;

void FieldPlayerSetScriptFlag80(void) {
    g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags |= 0x80;
}

// g_FieldActors is volatile, so retail re-reads the base once per component pair:
// the three pointer locals keep those reloads where the original has them.
// pPosition views the 16.16 fixed-point position as its halves: 1, 3 and 5 are
// the integer vx, vy and vz.
void FieldActorApplyPositionTransform(void) {
    FieldActor* pFieldActor;
    FieldActor* pFieldActor1;
    FieldActor* pFieldActor2;
    SpriteData* pSpriteData;
    ActorData* pActor;
    s16* pPosition;
    s32 x;
    s32 y;
    s32 z;
    s32 offset;
    u16 curY;

    pActor = g_FieldScriptVMCurActor;
    offset = g_FieldCurActorIndex * sizeof(FieldActor);
    pFieldActor = (FieldActor*)(offset + (s32)g_FieldActors);
    pPosition = (s16*)&pActor->position;

    x = pPosition[1];
    pSpriteData = pFieldActor->pSpriteData;
    pFieldActor->transformMatrix.t[0] = x;
    pFieldActor1 = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor->childMatrix.t[0] = x;

    y = pPosition[3];
    pFieldActor2 = (FieldActor*)(offset + (s32)g_FieldActors);
    pFieldActor1->transformMatrix.t[1] = y;
    pFieldActor1->childMatrix.t[1] = y;

    z = pPosition[5];
    pFieldActor2->transformMatrix.t[2] = z;
    pFieldActor2->childMatrix.t[2] = z;

    pSpriteData->position.x = pActor->position.vx;
    pSpriteData->position.y = pActor->position.vy;
    pSpriteData->position.z = pActor->position.vz;
    pSpriteData->step.y = 0;

    curY = ((u16*)pPosition)[3];
    pSpriteData->field_0x84 = curY;
    pActor->curYPos = curY;
}

void FieldScriptVMHandlerSetActorModelFromSpriteIndex0(void) {
    FieldSpriteDataBlob* pSpriteData;

    pSpriteData = g_FieldSpriteData;
    func_80076AC0(g_FieldCurActorIndex, 0, (void*)(pSpriteData->offsets[0] + (s32)pSpriteData), 0, 0, 0x80, 1);
    FieldActorApplyPositionTransform();
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x8 = 0x1;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// The wrapper struct is load-bearing: with a plain `extern s16` cc1 puts the actor
// base in $a0 with an extra nop and coalesces the `addiu 3` into the sum (75.67%).
typedef struct { s16 value; } FieldScriptGlobalS16;
extern FieldScriptGlobalS16 g_FieldGearObjectCapacity;

void FieldScriptVMHandlerSetGlobalS16(void) {
    g_FieldGearObjectCapacity.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerStoreDiscNumber(void) {
    int nAddress;

    nAddress = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    FieldScriptMemoryWriteU16(nAddress, ArchiveGetDiscNumber());
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern int g_FieldMode;

void FieldScriptVMHandlerWaitWhileFieldModeSet(void) {
    if (g_FieldMode == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern int g_FieldScriptCounter;

void FieldScriptVMHandlerIncrementCounter(void) {
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptCounter++;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// The model record g_FieldActorModelPointers holds and the sprite record it
// points at from +0x4. This unit only touches the scale, the y and the two x/z
// words.
typedef struct {
    /* 0x00 */ u8 unk0[0x5C];
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u8 unk60[4];
    /* 0x64 */ s32 unk64;
} FieldActorModelSprite;

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ FieldActorModelSprite* unk4;
    /* 0x08 */ u8 unk8[0x14];
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ u8 unk1E[0x16];
    /* 0x34 */ u8 unk34;
    /* 0x35 */ u8 unk35[0x2B];
    /* 0x60 */ s16 unk60;
} FieldActorModel;

extern FieldActorModel* g_FieldActorModelPointers[];
extern void LibGearFreeModel(int modelIndex);

void FieldScriptVMHandlerReleaseActorModel(void) {
    ActorData* pActor;
    s32 nModelIndex;

    pActor = g_FieldScriptVMCurActor;
    nModelIndex = pActor->flags12C_0xE;
    pActor->flags &= ~0x2000;
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldActorModelPointers[nModelIndex]->unk34 = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 1:
            LibGearFreeModel(pActor->flags12C_0xE);
            g_FieldActorModelSlotCount[0]--;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
    }
    g_FieldScriptVMStopFlag = 1;
}


extern s32 g_FieldArchiveStreamActive;
extern s32 FieldArchiveTryDataSync(void);
extern s32 ArchiveCdDataSync(s32 arg0);
extern s32 ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, s32 arg1, s32 arg2);
extern void func_801E742C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s16 arg4, s32 arg5,
                          s32 arg6, s32 arg7, SVECTOR* pVector);
extern StreamDataQueueEntry D_800B2394[];
extern s32 g_FieldActorModelSlotDataBuffers[];
extern s32 g_FieldActorModelSlotSpriteBuffers[];

// Per-slot scratch the loader fills: an SVECTOR per slot at +0x0, which the
// sprite update reads back, and the animation id taken from the model at +0x20.
typedef struct {
    /* 0x00 */ SVECTOR vectors[4];
    /* 0x20 */ s32 animationIds[8];
} FieldModelSlotScratch;

extern FieldModelSlotScratch g_FieldModelSlotScratch;

void FieldScriptVMHandlerUpdateActorModelSlot(void) {
    ActorData* pActor;
    s32 nSlot;
    s32 nArg;
    s32 nOffset;
    void* pBuffer;
    s16* pPosition;
    s16 nScale;
    s32 nPositionY;
    FieldActorModel* pModel;
    ActorData* pSpriteActor;

    if (g_FieldArchiveStreamActive || FieldArchiveTryDataSync() != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        g_FieldScriptVMStopFlag = 1;
        return;
    }

    pActor = g_FieldScriptVMCurActor;
    nSlot = pActor->flags12C_0xE;
    pActor->flags &= ~0x2000;
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);

    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldActorModelPointers[nSlot]->unk34 = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;

        case 1:
            LibGearFreeModel(g_FieldScriptVMCurActor->flags12C_0xE);
            nArg = FieldScriptVMGetArgument(5);
            g_FieldActorModelSlotArchiveBase[nSlot] = nArg * 2;
            D_800B2394[0].archiveIndex = nArg * 2 + FIELD_ARCHIVE_MODEL_ENTRY;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_FieldActorModelSlotArchiveBase[nSlot] + FIELD_ARCHIVE_MODEL_ENTRY), 0);
            g_FieldActorModelSlotDataBuffers[nSlot] = (s32)pBuffer;
            D_800B2394[0].pData = pBuffer;
            D_800B2394[1].archiveIndex = g_FieldActorModelSlotArchiveBase[nSlot] + FIELD_ARCHIVE_MODEL_ENTRY + 1;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_FieldActorModelSlotArchiveBase[nSlot] + FIELD_ARCHIVE_MODEL_ENTRY + 1), 1);
            g_FieldActorModelSlotSpriteBuffers[nSlot] = (s32)pBuffer;
            D_800B2394[1].pData = pBuffer;
            D_800B2394[2].archiveIndex = 0;
            D_800B2394[2].pData = NULL;
            ArchiveStartStreamQueue(D_800B2394, 0, 0);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;

        case 2:
            if (ArchiveCdDataSync(1) == 0) {
                FieldScriptVMGetArgument(2); // the original calls it and drops the result
                nOffset = nSlot + g_FieldActorModelSpriteRow[nSlot];
                func_801E742C(nSlot, 0, g_FieldActorModelSlotDataBuffers[nSlot], g_FieldActorModelSlotSpriteBuffers[nSlot],
                    (s16)(0x240 - (nOffset << 6)), 0x100, 0,
                    (s16)(nSlot + 0xFC), &g_FieldModelSlotScratch.vectors[nSlot]);
                g_FieldModelSlotScratch.animationIds[nSlot] = g_FieldActorModelPointers[nSlot]->unk1C;
                HeapFree((void*)g_FieldActorModelSlotSpriteBuffers[nSlot]);

                pSpriteActor = g_FieldScriptVMCurActor;
                pSpriteActor->scriptInstructionPointer += 4;
                pSpriteActor->flags |= 0x2000;
                nScale = (s16)pSpriteActor->scaleX;
                pPosition = (s16*)&pSpriteActor->position;
                g_FieldActorModelPointers[nSlot]->unk1C = (nScale * 5) >> 6;
                // The y is a fixed-point half-word: an int local keeps the sign-extending load.
                pModel = g_FieldActorModelPointers[nSlot];
                nPositionY = pPosition[3];
                pModel->unk60 = nPositionY;
                g_FieldActorModelPointers[nSlot]->unk4->unk5C = pPosition[1];
                g_FieldActorModelPointers[nSlot]->unk4->unk64 = pPosition[5];
            } else {
                g_FieldScriptVMCurActor->scriptInstructionPointer--;
            }
            break;
    }
    g_FieldScriptVMStopFlag = 1;
}

// RETAIL IS 396 bytes / 99 insns (0x31874-0x31A00), not 368: the old '84.19%/368' note compared against a wrong size.
// BEST 92.58% at 396 bytes / 99 insns, exactly retail's size, with the body below
// (each count access in its own do{...}while(0) so cc1 re-forms the symbol address).
// Residual, all one cause: retail forms the g_FieldActorModelSlotCount address four
// separate times (four 'lui %hi'+'lw %lo' pairs, and the increment store as
// 'lui at'+'sw %lo(at)'), where cc1 caches &count in a register for the third and
// fourth uses ('lui/addiu' then 'lw 0(a0)' twice, 'sw 0(a0)'). Retail's stock-based
// access is 2 insns per use; the cached base is 2 insns once plus 1 per use, so
// every spelling that breaks the cache also changes the byte count. Also, retail's
// mask 'lui a1,0xffff' fills the load-delay nop after 'lhu v1,88(v0)' and its
// 'ori a1,a1,0x1fff' lands after the sb; both follow from the same scheduling.
// Measured this round (all landed 396 or worse and scored below 92.58):
// - plain 4 statements, extern s32 g_FieldActorModelSlotCount[]: 92.02%, 388 bytes
//   (97 insns) - 2 insns short because of the cached base.
// - same, do-while(0) around each of the 4 statements: 92.58%, 396 (this park).
// - do-while(0) around the two store groups instead of each statement: same 92.58%.
// - extern volatile s32 g_FieldActorModelSlotCount[] (array): 90.84%, 392.
// - extern s32 g_FieldActorModelSlotCount; (scalar, gp-relative loads): 84.19%, 368.
// - extern volatile s32 g_FieldActorModelSlotCount; (scalar): 74.43%, 396 - the
//   right size and insn count, but gcc sinks the 0x12C RMW below the increment.
// - try_flags: cdk 88.88, 2.6.0 91.46, -G0 92.02, cdk -G0 73.38 - no preset wins.
/*
void func_800A1364(void) {
    int arg;
    FieldActor* pFieldActor;
    u8 unused[8]; // original has an unused stack slot; reproduces the frame

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    func_80076AC0(g_FieldCurActorIndex, 0, (void*)((*(s32*)(g_FieldSpriteData + 4)) + (s32)g_FieldSpriteData), 0, 0, 0x80, 1);
    arg *= 2;
    FieldActorApplyPositionTransform();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldActors[g_FieldCurActorIndex].status &= ~ACTOR_STATUS_INVISIBLE;
    g_FieldScriptVMCurActor->flags |= 0x2000;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    do {
        g_FieldActorModelSlotArchiveBase[g_FieldActorModelSlotCount[0]] = arg;
    } while (0);
    do {
        g_FieldActorModelSpriteRow[g_FieldActorModelSlotCount[0]] = 0;
    } while (0);
    do {
        g_FieldScriptVMCurActor->flags12C_0xE = g_FieldActorModelSlotCount[0] & 7;
    } while (0);
    do {
        g_FieldActorModelSlotCount[0]++;
    } while (0);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc6", func_800A1364);

void FieldScriptVMHandlerSetActorModelFromSpriteIndexWithFlags(void) {
    int arg;
    int arg3;
    FieldActor* pFieldActor;
    FieldSpriteDataBlob* pSpriteData;
    void* pSprite;

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    pSpriteData = g_FieldSpriteData;
    pSprite = (void*)(pSpriteData->offsets[arg] + (s32)pSpriteData);
    arg3 = FieldScriptVMGetArgument(3);
    func_80076AC0(g_FieldCurActorIndex, arg, pSprite, 0, arg3, arg | 0x80, 1);
    FieldActorApplyPositionTransform();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[g_FieldCurActorIndex].status &= ~ACTOR_STATUS_INVISIBLE;
}

void FieldScriptVMHandlerSetActorModelFromSpriteIndex(void) {
    int arg;
    FieldActor* pFieldActor;
    FieldSpriteDataBlob* pSpriteData;

    pFieldActor = &g_FieldActors[g_FieldCurActorIndex];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    arg = FieldScriptVMGetArgument(1);
    pSpriteData = g_FieldSpriteData;
    func_80076AC0(g_FieldCurActorIndex, arg, (void*)(pSpriteData->offsets[arg] + (s32)pSpriteData), 0, 0, arg | 0x80, 0);
    FieldActorApplyPositionTransform();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x100;
    g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x80;
    g_FieldScriptVMCurActor->flags &= ~0x800;
    g_FieldActors[g_FieldCurActorIndex].status &= ~ACTOR_STATUS_INVISIBLE;
}
