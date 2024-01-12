#include "cScriptCaller.h"

#include "resource.h"
#include "sDlg.h"
#include "CsListCtrl.h"
#include <string>

IMPLEMENT_DYNAMIC(CScriptCaller, CDialogEx)

BEGIN_MESSAGE_MAP(CScriptCaller, CDialogEx)
	ON_WM_PAINT()
END_MESSAGE_MAP()


CScriptCaller::CScriptCaller(CWnd* pParent, WORD scriptEntry)
	:CDialogEx(IDD_DIALOG2, pParent), InScript(scriptEntry)
{
}

void CScriptCaller::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List);
}


LPCSTR p_Script(int i);

BOOL CScriptCaller::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//窗口标题
	CString s;
	s.Format(L"查看 %4.4X 的脚本调用者", InScript);
	SetWindowText(s);

	RECT s_Rect;
	GetParent()->GetWindowRect(&s_Rect);
	s_Rect.top += 40; s_Rect.bottom -= 40; s_Rect.right -= 40; s_Rect.left += 40;
	MoveWindow(&s_Rect, TRUE);
	int h = s_Rect.bottom - s_Rect.top;
	int w = s_Rect.right - s_Rect.left;
	s_Rect.top = 10; s_Rect.bottom = h - 60; s_Rect.right = w - 30; s_Rect.left = 10;
	m_List.MoveWindow(&s_Rect, TRUE);

	auto Pal = ((CsDlg*)AfxGetApp()->m_pMainWnd)->Pal;

	m_List.SetExtendedStyle(
		LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_NOSORTHEADER);
	m_List.ModifyStyle(0, LVS_SINGLESEL | LVS_REPORT | LVS_SHOWSELALWAYS);

	m_List.InsertColumn(0, L"", 0, 0);
	m_List.InsertColumn(1, L"来  源", 0, 180);
	m_List.InsertColumn(2, L"调用者", 0, 180);
	m_List.InsertColumn(3, L"调用者位置", 0, 180);
	m_List.InsertColumn(4, L"调用者行16进制", 0, 180);
	do
	{
		auto &sgMark = Pal->pMark[InScript].s;
		if (sgMark.size() == 0)
			return FALSE;
		for (DWORD32 n=0;n<sgMark.size();n++)
		{
			CString cS;
			cS.Format(L"");
			m_List.InsertItem(n, cS);
			INT32 mark = sgMark[n];
			switch ((mark >> 28) & 0xf)
			{
			case 0:
				cS.Format(L"初始设定");
				break;
			default:
				cS.Format(L"%d.rpg", ((mark >> 28) & 0xf));
				break;
			}
			m_List.SetItemText(n, 1, cS);

			switch ((mark >> 24) & 0xf)
			{
			case 1:
				cS = L"对象";
				m_List.SetItemText(n, 2, cS);
				break;
			case 2:
				cS = L"场景";
				m_List.SetItemText(n, 2, cS);
				break;
			case 3:
				cS = L"事件";
				m_List.SetItemText(n, 2, cS);
				break;
			case 4:
				cS = L"脚本";
				m_List.SetItemText(n, 2, cS);
				//mark--;
				break;
			default:
				return FALSE;//出错
				break;
			}
			cS.Format(L"%d", (mark >> 16) & 0xff);
			m_List.SetItemText(n, 3, cS);
			cS.Format(L"%4.4X", (mark) & 0xffff);
			m_List.SetItemText(n, 4, cS);
		}
	} while (FALSE);
	m_List.SetFocus();
	return TRUE;
}

void CScriptCaller::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_List.GetSafeHwnd() == nullptr)
		return;
	m_List.MoveWindow(4, 4, cx - 4, cy - 4);

}


void CScriptCaller::OnPaint()
{
	if (!m_List.GetSafeHwnd())
		return;
	m_List.ShowWindow(TRUE);
	CDialogEx::OnPaint();
}

