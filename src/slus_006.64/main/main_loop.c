#include "common.h"
#include "main/main.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/controller.h"

#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/pc.h"

extern unsigned int g_CurGameState;
extern unsigned int g_CurGameStateOverlayID;
extern void* g_CurGameStateOverlayBuffer;

extern void* LZSSHeapDecompress(void*, int flags); 
extern u8 g_PublishedByLogoCompressed[];

extern void ArchiveReadFileToBuffer(s32, void*, u32, u32);

extern s32 g_DebugTable;
extern s8 g_StrHeapDumpPath;
extern char** D_8004F0C0;
extern u32 g_GameErrorCount;

/*
char** g_KernelErrorDescriptions = {
    "LsKernel:Program Not Defined",
    "LsKernel:PC File Not Found",
    "LsGetMem:Memory Not Enough",
    "LsFreeMem:Can't Release NULL Pointer",
    "LsGetMem:MCB Broken",
    "LsFreeMem:This ptr isn't MCB"
};
*/

void ChangeGameState(unsigned int state) {
    g_CurGameState = state;
    if (state != g_CurGameStateOverlayID) {
        if (g_CurGameStateOverlayBuffer) {
            HeapFree(g_CurGameStateOverlayBuffer);
            g_CurGameStateOverlayBuffer = NULL;
        }
        g_CurGameStateOverlayID = -1;
    }
}

void* LoadGameStateOverlay(unsigned int overlayIndex) {
    int nPrevSectionIndex;
    int nPrevEntryIndex;
    unsigned int nPrevHeapUser;
    unsigned int nPrevHeapErrorHandlerStatus;
    unsigned int nFileSize;
    void* pBuffer;
    
    if (g_CurGameStateOverlayID != overlayIndex) {
        g_CurGameStateOverlayID = overlayIndex;
        
        nPrevHeapUser = HeapGetCurrentUser();
        ArchiveGetArchiveOffsetIndices(&nPrevSectionIndex, &nPrevEntryIndex);
        HeapSetCurrentUser(HEAP_USER_SUGI);
        ArchiveSetIndex(ARCHIVE_DIR_CORE, 0x1);
        nPrevHeapErrorHandlerStatus = HeapToggleErrorHandler(0x1);

        nFileSize = ArchiveDecodeSize(g_GameStateOverlayArchiveOffsets[overlayIndex]);
        pBuffer = HeapAlloc(nFileSize, 0x1);
        g_CurGameStateOverlayBuffer = pBuffer;
        if (pBuffer != NULL) {
            // Read archive file into buffer
            ArchiveReadFileToBuffer(g_GameStateOverlayArchiveOffsets[overlayIndex], pBuffer, 0, 0);
        } else {
            g_CurGameStateOverlayID = -1;
        }

        HeapToggleErrorHandler(nPrevHeapErrorHandlerStatus);
        ArchiveSetIndex(nPrevSectionIndex, nPrevEntryIndex);
        HeapSetCurrentUser(nPrevHeapUser);
    }

    return g_CurGameStateOverlayBuffer;
}

void MainLoop(int errorCode) {
    char sp[8];
    unsigned int nCallerAddr;
    MainGameState* pGameState;
    void* pOverlayData;

    // Error handler
    if (errorCode) {
        asm volatile(
            "move $t7, %0\n\t"
            "sw $ra, 0($t7)\n\t"
        :: "r"(&nCallerAddr));
        GameHandleError(errorCode, nCallerAddr);
    }
    
    pGameState = &g_MainGameStates[g_CurGameState];
    ResetGraph(1);
    DrawSyncCallback(NULL);
    ControllerSetFrameCallback(0);
    DrawSync(0);
    Vsync(2);
    HeapRelocate(pGameState->pHeapStart + 0x800);
    HeapResetUser();
    
    if (pGameState->hasOverlay) {
        ClearMemory(pGameState->pMemStart, pGameState->pHeapStart);
        pOverlayData = LoadGameStateOverlay(g_CurGameState);
        ArchiveCdDataSync(0);

        // Decompress overlay into global overlay buffer
        LZSSDecompress(pOverlayData, g_MainGameStateOverlayBuffer);
        
        DrawSync(0);
        Vsync(0);
        EnterCriticalSection();
        DrawSync(0);
        Vsync(0);
        FlushCache();
        ExitCriticalSection();
    }
    
    func_80019548(); // Does nothing, likely a debug function
    HeapRelocate(pGameState->pHeapStart + 4);
    HeapReset();
    ControllerResetState();
    ChangeGameState(0);
    pGameState->pFnMain();
    MainLoop(0);
}

extern char g_StrCoreDumpPath; // "c:\\core"
void GameCreateCoreDump(void) {
  int hFile;
  
  PCinit();
  hFile = PCcreate(&g_StrCoreDumpPath, 0);
  PCwrite(hFile, 0x80000000, 0x200000);
  PCclose(hFile);
}

void HeapResetUser() {
    HeapChangeCurrentUser(HEAP_USER_UNKNOWN, 0);
}

void GameCheckAndHandleSoftReset(void) {
    if (g_C1ButtonState == (CTRL_BTN_START | CTRL_BTN_SELECT | CTRL_BTN_R1 | CTRL_BTN_L1)) {
        GameSoftReset();
    }
}

void GameSoftReset(void) {
    SwExitCriticalSection();
    ResetGraph(0);
    ArchiveReset();
    SoundReset();
    SpuQuit();
    ControllerSetFrameCallback(0);
    DrawSyncCallback(0);
    VSyncCallback(0);
    CdFlush();
    StopPAD();
    SwEnterCriticalSection();
    start();
}

void GameShowSplashScreen(void) {
    DRAWENV drawEnv;
    DISPENV dispEnv;
    SPRT sprite;
    RECT rect;
    int i;
    void* pClutData;
    void* pImage;

    HeapSetCurrentUser(HEAP_USER_SUGI);
    pImage = LZSSHeapDecompress(&g_PublishedByLogoCompressed, 0x1);

    // Load CLUT
    pClutData = pImage + 0x14;
    rect.y = 0xF0;
    rect.w = 0x10;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, pClutData);

    // Load Image Data
    rect.x = 0x280;
    rect.y = 0;
    rect.w = 0x40;
    rect.h = 0x30;
    LoadImage(&rect, pImage + 0x40);
    
    setSprt(&sprite);
    setXY0(&sprite, 0x20, 0x58);
    sprite.v0 = 0;
    sprite.u0 = 0;
    setWH(&sprite, 0x100, 0x30);
    setClut(&sprite, 0x0, 0xF0);
    
    SetDefDrawEnv(&drawEnv, 0, 0, 0x140, 0xE0);
    SetDefDispEnv(&dispEnv, 0, 0, 0x140, 0xE0);
    PutDrawEnv(&drawEnv);
    PutDispEnv(&dispEnv);
    DrawSync(0);
    
    for (i = 0; i < 0x80; i += 8) {
        setRGB0(&sprite, i, i, i);
        DrawPrim(&sprite);
        Vsync(0);
    }

    for (i = 0x6D; i != -1; i--) {
        Vsync(0);
    }

    for (i = 0x80; i >= 0; i -= 8) {
        setRGB0(&sprite, i, i, i);
        DrawPrim(&sprite);
        Vsync(0);
    }
    
    HeapFree(pImage);
}

// These are all in .rodata, indicating -G0?
extern char g_StrSystemErrorNo; // "System Error No %d\n"
extern char g_StrErrorSourceFrom; // "From  %08x\n"
extern char g_StrErrorCount; // "Count %d\n"
extern char g_StrErrorFrame; // "Frame %d\n"
extern char g_StrMcbLogPath; // "MCBlog -> c:\\lserrmem.txt\n"
extern char g_StrNewline; // "\n"
extern char g_StrErrorDescription; // "%s\n"
extern char g_StrErrorProgramFrom; // "Program From %08x\n"
extern char g_StrErrorFailureSize; // "Failure Size %d (%xh) byte \n"
extern char g_StrErrorFailurePointer; // "Failure Pointer %p\n"

void GameHandleError(unsigned int errorCode, unsigned int sourceAddress) {
    DRAWENV* pDrawEnv;
    DISPENV* pDispEnv;
    DRAWENV drawEnvs[2];
    DISPENV dispEnvs[2];
    RECT rect;
    unsigned int nFrame;
    char _pad[4];
    unsigned int allocSourceAddress;
    unsigned int allocSize;

    nFrame = 0;
    
    if ((u32) (g_DebugTable + 1) >= 0x2) {
        HeapDumpToFile(&g_StrHeapDumpPath);
    } else {
        rect.w = 0x280;
        rect.x = 0;
        rect.y = 0;
        rect.h = 0x1E0;
        ClearImage(&rect, 0xFF, 0, 0);
        while(1) {}
    }
    
    HeapForceFreeAllBlocks();
    SetDefDrawEnv(&drawEnvs[0], 0, 0, 0x180, 0xF0);
    SetDefDispEnv(&dispEnvs[0], 0, 0xF0, 0x180, 0xF0);
    SetDefDrawEnv(&drawEnvs[1], 0, 0xF0, 0x180, 0xF0);
    SetDefDispEnv(&dispEnvs[1], 0, 0, 0x180, 0xF0);
    DrawSyncCallback(0);
    ControllerSetFrameCallback(0);
    FontLoadFont(0x10, 0x10, 0x120, 0xF0, 0x1F4, 0, 0x3C0, 0x100, 0x3C0, 0x1FF, 0);
    drawEnvs[1].isbg = 1;
    drawEnvs[0].isbg = 1;
    drawEnvs[0].r0 = 0;
    drawEnvs[0].g0 = 0;
    drawEnvs[0].b0 = 0;
    drawEnvs[1].r0 = 0;
    drawEnvs[1].g0 = 0;
    drawEnvs[1].b0 = 0;
    g_GameErrorCount += 1;
    SetDispMask(1);

    // Does not match if using while(1) { ... }
    loop:
        pDrawEnv = &drawEnvs[0];
        if (!(nFrame & 1))
            pDrawEnv = &drawEnvs[1];
        PutDrawEnv(pDrawEnv);
    
        pDispEnv = &dispEnvs[0];
        if (!(nFrame & 1))
            pDispEnv = &dispEnvs[1];
        PutDispEnv(pDispEnv);
        
        FontDrawLetters(0);
        FontPrintf(&g_StrSystemErrorNo, errorCode);
        FontPrintf(&g_StrErrorSourceFrom, sourceAddress);
        FontPrintf(&g_StrErrorCount, g_GameErrorCount);
        FontPrintf(&g_StrErrorFrame, nFrame);
        FontPrintf(&g_StrMcbLogPath);
        FontPrintf(&g_StrNewline);
        
        if (errorCode & ERR_KERNEL) {
            /*
            D_8004F0C0:
            0x80: "LsKernel:Program Not Defined
            0x81: LsKernel:PC File Not Found
            0x82: LsGetMem:Memory Not Enough
            0x83: LsFreeMem:Can't Release NULL Pointer
            0x84: LsGetMem:MCB Broken
            0x85: LsFreeMem:This_ptr_isn't MCB
            */
            FontPrintf(&g_StrErrorDescription, *(&D_8004F0C0 + errorCode) );
            
            if (errorCode == ERR_HEAP_OUT_OF_MEMORY) {
                HeapGetAllocInformation(&allocSourceAddress, &allocSize);
                FontPrintf(&g_StrErrorProgramFrom, allocSourceAddress);
                FontPrintf(&g_StrErrorFailureSize, allocSize, allocSize);
            }
            
            if (errorCode == 0x85) {
                HeapGetAllocInformation(&allocSourceAddress, &allocSize);
                FontPrintf(&g_StrErrorProgramFrom, allocSourceAddress);
                FontPrintf(&g_StrErrorFailurePointer, allocSize);
            }
        }
        
        Vsync(0);
        nFrame += 1;
    goto loop;
}