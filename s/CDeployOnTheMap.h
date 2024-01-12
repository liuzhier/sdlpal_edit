#pragma once
#ifndef  CDEPLOYONTHEMAP
#define CDEPLOYONTHEMAP

#include "CPalMapEdit.h"
#include "../main/palgpgl.h"
#include <vector>
//#include "cMap_Dlg.h"
typedef struct ragSceneUndo
{
    DWORD tPos;
    EVENTOBJECT tOld;
    EVENTOBJECT tNew;
}SceneUndo;
typedef std::vector<SceneUndo> MapSceneUndoArray;

class CDeployOnTheMap :
    public CPalMapEdit
{
    MapSceneUndoArray UndoArray;
public:

    CDeployOnTheMap(CMap_Dlg* para= nullptr);
    virtual ~CDeployOnTheMap();
    virtual void Initializes_Tilelist(CImageList&,class CMapList& ) override;
    // 取弹出菜单文字数组
    virtual LPCTSTR* get_Menu_Str()const override;
    //处理弹出菜单返回值 //返回 0 取消 1 刷新 -1 正常 1
    virtual int MapPopMenuRetuen(int msg, DWORD& flags)override;
    //列表选择行返回对应值
    virtual INT  ListRowToReturn(INT r)override;
    //生成显示用的大图
    virtual INT MakeMapImage(CImage& m, const DWORD flag)override;
    // 更新Tile
    virtual VOID UpdateTile(DWORD flg)override;
    // 取显示信息字符串
    virtual VOID get_Msg_Str(CString& inStr, DWORD MapSelectedTPos, 
        DWORD TileSelected, DWORD, DWORD)override;
    //退出时存储
    virtual INT SaveMapTiles()override;
    //检查修改次数
    virtual DWORD GetUndoSize()override;
    virtual INT doUndo()override;
};


#endif // ! CDEPLOYONTHEMAP