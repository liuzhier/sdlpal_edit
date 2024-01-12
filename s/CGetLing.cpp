// CGetLing.cpp: 实现文件
//
//#define _AFXDLL

#include "stdafx.h"
//#include "pch.h"
#include "s.h"
#include "afxdialogex.h"
#include "CGetLing.h"


// CGetLing 对话框

IMPLEMENT_DYNAMIC(CGetLing, CDialog)

CGetLing::CGetLing(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG1, pParent)
	, row(0)
{

	//row = 0;
}

CGetLing::~CGetLing()
{
}

void CGetLing::DoDataExchange(CDataExchange* pDX)
{
	
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGetLing, CDialog)
END_MESSAGE_MAP()


// CGetLing 消息处理程序


void CGetLing::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	CDialog::OnOK();
	CString str;
	GetDlgItem(IDC_EDIT2)->GetWindowTextW(str);
	row = _ttoi(str);
}


BOOL CGetLing::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	if (!sText.IsEmpty())
		GetDlgItem(IDC_STATIC)->SetWindowText(sText);
	CString str;
	str.Format(L"%d", row);

	GetDlgItem(IDC_EDIT2)->SetWindowText(str);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
