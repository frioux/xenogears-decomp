#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "libgear/main.h"

extern s32 D_80050100;

// Interpolation helpers; func_801E34BC picks one for a type
s16 func_801E0850(s16 angle, s16 divisor, s16 offset);
s16 func_801E08D4(s16 a, s16 b, s16 c);
s16 func_801E0938(s16 a, s16 b, s16 c);
s16 func_801E0988(s16 a, s16 b, s16 c);

extern u32 D_801E85CC;
extern u16 D_801E863C;
extern u16 D_801E86B0;
extern s32 D_801E8640;
extern s16 D_801E869C;
extern VertexBuffer g_LibGearVertexBuffer;

ModelMesh* LibGearInitializeMesh(ModelFileHeader* pModelFile, ModelMesh* pModel) {
    u_int numParts;
    int i;

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    numParts = ModelResolvePointers(pModelFile);
    pModel->pParts = HeapAlloc(numParts * sizeof(ModelPart*), 0x0);
    pModel->numParts = numParts;
    if (pModel->pParts != NULL) {
        for (i = 0; i < numParts; i++) {
            pModel->pParts[i] = &pModelFile->modelParts[i];
        }
    }
    return pModel;
}

ModelJoint* LibGearInitializeSkeleton(ModelMesh* pModel, ModelJointEntry* pJointEntries, s32 arg2, s32 hasTexture, s16 texX, s16 texY, s16 clutX, s16 clutY) {
    int i;
    int numJoints;
    u_short jointIndex;
    u_short parentIndex;
    ModelJointEntry* pCurJointEntry;
    ModelJoint* pSkeleton;
    ModelJoint* pCurJoint;

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    
    pCurJointEntry = pJointEntries;
    jointIndex = pCurJointEntry->jointIndex;
    numJoints = 0;
    while (jointIndex < pModel->numParts || jointIndex == 0xFFFF) {
        pCurJointEntry++;
        jointIndex = pCurJointEntry->jointIndex;
        numJoints++;
    }

    if (numJoints == 0) {
        return NULL;
    }

    // Always account for the root bone / joint
    numJoints++;
    
    pSkeleton = HeapAlloc(numJoints * sizeof(ModelJoint), 0x0);
    if (pSkeleton == NULL) {
        return NULL;
    }
    
    pCurJointEntry = pJointEntries;
    pCurJoint = pSkeleton + 1;
    i = 1;
    jointIndex = pCurJointEntry->jointIndex;
    parentIndex = pCurJointEntry->parentIndex;

    // Root bone / joint
    pSkeleton[0].doTransform = TRUE;
    pSkeleton[0].doRotate = TRUE;
    pSkeleton[0].rotDirection = 1;
    pSkeleton[0].scale.vx = 0x1000;
    pSkeleton[0].scale.vy = 0x1000;
    pSkeleton[0].scale.vz = 0x1000;
    pSkeleton[0].pParent = NULL;
    pSkeleton[0].unk7 = 0;
    pSkeleton[0].jointIndex = 0xFFFF;
    pSkeleton[0].numChildren = numJoints;
    pSkeleton[0].pModelPacketBuffer = NULL;
    pSkeleton[0].pCurModelPacket = NULL;
    pSkeleton[0].rotation.vx = 0;
    pSkeleton[0].rotation.vy = 0;
    pSkeleton[0].rotation.vz = 0;
    pSkeleton[0].translation.x = 0;
    pSkeleton[0].translation.y = 0;
    pSkeleton[0].translation.z = 0;
    pSkeleton[0].unk70 = 0;
    pSkeleton[0].unk74 = 0;
    pSkeleton[0].unk78 = 0;
  
    while (jointIndex < pModel->numParts || jointIndex == 0xFFFF) {
        if (parentIndex == 0xFFFF) {
            pCurJoint->pParent = NULL;
        } else {
            pCurJoint->pParent = &pSkeleton[parentIndex] + 1;
        }
        
        pCurJoint->numChildren = i++;
        pCurJoint->doTransform = TRUE;
        pCurJoint->doRotate = TRUE;
        pCurJoint->unk7 = 1;
        pCurJoint->scale.vx = 0x1000;
        pCurJoint->scale.vy = 0x1000;
        pCurJoint->scale.vz = 0x1000;
        pCurJoint->rotDirection = 0;
        pCurJoint->scale.pad = 0;
        pCurJoint->jointIndex = jointIndex;
    
        if (jointIndex != 0xFFFF) {
            // Allocate model packets
            ModelAllocatePackets(pModel->pParts[jointIndex], &pCurJoint->pModelPacketBuffer, &pCurJoint->pCurModelPacket);
            if (pCurJoint->pModelPacketBuffer == NULL) {
                LibGearFreeModelSkeleton(pSkeleton);
                return NULL;
            }
    
            if (hasTexture) {
                ModelPacketSetBaseTexPage(texX, texY);
                ModelPacketSetBaseClut(clutX, clutY);
            }
    
            // Initialize model packets
            ModelInitializePacket(pModel->pParts[jointIndex], pCurJoint->pModelPacketBuffer, arg2);
            memcpy(pCurJoint->pCurModelPacket, pCurJoint->pModelPacketBuffer, pModel->pParts[jointIndex]->unk20[1].modelPacketSize);
        } else {
            pCurJoint->pModelPacketBuffer = NULL;
            pCurJoint->pCurModelPacket = NULL;
        }
    
        pCurJoint->rotation.vx = 0;
        pCurJoint->rotation.vy = 0;
        pCurJoint->rotation.vz = 0;
        pCurJoint->translation.x = 0;
        pCurJoint->translation.y = 0;
        pCurJoint->translation.z = 0;
        pCurJoint->unk70 = 0;
        pCurJoint->unk74 = 0;
        pCurJoint->unk78 = 0;
        
        pCurJoint++;
        pCurJointEntry++;
        
        jointIndex = pCurJointEntry->jointIndex;
        parentIndex = pCurJointEntry->parentIndex;
    }
    
    return pSkeleton;
}

// Computes the local and global transform matrices for the joints of a skeleton
unsigned LibGearComputeJointMatrices(ModelJoint* pJoint, int degrees) {
    ModelJoint* pSkeleton = pJoint;
    MATRIX* scratch = (MATRIX*)PSX_SCRATCH;
    unsigned i;
    unsigned numChildren;

    pJoint->globalTransform.t[0] = pJoint->translation.x;
    pJoint->globalTransform.t[1] = pJoint->translation.y;
    pJoint->globalTransform.t[2] = pJoint->translation.z;

    numChildren = pJoint->numChildren;

    if (pJoint->rotDirection != 0) {
        RotMatrixYXZ(&pJoint->rotation, &pJoint->globalTransform);
    } else {
        RotMatrix(&pJoint->rotation, &pJoint->globalTransform);
    }

    scratch->m[0][0] = (degrees * pJoint->scale.vx) >> 0xC;
    scratch->m[0][1] = 0;
    scratch->m[0][2] = 0;
    scratch->m[1][0] = 0;
    scratch->m[1][1] = (degrees * pJoint->scale.vy) >> 0xC;
    scratch->m[1][2] = 0;
    scratch->m[2][0] = 0;
    scratch->m[2][1] = 0;
    scratch->m[2][2] = (degrees * pJoint->scale.vz) >> 0xC;

    MulMatrix0(&pJoint->globalTransform, scratch, &pJoint->localTransform);

    pJoint->localTransform.t[0] = pJoint->globalTransform.t[0];
    pJoint->localTransform.t[1] = pJoint->globalTransform.t[1];
    pJoint->localTransform.t[2] = pJoint->globalTransform.t[2];

    for (i = 1; i < numChildren; i++) {
        pJoint++;

        if (pJoint->doRotate) {
            if (pJoint->rotDirection != 0) {
                RotMatrixYXZ(&pJoint->rotation, &pJoint->localTransform);
                pJoint->doRotate = FALSE;
            } else {
                RotMatrix(&pJoint->rotation, &pJoint->localTransform);
                pJoint->doRotate = FALSE;
            }
        }

        if ((pJoint->pParent != NULL) && (pJoint->pParent->doTransform == TRUE)) {
            pJoint->doTransform = TRUE;
        }

        if(pJoint->doTransform) {
            pJoint->localTransform.t[0] = pJoint->translation.x;
            pJoint->localTransform.t[1] = pJoint->translation.y;
            pJoint->localTransform.t[2] = pJoint->translation.z;

            if(pJoint->pParent != NULL) {
                CompMatrix(&pJoint->pParent->globalTransform, &pJoint->localTransform, &pJoint->globalTransform);
            } else {
                pJoint->globalTransform = pJoint->localTransform;
            }
        }
    }

    for(i = 1; i < numChildren; i++) {
        pSkeleton++;
        pSkeleton->doTransform = FALSE;
    }

    return numChildren;
}


INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DC848);

void func_801DCC34(void) {}

// https://decomp.me/scratch/y7Bpn
// objdiff 98.68%: identical except that the `arg4` load (0x48(sp)) is
// scheduled right after `lw v1,0(s7)` instead of just before the call
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", LibGearRenderModel);
// void LibGearRenderModel(Model* pModel, ModelJoint* pSkeleton, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 arg4, s32 arg5, s32 packetIndex) {
//     MATRIX* pRotMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x20);
//     MATRIX* pTransMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x40);
//     MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
//     u_int numChildren;
//     u_int i;
//
//     MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pRotMatrix);
//     CompMatrix(pLightMatrix, &pSkeleton->localTransform, pTransMatrix);
//
//     numChildren = pSkeleton->numChildren;
//     pSkeleton++;
//
//     for (i = 1; i < numChildren; i++) {
//         if (pSkeleton->jointIndex != 0xFFFF) {
//             ModelPart* pPart;
//             void* pPacket;
//
//             MulMatrix0(pRotMatrix, &pSkeleton->globalTransform, pMatrix);
//             SetLightMatrix(pMatrix);
//             CompMatrix(pTransMatrix, &pSkeleton->globalTransform, pMatrix);
//             SetRotMatrix(pMatrix);
//             SetTransMatrix(pMatrix);
//
//             pPart = pModel->pMesh->pParts[pSkeleton->jointIndex];
//             pPacket = ((void**)&pSkeleton->pModelPacketBuffer)[packetIndex];
//             ModelRenderPacket(pPart, pPacket, arg5, arg4);
//         }
//
//         pSkeleton++;
//     }
// }

void LibGearFreeModelSkeleton(ModelJoint* pSkeleton) {
    int i;
    ModelJoint* pJoint = pSkeleton;

    if (pSkeleton != NULL) {
        for (i = 0; i < pSkeleton->numChildren; i++) {
            if (pJoint->pModelPacketBuffer != NULL) {
                HeapFree(pJoint->pModelPacketBuffer);
                pJoint->pModelPacketBuffer = NULL;
                pJoint->pCurModelPacket = NULL;
            }
            pJoint++;
        }
        pSkeleton->numChildren = 0;
        HeapFree(pSkeleton);
    }
}

void LibGearFreeModelMesh(ModelMesh* pMesh, s32 shouldFreeLightData) {
    int i;

    if (pMesh != NULL) {
        for (i = 0; i < pMesh->numParts; i++) {
            if (pMesh->pParts != NULL) {
                if (pMesh->pParts[i] && shouldFreeLightData) {
                    ModelPartFreeLightData(pMesh->pParts[i]);
                }
            }
        }
        
        if (pMesh->pParts != NULL) {
            HeapFree(pMesh->pParts);
            pMesh->pParts = NULL;
        }
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DCEC8);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DDBF8);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DEF10);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF0B4);

void func_801DF52C(Temp4* pList, ModelJoint* pSkeleton, s32 index, s32 flags) {
    if (index < pSkeleton->numChildren) {
        pSkeleton += index;

        if (pSkeleton->unk70 != NULL && (flags & 0x1)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != NULL && (flags & 0x2)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != NULL && (flags & 0x4)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }
    }
}

void* func_801DF5F4(Temp4* pUnk, int numObjects) {
    if (numObjects <= 0) {
        return NULL;
    }

    pUnk->numObjects = numObjects;
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    pUnk->pObjects = HeapAlloc(numObjects * sizeof(Temp3), 0x0);
    if (pUnk->pObjects) {
        func_801DF6A8(pUnk);
        return pUnk;
    }
    
    return NULL;
}

void func_801DF668(Temp4* pUnk) {
    pUnk->unk4 = 0;
    if (pUnk->pObjects != NULL) {
        HeapFree(pUnk->pObjects);
    }
    pUnk->pObjects = NULL;
}

void func_801DF6A8(Temp4* pUnk) {
    int i;
    Temp3* pCurObject;

    if (pUnk->pObjects == NULL) {
        return;
    }

    pCurObject = pUnk->pObjects;
    pUnk->unk4 = 0;

    for (i = 0; i < pUnk->numObjects; i++) {
        pCurObject->unk0 = 0;
        pCurObject++;
    }
}

Temp3* func_801DF6F0(Temp4* pList) {
    Temp3* pObject;

    if (pList->unk4 < pList->numObjects) {
        pObject = &pList->pObjects[pList->unk4];
        if (pObject->unk0 == 0) {
            for (
                pList->unk4++;
                pList->unk4 < pList->numObjects;
                pList->unk4++
            ) {
                if (pList->pObjects[pList->unk4].unk0 == 0) {
                    break;
                }
            }
            return pObject;
        }
    }

    return NULL;
}

int func_801DF7A8(Temp4* pUnk, s8* arg1) {
    int index;

    if (arg1 == NULL) {
        return -1;
    }
    
    index = ((mem_addr)arg1 - (mem_addr)pUnk->pObjects) / sizeof(Temp3);
    if (index < pUnk->unk4) {
        pUnk->unk4 = index;
    }
    *arg1 = 0;
    return index;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF7F4);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DFAC4);

void func_801DFE8C(Temp4* pList, ModelJoint* pSkeleton) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->unk70 != 0 && ((Temp3*)pSkeleton->unk70)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != 0 && ((Temp3*)pSkeleton->unk74)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != 0 && ((Temp3*)pSkeleton->unk78)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }

        pSkeleton++;
    }
}

void func_801DFF78(Temp4* pList, ModelJoint* pSkeleton, u8 type) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->unk70 != 0 && ((Temp3*)pSkeleton->unk70)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != 0 && ((Temp3*)pSkeleton->unk74)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != 0 && ((Temp3*)pSkeleton->unk78)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }

        pSkeleton++;
    }
}

void* LibGearInitializeVertexBuffer(VertexBuffer* pVertexBuffer, u32 capacity) {
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    pVertexBuffer->capacity = capacity;
    pVertexBuffer->curIndex = 0;
    pVertexBuffer->pVertices = HeapAlloc((capacity + 1) * sizeof(Vertex), 0x0);
    
    if (pVertexBuffer->pVertices) {
        LibGearInitializeVertices(pVertexBuffer);
        return pVertexBuffer;
    }
    
    return NULL;
}

void LibGearFreeVertexBuffer(VertexBuffer* pVertexBuffer) {
    pVertexBuffer->capacity = 0;
    pVertexBuffer->curIndex = 0;
    if (pVertexBuffer->pVertices != NULL) {
        HeapFree(pVertexBuffer->pVertices);
    }
    pVertexBuffer->pVertices = NULL;
}

void LibGearInitializeVertices(VertexBuffer* pVertexBuffer) {
    int i;
    int j;
    Vertex* pVertex;

    pVertex = pVertexBuffer->pVertices;
    for (i = 0; i < pVertexBuffer->capacity + 1; i++) {
        pVertex->unk16 = -1;
        pVertex->unk1E = 0;
        
        for (j = 0; j < 2; j++) {
            SetPolyFT4(&pVertex->polys[j]);
            SetSemiTrans(&pVertex->polys[j], 1);
            
            pVertex->polys[j].clut = GetClut(0, 0x1CD);
            pVertex->polys[j].tpage = GetTPage(0, 1, 0x340, 0x100);

            setUV4(
                &pVertex->polys[j],
                0x0, 0xBD,
                0x0, 0xBD,
                0xF, 0xBD,
                0xF, 0xBD
            );
        }

        pVertex++;
    }
}

void* func_801E0248(VertexBuffer* pVertexBuffer, short isSemiTrans) {
    Vertex* pVertex;
    
    if (pVertexBuffer->curIndex < pVertexBuffer->capacity) {
        pVertex = &pVertexBuffer->pVertices[pVertexBuffer->curIndex];
        if (pVertex->unk16 == -1) {
            for (
                pVertexBuffer->curIndex++; 
                pVertexBuffer->curIndex < pVertexBuffer->capacity; 
                pVertexBuffer->curIndex++
            ) {
                if (pVertexBuffer->pVertices[pVertexBuffer->curIndex].unk16 == -1) {
                    break;
                }
            }
            
            SetSemiTrans(&pVertex->polys[0], isSemiTrans);
            SetSemiTrans(&pVertex->polys[1], isSemiTrans);
            return pVertex;
        }
    }
    
    return &pVertexBuffer->pVertices[pVertexBuffer->capacity];
}

int func_801E0354(VertexBuffer* pVertexBuffer, Vertex* pObject) {
    int index;

    index = ((mem_addr)pObject - (mem_addr)pVertexBuffer->pVertices) / sizeof(Vertex);
    if (pVertexBuffer->curIndex >= index) {
        pVertexBuffer->curIndex = index;
    }
    
    pObject->unk16 = -1;
    return index;
}

void func_801E0398(VertexBuffer* pVertexBuffer, MATRIX* pMatrix, s32 arg2, s32* pOT, s32 renderCtx) {
    long otz;
    int i;
    Vertex* pVertex;

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    
    pVertex = pVertexBuffer->pVertices;
    for (i = 0; i < pVertexBuffer->capacity; i++, pVertex++) {
        if (pVertex->unk16 == -1) {
            continue;
        }
        
        if (pVertex->unk16 >= pVertex->unk1E) {
            func_801E0354(pVertexBuffer, pVertex);
            continue;
        }

        // Does not match w/ setRGB0 macro
        pVertex->polys[renderCtx].r0 = pVertex->red >> 6;
        pVertex->polys[renderCtx].g0 = pVertex->green >> 6;
        pVertex->polys[renderCtx].b0 = pVertex->blue >> 6;
        
        if (pVertex->unkE == 0) {
            setXY4(
                &pVertex->polys[renderCtx],
                pVertex->x0, pVertex->y0,
                pVertex->x1, pVertex->y1,
                pVertex->x2, pVertex->y2,
                pVertex->x3, pVertex->y3
            );
            addPrim(pOT, &pVertex->polys[renderCtx]);
        } else {
            // RotTransPers3
            gte_ldv3(
                (SVECTOR*) &pVertex->x0,
                (SVECTOR*) &pVertex->x1,
                (SVECTOR*) &pVertex->x2
            );
            gte_rtpt();
            gte_stsxy3(
                (long*) &pVertex->polys[renderCtx].x0,
                (long*) &pVertex->polys[renderCtx].x1,
                (long*) &pVertex->polys[renderCtx].x2
            );
            gte_stszotz(&otz);
            otz >>= D_80050100;

            // RotTransPers
            gte_ldv0((SVECTOR*) &pVertex->x3);
            gte_rtps();
            gte_stsxy((long*) &pVertex->polys[renderCtx].x3);

            addPrim(&pOT[otz], &pVertex->polys[renderCtx]);
        }
        
        pVertex->unk16 += arg2;
        pVertex->red -= pVertex->redDelta * arg2;
        pVertex->green -= pVertex->greenDelta * arg2;
        pVertex->blue -= pVertex->blueDelta * arg2;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E0698);

void func_801E0844(short* pValue) {
    *pValue = -1;
}

s16 func_801E0850(s16 angle, s16 divisor, s16 offset) {
    return offset + (rsin(angle) + 0x1000) / divisor;
}

s16 func_801E08D4(s16 a, s16 b, s16 c) {
    s16 result = c + a / b;

    if (result > 0x20) {
        return -1;
    } else {
        return result;
    }
}

s16 func_801E0938(s16 a, s16 b, s16 c) {
    return c - a / b;
}

s16 func_801E0988(s16 a, s16 b, s16 c) {
    s16 result = 0x20 - a / b;

    if (result < c) {
        result = c;
    }

    return result;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E0A00);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1258);

void func_801E165C(Temp8* pTexture) {
    if (pTexture->unk1A != 0) {
        if (pTexture->pUnk4 != NULL) {
            if (pTexture->unk10 < 4) {
                LoadImage(&pTexture->rect, (u_long*)pTexture->pUnk4);
            }
            HeapFree(pTexture->pUnk4);
            pTexture->pUnk4 = NULL;
        }

        if (pTexture->pUnk8 != NULL) {
            HeapFree(pTexture->pUnk8);
            pTexture->pUnk8 = NULL;
        }

        if (pTexture->pUnkC != NULL) {
            HeapFree(pTexture->pUnkC);
            pTexture->pUnkC = NULL;
        }

        pTexture->unk1A = 0;
    }
}

void func_801E1708(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pUnk1C + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = (*pSrc * scale) / 0x20;
            pSrc++;
        }
    }
}

void func_801E17B8(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    u16* pDelta = pTexture->pUnk8;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pUnk1C + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = *pDelta + (*pSrc - *pDelta) * scale / 0x20;
            pSrc++;
            pDelta++;
        }
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1880);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1A14);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E22F8);

void func_801E3438(Temp9* pEntry) {
    if (pEntry->unk14 != NULL) {
        HeapFree(pEntry->unk14);
        HeapFree(pEntry->unk1C[0]);
        HeapFree(pEntry->unk1C);
        HeapFree(pEntry->unk20);

        if (pEntry->unk18 != NULL) {
            HeapFree(pEntry->unk18);
        }

        pEntry->unk14 = NULL;
    }
}

s16 (*func_801E34BC(s32 type))(s16, s16, s16) {
    switch (type) {
        case 1:
            return func_801E08D4;
        case 2:
            return func_801E0938;
        case 3:
            return func_801E0988;
        default:
            return func_801E0850;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E3534);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E35D0);

s32 func_801E36BC(Model* pModel, s32 arg1, s32 count, s32 arg3, s32 arg4) {
    int i;
    s32 result;

    if (pModel->pMesh != NULL) {
        result = 0;

        if (pModel->unk34) {
            func_801E7298(pModel);

            if (pModel->unk37) {
                func_801DC848(pModel->pSkeleton, pModel->scale);
            } else {
                LibGearComputeJointMatrices(pModel->pSkeleton, pModel->scale);
            }

            for (i = 0; i < count; i++) {
                result |= func_801DDBF8(arg1, pModel->pSkeleton, pModel->unk3C, pModel->scale);
                func_801E5D44(pModel, arg1, arg3);
            }
        }

        func_801E39F0(pModel, arg1, result, count, arg4);
    }

    return result;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E37D0);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E39F0);

void func_801E59D4(Temp4* pList, ModelJoint* pJoint, s32 numFrames, s32 x, s32 y, s32 z) {
    Temp3* pObject;

    if (numFrames < 2) {
        pJoint->rotation.vx = x;
        pJoint->rotation.vy = y;
        pJoint->rotation.vz = z;
        pJoint->doRotate = TRUE;
        return;
    }

    if (pJoint->rotation.vx == x && pJoint->rotation.vy == y && pJoint->rotation.vz == z) {
        return;
    }

    if (pJoint->unk70 == NULL) {
        pObject = func_801DF6F0(pList);
    } else {
        pObject = (Temp3*)pJoint->unk70;
    }

    if (pObject != NULL) {
        pObject->unk0 = 1;
        pObject->unk1 = 0;
        pObject->unk2 = 3;
        pObject->unk3 = 0xFE;

        pObject->unk4 = pJoint->rotation.vx;
        pObject->unk6 = pJoint->rotation.vy;
        pObject->unk8 = pJoint->rotation.vz;

        x = (x - pJoint->rotation.vx) & 0xFFF;
        if (x >= 0x800) {
            x -= 0x1000;
        }
        pObject->unkA = x;

        y = (y - pJoint->rotation.vy) & 0xFFF;
        if (y >= 0x800) {
            y -= 0x1000;
        }
        pObject->unkC = y;

        z = (z - pJoint->rotation.vz) & 0xFFF;
        if (z >= 0x800) {
            z -= 0x1000;
        }
        pObject->unkE = z;

        pObject->unk10 = 0;
        pObject->unk12 = numFrames;
        pJoint->unk70 = (u32)pObject;
    }
}

void func_801E5B50(Temp4* pList, ModelJoint* pJoint, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 x, s32 y, s32 z) {
    Temp3* pObject;
    int dx;
    int dy;
    int dz;

    if (pJoint->unk70 == NULL) {
        pObject = func_801DF6F0(pList);
    } else {
        pObject = (Temp3*)pJoint->unk70;
    }

    if (pObject != NULL) {
        pObject->unk0 = 1;
        pObject->unk1 = 0;
        pObject->unk2 = arg2 + 7;
        pObject->unk3 = 0xFE;

        dx = x - pJoint->translation.x;
        dy = y - pJoint->translation.y;
        dz = z - pJoint->translation.z;

        pObject->unk4 = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
        pObject->unk6 = arg3;
        pObject->unk8 = arg4;
        pObject->unkA = x;
        pObject->unkC = y;
        pObject->unkE = z;
        pObject->unk10 = 0;
        pObject->unk12 = arg5;

        pJoint->unk70 = (u32)pObject;
    }
}

void func_801E5C74(Model* pModel, Temp7* pEntry, s32 arg2) {
    if (pEntry->unk12 != 0) {
        pModel->unk98 = 0;
        if (arg2 != 0) {
            pModel->unk9A = pEntry->unk2;
        } else {
            pModel->unk9A = -1;
        }
        pModel->unk9C = 0;
        pModel->unk9E = pEntry->unk12;
        pModel->unkA0 = (u8*)pEntry + pEntry->unk14;
        pModel->unkA4 = pModel->unkA0;
    } else {
        pModel->unk98 = -1;
    }
}

// objdiff 64.59%: original keeps a separate copy of the `lhu 0x14 / sll 16`
// tail for the type==1 case; gcc cross-jumps all three here
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5CD8);
// s32 func_801E5CD8(Model* pModel, s32 type) {
//     if (type == 0) {
//         return ((Temp8*)D_8005919C)->unk14 << 16;
//     } else if (type == 1) {
//         return ((Temp8*)((Temp8*)pModel->unkB0)->pUnk8)->unk14 << 16;
//     } else if (type == 2) {
//         return ((Temp8*)((Temp8*)pModel->unkB4)->pUnk8)->unk14 << 16;
//     }
//
//     return 2;
// }

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5D44);

void func_801E632C(Model* pModel) {
    pModel->unk98 = -1;
}

int func_801E6338(Model* pModel) {
    int dx = pModel->unk70[0xC] - pModel->pSkeleton->translation.x;
    int dy = pModel->unk70[0xD] - pModel->pSkeleton->translation.y;
    int dz = pModel->unk70[0xE] - pModel->pSkeleton->translation.z;

    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E63A8);

// objdiff 97.43%: body is identical, but pList/pSkeleton land in s4/s3 and
// i/pJoint in s0/s1 instead of s3/s4 and s1/s0; declaration order has no effect
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6578);
// void func_801E6578(Temp4* pList, s32 jointIndex, ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
//     ModelJoint* pJoint = &pSkeleton[jointIndex];
//     ModelJoint* pTarget = &pTargetSkeleton[jointIndex];
//     ModelJoint* pChild;
//     int i;
//     int numChildren;
//
//     numChildren = pSkeleton->numChildren;
//     pJoint->unk7 = 0;
//     pTarget->unk7 = 1;
//
//     func_801DF7A8(pList, (s8*)pJoint->unk70);
//     pJoint->unk70 = 0;
//     func_801DF7A8(pList, (s8*)pJoint->unk74);
//     pJoint->unk74 = 0;
//     func_801DF7A8(pList, (s8*)pJoint->unk78);
//     pJoint->unk78 = 0;
//
//     pChild = pSkeleton;
//     for (i = 1; i < numChildren; i++) {
//         pChild++;
//
//         if (pChild->pParent == pJoint) {
//             func_801E6578(pList, pChild->numChildren, pSkeleton, pTargetSkeleton);
//         }
//     }
// }

void func_801E6668(ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
    int i;
    u_short numChildren = pSkeleton->numChildren;

    for (i = 1; i < numChildren; i++) {
        pSkeleton++;
        pTargetSkeleton++;

        if (pSkeleton->unk7 != 0) {
            pSkeleton->unk7 = 0;
            pTargetSkeleton->unk7 = 1;
        }
    }
}

s16 func_801E66BC(VECTOR* pNormal, VECTOR* pVec0, VECTOR* pVec1, s32 divisor) {
    VECTOR outer;
    s32 dot;
    s32 length;

    OuterProduct12(pVec0, pVec1, &outer);
    dot = outer.vx * pNormal->vx + outer.vy * pNormal->vy + outer.vz * pNormal->vz;
    length = SquareRoot0(outer.vx * outer.vx + outer.vy * outer.vy + outer.vz * outer.vz) + 1;
    return ((dot << 4) / length << 8) / divisor;
}

int func_801E67F8(void) {
    u_short bits = D_801E863C;
    int i;

    for (i = 0; i < 8; i++) {
        if ((bits >> i) & 0x1) {
            break;
        }
    }

    return i;
}

// objdiff 92.95%: the u8 conversion of `index` is emitted once at the shared
// tail instead of being duplicated into each branch's delay slot
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6830);
// void func_801E6830(Model* pModel, u8 type, s16* pMask) {
//     u8 index;
//
//     if (type == 0xFF) {
//         index = func_801E67F8();
//     } else if (type == 0xFE) {
//         index = D_801E86B0;
//     } else if (type == 0xFD || type == 0xF9) {
//         index = pModel->unk20;
//     } else if (type == 0xFC) {
//         index = pModel->unk21;
//     } else if (type == 0xFA) {
//         index = 0xA;
//     } else if (type == 0xF8) {
//         index = pModel->unk20 * 2 + 8;
//     } else if (type == 0xF7) {
//         index = pModel->unk20 * 2 + 9;
//     } else {
//         index = type;
//     }
//
//     *pMask = 1 << index;
// }

void* func_801E6910(Model* pModel, u8 index, s32* pFlags) {
    void* pResult;

    *pFlags = 0;
    if (index >= 0xFE) {
        u8 entry = pModel->unk2A;

        index = entry & 0x7F;
        *pFlags = entry & 0x80;
    }

    if (index >= 0x40) {
        pResult = pModel->unk18[index - 0x3F];
    } else {
        pResult = pModel->unk14[index + 1];
    }

    return pResult;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6974);

void func_801E6D94(Model* pModel, ModelJoint* pJoint, s32 flags) {
    ModelJoint* pChild;
    int i;

    pJoint->unk7 = flags & 0x1;

    if (flags & 0x80) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                func_801E6D94(pModel, pChild, flags);
            }
        }
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6E48);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6F64);

void func_801E7094(Model* pModel, ModelJoint* pJoint, u8 flags, s16 x, s16 y, s16 z) {
    ModelJoint* pChild;
    int i;

    if ((flags & 0x7) == 0) {
        if (flags & 0x20) {
            pJoint->rotation.vx += x;
            pJoint->rotation.vy += y;
            pJoint->rotation.vz += z;
        } else {
            pJoint->rotation.vx = x;
            pJoint->rotation.vy = y;
            pJoint->rotation.vz = z;
        }
    } else if ((flags & 0x7) == 1) {
        if (flags & 0x20) {
            pJoint->translation.x += x;
            pJoint->translation.y += y;
            pJoint->translation.z += z;
        } else {
            pJoint->translation.x = x;
            pJoint->translation.y = y;
            pJoint->translation.z = z;
        }
    } else {
        if (flags & 0x20) {
            pJoint->scale.vx += x;
            pJoint->scale.vy += y;
            pJoint->scale.vz += z;
        } else {
            pJoint->scale.vx = x;
            pJoint->scale.vy = y;
            pJoint->scale.vz = z;
        }
    }

    pJoint->doTransform = TRUE;
    pJoint->doRotate = TRUE;

    if (flags & 0x80) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                func_801E7094(pModel, pChild, flags, x, y, z);
            }
        }
    }
}

// objdiff 99.77%: the local sits at sp+0x10 in a 0x18 frame (as if the
// out-arg area were reserved); ours is sp+0x2 in an 0x8 frame
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E7298);
// void func_801E7298(Model* pModel) {
//     SVECTOR offset;
//
//     offset.vy = pModel->ground;
//     if (pModel->unk36 == 0) {
//         pModel->pSkeleton->translation.y = offset.vy;
//     }
// }

// Computes the global transform matrix for a certain joint?
void func_801E72CC(MATRIX* pMatrix, void* _unk, int index, int jointIndex) {
    MATRIX _mat;
    Model* pModel;

    pModel = g_LibGearModels[index];
    if (pModel == NULL) {
        return;
    }
    
    if (jointIndex != 0) {
        CompMatrix(
            &pModel->pSkeleton[0].localTransform,
            &pModel->pSkeleton[jointIndex].globalTransform, 
            pMatrix
        );
        return;
    }
    
    *pMatrix = pModel->pSkeleton[0].localTransform;
}

void func_801E7378(u32 shouldRender) {
    D_801E85CC = shouldRender & 0x1;
}

void LibGearInitialize(int capacity) {
    int i;

    D_801E8640 = 0;
    D_801E869C = 0;
    
    func_801DF5F4(&D_801E86A8, capacity);
    LibGearInitializeVertexBuffer(&g_LibGearVertexBuffer, 0x10);

    for (i = 0; i < MAX_MODELS; i++) {
        g_LibGearModels[i] = NULL;
    }

    for (i = 0; i < MAX_MODEL_MESHES; i++) {
        g_LibGearMeshes[i].pParts = NULL;
    }

    for (i = 0; i < 2; i++) {
        D_801E8648[i].unk6 = 0;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E742C);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E7D14);

void func_801E7FD4(void) {
    int i;

    for (i = 0; i < MAX_MODELS; i++) {
        func_801E8030(i);
    }

    func_801DF668(&D_801E86A8);
    LibGearFreeVertexBuffer(&g_LibGearVertexBuffer);
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E8030);

void func_801E8330(u16 modelIndex, u16 bits, s32 arg2) {
    Model* pModel = g_LibGearModels[modelIndex];

    D_801E86B0 = modelIndex;
    D_801E863C = bits;
    pModel->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        func_801E35D0(g_LibGearModels[modelIndex], g_LibGearModels[modelIndex], &D_801E86A8, arg2);
    }
}

void func_801E8394(s32 arg0, u16 modelIndex, u16 arg2, s32 arg3) {
    D_801E86B0 = modelIndex;
    D_801E863C = arg2;
    g_LibGearModels[modelIndex]->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (modelIndex != func_801E67F8()) {
            func_801E35D0(g_LibGearModels[modelIndex], arg0, &D_801E86A8, arg3);
        }
    }
}

int func_801E8430(int modelIndex) {
    Model* pModel = g_LibGearModels[modelIndex];
    int result = 0;

    if (pModel != NULL) {
        result = (pModel->unk24 * ((pModel->scale * pModel->pSkeleton->scale.vy) >> 0xC)) >> 0xC;
    }

    return result;
}

int func_801E8480(int modelIndex) {
    int factor;
    int jointScale;
    int result = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (g_LibGearModels[modelIndex]->unk4A & 0x8) {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vx;
            factor = g_LibGearModels[modelIndex]->unk26;
        } else {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vz;
            factor = g_LibGearModels[modelIndex]->unk28;
        }

        result = factor * ((g_LibGearModels[modelIndex]->scale * jointScale) >> 0xC) >> 0xC;
    }

    return result;
}

void func_801E8510(Model* pModel) {
    int i;
    UnkInner1* pObjects;

    if (pModel->unk10C) {
        pObjects = HeapAlloc(pModel->unk10C * sizeof(UnkInner1), 0x0);
        for (i = 0; i < pModel->unk10C; i++) {
            pObjects[i].unk0 = -1;
            pObjects[i].unk8 = 0;
        }
        pModel->unk110 = pObjects;
    }
}
