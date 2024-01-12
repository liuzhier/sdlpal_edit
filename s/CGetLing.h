#pragma once
#include "afxdialogex.h"


// CGetLing 对话框
//选择行
class CGetLing : public CDialog
{
	DECLARE_DYNAMIC(CGetLing)

public:
	CGetLing(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CGetLing();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	int row;
	CString sText;
	virtual void OnOK();
private:
public:
	virtual BOOL OnInitDialog();
};
