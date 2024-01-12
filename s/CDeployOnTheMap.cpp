
#include "stdafx.h"
#include "CDeployOnTheMap.h"
#include "cMap_Dlg.h"
#include "CMapList.h"
#include "../main/cscript.h"
#include "sDlg.h"

static LPCTSTR pMenu_Str[] =
{
	L"显示障碍",
	L"显示全体对象",
	L"固定显示对象%3.3d",
	L"移动对象 %3.3d",
	//L"运行自动脚本",
	L"修改对象%3.3d触发范围",
	nullptr
};

enum tagMapFlags
{
	a_ShowBarrier = 1 << 0,//障碍标识
	a_AllObject = 1 << 1,//显示全体对象
	a_TheObject = 1 << 2,//固定显示对象%3.3d
	a_MoveObject = 1 << 3,//移动对象 % 3.3d
	//a_AutoScript = 1 << 4, //	运行自动脚本
	a_MakeTriggerMode = 1 << 4, //修改对象%3.3d触发范围
};

CDeployOnTheMap::CDeployOnTheMap(CMap_Dlg* para)
	:CPalMapEdit(para)
{
	auto g = Pal->gpGlobals;
	//iScene = g->wNumScene;
	iScene = pPara->m_nCscene;
	g->wNumScene = iScene;
	iMapNum = g->g.rgScene[iScene - 1].wMapNum;
	Pal->PAL_SetLoadFlags(kLoadScene | kLoadPlayerSprite);
	Pal->PAL_LoadResources();
	pPara->m_NoDrawPoint = TRUE;

	//同步数据
	GAMEDATA* sp = &((CsDlg*)AfxGetApp()->m_pMainWnd)->Pal->gpGlobals->g;
	GAMEDATA* dp = &Pal->gpGlobals->g;
	memcpy(dp->lprgEventObject, sp->lprgEventObject,
		dp->nEventObject * sizeof(EVENTOBJECT));

}

CDeployOnTheMap::~CDeployOnTheMap()
{

}

void CDeployOnTheMap::Initializes_Tilelist(CImageList& m_ImageList, class CMapList& m_List)
{
	m_ImageCount = Pal->gpGlobals->g.rgScene[iScene].wEventObjectIndex -
		Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex;
	
	m_List.DeleteAllItems();

	m_List.SetExtendedStyle(//m_ListCtrl.GetExtendedStyle() |
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER);

	m_List.ModifyStyle(0, LVS_SINGLESEL| LVS_REPORT | LVS_SHOWSELALWAYS);
	
	//生成列表
	m_List.InsertColumn(0, L"", 0, 0);
	m_List.InsertColumn(1, L"对象", 0, 60);
	m_List.InsertColumn(2, L"位置X", 0, 60);
	m_List.InsertColumn(3, L"位置Y", 0, 60);
	m_List.InsertColumn(4, L"形象", 0, 60);

	for (int n = 0; n < m_ImageCount; n++)
	{
		CString strText;
		WORD k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex + n;
		LPEVENTOBJECT p = &Pal->gpGlobals->g.lprgEventObject[k];
		m_List.InsertItem(n, L"");
		strText.Format(_T("%4.4d"), k); 
		m_List.SetItemText(n,1, strText);
		strText.Format(_T("%4d"), p->x);
		m_List.SetItemText(n, 2, strText);
		strText.Format(_T("%4d"), p->y);
		m_List.SetItemText(n, 3, strText);
		strText.Format(_T("%4d"), p->wSpriteNum);
		m_List.SetItemText(n, 4, strText);
	}
	//设置焦点
	m_List.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED); 
	m_List.SetSelectionMark(0);
}

LPCTSTR* CDeployOnTheMap::get_Menu_Str() const
{
	return pMenu_Str;
}

//处理弹出菜单返回值 //返回 0 取消 1 刷新 -1 正常 1
int CDeployOnTheMap::MapPopMenuRetuen(int msg, DWORD& m_Flags)
{
	m_Flags ^= 1 << msg;
	switch (1 << msg)
	{
	case a_AllObject:
		break;
	case a_TheObject:
		if (!(m_Flags & a_TheObject))
			m_Flags &= ~a_MoveObject;
		break;
	case a_MoveObject:
		if (m_Flags & a_MoveObject)
		{
			//m_Flags &= ~a_AllObject;
			m_Flags |= a_TheObject;
			m_Flags &= ~a_MakeTriggerMode;
		}
		break;
	case a_MakeTriggerMode:
		if (m_Flags & a_MakeTriggerMode)
		{
			m_Flags &= ~a_MoveObject;
			WORD k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex +
				pPara->m_ListSelectRow;
			LPEVENTOBJECT p = &Pal->gpGlobals->g.lprgEventObject[k];
			TriggerMode = p->wTriggerMode;
		}
		break;
	default:
		break;
	}
	return -1;
}

INT CDeployOnTheMap::ListRowToReturn(INT r)
{
	INT k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex +
		r;
	return k;
}

INT CDeployOnTheMap::MakeMapImage(CImage& mm, const DWORD flag)
{
	if (mm.IsNull())
		mm.Create(MapWidth, -MapHight, 24);//建立表
	CImage m;//中间表
	m.Create(MapWidth, -MapHight, 8);

	m.SetColorTable(0, 256, SDL_ColorsToRGBQUADS(Pal->gpPalette->colors));
	SDL_Surface mSuf{};
	//将MFC图转化为SDL表面指针
	mSuf.w = MapWidth;
	mSuf.h = MapHight;
	mSuf.pixels = m.GetBits();
	mSuf.pitch = m.GetPitch();
	SDL_Rect rect = { 0,0,MapWidth,MapHight };
	//画底层
	PAL_MapBlitToSurface(this, &mSuf, &rect, 0);
	//画顶层
	PAL_MapBlitToSurface(this, &mSuf, &rect, 1);
	//画障碍
	if (flag & a_ShowBarrier)
		DrawObstacles(&m, &rect);
	//画全部对象
	if (flag & (a_AllObject))
	{
		//
		// Event Objects (Monsters/NPCs/others)
		//
		int i, x, y, vy;
		auto gpGlobals = Pal->gpGlobals;
		for (i = gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex;
			i < gpGlobals->g.rgScene[iScene].wEventObjectIndex; i++)
		{
			LPCBITMAPRLE     lpFrame{ nullptr };
			LPCSPRITE        lpSprite{ nullptr };

			LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

			int              iFrame{ 0 };

			//画对象所在位置，用黄笔
			BoundingLiteBox(&m, 32, 16, lpEvtObj->x - 16, lpEvtObj->y - 8, RGB(240, 240, 0));

			//
			// Get the sprite
			//
			lpSprite = Pal->PAL_GetEventObjectSprite((WORD)i + 1);
			if (lpSprite == NULL)
			{
				continue;
			}

			iFrame = lpEvtObj->wCurrentFrameNum;
			if (lpEvtObj->nSpriteFrames == 3)
			{
				//
				// walking character
				//
				if (iFrame == 2)
				{
					iFrame = 0;
				}

				if (iFrame == 3)
				{
					iFrame = 2;
				}
			}

			lpFrame = PAL_SpriteGetFrame(lpSprite,
				lpEvtObj->wDirection * lpEvtObj->nSpriteFrames + iFrame);

			if (lpFrame == NULL)
			{
				continue;
			}

			//
			// Calculate the coordinate and check if outside the screen
			//
			x = (SHORT)lpEvtObj->x;
			x -= Pal->PAL_RLEGetWidth(lpFrame) / 2;

			y = (SHORT)lpEvtObj->y;
			y += lpEvtObj->sLayer * 8 + 9;

			vy = y - Pal->PAL_RLEGetHeight(lpFrame) - lpEvtObj->sLayer * 8 + 2;

			Pal->PAL_RLEBlitToSurface(lpFrame, &mSuf, PAL_XY(x, vy));

		}

	}
	//固定显示对象
	if (flag & (a_TheObject | a_MoveObject))
	{
		WORD k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex +
			pPara->m_ListSelectRow;
		LPEVENTOBJECT p = &Pal->gpGlobals->g.lprgEventObject[k];
		fixedpoint.x = p->x ;
		fixedpoint.y = p->y ;
		TriggerMode = p->wTriggerMode;
	}
	else
	{
		TriggerMode = fixedpoint.x = fixedpoint.y = 0;
	}

	HDC ddc = mm.GetDC();
	m.Draw(ddc, 0, 0, MapWidth, MapHight);
	mm.ReleaseDC();
	return 0;
}

VOID CDeployOnTheMap::UpdateTile(DWORD flg)
{
	if (pPara->m_Flags & a_MoveObject)
	{
		//移动对象
		WORD k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex
			+ pPara->m_ListSelectRow;
		RECT mRect = pPara->m_MapSelectRect;
		DWORD pos = PAL_XY(mRect.left + 16, mRect.top + 8);
		LPEVENTOBJECT p = &Pal->gpGlobals->g.lprgEventObject[k];
		if (pos == PAL_XY(p->x, p->y))
			return;
		SceneUndo s{ 0 };
		s.tOld = s.tNew = *p;
		s.tPos = k;
		s.tNew.x = PAL_X(pos);
		s.tNew.y = PAL_Y(pos);
		UndoArray.push_back(s);

		p->x = PAL_X(pos);
		p->y = PAL_Y(pos);
		fixedpoint.x = p->x;
		fixedpoint.y = p->y;
		Initializes_Tilelist(pPara->m_ImageList, pPara->m_List);
	}
	else if (pPara->m_Flags & a_MakeTriggerMode)
	{
		//修改触发方式
		WORD k = Pal->gpGlobals->g.rgScene[iScene - 1].wEventObjectIndex
			+ pPara->m_ListSelectRow;
		RECT mRect = pPara->m_MapSelectRect;
		DWORD pos = PAL_XY(mRect.left + 16, mRect.top + 8);
		LPEVENTOBJECT p = &Pal->gpGlobals->g.lprgEventObject[k];
		if (pos != PAL_XY(p->x, p->y))
			return;//未点击在目标上
		CPoint point{ 0 };
		GetCursorPos(&point);
		//生成菜单
		CMenu pMenu;
		pMenu.CreatePopupMenu();
		for (int n = 0; n < 8; n++)
		{
			UINT32 flag = MF_STRING | ((n == p->wTriggerMode)
				? MF_CHECKED : MF_UNCHECKED);
			CString s;
			s.Format(n < 4 ? L"被动触发 %d" : L"主动触发 %d", n);
			pMenu.AppendMenu(flag, n + 1, s);
		}
		SetForegroundWindow(pPara->GetSafeHwnd());
		int ret = pMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, point.x, point.y, pPara);
		if (ret == 0 || ret - 1 == p->wTriggerMode)
			return;

		SceneUndo s{ 0 };
		s.tOld = s.tNew = *p;
		s.tPos = k;
		s.tNew.wTriggerMode = TriggerMode = p->wTriggerMode = ret -1;
		UndoArray.push_back(s);
	}
	else
		return;

	//重新生成大地图
	MakeMapImage(pPara->m_MapImage, flg);
	return VOID();
}

VOID CDeployOnTheMap::get_Msg_Str(CString& inStr, DWORD MapSelectedTPos, 
	DWORD TileSelected, DWORD row, DWORD m_Flags)
{
	CString s1;
	inStr.Format(L"显示障碍 %s   ",
		TileSelected & 0x2000 ? L"√" : L"×");
	for (size_t n = 0; n < 2; n++)
	{
		s1.Format(L" %s%s ", get_Menu_Str()[n], m_Flags & 1 << n ? L"√" : L"×");
		inStr += s1;
	}

	s1.Format(get_Menu_Str()[2], ListRowToReturn(row));
	inStr += s1;

	if (UndoArray.size())
	{
		//历史不为空，显示历史数据
		s1.Format(L"已经修改 %d 处，Ctr+Z 撤销， ", UndoArray.size());
		inStr += s1;
	}
	for (size_t n = 3; get_Menu_Str()[n]; n++)
	{
		if (m_Flags & 1 << n)
		{
			inStr += L"  编辑(双击)：";
			s1.Format(get_Menu_Str()[n], ListRowToReturn(row));
			inStr += s1;
		}
	}
	return;
}

INT CDeployOnTheMap::SaveMapTiles()
{
	//存储数据，将临时结构数据转为系统数据
	GAMEDATA* dp = &((CsDlg*)AfxGetApp()->m_pMainWnd)->Pal->gpGlobals->g;
	GAMEDATA* sp = &Pal->gpGlobals->g;

	memcpy(dp->lprgEventObject, sp->lprgEventObject, 
		dp->nEventObject * sizeof(EVENTOBJECT));
	return 0;
}

DWORD CDeployOnTheMap::GetUndoSize()
{
	return UndoArray.size();
}

INT CDeployOnTheMap::doUndo()
{
	if (UndoArray.size() == 0)
		return 0;
	int k = UndoArray[UndoArray.size() - 1].tPos;
	Pal->gpGlobals->g.lprgEventObject[k] =
		UndoArray[UndoArray.size() - 1].tOld;
	UndoArray.pop_back();
	return 1;
}
