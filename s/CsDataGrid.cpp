
//#define _AFXDLL

#include "stdafx.h"
//#include "GridCtrl_src/GridCtrl.h"
#include "CsDataGrid.h"
#include "resource.h"
#include <string>
#include "CGetLing.h"
#include "sDlg.h"
#include "cMap_Dlg.h"
#include "CListScriptEntry.h"

#include <map>
#include <iterator>

char* va(const char* format, ...)
/*++
  Purpose:

	Does a varargs printf into a temp buffer, so we don't need to have
	varargs versions of all text functions.

  Parameters:

	format - the format string.

  Return value:

	Pointer to the result string.

--*/
{
	static char string[256];
	va_list     argptr;

	va_start(argptr, format);
	vsnprintf(string, 256, format, argptr);
	va_end(argptr);

	return string;
}

//BEGIN_MESSAGE_MAP(CsDataGrid, CWnd)
//END_MESSAGE_MAP()



CsDataGrid::CsDataGrid()
	:m_pList(nullptr)
	,m_popMenuFlags(0)
	,isLineTop(1)
	,UndoCount(0)
	,Set_Col_Data(nullptr)
{
	//m_List.Create();

}


CsDataGrid::~CsDataGrid()
{
	
}

void tagColData::GetData(LPSTR t_ColTop, WORD Width, WORD t_col, col_CTRL t_Ctrl,
	col_TYPE  t_DataStat, pSelect_Item t_CtrlData, SrcToStr t_SrcToStr, StrToSrc t_StrToSrc, INT isScript)
{
	ColTop = t_ColTop;
	ColWidth = Width; Col = t_col; Ctrl = t_Ctrl; DataStat = t_DataStat;
	p_CtrlList = t_CtrlData, p_InCol = t_SrcToStr; p_OutCol = t_StrToSrc;
	m_isScript = isScript;
}

tagColData::~tagColData()
{

}
//LPTSTR StringToWString(LPCSTR s);

// 设置表头，Col 行数，p_ColArray 表头结构数组
void CsDataGrid::SetColClass(int col, ColArray& p_ColArray,BOOL IsLineTop)
{
	// TODO: 在此处添加实现代码.
	isLineTop = IsLineTop;
	m_ColArray.resize(0);
	m_ColArray = p_ColArray;

	int n;
	SetColumnCount(col);
	//最少二行，第一行固定
	if (GetRowCount() < 2)
		SetRowCount(2);
	SetFixedRowCount(1);

	//设置固定竖表头
	for (n = 0; n < col && m_ColArray[n].Ctrl == ctrl_Fix; n++);
	if (n < col)
	{
		SetFixedColumnCount(n);
	}
	for (n = 0; n < col; n++)
	{
		CString s = (m_ColArray[n].ColTop);
		SetItemText(0, n, s);
		SetColumnWidth(n, m_ColArray[n].ColWidth);
	}
}

// 设置表数据，row 行数 Col 列数 ROWData 表数据数组
void CsDataGrid::SetDataClass(int row, int col, DataArray& RowData)
{
	// TODO: 在此处添加实现代码.
	m_DataArray.clear();
	//复制拷贝
	m_DataArray = RowData;
	ASSERT(m_DataArray.size() >= row);
	ShowWindow(TRUE);
	//清空表格,不用清空只要用新表填充即可
	//增加虚拟表格处理，加快处理速度
	SetRowCount(row + 1);//设置表行
	SetFixedRowCount(1);
	//设置可见部分表数据
	SetVisibleCellRangesData();
}

DataArray& CsDataGrid::GetDataClass()
{
	//处理数据
	int row;
	for (row = GetFixedRowCount(); row < GetRowCount(); row++)
	{
		if (m_DataArray.size() && m_DataArray[row - 1].RowMark)
		{
			Get_DataRow(row );
		}
	}

	//返回结构数组
	return m_DataArray;
}

//IMPLEMENT_DYNAMIC(CsDataGrid, CGridCtrl)

BEGIN_MESSAGE_MAP(CsDataGrid, CGridCtrl)
	ON_MESSAGE(WM_EditFormat, &CsDataGrid::Edit_Format)
	ON_WM_VSCROLL()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(WM_GRID_POPMENU, &CsDataGrid::OnMenuToTop)
	ON_COMMAND(WM_GRID_POPMENU + 1, &CsDataGrid::OnMenuToRow)
	ON_COMMAND(WM_GRID_POPMENU + 2, &CsDataGrid::OnMenuToBottom)
	ON_COMMAND_RANGE(WM_GRID_POPMENU + 3, WM_GRID_POPMENU + 19,&CsDataGrid::OnMenuRange)
	ON_MESSAGE(WM_GRID_CELL_CHARGE, &CsDataGrid::OnGridCellCharge)
	ON_MESSAGE(WM_GRID_ROW_CHARGE, &CsDataGrid::OnGridRowCharge)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

//处理编辑控件的键盘输入信息，返回0 接受输入什么都不做，1 不接受输入回退
LRESULT CsDataGrid::Edit_Format(WPARAM w_wparam, LPARAM w_lparam)
{
	GV_DISPINFO* pgvDispInfo = (GV_DISPINFO*)w_lparam;
	GV_ITEM* pgvItem = &pgvDispInfo->item;
	if (w_wparam == 8)
		return 0;
	switch (m_ColArray[pgvItem->col].DataStat)
	{
	case tWORD:
		if (!(w_wparam >= '0' && w_wparam <= '9'))
			return 1;
		break;
	case tINT:
	case tSHORT:
		if (!(w_wparam >= '0' && w_wparam <= '9' || w_wparam == '-'))
			return 1;
		break;
	case tHEX:
		if (!strchr("0123456789abcdefABCDEF", (INT)w_wparam))
			return 1;
		break;
	case tBOOL:
		if (!strchr("YyNn", (INT)w_wparam))
			return 1;
		break;
	case tFLOAT:
		if (!strchr("1234567890-.", (INT)w_wparam))
			return 1;
		break;
	case tSTR:
	default:
		break;
	};
	return 0;
}

void CsDataGrid::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CGridCtrl::OnVScroll(nSBCode, nPos, pScrollBar);

	//在此使用虚拟报表技术
	SetVisibleCellRangesData();

}

//设置可见部分行的表数据
void CsDataGrid::SetVisibleCellRangesData()
{
	CCellRange  rc = GetVisibleFixedCellRange();
	BOOL isChange = FALSE;
	for (int n_Row = rc.GetMinRow(); n_Row <= rc.GetMaxRow(); n_Row++)
	{
		if (m_DataArray[n_Row - GetFixedRowCount()].RowMark == 0)
		{
			Set_DataRow(n_Row);
			m_DataArray[n_Row - GetFixedRowCount()].RowMark = 1;
			isChange = TRUE;
		}
	}
	if (isChange)
	{
		//有数据更新，刷新场景
		CRect rect;
		GetClientRect(rect);
		InvalidateRect(rect);
		ShowWindow(TRUE);
		isChange = FALSE;
	}
	//m_pList->ShowWindow(FALSE);
	m_pList->PostMessage(WM_KILLFOCUS, -1, 0);

}

//读取单行表数据到数组结构
int CsDataGrid::Get_DataRow(long n_Row)
{
	
	if (n_Row < 0 || n_Row >= GetRowCount())
		ASSERT(FALSE);
	for (int n_col = 0; n_col < GetColumnCount(); n_col++)
	{
		CString n_Text = GetItemText(n_Row, n_col);

		std::string s_Text = CStringA(n_Text).GetString();
		COLVAR  u_col;
		//USES_CONVERSION;
		if (m_ColArray[n_col].p_OutCol)
			u_col = (m_ColArray[n_col].p_OutCol)(s_Text.c_str());
		else if (m_ColArray[n_col].Ctrl > 1)
		{
			//u_col.s = s_text;

			switch (m_ColArray[n_col].DataStat)
			{
			case tDWORD:
			{
				int i;
				sscanf_s(s_Text.c_str(), "%lu", &i);
				u_col = i;
				break;
			}
			case tWORD:
			{
				int i;
				sscanf_s(s_Text.c_str(), "%u", &i);
				u_col = i;
				break;
			}
			case tINT:
			case tSHORT:
				//case tNull:
			{
				int i;
				sscanf_s(s_Text.c_str(), "%d", &i);
				u_col = i;
				break;
			}
			case tHEX:
			{
				int i;
				sscanf_s(s_Text.c_str(), "%x", &i);
				u_col = i;
				break;
			}
			case tFLOAT:
			{
				double i;
				sscanf_s(s_Text.c_str(), "%lf", &i);
				u_col = i;
				break;
			}
			case tSTR:
			{
				string s = s_Text + "";
				u_col = s;
				break;
			}
			case tBOOL:
				u_col = n_Text == L"Y" || n_Text == L"y";
				break;
			default:
				continue;
			}
			m_DataArray[n_Row - 1].Col[n_col] = u_col;
		}
	}
	return 0;
}

// 设置表格单行数据
int CsDataGrid::Set_DataRow(long n_Row)
{
	// TODO: 在此处添加实现代码.
	if (n_Row < 0 || n_Row >= GetRowCount())
		ASSERT(FALSE);

	CString s_Text;
	//数据比实际行数少表头行数
	n_Row -= GetFixedRowCount();
	//
	if (Set_Col_Data)
		Set_Col_Data(m_DataArray[n_Row]);

	for (int n_Col = 0; n_Col < GetColumnCount(); n_Col++)
	{
		if (m_ColArray[n_Col].p_InCol)
			s_Text = (m_ColArray[n_Col].p_InCol)(get<int>(m_DataArray[n_Row].Col[n_Col]));

		else
		{

			switch (m_ColArray[n_Col].DataStat)
			{
			case tINT:
			case tNull:
				s_Text.Format(_T("%d"), get<int>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tWORD:
				s_Text.Format(_T("%u"), (WORD)get<int>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tDWORD:
				s_Text.Format(_T("%lu"), (DWORD)get<int>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tSHORT:
				s_Text.Format(_T("%d"), (SHORT)get<int>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tHEX:
				s_Text.Format(_T("%4.4X"), (WORD)get<int>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tFLOAT:
				s_Text.Format(_T("%f"), get<double>(m_DataArray[n_Row].Col[n_Col]));
				break;
			case tSTR:
				//处理字符串
				s_Text = CString(get<string>(m_DataArray[n_Row].Col[n_Col]).c_str());
				break;
			case tBOOL:
				//布尔数
				s_Text = get<int>(m_DataArray[n_Row].Col[n_Col]) == 0 ? "N" : "Y";
				break;
			default:
				break;
			};
		}
		SetItemText(n_Row + GetFixedRowCount(), n_Col, s_Text);
		SetItemState(n_Row + 1, n_Col, GetItemState(n_Row + 1, n_Col) & ~GVIS_READONLY);
		//设定格子属性
		switch (m_ColArray[n_Col].Ctrl)
		{
		case ctrl_Null:
		case ctrl_Fix:
		case ctrl_List:
		case ctrl_MapEdit:
			//只读
			SetItemState(n_Row + 1, n_Col, GetItemState(n_Row + 1, n_Col) | GVIS_READONLY);
			break;
		case ctrl_Edit:
		case ctrl_Radio:
			SetCellType(n_Row + 1, n_Col, RUNTIME_CLASS(CGridCell));
			break;
		case ctrl_Combo:
			break;
		//case ctrl_InScript:
			break;
		default:
			break;

		}
	}
	return 0;
}


LPCTSTR pDataGridMenuStr[] =
{
	L"到顶",
	L"跳转到行",
	L"到底",
	L"行前插入",
	L"尾部添加",
	L"删除行",
	L"撤消 Ctrl+Z",
	L"运行",//7
	L"使用存档运行缺省",//8
	L"在地图上部署",//9
	L"查看脚本",//10
	L"查看调用者"//11
};

void CsDataGrid::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	m_pList->PostMessage(WM_KILLFOCUS, -1, 0);
	//取单元格位置
	CCellID cell = GetCellFromPt(point, FALSE);
	//
	m_ClickCell = cell;
	if (cell.col < 0 || cell.row < 0)
		return CGridCtrl::OnRButtonDown(nFlags, point);//指向表头，什么都不做
	//生成弹幕
	CMenu m;
	m.CreatePopupMenu();
	ClientToScreen(&point);
	for (int n = 0; n < sizeof(pDataGridMenuStr) / sizeof(LPCTSTR); n++)
	{

		UINT32 flag{};
		switch (n)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			flag = MF_STRING |
				((m_popMenuFlags & (1 << n)) || (mUndoArray.size() && n == 6)
					? MF_BYPOSITION : MF_DISABLED | MF_BYPOSITION);
			m.AppendMenu(flag, WM_GRID_POPMENU + n, pDataGridMenuStr[n]);
			break;
		case 7:
		case 8:
		case 9:
			//7运行，如不可用，跳过
			if (!(m_popMenuFlags & (1 << n)))
				continue;
			m.AppendMenu(flag, WM_GRID_POPMENU + n, pDataGridMenuStr[n]);
			break;
		case 10:
			if (m_ColArray[cell.col].m_isScript == 1 &&
				get<int>(m_DataArray[cell.row - 1].Col[cell.col]) != 0)
			{
				//显示脚本调用菜单
				m.AppendMenu(MF_BYPOSITION, WM_GRID_POPMENU + n, pDataGridMenuStr[n]);
			}
			else
				continue;
			break;
		case 11:
			if (m_ColArray[cell.col].m_isScript == 2 &&
				get<int>(m_DataArray[cell.row - 1].Col[cell.col]) != 0)
				{
				//显示调用次数
				m.AppendMenu(MF_BYPOSITION, WM_GRID_POPMENU + n, pDataGridMenuStr[n]);

			}
			break;
		default:
			break;
		}
	}

	SetForegroundWindow();
	m.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this);

	CGridCtrl::OnRButtonDown(nFlags, point);
}


void CsDataGrid::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CGridCtrl::OnLButtonDown(nFlags, point);

	//取点击单元格
	CCellID cell = GetCellFromPt(point, FALSE);
	m_ClickCell = cell;
	CWnd* s_parent = GetParent();
	RECT s_rect;
	GetCellRect(cell,&s_rect);
	ClientToScreen(&s_rect);

	m_pList->SetExtendedStyle(//m_ListCtrl.GetExtendedStyle() |
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER
	);// LVS_EX_HEADERDRAGDROP || LVS_LIST| LVS_SINGLESEL);
	m_pList->ShowWindow(FALSE);
	m_pList->SetOwner(this);
	if (cell.col > 0 && cell.row > 0 && m_ColArray[cell.col].Ctrl == ctrl_List)
		//如果单元是下拉表，则弹出下拉表
	{
		//
		m_pList->DeleteAllItems();
		while (m_pList->DeleteColumn(0));
		m_pList->InsertColumn(0, L"", 0, 0);
		m_pList->InsertColumn(1, L"行", 0, 50);
		m_pList->InsertColumn(2, L"Id", 0, 60);
		m_pList->InsertColumn(3, L"名称", 0, 120);
		auto p_array = m_ColArray[cell.col].p_CtrlList;
		for (int n = 0; n < p_array->data.size(); n++)
		{
			CString s;
			m_pList->InsertItem(n, L"");
			s.Format(L"%3.3d", n);
			m_pList->SetItemText(n,1, s);
			s.Format(L"%4.4X", p_array->data[n].item);
			m_pList->SetItemText(n, 2, s);
			s = (LPSTR)p_array->data[n].s.c_str();
			m_pList->SetItemText(n, 3, s);
		}

		int item = get<int>(m_DataArray[cell.row - 1].Col[cell.col]);
		for (int n = 0; n < p_array->data.size(); n++)
		{
			if (p_array->data[n].item == item)
			{
				//下拉表选择并显示
				m_pList->SetItemState(n, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);
				m_pList->EnsureVisible(n, FALSE);
				break;
			}
		}

		RECT d_rect = { s_rect.left, s_rect.bottom,s_rect.left + 260,s_rect.bottom + 310 };
		m_pList->SetParent(0);
		m_pList->MoveWindow(&d_rect);

		m_pList->ShowWindow(TRUE);
		m_pList->SetFocus();

		//刷新
		CRect rect;
		GetClientRect(rect);
		InvalidateRect(rect);
		return;
	}
	else 
		m_pList->PostMessage(WM_KILLFOCUS, -1, 0);

	if (cell.col > 0 && cell.row > 0 && m_ColArray[cell.col].Ctrl == ctrl_MapEdit)
	{
		//地图编辑
		::PostMessage(GetParent()->m_hWnd, WM_MapEdit_Map, 0, m_ClickCell.row - 1);
		PostMessage(WM_KILLFOCUS, -1, 0);
	}
}



void CsDataGrid::OnMenuToTop()
{
	// TODO: 在此添加命令处理程序代码
	SendMessage(WM_VSCROLL, SB_TOP, 0);
	//使行被选择，加亮
	SetSelectedRange(1, GetFixedColumnCount(),
		1, GetColumnCount() - 1);

	CString t_msg;
	t_msg.Format(L"跳转到顶");
	GetParent()->SendMessage(WM_SEND_MSG_STR, 0, (LPARAM)(t_msg.GetBuffer()));
	
}


void CsDataGrid::OnMenuToRow()
{
	// TODO: 在此添加命令处理程序代码
	
	//选择行对话框;
	CGetLing toLing;
	if(toLing.DoModal() != IDOK)
		return;
	int row  = toLing.row;
	row = min(row, GetRowCount() - 1);
	if (row < 1 )
		return;
	CString msg;
	msg.Format(L"跳转到 %d 行！", row);

	GetParent()->SendMessage(WM_SEND_MSG_STR, 0, (LPARAM)(msg.GetBuffer()));

	CCellID idCell = GetTopleftNonFixedCell();
	EnsureVisible(row,idCell.col);

	//使行被选择，加亮
	SetSelectedRange(row, GetFixedColumnCount(),
		row, GetColumnCount() - 1);
}


void CsDataGrid::OnMenuToBottom()
{
	// TODO: 在此添加命令处理程序代码
	SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
	//使行被选择，加亮
	SetSelectedRange(GetRowCount() - 1, GetFixedColumnCount(),
		GetRowCount() - 1, GetColumnCount() - 1);
	CString t_msg;
	t_msg.Format(L"跳转到底");

	GetParent()->SendMessage(WM_SEND_MSG_STR, 0, (LPARAM)(t_msg.GetBuffer()));

}


void CsDataGrid::OnMenuRange(UINT msg)
{
	// TODO: 在此添加命令处理程序代码
	UnDoRowData undo;
	ColumnClass line;
	line = m_DataArray[m_ClickCell.row - GetFixedRowCount()];
	undo.OldRowData = m_DataArray[m_ClickCell.row - GetFixedRowCount()];

	string  s;
	msg -= WM_GRID_POPMENU + 3;
	switch (msg)
	{
	case 0:
	{
		s = "在此行前插入";
		line.oldRow = 0;
		line.RowMark = 0;
		line.Col[0] = m_ClickCell.row - GetFixedRowCount() ;
		m_DataArray.insert(m_DataArray.begin() + m_ClickCell.row - GetFixedRowCount(),	line);
		undo.newRow = m_ClickCell.row;
		undo.oldRow = 0;
		for (int n = m_ClickCell.row - GetFixedRowCount() ; n < m_DataArray.size(); n++)
		{
			m_DataArray[n].Col[0] = n;
			m_DataArray[n].RowMark = 0;
		}
		break;
	}
	case 1:
	{
		s = "尾部添加";
		line.oldRow = 0;
		line.RowMark = 0;
		line.Col[0] = (int)m_DataArray.size();
		m_DataArray.push_back(line);
		undo.OldRowData = line;
		undo.newRow = m_DataArray.size() - 1+ GetFixedRowCount();
		undo.oldRow = 0;
		break;
	}
	case 2:
	{
		s = "删除";
		undo.newRow = 0;
		undo.oldRow = m_ClickCell.row;
		m_DataArray.erase(m_DataArray.begin() + m_ClickCell.row - GetFixedRowCount(),
			m_DataArray.begin() + m_ClickCell.row - GetFixedRowCount() + 1);
		for (int n = m_ClickCell.row - GetFixedRowCount(); n < m_DataArray.size(); n++)
		{
			m_DataArray[n].Col[0] = get<int>(m_DataArray[n].Col[0]) - 1;
			m_DataArray[n].RowMark = 0;
		}
		break;
	}
	case 3:
		//撤消
		Undo();
		break;
	case 4://
	case 5://测试运行
	{
		s = "测试运行";
		::PostMessage(GetParent()->m_hWnd, WM_Test_Run, msg - 3, m_ClickCell.row - 1);
		return;
		break;
	}
	case 6:
	{
		//在地图上部署
		s = "在地图上部署";
		::PostMessage(GetParent()->m_hWnd, WM_Deploy_On_The_Map, 1, m_ClickCell.row - 1);
		break;
	}
	case 7:
	{
		//查看脚本 View script  _ckjb
		int View_script = get<int>(m_DataArray[m_ClickCell.row - 1].Col[m_ClickCell.col]);
		s = va("查看脚本 %4.4X", View_script);
		::PostMessage(GetParent()->m_hWnd, WM_ListScriptEntry, View_script, 0);

		break;
	}
	case 8:
	{
		//查看调用者 Script_caller
		int scriptcaller = m_DataArray[m_ClickCell.row - 1].oldRow - 1;
		s = va("查看原脚本 %4.4X 的调用者", scriptcaller);
		::PostMessage(GetParent()->m_hWnd, WM_ScriptCaller, scriptcaller, 0);
		break;
	}
	default:
		return;
	}

	if (msg < 3)
	{
		mUndoArray.push_back(undo);
		UndoCount++;
	}
	//发送信息
	if (s.size()) ShowMsg(s.c_str());
	SetRowCount(m_DataArray.size() + 1);
	//刷新
	SetVisibleCellRangesData();
	SetFocus();
}


afx_msg LRESULT CsDataGrid::OnGridCellCharge(WPARAM wParam, LPARAM lParam)
{
	int row = m_ClickCell.row;
	if (get<int>( m_DataArray[row - GetFixedRowCount()].Col[m_ClickCell.col]) ==  wParam)
		return 0;

	//发送行变动信息
	SendMessage(WM_GRID_ROW_CHARGE, m_ClickCell.row, m_ClickCell.row);
	//选择的单元值 改变 整数值为wParam
	//数据行比表行少了固定行
	m_DataArray[row - GetFixedRowCount()].Col[m_ClickCell.col] = (int)wParam;
	CString s = GetItemText(m_ClickCell.row, m_ClickCell.col);
	Set_DataRow(row);
	//m_DataArray[row - GetFixedRowCount()].RowMark = 0;
	//刷新
	SetVisibleCellRangesData();
	return 0;
}

void CsDataGrid::OnEndEditCell(int nRow, int nCol, CString str)
{
	if (nRow > m_DataArray.size())
		return;//此调用不正常原因不明
	CString strCurrentText = GetItemText(nRow, nCol);
	if (strCurrentText != str)
	{
		//已经修改，将行加入撤消数组
		SendMessage(WM_GRID_ROW_CHARGE, nRow, nRow);
	}
		
	CGridCtrl::OnEndEditCell(nRow, nCol, str);
	if (strCurrentText != str)
	{
		//更新屏幕
		Get_DataRow(nRow);
		m_DataArray[nRow - GetFixedRowCount()].RowMark = 0;
		SetVisibleCellRangesData();
	}
}


afx_msg LRESULT CsDataGrid::OnGridRowCharge(WPARAM wParam, LPARAM lParam)
{
	//已经修改，将行加入撤消数组
	//第一个参数旧行，第二个参数 新行
	//if (m_DataArray.size() < wParam)
		//return 0;
	int nRow = wParam;
	UnDoRowData Undo;
	Undo.OldRowData = m_DataArray[nRow - GetFixedRowCount()];
	Undo.oldRow = nRow;
	Undo.newRow = lParam;
	mUndoArray.push_back(Undo);
	UndoCount++;
	ShowMsg("保存第 %d 次修改信息，修改第 %d 行,", mUndoArray.size(), nRow);
	return 0;
}


void CsDataGrid::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (IsCTRLpressed() && (nChar == 'Z' || nChar == 'U'))
	{
		Undo();
		return;
	}
	CGridCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


// 撤消修改
VOID CsDataGrid::Undo()
{
	// TODO: 在此处添加实现代码.
	if (mUndoArray.size())
	{
		ASSERT(UndoCount == mUndoArray.size());
		UnDoRowData* undo = &mUndoArray[mUndoArray.size() - 1];
		if (undo->newRow == undo->oldRow)
		{
			m_DataArray[undo->newRow - GetFixedRowCount()] = undo->OldRowData;
			m_DataArray[undo->newRow - GetFixedRowCount()].RowMark = 0;
		}
		else if (undo->newRow)
		{
			//原插入现执行删除
			m_DataArray.erase(m_DataArray.begin() + undo->newRow - GetFixedRowCount(),
				m_DataArray.begin() + undo->newRow - GetFixedRowCount() + 1);
			for (int n = undo->newRow - GetFixedRowCount(); n < m_DataArray.size(); n++)
			{
				m_DataArray[n].Col[0] = get<int>(m_DataArray[n].Col[0]) - 1;
				m_DataArray[n].RowMark = 0;
			}
		}
		else
		{
			//原删除现执行插入
			if (undo->oldRow - GetFixedRowCount() >= m_DataArray.size())
				m_DataArray.push_back(undo->OldRowData);
			else
				m_DataArray.insert(m_DataArray.begin() + undo->oldRow - GetFixedRowCount(), undo->OldRowData);
			m_DataArray[undo->oldRow - GetFixedRowCount()].RowMark = 0;
			for (int n = undo->oldRow - GetFixedRowCount() + 1; n < m_DataArray.size(); n++)
			{
				m_DataArray[n].Col[0]=get<int>(m_DataArray[n].Col[0])+1;
				m_DataArray[n].RowMark = 0;
			}
		}

		mUndoArray.pop_back();
		UndoCount--;
		ShowMsg("撤消..., 还有 %d 处修改", UndoCount);
		SetRowCount(m_DataArray.size() + 1);
		//刷新
		SetVisibleCellRangesData();
	}
}

void CsDataGrid::EndEditing()
{
	//结束正在编辑的格子
	CGridCtrl::EndEditing();
}

