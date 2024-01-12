#pragma once
#include "stdafx.h"
#include "afxdialogex.h"
#include "CMapList.h"
#include <vector>
#include <SDL.h>

//class CMapList;

// cMap_Dlg 对话框



class CMap_Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMap_Dlg)

	//friend class CsDataGrid;

public:
	CMap_Dlg(CWnd* pParent, int blogo = 0);   // 构造函数
	virtual ~CMap_Dlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAP_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	BOOL OnInitDialog();
	//处理弹出菜单返回值
	afx_msg void MapPopMenuRetuen(UINT msg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnListSelectedRow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

	BOOL PreTranslateMessage(MSG* pMsg);
	BOOL isChanged();
public:
	CImageList m_ImageList;//tile图列表
	INT logo{};//标识，0 地图编辑 1 在地图上部署
	int m_nMap{};//地图号
	int m_nCscene{};//场景号
	int m_Times{ 0 };
public:
	CMapList   m_List;//显示列表
	CStatic    m_Map;//地图显示构件
	CStatic    m_MiniMap;//小地图构件
	CScrollBar m_vScrollBar;//垂直滚动条
	CScrollBar m_hScrollBar;//水平滚动条
	CStatic    m_TextStatic;//文本区
	CRect      m_MapSelectRect{};//鼠标点击Tile范围
	CImage     m_MapImage{}; //地图

	DWORD	m_ListSelectRow{ 0 };//列表控件选择的行
	DWORD   m_MapSelectedPos{ 0 };//上次鼠标点击选择的tile
	DWORD   m_TileSelected{ 0 };//选择的Tele值
	DWORD   m_Flags{ 0 };//显示标识
	BOOL    m_NoDrawPoint{ 0 };//不显示上次点击
	DOUBLE  m_MapZoom;//大地图缩放比率

protected:
	RECT    m_MapRect;//地图显示区域
	SIZE    m_MapShowSize;//地图显示尺寸
	INT     lxOrigin{}, lyOrigin{};
	DOUBLE	lOriginZoom{1.0};//原点缩放比率
	CPoint  m_OldPt;//上次点击鼠标位置

	class	CGetPalData* Pal{};

private:
	class CPalMapEdit* pMapClass;//地图数据结构

protected:
	BOOL isMousePtIn(const CPoint pt, CWnd* wp);
	// 使用鼠标点，生成相关操作
	VOID MapSetPoint(CPoint& pt);
	//检测压缩文件是否使用的是YJ_1
	//不是，返回0 出错 - 1 是返回1
	INT isUseYJ_1(FILE* f);
	LPRGBQUAD SDL_ColorsToRGBQUADS(SDL_Color* colo)
	{
		static RGBQUAD wColor[256];
		for (int n = 0; n < 256; n++)
		{
			wColor[n].rgbRed = colo[n].r;
			wColor[n].rgbBlue = colo[n].b;
			wColor[n].rgbGreen = colo[n].g;
		}
		return wColor;
	}

};


