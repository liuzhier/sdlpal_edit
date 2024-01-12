#pragma once
#include "PackedPict_Dlg.h"
class MapPict_Dlg:public PackedPict_Dlg
{
	DECLARE_DYNAMIC(MapPict_Dlg)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL makeList();
public:
	MapPict_Dlg(CWnd* pParent, OPENFILENAME* openfilename);
	virtual ~MapPict_Dlg();
	afx_msg LRESULT OnListSelectedRow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void ObjectPopMenuRetuen(UINT msg);

private:
	FILE* fpMAP{};
	FILE* fpGOP{};
	class CPalMapEdit* pMapClass{};//地图数据结构
	//LPBYTE pTileSprite{};
public:
};

