#ifndef CSCRIPT_H
#define CSCRIPT_H

#include "command.h"
#include "cpalmain.h"


typedef std::vector<INT32>TestData;

//本类是游戏的集中流程控制，所有上层均调用本类，
//本类需要通过接口反过来调用上层类
class CScript :public CPalMain
{
public:
    CScript();
    ~CScript();
    CScript(int save,BOOL noRun = 0);
    WORD PAL_InterpretInstruction(WORD wScriptEntry, WORD wEventObjectID);
    WORD PAL_RunAutoScript(WORD wScriptEntry, WORD wEventObjectID);
    WORD PAL_RunTriggerScript(WORD wScriptEntry,WORD wEventObjectID);
    VOID PAL_ShowFBP(WORD wChunkNum, WORD wFade);
    VOID PAL_PartyRideEventObject(WORD wEventObjectID, INT x, INT y, INT h, INT iSpeed);
    VOID PAL_PaletteFade(INT iPaletteNum, BOOL fNight, BOOL fUpdateScene);
    VOID PAL_MonsterChasePlayer(WORD wEventObjectID, WORD wSpeed, WORD wChaseRange, BOOL fFloating);
    VOID PAL_EndingSetEffectSprite(WORD wSpriteNum);
    VOID PAL_ScrollFBP(WORD wChunkNum, WORD wScrollSpeed, BOOL fScrollDown);
    VOID PAL_NEW_EndingScreen2(VOID);
    VOID PAL_EndingAnimation(VOID);
    VOID PAL_NEW_EndingScreen1(VOID);
    VOID PAL_EndingScreen(VOID);
private:

};

#endif // CSCRIPT_H
