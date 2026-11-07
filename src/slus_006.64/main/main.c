#include "common.h"

#include "system/kernel.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/controller.h"

#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/libapi.h"

extern unsigned int g_CurGameStateOverlayID;
extern void* g_CurGameStateOverlayBuffer;
extern u_short g_C1ButtonState;

extern s32 D_80059560;
extern s32 D_800595AC;
extern u_char D_8004FE44;
extern u_char D_8004FE45;
extern u_char D_8004FE46;
extern u_char D_8004FE47;
extern u_char D_80010004[];
extern u_char D_80018004[];

extern void func_8003634C(void);
extern int func_8002DFE0(void);
extern void func_8004B7D0(void*);
extern void func_8003BDFC(int);
extern void func_8001AADC(void);
extern void func_8001BB50(void);
extern void func_80024F20(void);
extern void func_800379B4(int);
extern void func_8001B6BC(void);
extern void GameShowSplashScreen(void);
extern void ChangeGameState(unsigned int);
extern void MainLoop(int);
extern int ControllerGetType(int);
extern int ControllerPopState(void);
extern void* LZSSHeapDecompress(void*, int);
extern int SoundLoadWdsFile(void*, int);
extern void SoundInitialize(int);
extern void SoundReset(void);
extern int ArchiveGetDiscNumber(void);
extern void ArchiveCdDataSync(int);
extern void ArchiveReadFileToBuffer(s32, void*, u32, u32);
extern void SystemInitializeFont(void*);
extern void SystemInitializeData(void*);

void func_80019578(void) {
    RECT rect;
    void* pWdsFile2;
    void* pWdsFile3;
    void* pWdsFile4;
    void* pWdsFile5;
    void* pArchiveData;
    int prevHeapUser;
    int discNumber;

    ResetCallback();
    SetGraphDebug(0);
    SetVideoMode(0);
    ResetGraph(0);
    rect.w = 0x180;
    rect.x = 0;
    rect.y = 0;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    SetDispMask(1);
    InitGeom();
    ControllerInit();
    InitCARD(1);
    StartCARD();
    _bu_init();
    func_8004B7D0(func_8003634C);
    HeapInit((void*)func_8002DFE0(), 0x801FC000);
    SpuInit();
    ArchiveInit(D_80010004, D_80018004, D_80010000);
    SoundInitialize(0);
    ArchiveSetIndex(0, 1);
    pWdsFile2 = HeapAlloc(ArchiveDecodeSize(2), 0);
    pWdsFile3 = HeapAlloc(ArchiveDecodeSize(3), 0);
    pWdsFile4 = HeapAlloc(ArchiveDecodeSize(4), 0);
    pWdsFile5 = HeapAlloc(ArchiveDecodeSize(5), 0);
    ArchiveReadFileToBuffer(2, pWdsFile2, 0, 0);
    ArchiveReadFileToBuffer(3, pWdsFile3, 0, 0);
    ArchiveReadFileToBuffer(4, pWdsFile4, 0, 0);
    ArchiveReadFileToBuffer(5, pWdsFile5, 0, 0);
    ArchiveCdDataSync(0);
    SoundLoadWdsFile(pWdsFile2, 0);
    D_80059560 = SoundLoadWdsFile(pWdsFile3, 0);
    SoundLoadWdsFile(pWdsFile4, 0);
    D_800595AC = SoundLoadWdsFile(pWdsFile5, 0);

    // Font and gauge data live in a different heap
    prevHeapUser = HeapGetCurrentUser();
    HeapSetCurrentUser(HEAP_USER_SUGI);
    pArchiveData = HeapAlloc(ArchiveDecodeSize(6), 0);
    ArchiveReadFileToBuffer(6, pArchiveData, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x30);
    SystemInitializeFont(LZSSHeapDecompress(pArchiveData, 1));
    HeapFree(pArchiveData);

    pArchiveData = HeapAlloc(ArchiveDecodeSize(7), 0);
    ArchiveReadFileToBuffer(7, pArchiveData, 0, 0);
    ArchiveCdDataSync(0);
    HeapSetCurrentContentType(0x31);
    SystemInitializeData(LZSSHeapDecompress(pArchiveData, 1));
    HeapFree(pArchiveData);
    HeapSetCurrentUser(prevHeapUser);

    func_8003BDFC(0x10);
    HeapFree(pWdsFile2);
    HeapFree(pWdsFile3);
    HeapFree(pWdsFile4);
    HeapFree(pWdsFile5);

    func_8001AADC();
    func_8001BB50();
    func_80024F20();
    func_800379B4(0);

    g_CurGameStateOverlayID = -1;
    g_CurGameStateOverlayBuffer = NULL;
    D_8004FE44 = 1;
    D_8004FE46 = 1;
    D_8004FE47 = 0;
    discNumber = ArchiveGetDiscNumber();
    D_8004FE45 = discNumber == 1 ? 0x10 : 0x7;

    if (ControllerGetType(0) != 0) {
        while (g_C1ButtonState == 0x90C) {
            ControllerPopState();
        }
    }

    GameShowSplashScreen();
    func_8001B6BC();
    ChangeGameState(6);
    MainLoop(0);
}
