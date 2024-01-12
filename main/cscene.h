#ifndef CSCENE_H
#define CSCENE_H
#include "cpalbaseio.h"
#include <vector>
//#include "cscript.h"

//
#define MAX_SPRITE_TO_DRAW         2048

class CScene: public CPalBaseIO
{
    typedef struct tagSPRITE_TO_DRAW
    {
        LPCBITMAPRLE     lpSpriteFrame; // pointer to the frame bitmap
        PAL_POS          pos;           // position on the scene
        int              iLayer;        // logical layer
    } SPRITE_TO_DRAW;
    typedef vector<SPRITE_TO_DRAW> SPRITE_TO_DRAW_ARRAY;
    SPRITE_TO_DRAW_ARRAY    g_rgSpriteToDraw;
    //int               g_nSpriteToDraw;

public:
    CScene();
private:
    VOID PAL_AddSpriteToDraw(LPCBITMAPRLE lpSpriteFrame, int x, int y, int iLayer);
    VOID PAL_CalcCoverTiles(SPRITE_TO_DRAW* lpSpriteToDraw);
    VOID PAL_SceneDrawSprites(CGL_Texture * iText);
    VOID PAL_Drow_Object(VOID);
public:
    VOID PAL_UpdateParty(VOID);
    VOID PAL_NPCWalkOneStep(WORD wEventObjectID, INT iSpeed);
    BOOL PAL_NPCWalkTo(WORD wEventObjectID, INT x, INT y, INT h, INT iSpeed);
    VOID PAL_MakeScene(VOID);
    VOID PAL_UpdatePartyGestures(BOOL fWalking);
    BOOL PAL_CheckObstacle(PAL_POS pos, BOOL fCheckEventObjects, WORD wSelfObject);
};

#endif // CSCENE_H
