#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// The pixel depth of the TIM a battle quad samples: GetTPage's tp argument, which
// also says how many texels one halfword of the image holds.
#define BATTLE_TEX_DEPTH_4BIT 0
#define BATTLE_TEX_DEPTH_8BIT 1
#define BATTLE_TEX_DEPTH_16BIT 2

// g_BattleControlFlags bit 0: set while the player, not the COM, drives both sides.
#define BATTLE_CONTROL_FLAG_USER 0x1

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern s16 g_BattleEffectFrameTimer;
extern s32 g_BattleScreenMode;
extern s32 g_BattleMode;
extern u8 g_BattleTransitionFlag;
extern u8* g_BattleSlotEntryBuffers[];
extern s32 g_BattleSedsEntryId;
extern s32 g_BattleAudioManagerEnabled;
extern s32 g_BattlingAudioManager;
extern u8 g_BattleSlot1EntryId;
extern u8 g_BattleSlot0EntryId;
extern u8 g_BattleRotatedPartModel[];
extern u8 g_BattleControlFlags;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_SpuChannelMask;
extern s32 g_BattleMotionSpeedScale;
extern u8* g_BattleRootOrderingTable;
extern u8* g_BattleEffectImageActive;


extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(s32 mode);
extern void func_8008E120(void);
extern void func_8007F258(void* pOt, s32 arg);
extern void BattleDrawTextPrims(void* pOt);
extern void func_8008E3CC(void* pOt, s32 arg1, s32 arg2);
extern void func_8008BC04(void);
extern void SoundSetAudioManagerMasterVolumeWithFade(s32 arg0, s32 arg1, s32 arg2);
extern void BattleEffectFreeResources(void);
extern void BattleClearScreen(s32 arg);
extern void BattleSetScreenMode(s32 arg);
extern void SoundRemoveSedsEntry(s32 arg);
extern void SoundStopAudioManager(s32 arg);
extern void SoundDestroyAudioManager(s32 arg);
extern void func_80088A40(void);
extern void ChangeGameState(s32 state);
extern void Vsync(s32 mode);
extern void MainLoop(s32 arg);
extern s32 BattleCreateObject(void);
extern s32 BattleCreateModelPart(void);
extern void BattleSetNodeModelPart(void* pA, void* pB);
extern void BattleAttachModelPart(void* pA, void* pB);
extern void BattleAppendListEntry(s32 handle, void* pB);
extern void* BattleLoadArchiveEntry(s32 arg);
extern void BattleAttachActorPrims(void* pData);
extern s32 BattleCheckActorBelowBreakThreshold(s32 arg0, s32 arg1);
void BattleAddSharedPrim(void);

// The header at the start of the block pData->pHeader points at, once
// BattleRelocateBlockPointers has fixed up its +0x30 offset. It is the block the
// actor's script pointer (+0x8FC) addresses, so the fields the units that read
// the script name carry their names: src/battling/main/misc4.c's ActorScript (the
// part indices, the two scales, the facing) and misc1.c's BattleScriptBlock (the
// render pair).
typedef struct {
    /* 0x00 */ u8 basePartIndex;
    /* 0x01 */ u8 partIndex0;
    /* 0x02 */ u8 partIndex1;
    /* 0x03 */ u8 radius;
    /* 0x04 */ s16 vertexIndex0;
    /* 0x06 */ s16 vertexIndex1;
    /* 0x08 */ s16 baseVertexIndex;
    /* 0x0A */ u8 pad0A[0x2];
    /* 0x0C */ u16 speedScale; // 8.8 speed, copied into the actor's moveSpeed/moveSpeedBase
    /* 0x0E */ u8 visiblePartCount;
    /* 0x0F */ u8 stepScale; // copied into the actor's stepScale by misc4's func_80078F00
    /* 0x10 */ u8 effectColorR;
    /* 0x11 */ u8 effectColorG;
    /* 0x12 */ u8 effectColorB;
    /* 0x13 */ u8 pad13;
    /* 0x14 */ s16 angleX;
    /* 0x16 */ s16 angleYStep;
    /* 0x18 */ s16 offsetY;
    /* 0x1A */ s16 offsetZ;
    /* 0x1C */ s16 modelColor;
    /* 0x1E */ u16 angleY;
    /* 0x20 */ u16 facing;
    /* 0x22 */ u8 pad22[0x2];
    // The actor's camera rig (misc4's func_8007A768 copies these into the
    // g_BattleRender* globals func_8007A958 feeds to rcos/rsin).
    /* 0x24 */ s16 cameraAngleOffset;
    /* 0x26 */ s16 cameraYOffset;
    /* 0x28 */ s16 cameraTargetYOffset;
    /* 0x2A */ s16 cameraRadius;
    /* 0x2C */ u8 pad2C[0x4];
    /* 0x30 */ s32 visiblePartIndicesOffset; // byte offset of the sprite table from the block start
} Misc10SpriteHeader; // Size: 0x34
STATIC_ASSERT_SIZEOF(Misc10SpriteHeader, 0x34);

// One 4-byte action slot of the block's +0x18 array; only the flag byte is read.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 flags;
} Misc10ActionSlot; // Size: 0x4

// One 0x20-byte entry of the D_80092874 table (the same table src/battling/
// main/misc6.c calls BattleFilterEntry); +0xA holds one flag byte per action
// slot and this unit counts 14 of them.
typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ s16 requiredProgress; // the word src/battling/main/misc6.c reads as the entry's progress gate
    /* 0x06 */ u8 unk06[0x4];
    /* 0x0A */ u8 slotFlags[0xE];
    /* 0x18 */ u8 unk18[0x8];
} Misc10ActionRow; // Size: 0x20

// The relocated resource block pData points at: the layout
// BattleRelocateBlockPointers in src/battling/main/misc14.c fixes up, so each
// pointer below is a byte offset from the block start before that call. The
// three pointers at 0x00-0x08 are what that file's func_8008B38C walks to
// build the actor's nodes: the part descriptors, the model data they index and
// the per-part animation list.
typedef struct {
    /* 0x00 */ u8* pPartList; // count word followed by 0x10-byte part descriptors
    /* 0x04 */ u8* pModelData; // base of the 0x38-byte model part records
    /* 0x08 */ u8* pAnimList; // count word followed by 0x14-byte entries
    /* 0x0C */ u8* pTimImage; // TIM uploaded with the battle-wide tpage/clut (misc14's BattleRelocateBlockPointers)
    /* 0x10 */ Misc10SpriteHeader* pHeader;
    /* 0x14 */ u8* pPartTimings;
    /* 0x18 */ Misc10ActionSlot* pActions;
    /* 0x1C */ u32 pSelf; // the block's own address, the base every pointer above is rebased against
    /* 0x20 */ u8* pImage;
} Misc10Block; // Size: 0x24

// A 32-bit component vector; the actor's position blocks are this wide.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10
STATIC_ASSERT_SIZEOF(Vec4, 0x10);

// One of the actor's eight 0x44-byte effect slots; the layout and the names are
// src/battling/main/misc2.c's, the unit that drives its fields.
typedef struct {
    /* 0x00 */ VECTOR pos;
    /* 0x10 */ VECTOR prevPos;
    /* 0x20 */ SVECTOR velocity;
    /* 0x28 */ SVECTOR direction;
    /* 0x30 */ u8 active;
    /* 0x31 */ u8 scale;
    /* 0x32 */ u8 drawKind;
    /* 0x33 */ u8 mode;
    /* 0x34 */ u8 pad34[0x4];
    /* 0x38 */ s32 key;
    /* 0x3C */ s16 percent;
    /* 0x3E */ s16 dirWeight;
    /* 0x40 */ s16 life;
    /* 0x42 */ u8 pad42[0x2];
} BattleEffectSlot; // Size: 0x44
STATIC_ASSERT_SIZEOF(BattleEffectSlot, 0x44);

// A battle participant as this unit drives it. The offsets are the full actor
// src/battling/main/misc4.c's Actor, misc2.c's Actor and misc3.c's BattleActor
// declare, and the fields below carry their names.
typedef struct {
    /* 0x000 */ Vec4 position;
    /* 0x010 */ s32 vx;
    /* 0x014 */ s32 vy;
    /* 0x018 */ s32 vz;
    /* 0x01C */ u8 pad1C[0x4];
    /* 0x020 */ s32 pushVx;
    /* 0x024 */ s32 pushVy;
    /* 0x028 */ s32 pushVz;
    /* 0x02C */ u8 pad2C[0x4];
    /* 0x030 */ s32 steerVx;
    /* 0x034 */ s32 steerVy;
    /* 0x038 */ s32 steerVz;
    /* 0x03C */ u8 pad3C[0x4];
    /* 0x040 */ s32 stepDistance;
    /* 0x044 */ s32 steerSpeed;
    /* 0x048 */ s32 distance;
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 pad4D;
    /* 0x04E */ u8 prevPartIndex;
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ s16 lastFrame;
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 pad53;
    /* 0x054 */ s32 angle; // the actor's facing, added to the script's camera angle before rcos/rsin (misc4)
    /* 0x058 */ u8* pTimData;
    /* 0x05C */ u8* pModelLink;
    /* 0x060 */ u8* pObject;
    /* 0x064 */ u8 pad64[0x8];
    /* 0x06C */ Vec4 prevPosition; // the position x,y,z,pad copied here before the frame's movement
    /* 0x07C */ u8* pPartTimings;
    /* 0x080 */ Misc10ActionSlot* pActions;
    /* 0x084 */ u8* pActionEntry; // the entry the move in progress selected (src/battling/main/misc4.c)
    // Ramped 0..0x100 deflection of one input direction each: misc3's func_80076884
    // charges the direction the input selects and decays the other three.
    /* 0x088 */ s32 inputLeft;
    /* 0x08C */ s32 inputUp;
    /* 0x090 */ s32 inputRight;
    /* 0x094 */ s32 inputDown;
    /* 0x098 */ s32 inputRampStep;
    /* 0x09C */ s32 inputDecayStep;
    /* 0x0A0 */ u8 padA0[0x8];
    /* 0x0A8 */ s32 unkA8;
    /* 0x0AC */ u8 padAC[0x4];
    /* 0x0B0 */ s32 floorY;
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ s16 prevGaugeValue;
    /* 0x0BA */ s16 gaugeLevel;
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 breakThresholdOffset;
    /* 0x0C0 */ u8 gaugeWidth;
    /* 0x0C1 */ u8 prevGaugeWidth;
    /* 0x0C2 */ u8 flashTimer;
    /* 0x0C3 */ u8 actionPhaseTimer;
    /* 0x0C4 */ u8 state;
    /* 0x0C5 */ u8 actionPhase;
    /* 0x0C6 */ u8 prevState;
    /* 0x0C7 */ u8 padC7;
    /* 0x0C8 */ s16 moveEndTimer;
    /* 0x0CA */ s16 moveTimer;
    /* 0x0CC */ u16 sideBaseAngle;
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u32 stateFlags; // masked and tested by the movement code (src/battling/main/misc4.c)
    /* 0x0D8 */ u8* pTarget;
    /* 0x0DC */ u8 unk0DC[0x1A8];
    /* 0x284 */ u32* pDisplayWork;
    /* 0x288 */ u8 pad288[0x3BC];
    /* 0x644 */ s32 gaugePercent;
    /* 0x648 */ s32 facingAngle;
    /* 0x64C */ BattleEffectSlot effectSlots[8];
    /* 0x86C */ u8 pad86C[0x90];
    /* 0x8FC */ u8* pScript;
    /* 0x900 */ u8* pModelScriptOffsets; // one s16 script offset per model index
    /* 0x904 */ u8* pVisiblePartIndices;
    /* 0x908 */ u8 visiblePartCount;
    /* 0x909 */ u8 typeId;
    /* 0x90A */ u8 typeFlags;
    /* 0x90B */ u8 timer; // counted down by the ground-effect pass (src/battling/main/misc4.c)
    /* 0x90C */ s16 actionAngle; // latched when actionPhase 2 starts, copied into `angle` each frame (misc3's func_80078194)
    /* 0x90E */ u8 savedModelIndex;
    /* 0x90F */ u8 unk90F;
    /* 0x910 */ u8 soundTimer; // frames left before the gauge sound repeats (src/battling/main/misc3.c)
    /* 0x911 */ u8 flaggedActionCount;
    /* 0x912 */ u8 unflaggedActionCount;
    // 0x914..0x15D0 hold the action timers, the four bound/mid points, the part
    // animation state, the message queue and the 0xC00 state record misc3's
    // BattleActor names; this unit's own code stops at 0x912.
    /* 0x913 */ u8 unk913[0xCC1];
    /* 0x15D4 */ u8 effectColorR;
    /* 0x15D5 */ u8 effectColorG;
    /* 0x15D6 */ u8 effectColorB;
    /* 0x15D7 */ u8 pad15D7;
    /* 0x15D8 */ u8 pad15D8[0x10];
    /* 0x15E8 */ s16 unk15E8;
    /* 0x15EA */ s16 unk15EA;
    /* 0x15EC */ s16 unk15EC; // read as two 16-bit words by misc2's BattleComputeScaledFieldDelta
    /* 0x15EE */ s16 unk15EE;
    /* 0x15F0 */ s16 stepScale; // from the script header's stepScale; multiplies the distance into stepDistance
    /* 0x15F2 */ s16 moveSpeed;
    /* 0x15F4 */ s16 moveSpeedBase;
    /* 0x15F6 */ s16 unk15F6; // the round reset sets 0x100 here and 0 at the next word (misc4)
    /* 0x15F8 */ s16 unk15F8;
    /* 0x15FA */ u8 pad15FA[0x2];
    /* 0x15FC */ void* pWork; // the per-slot work record the state handlers run on (misc17/misc18's pWork)
    /* 0x1600 */ Misc10ActionRow* pActionRow;
} Misc10Actor; // Size: 0x1604
STATIC_ASSERT_SIZEOF(Misc10Actor, 0x1604);

typedef struct {
    /* 0x00 */ u8 unk00[0x44];
    /* 0x44 */ s16 rotX;
    /* 0x46 */ s16 rotY;
    /* 0x48 */ s16 rotZ;
} Misc10Draw; // Size: unknown

typedef struct {
    /* 0x0 */ u16 entryIndex;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 buffer;
} Misc10Resource; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x310];
} Misc10Prim; // Size: 0x310

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} Misc10Point; // Size: 0x4

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 w;
    /* 0x6 */ u16 h;
} Misc10TexRect; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x04];
    /* 0x04 */ Misc10Point* crect;
    /* 0x08 */ u8 unk08[0x04];
    /* 0x0C */ Misc10TexRect* prect;
} Misc10TagSrc; // Size: 0x10

extern Misc10ActionRow* D_80092874;
extern u8 g_BattleActor1[];
extern u8 g_BattleActor0[];
extern Misc10Prim g_BattleSharedPrimBuffer[];

extern void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY);
extern void BattleClearTpageAndClut(void);
extern void* BattleRelocateBlockPointers(void* pData);
extern u8* func_8008B38C(u8* pArg);
extern u8* BattleSetActorSoundTable(Misc10Actor* pActor);

s32 BattleQueueActorPrims(Misc10Actor* pActor) {
    u32* pData = pActor->pDisplayWork;

    func_8008E3CC((u8*)pData[g_BattleDrawBufferIndex + 1] + 8, 0xC0, 0);
    BattleDrawTextPrims((u8*)pData[g_BattleDrawBufferIndex + 1] + 4);
    BattleAttachActorPrims(pData);
    BattleAddSharedPrim();
    return 0;
}

void BattleDrawEffectFrame(void) {
    if (g_BattleEffectFrameTimer != 0) {
        func_8008E120();
        func_8007F258(g_BattleRootOrderingTable, 0);
        BattleDrawTextPrims(g_BattleRootOrderingTable);
        func_8008E3CC(g_BattleRootOrderingTable, 0xC0, 1);
        func_8008BC04();
    }
}

void BattleUpdateEffectTimer(void) {
    s16 timer;

    timer = g_BattleEffectFrameTimer;
    if (timer != 0) {
        timer = timer - 4;
        g_BattleEffectFrameTimer = timer;
        if (timer <= 0) {
            g_BattleEffectFrameTimer = 0;
            g_BattleEffectImageActive = 0;
            SoundSetAudioManagerMasterVolumeWithFade(g_BattlingAudioManager, 0x100, 0);
            g_BattleMotionSpeedScale = 0x100;
            BattleEffectFreeResources();
        } else {
            func_8008E3CC(g_BattleRootOrderingTable, timer, 1);
        }
    } else {
        g_BattleEffectImageActive = 0;
    }
}

void BattleActorCreateRotatedPart(Misc10Actor* pActor) {
    u8* pScript;
    s32 arg0;
    Misc10Draw* pDraw;
    u8* pBuf;

    pScript = *(u8**)((u8*)pActor + 0x5C);
    pScript = *(u8**)(pScript + 4);
    pScript = *(u8**)(pScript + 4);
    arg0 = *(s32*)(pScript + 0x30);
    pDraw = (Misc10Draw*)BattleCreateObject();
    pBuf = (u8*)BattleCreateModelPart();
    BattleSetNodeModelPart(pDraw, pBuf);
    BattleAttachModelPart(pBuf, g_BattleRotatedPartModel);
    BattleAppendListEntry(arg0, pDraw);
    pDraw->rotY = 0xC00;
    pDraw->rotX = 0;
    pDraw->rotZ = 0x400;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

/* 89.37% objdiff, 832 bytes against retail 840. Structure, call order, the
   prologue save order and every s-register except one pair match, and the
   jump table comes out at the unit rodata start where retail has it. Because
   the stub owns a jtbl_ table, this function can never print MATCH in
   diff_func.sh (the original dump stops at the first case label, which gas
   makes a symbol when .rodata references it) - objdiff% and check_bin.sh are
   the only judges.
   What is still wrong:
   1. Best body below keeps pActor->pModelLink = pNode AFTER the switch, which is
      what makes gcc hand pActor s0 / pNode s1 (global-alloc priority order
      pActor, pData, arg2, pNode, with s1 already reserved by the flip-loop
      buffer through local-alloc). Moving that store to its retail position
      (middle block, before the pActionRow store) makes pNode block-local, so
      local-alloc takes s0 for it and pActor falls to s1 - the whole function
      then reads 86% with s0/s1 swapped. That trade-off is the whole gap.
   2. pActor->typeFlags |= 1; pActor->typeFlags |= 2; (retail case 35/36) is cse-d
      into one |= 3 by gcc; retail has two lbu/ori/sb pairs. Untried:
      bitfields, or a union member for that byte.
   3. gcc cross-jumps the three case-body sb v0,0x90A into one shared tail;
      retail has one per case (2 instructions, the size difference).
   Key trick that fixed the prologue: assign pSrc = pData->pImage + 0x200
   BEFORE HeapAlloc in the arg2 == 0 branch - retail keeps it in s0 across
   the HeapAlloc call (its addiu lands in the call delay slot), which is what
   pushes the heap buffer to s1 and pData/arg2 to s2/s3.
   void func_80084C88(Misc10Actor* pActor, Misc10Block* pData, s32 arg2) {
       RECT rect;
       Misc10SpriteHeader* pHeader;
       Misc10ActionRow* pRow;
       u8* pNode;
       u8* pObj;
       u8* pBuf;
       u8* pSrc;
       u8* pDst;
       u8* pRead;
       s32 row;
       s32 col;
       s32 i;

       pActor->flags = (pActor->flags & 0xF7FFFFFF) | ((arg2 & 1) << 27);
       if (arg2 != 0) {
           BattleSetTpageAndClut(0x380, 0, 0, 0x1FE);
           pActor->pTarget = g_BattleActor1;
       } else {
           BattleSetTpageAndClut(0x3C0, 0, 0, 0x1FF);
           pActor->pTarget = g_BattleActor0;
       }
       BattleRelocateBlockPointers(pData);
       pNode = func_8008B38C((u8*)pData);
       pObj = (u8*)BattleCreateObject();
       pActor->pObject = pObj;
       BattleAppendListEntry(pObj, pNode);
       pActor->pActionRow = &D_80092874[pActor->typeId];
       pActor->typeFlags = 0;
       switch (pActor->typeId) {
       case 3:
       case 14:
       case 27:
       case 34:
       case 39:
       case 41:
       case 42:
           pActor->typeFlags |= 4;
           break;
       case 13:
           BattleClearTpageAndClut();
           BattleActorCreateRotatedPart(pActor);
           break;
       case 35:
       case 36:
           pActor->typeFlags |= 1;
           pActor->typeFlags |= 2;
           break;
       case 38:
           pActor->typeFlags |= 2;
           break;
       }
       pActor->pModelLink = pNode;
       BattleSetActorSoundTable(pActor);
       pHeader = pData->pHeader;
       pActor->pScript = (u8*)pHeader;
       pActor->pPartTimings = pData->pPartTimings;
       pActor->pModelScriptOffsets = (u8*)pHeader + 0x34;
       pActor->pActions = pData->pActions;
       pActor->pVisiblePartIndices = (u8*)pHeader + pHeader->visiblePartIndicesOffset;
       pActor->visiblePartCount = pHeader->visiblePartCount;
       pActor->effectColorR = pHeader->effectColorR;
       pActor->effectColorG = pHeader->effectColorG;
       pActor->effectColorB = pHeader->effectColorB;
       pActor->flaggedActionCount = 0;
       pActor->unflaggedActionCount = 0;
       pRow = pActor->pActionRow;
       for (i = 0; i < 0xE; i++) {
           if (pRow->slotFlags[i] != 0) {
               if (pActor->pActions[i].flags != 0) {
                   pActor->flaggedActionCount++;
               } else {
                   pActor->unflaggedActionCount++;
               }
           }
       }
       rect.x = 0;
       rect.y = arg2 + 0x1F6;
       rect.w = 0x100;
       rect.h = 1;
       LoadImage(&rect, (u_long*)pData->pImage);
       rect.x = arg2 * 0x10 + 0x380;
       rect.y = 0x100;
       rect.w = 0xB;
       rect.h = 0x16;
       if (arg2 != 0) {
           LoadImage(&rect, (u_long*)(pData->pImage + 0x200));
       } else {
           pSrc = pData->pImage + 0x200;
           pBuf = (u8*)HeapAlloc(0x1E4, 0);
           pDst = pBuf;
           for (row = 0; row < 0x16; row++) {
               pRead = pSrc + 0x15;
               for (col = 0; col < 0x16; col++) {
                   *pDst++ = *pRead--;
               }
               pSrc += 0x16;
           }
           LoadImage(&rect, (u_long*)pBuf);
           HeapDelayedFree(pBuf, 1);
       }
       rect.x = 0x3A0;
       rect.y = arg2 * 8 + 0x100;
       rect.w = 0x10;
       rect.h = 8;
       LoadImage(&rect, (u_long*)(pData->pImage + 0x3E4));
   }
*/

void BattleUpdateEffectImage(void) {
    if (g_BattleEffectImageActive != NULL && (g_SpuChannelMask & 0x1)) {
        func_8008E120();
    }
}

void BattleSetBothSidesCom(void) {
    g_BattleControlFlags &= ~BATTLE_CONTROL_FLAG_USER;
    BattleClearScreen(0);
    BattleSetScreenMode(5);
    g_BattlePlayer2ComControlled = 1;
    g_BattlePlayer1ComControlled = 1;
    g_BattleMode = 6;
}

void BattleSetBothSidesUser(void) {
    u8 flags = g_BattleControlFlags;

    g_BattlePlayer2ComControlled = 0;
    g_BattlePlayer1ComControlled = 0;
    g_BattleControlFlags = flags | BATTLE_CONTROL_FLAG_USER;
}

void BattleLoadSlotEntry(s32 index, s32 value) {
    if (index != 0) {
        g_BattleSlot1EntryId = value;
    } else {
        g_BattleSlot0EntryId = value;
    }
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
    ArchiveSetIndex(0x30, 1);
    g_BattleSlotEntryBuffers[index] = BattleLoadArchiveEntry(value + 2);
    ArchiveSetIndex(0x30, 0);
}

void BattleFreeSlotEntry(s32 index) {
    ArchiveCdDataSync(0);
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
}

void BattleAllocResourceBuffer(Misc10Resource* pResource, u_int allocFlags) {
    pResource->buffer = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->entryIndex), allocFlags);
}

void BattleReturnToMainLoop(void) {
    SoundRemoveSedsEntry(g_BattleSedsEntryId);
    if (g_BattleAudioManagerEnabled != 0) {
        SoundStopAudioManager(g_BattlingAudioManager);
        SoundDestroyAudioManager(g_BattlingAudioManager);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    g_BattleTransitionFlag = 1;
    MainLoop(0);
}

s32 BattleCheckModeOver(void) {
    s32 result = 0;

    if (g_BattleScreenMode == 5 || g_BattleScreenMode == 7) {
        result = 1;
    } else if (g_BattleScreenMode == 1 && g_BattleMode == 4 && g_BattleTransitionFlag == 0) {
        result = 1;
    }
    return result;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800852C4);

void BattleCopyVertexOffsetAdd(u16* pSrc, u16* pDst) {
    pDst[0] = pSrc[0] + 0x18;
    pDst[1] = pSrc[1] + 0x6;
    pDst[0] += 0x4F;
}

void BattleCopyVertexOffsetFlip(u16* pSrc, u16* pDst) {
    pDst[0] = 0x8B - pSrc[0];
    pDst[1] = 0x20 - pSrc[1];
    pDst[0] += 0x4F;
}

void BattleSetVertexXForFlipped(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0xDA - value;
    } else {
        *pDst = 0x67 + value;
    }
}

void BattleSetVertexYForFlipped(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0x20 - value;
    } else {
        *pDst = 0x6 + value;
    }
}

// Packet word store, as include/system/font.h's setXY0Fast / setUV0Fast family.
#define Misc10SetXY0(p, _x0, _y0) *(s32*)&(p)->x0 = (_x0) | ((_y0) << 0x10)
#define Misc10SetXY1(p, _x1, _y1) *(s32*)&(p)->x1 = (_x1) | ((_y1) << 0x10)
#define Misc10SetXY2(p, _x2, _y2) *(s32*)&(p)->x2 = (_x2) | ((_y2) << 0x10)
#define Misc10SetXY3(p, _x3, _y3) *(s32*)&(p)->x3 = (_x3) | ((_y3) << 0x10)
#define Misc10SetUV0(p, _u0, _v0) *(u16*)&(p)->u0 = (_u0) | ((_v0) << 0x8)
#define Misc10SetRGBC(p, _r0, _g0, _b0, _code) \
    *(s32*)&(p)->r0 = (_r0) | ((_g0) << 0x8) | ((_b0) << 0x10) | ((_code) << 0x18)

// The two 0x310-byte gauge draw environments that src/battling/main/misc11.c's
// BattleInitDrawEnvPair fills through func_80085EC8 and then swaps between. Every
// entry is a GPU packet template: the DMA word count in tag byte 3, the colour and
// primitive code word, then the vertex pairs the offset table D_800917F4 fills in.
typedef struct {
    /* 0x00 */ u8 pad00[0x3];
    /* 0x03 */ u8 len;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ u16 x0, y0;
    /* 0x0C */ u16 x1, y1;
    /* 0x10 */ u16 x2, y2;
    /* 0x14 */ u16 x3, y3;
} Misc10Quad; // Size: 0x18

// A gauge segment: the same packet with three vertex pairs.
typedef struct {
    /* 0x00 */ u8 pad00[0x3];
    /* 0x03 */ u8 len;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ u16 x0, y0;
    /* 0x0C */ u16 x1, y1;
    /* 0x10 */ u16 x2, y2;
} Misc10Triangle; // Size: 0x14

// A background quad followed by the far corner pair the init code stamps.
typedef struct {
    /* 0x00 */ Misc10Quad quad;
    /* 0x18 */ u32 unk18;
} Misc10BackgroundQuad; // Size: 0x1C

typedef struct {
    /* 0x000 */ DR_TPAGE tpage[2];
    /* 0x010 */ Misc10BackgroundQuad background[4];
    /* 0x080 */ Misc10Quad barPrim[6];
    /* 0x110 */ DR_TPAGE barTpage;
    /* 0x118 */ Misc10Quad barFront[6];
    /* 0x1A8 */ Misc10Quad barBack[6];
    /* 0x238 */ Misc10Triangle gauge[3];
    /* 0x274 */ Misc10Triangle gaugeMirror[3];
    /* 0x2B0 */ Misc10Quad meter[4];
} Misc10GaugeEnv; // Size: 0x310

// The vertex offsets the packets above are filled from: eight halfword pairs.
extern u16 D_800917F4[];

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085EC8);

/* 95.39%, retail's exact 1516 bytes, 6 shapes. Everything in the body is
   instruction-for-instruction retail except register naming: the D_800917F4
   offset bases get s1/s2/s3 where retail has s3/s1/s2 (+0xC is the first
   defined and retail gives it s3), the meter[2]/meter[3] constant block is
   hoisted in retail's order but into different registers, and the gauge loop
   hoists the src.x0 load above src.y0/y1 where retail leaves it third. The
   loop shape that got the exact size was gauge[3] + gaugeMirror[3] (one biv,
   offsets 576/636 off pEnv); gauge[6] with gauge[i+3] makes a second giv
   (+4 bytes) and a walking pointer re-bases onto pEnv+0x284 (+0 bytes).
void func_80085EC8(Misc10GaugeEnv* pEnv) {
    s32 i;

    SetDrawTPage(&pEnv->tpage[0], 0, 1, (u16)GetTPage(0, 2, 0, 0));
    SetDrawTPage(&pEnv->tpage[1], 0, 0, (u16)GetTPage(0, 1, 0, 0));

    pEnv->background[0].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[0].quad, 0, 0, 0, 0x4C);
    pEnv->background[0].unk18 = 0x55555555;
    pEnv->background[1].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[1].quad, 0, 0, 0, 0x4C);
    pEnv->background[1].unk18 = 0x55555555;
    pEnv->background[2].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[2].quad, 0, 0, 0, 0x4C);
    pEnv->background[2].unk18 = 0x55555555;
    pEnv->background[3].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[3].quad, 0, 0, 0, 0x4C);
    pEnv->background[3].unk18 = 0x55555555;

    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->background[0].quad.x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[2], &pEnv->background[0].quad.x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[4], &pEnv->background[0].quad.x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->background[0].quad.x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->background[1].quad.x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->background[1].quad.x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xA], &pEnv->background[1].quad.x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->background[1].quad.x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->background[2].quad.x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[2], &pEnv->background[2].quad.x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[4], &pEnv->background[2].quad.x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->background[2].quad.x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->background[3].quad.x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->background[3].quad.x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xA], &pEnv->background[3].quad.x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->background[3].quad.x3);

    MargePrim(&pEnv->background[0], &pEnv->background[1]);
    MargePrim(&pEnv->background[2], &pEnv->background[3]);

    pEnv->background[0].quad.x0 = 0x1D;
    pEnv->background[2].quad.x0 = 0x121;

    pEnv->barPrim[0].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[0], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[1].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[1], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[2].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[2], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[3].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[3], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[4].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[4], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[5].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[5], 0xFF, 0, 0, 0x28);

    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->barPrim[0].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[0].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xA], &pEnv->barPrim[0].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->barPrim[0].x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xE], &pEnv->barPrim[1].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xC], &pEnv->barPrim[1].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->barPrim[1].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[1].x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xC], &pEnv->barPrim[2].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[2], &pEnv->barPrim[2].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[2].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[4], &pEnv->barPrim[2].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->barPrim[3].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xE], &pEnv->barPrim[3].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xA], &pEnv->barPrim[3].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->barPrim[3].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xE], &pEnv->barPrim[4].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xC], &pEnv->barPrim[4].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->barPrim[4].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->barPrim[4].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xC], &pEnv->barPrim[5].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[2], &pEnv->barPrim[5].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->barPrim[5].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[4], &pEnv->barPrim[5].x3);

    SetDrawTPage(&pEnv->barTpage, 0, 1, (u16)GetTPage(0, 1, 0, 0));
    func_800732AC(&pEnv->barFront[0], &pEnv->barPrim[0], 0x90);
    func_800732AC(&pEnv->barBack[0], &pEnv->barPrim[0], 0x90);

    for (i = 0; i < 6; i++) {
        pEnv->barFront[i].len = 5;
        Misc10SetRGBC(&pEnv->barFront[i], 0x60, 0x60, 0x80, 0x28);
    }
    for (i = 0; i < 6; i++) {
        pEnv->barBack[i].len = 5;
        Misc10SetRGBC(&pEnv->barBack[i], 0xFF, 0, 0, 0x28);
    }

    Misc10SetXY0(&pEnv->gauge[0], 0x14, 0x20);
    Misc10SetXY1(&pEnv->gauge[0], 0x1C, 0x20);
    Misc10SetXY2(&pEnv->gauge[0], 0x1C, 0x28);
    Misc10SetXY0(&pEnv->gauge[1], 0x13, 0x20);
    Misc10SetXY1(&pEnv->gauge[1], 0x13, 0x29);
    Misc10SetXY2(&pEnv->gauge[1], 0x1B, 0x29);
    Misc10SetXY0(&pEnv->gauge[2], 0x13, 0x32);
    Misc10SetXY1(&pEnv->gauge[2], 0x13, 0x2A);
    Misc10SetXY2(&pEnv->gauge[2], 0x1B, 0x2A);

    // Six iterations: retail's own loop writes three mirrored segments past the
    // end of the array, into the empty meter slots below it.
    for (i = 0; i < 6; i++) {
        pEnv->gaugeMirror[i].x0 = 0x140 - pEnv->gauge[i].x0;
        pEnv->gaugeMirror[i].y0 = pEnv->gauge[i].y0;
        pEnv->gauge[i].len = 4;
        Misc10SetRGBC(&pEnv->gauge[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].len = 4;
        Misc10SetRGBC(&pEnv->gaugeMirror[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].y1 = pEnv->gauge[i].y1;
        pEnv->gaugeMirror[i].y2 = pEnv->gauge[i].y2;
        pEnv->gaugeMirror[i].x1 = 0x140 - pEnv->gauge[i].x1;
        pEnv->gaugeMirror[i].x2 = 0x140 - pEnv->gauge[i].x2;
    }

    Misc10SetRGBC(&pEnv->meter[0], 0, 0, 0, 0x28);
    Misc10SetRGBC(&pEnv->meter[1], 0, 0, 0, 0x28);
    pEnv->meter[0].x1 = 0x63;
    pEnv->meter[0].x3 = 0x5E;
    pEnv->meter[0].y1 = 9;
    pEnv->meter[0].y0 = 9;
    pEnv->meter[0].y3 = 0x14;
    pEnv->meter[0].y2 = 0x14;
    pEnv->meter[1].x2 = 0x122;
    pEnv->meter[1].x0 = 0x122;
    pEnv->meter[1].x1 = 0xE3;
    pEnv->meter[1].x3 = 0xDE;
    pEnv->meter[1].y1 = 0x13;
    pEnv->meter[1].y0 = 0x13;
    pEnv->gaugeMirror[1].y0--;
    pEnv->meter[0].x2 = 0x1E;
    pEnv->meter[0].x0 = 0x1E;
    pEnv->meter[1].y3 = 0x1E;
    pEnv->meter[1].y2 = 0x1E;
    pEnv->gaugeMirror[1].x2--;
    pEnv->meter[0].len = 5;
    pEnv->meter[1].len = 5;
    pEnv->meter[2].len = 5;
    Misc10SetRGBC(&pEnv->meter[2], 0xFF, 0, 0, 0x28);
    Misc10SetXY0(&pEnv->meter[2], 6, 0x32);
    Misc10SetXY1(&pEnv->meter[2], 0xA, 0x36);
    Misc10SetXY2(&pEnv->meter[2], 6, 0x4C);
    Misc10SetXY3(&pEnv->meter[2], 0xA, 0x48);
    pEnv->meter[3].len = 5;
    Misc10SetRGBC(&pEnv->meter[3], 0xFF, 0, 0, 0x28);
    Misc10SetXY0(&pEnv->meter[3], 0x13A, 0x32);
    Misc10SetXY1(&pEnv->meter[3], 0x136, 0x36);
    Misc10SetXY2(&pEnv->meter[3], 0x13A, 0x4C);
    Misc10SetXY3(&pEnv->meter[3], 0x136, 0x48);
}
*/

void FieldSoundResetActorEffects(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;
    s32 right;

    switch (mode) {
    case BATTLE_TEX_DEPTH_4BIT:
        scale = 4;
        break;
    case BATTLE_TEX_DEPTH_8BIT:
        scale = 2;
        break;
    case BATTLE_TEX_DEPTH_16BIT:
        scale = 1;
        break;
    }

    ((u8*)pPoly)[3] = 9;
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x;
    right = x + p->prect->w * scale;
    if (scale == 2) {
        pPoly->x3 = right + 1;
    } else {
        pPoly->x3 = right;
    }
    pPoly->y0 = pPoly->y1 = y;
    pPoly->x1 = pPoly->x3 = pPoly->x3;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}


void BattleBuildFlippedTexturedQuad(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;

    switch (mode) {
    case BATTLE_TEX_DEPTH_4BIT:
        scale = 4;
        break;
    case BATTLE_TEX_DEPTH_8BIT:
        scale = 2;
        break;
    case BATTLE_TEX_DEPTH_16BIT:
        scale = 1;
        break;
    }

    ((u8*)pPoly)[3] = 9;
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x + p->prect->w * scale;
    pPoly->x1 = pPoly->x3 = x;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale - 1;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale - 1;
    pPoly->y0 = pPoly->y1 = y;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}

extern POLY_FT4 D_80095698[16];
extern DR_TPAGE D_80095918[4];
extern u8 D_80091814[];
extern u8 g_BattleBarTexV;
extern u8 D_80092864;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

/* 71.59% (1228 bytes against retail 1348), 3 attempts, parked for time.
   The unit's data at D_80095698 is POLY_FT4[16] (0x280 bytes, ending exactly at
   D_80095918) followed by DR_TPAGE[4]; every retail offset lands on a POLY_FT4
   field (+3 len, +4 r0/g0/b0/code, +8 x0/y0, +0xC u0/v0, +0xE clut, +0x10
   x1/y1), so the packets are POLY_FT4s. The word/halfword stores retail has
   (sw 0x090007 at +8, sh 0 at +0xC, sw 0x160016 at +0x10, sw 0x2C000080 at +4)
   are the setXY0Fast/setUV0Fast family from include/system/font.h: gcc 2.7.2
   does NOT merge adjacent short/byte stores, so they must be written as word
   stores (the macros above). This body has all the values, both TIM sections,
   the SetDrawTPage/DR_TPAGE pairs, the three GetClut calls, both blocks of
   sprite setups and all five 0x28-byte copies in retail order.
   What is left: (1) about 30 instructions of register/loop-setup difference -
   retail keeps the POLY base in s1 and forms the copy pointers base-relative
   (addiu s2,s1,0xA0 / addiu t0,s1,0xF0) while keeping the scalar field stores
   ABSOLUTE (lui $at,%hi/%lo), and its copies do not re-materialise the base per
   copy; (2) retail's frame is 0x50 with s3-s0 saved (pActor in s3), ours 0x48
   with s2-s0, i.e. we are one callee-saved register short; (3) the two
   g_BattleBarTexV/D_80092864 lbu's sit earlier in retail than the stores.
   Tried: all-array-symbol accesses (55.28%, copies also go absolute), all
   pointer-local accesses (64.58%, scalar stores then go base-relative).
   The form below is the mixed one.
   void func_800868E0(Misc10Actor* pActor) {
    TIM_IMAGE tim;
    RECT rect;
    POLY_FT4* p;
    u16* pClut;

    p = D_80095698;
    OpenTIM((u_long*)pActor->pTimData);
    ReadTIM(&tim);
    pClut = (u16*)tim.caddr;
    pClut[0] = 0;
    pClut[1] = 0x8000;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    p[5] = p[4];
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 7, &p[0], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 7, &p[2], 0);
    SetDrawTPage(&D_80095918[0], 0, 1, GetTPage(1, 0, 0x380, 0x100));
    D_80095918[1] = D_80095918[0];
    SetDrawTPage(&D_80095918[2], 0, 1, GetTPage(0, 0, 0x380, 0x100));
    D_80095918[3] = D_80095918[2];
    setlen(&D_80095698[4], 4);
    D_80095698[4].code = 0x65;
    Misc10SetXY0(&D_80095698[4], 7, 9);
    Misc10SetUV0(&D_80095698[4], 0, 0);
    Misc10SetXY1(&D_80095698[4], 0x16, 0x16);
    D_80095698[4].clut = GetClut(0, 0x1F6);
    p[6] = p[5];
    Misc10SetXY0(&D_80095698[6], 0x123, 0);
    D_80095698[6].u0 = 0x20;
    D_80095698[6].clut = GetClut(0, 0x1F7);
    OpenTIM((u_long*)pActor->pModelLink);
    ReadTIM(&tim);
    LoadImage(tim.prect, tim.paddr);
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 0x20, &p[12], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 0x20, &p[14], 0);
    Misc10SetXY1(&D_80095698[8], 0x40, 8);
    Misc10SetRGBC(&D_80095698[12], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[13], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[14], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[15], 0x80, 0, 0, 0x2C);
    setlen(&D_80095698[12], 9);
    setlen(&D_80095698[13], 9);
    setlen(&D_80095698[14], 9);
    setlen(&D_80095698[15], 9);
    setlen(&D_80095698[8], 4);
    D_80095698[8].code = 0x65;
    Misc10SetUV0(&D_80095698[8], 0x80, 0);
    g_BattleBarTexV = D_80095698[12].v0;
    D_80092864 = D_80095698[14].v0;
    D_80095698[13].clut = D_80095698[0].clut;
    D_80095698[12].clut = D_80095698[0].clut;
    D_80095698[15].clut = D_80095698[0].clut;
    D_80095698[14].clut = D_80095698[0].clut;
    D_80095698[8].clut = GetClut(0x3A0, 0x110);
    Misc10SetXY0(&D_80095698[8], 0x1E, 0xB);
    p[10] = p[8];
    Misc10SetUV0(&D_80095698[10], 0x80, 8);
    Misc10SetXY0(&D_80095698[10], 0xE3, 0x15);
    p[9] = p[8];
    p[11] = p[10];
    setRECT(&rect, 0x3A0, 0x110, 0x10, 1);
    LoadImage(&rect, (u_long*)D_80091814);
}
*/
void BattleAddSharedPrim(void) {
    AddPrim(g_BattleRootOrderingTable, &g_BattleSharedPrimBuffer[g_BattleDrawBufferIndex]);
}

void func_80086E70(u8* pOt, u8* pBase, s32 value, s32 flag) {
    u16* pTaper;
    s32 edge;

    if (value >= 0x38) {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), 0x38);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), 0x34);
        AddPrim(pOt, pBase);
    } else {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), value);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), value - 4);
        AddPrim(pOt, pBase);
        return;
    }
    if (value >= 0x48) {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), 0x48);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x2C), 0x40);
        BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E), 0x10);
        AddPrim(pOt, pBase + 0x18);
    } else {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), value);
        pTaper = (u16*)(pBase + 0x2C);
        edge = value - 4;
        BattleSetVertexXForFlipped(flag, pTaper, edge - (value - 0x38) / 4);
        BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E), (value - 0x38) / 2 + 8);
        AddPrim(pOt, pBase + 0x18);
        return;
    }
    BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x3C), value);
    BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x44), value - 8);
    AddPrim(pOt, pBase + 0x30);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086FF8);

/* 84.25% - the two value branches each re-materialise `li 0x28000000` (gcc
   rematerialises the constant per branch); the original hoists one lui into
   the beqz delay slot and keeps `ori v1,v1,imm` in each arm, plus the 1 return
   in v0 (ours puts value in a0 and 5 in v1). Tried: full constants in each
   arm, `value = 0x28000000 | (c ? 0xFFFF : 0xFF)`, the ternary inside the
   store, `value |= c ? 0xFFFF : 0xFF`, u32 value, `0x28 << 24`, inverted
   condition - all 82-84%, and -G0 / 2.6.0 score the same.
s32 func_80086FF8(s32 arg0, u8* pOut) {
    s32 value;

    if (BattleCheckActorBelowBreakThreshold(arg0, 0) != 0) {
        return 0;
    }
    value = 0x28000000;
    if (BattleCheckActorBelowBreakThreshold(arg0, 1) != 0) {
        value |= 0xFFFF;
    } else {
        value |= 0xFF;
    }
    pOut[3] = 5;
    *(s32*)(pOut + 4) = value;
    return 1;
}
*/
