#include "CListScriptEntry.h"
#include "resource.h"
#include "sDlg.h"
#include "CsListCtrl.h"
#include <string>

IMPLEMENT_DYNAMIC(CListScriptEntry, CDialogEx)

BEGIN_MESSAGE_MAP(CListScriptEntry, CDialogEx)
	ON_WM_PAINT()
END_MESSAGE_MAP()


CListScriptEntry::CListScriptEntry(CWnd* pParent, WORD scriptEntry)
	:CDialogEx(IDD_DIALOG2, pParent), InScript(scriptEntry)
{
}

void CListScriptEntry::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
}


LPCSTR p_Script(int i);

BOOL CListScriptEntry::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//窗口标题
	CString s;
	s.Format(L"查看 %4.4X 脚本调用", InScript);
	SetWindowText(s);

	RECT s_Rect;
	GetParent()->GetWindowRect(&s_Rect);
	s_Rect.top += 40; s_Rect.bottom -= 40; s_Rect.right -= 40; s_Rect.left += 40;
	MoveWindow(&s_Rect, TRUE);
	int h = s_Rect.bottom - s_Rect.top;
	int w = s_Rect.right - s_Rect.left;
	s_Rect.top = 10; s_Rect.bottom = h - 60; s_Rect.right = w - 30; s_Rect.left = 10;
	m_List.MoveWindow(&s_Rect,TRUE);

	auto Pal = ((CsDlg*)AfxGetApp()->m_pMainWnd)->Pal;

	m_List.SetExtendedStyle(
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER);
	m_List.ModifyStyle(0, LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS);

	m_List.InsertColumn(0, L"", 0, 0);
	m_List.InsertColumn(1, L"脚本ID", 0, 90);
	m_List.InsertColumn(2, L"入口 ", 0, 90);
	m_List.InsertColumn(3, L"参数1", 0, 90);
	m_List.InsertColumn(4, L"参数2", 0, 90);
	m_List.InsertColumn(5, L"参数3", 0, 90);
	m_List.InsertColumn(6, L"说  明", 0, 2000);
	int line{ 0 };
	int sn = InScript;
	do
	{
		MAPScript mMapScrict;
		MAPScript::iterator iter;
		Pal->MarkSprictJumpsAddress(InScript, mMapScrict);
		for (iter = mMapScrict.begin(); iter != mMapScrict.end(); iter++)
		{
			CString cS;
			sn = iter->first;
			if (sn == 0)
				continue;
			LPSCRIPTENTRY p = &Pal->gpGlobals->g.lprgScriptEntry[sn];
			cS.Format(L"");
			m_List.InsertItem(line, cS);
			cS.Format(L"%4.4X", sn);
			m_List.SetItemText(line, 1, cS);
			cS.Format(L"%4.4X", p->wOperation);
			m_List.SetItemText(line, 2, cS);
			cS.Format(L"%4.4X", p->rgwOperand[0]);
			m_List.SetItemText(line, 3, cS);
			cS.Format(L"%4.4X", p->rgwOperand[1]);
			m_List.SetItemText(line, 4, cS);
			cS.Format(L"%4.4X", p->rgwOperand[2]);
			m_List.SetItemText(line, 5, cS);
			cS.Format(L"%ls", CString(p_Script((INT)p->wOperation)).GetString());
			m_List.SetItemText(line, 6, cS);
			line++;
			if (p->wOperation == 0xffff)
			{
				cS.Format(L"");
				m_List.InsertItem(line, cS);
				cS.Format(L"%s", CString(Pal->PAL_GetMsg(p->rgwOperand[0])).GetString());
				m_List.SetItemText(line, 6, cS);
				line++;
			}
		}
	} while (false);
	m_List.SetFocus();
	return TRUE;
}

void CListScriptEntry::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_List.GetSafeHwnd() == nullptr)
		return;
	m_List.MoveWindow(4, 4, cx -4, cy - 4);
	
}


void CListScriptEntry::OnPaint()
{
	if (!m_List.GetSafeHwnd() )
		return;
	m_List.ShowWindow(TRUE);
	CDialogEx::OnPaint();
}
