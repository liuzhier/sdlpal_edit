#ifndef PACKEDPICT_DLG_H
#define PACKEDPICT_DLG_H

#pragma once
#include "stdafx.h"
#include "afxdialogex.h"
#include "CsListCtrl.h"
#include <vector>
#include "CMapList.h"
#include "cMap_Dlg.h"

class PackedPict_Dlg : public CMap_Dlg
{
	DECLARE_DYNAMIC(PackedPict_Dlg)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

protected:
	FILE* f{};
	OPENFILENAME *OpenFileName;
	CWnd*	m_Parent{};
	//class	CGetPalData* Pal{};
	BOOL	OldCompressMode{};
	INT		m_ItemW{};//栏宽,,设置与栏高相等
	INT		m_ItemH{};//栏高
	INT		m_colCont{};//栏数
	INT		m_itemCont{};
public:
	//PackedPict_Dlg(CWnd* pParent, FILE* f, LPCWSTR dir);
	PackedPict_Dlg(CWnd* pParnnt, OPENFILENAME * openfilename);
	virtual ~PackedPict_Dlg();

	afx_msg void ObjectPopMenuRetuen(UINT msg);
	afx_msg LRESULT OnListSelectedRow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnInitDialog();

protected:
	BOOL TestSaverDir(CString & dir );
	BOOL makeChunkfromAll(std::vector<WORD>& chunk,std::vector<WORD> * mapChunk = NULL);
	virtual BOOL makeList();

};

#endif PBJECT_DLG_H

