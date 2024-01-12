#pragma once
#include "GridCtrl_src/GridCtrl.h"
#include <string>
#include <functional>
#include <variant>
#include "../main/palgpgl.h"
#include "CsListCtrl.h"
using namespace std;
// CsDataEdit
//本类实现类加入的编辑数据功能
//增加数据定义结构，增加相关函数
//包括指向数据结构的指针，由外部传入，通过getEditClass
//数据转换函数指针 将INT 转换成显示的数据TEXT 通过SetConvertData
//

typedef struct tagCellDataStruct//表单元数据结构
{
	int i ;
	double d;
	std::string  s;
}CellDataStruct, * LPCellDataStruct;

//typedef class tagCOLVAR :public std::variant<int, double, string>{}COLVAR;
using COLVAR = std::variant<int, double, string>;

typedef struct tagColumnClass
{
	vector <COLVAR> Col;
	SHORT RowMark = 0;//0 未动，1 显示过
	SIZE_T oldRow = 0;
}ColumnClass, * LPColumnClass;

typedef vector<ColumnClass>DataArray;//表数据数组结构

typedef struct tagUnDoData
{
	ColumnClass OldRowData;
	int oldRow;
	int newRow;
}UnDoRowData;


using namespace std;
typedef std::function<LPCSTR(int)>SrcToStr;

typedef std::function<int(LPCSTR)>StrToSrc;




//表栏操作控制类型
typedef enum tagCellCTRL
{
	ctrl_Null = 0,//什么都不做
	ctrl_Fix,//列头固定(永远显示)
	ctrl_Edit,//Edit
	ctrl_Combo,//下拉框
	ctrl_List,//,listbox
	ctrl_Radio,//单选框
	ctrl_MapEdit,//地图编辑
	//ctrl_InScript,//用于显示被调用脚本的次数
}col_CTRL;

//表数据类型
typedef enum tagColtype
{
	tINT = 0,
	tWORD,
	tDWORD,
	tSHORT,
	tHEX,
	tFLOAT,
	tSTR,
	tBOOL,
	tNull,
	tLine,
}col_TYPE;

//下拉框单行数据结构
typedef struct tagSelect_Item_Data
{
	string s;
	int item;
	int row; ;
}Select_Item_Data;

//下拉框结构数组
typedef vector<Select_Item_Data>select_Item_ColArray;

//下拉框数据结构
typedef  struct  tagSelect_Item
{
	select_Item_ColArray data;
	int row = 0;
}*pSelect_Item;

typedef vector<tagSelect_Item>SELECT_ITEN_ARRAY;


typedef struct tagColData //每一栏属性结构
{
	CString ColTop;//表头文字 
	WORD ColWidth{ 0 };//列宽
	WORD Col{ 0 };//列号
	col_CTRL Ctrl{};//0 什么都不做，1，列头固定(永远显示)，2，Edit 3，下拉框 4,listbox 5 单选框 6 调用表
	col_TYPE DataStat{};//原数据类型 0  整数 ，1 WORD, 2 SHORT, 3 16进制数，4 浮点数，5 字符串 6 布尔数
	pSelect_Item p_CtrlList = nullptr;//下拉框数据索引
	SrcToStr  p_InCol{ nullptr };//指向函数的指针，将源数据转化为显示数据字串
	StrToSrc  p_OutCol{ nullptr };//指向函数的指针，将显示字串转化为源
	INT  m_isScript{0};//列标识，标识列是否是脚本


	//输入：表头文字 ;
	// 列宽;
	// 表头序号；
	// 控制：0 什么都不做，1，列头固定(永远显示)，2，Edit 3，下拉框 4,listbox 5 单选框
	//下拉数据索引；
	//函数指针将源数据转化为显示数据字串；
	//指向函数的指针，将显示字串转化为源
	//m_isScript = 1 时是脚本号 = 2时是脚本调用次数
	void GetData(LPSTR t_ColTop, WORD Width, WORD t_col, col_CTRL t_Ctrl,
		col_TYPE  t_DataStat = tWORD, pSelect_Item t_CtrlData = nullptr, 
		SrcToStr t_SrcToStr = nullptr, StrToSrc t_SrcTostr = nullptr,INT  m_isScript = 0);
	tagColData::~tagColData();
}COLDATA;



typedef vector<COLDATA> ColArray;
//设置行更新时的操作函数指针
typedef VOID(*SET_COL_DATA)(ColumnClass& d);


class CsDataGrid :
	public CGridCtrl
{
protected:
	DECLARE_MESSAGE_MAP()

//public:
private:
	//表头数据数组
	ColArray m_ColArray;
	//表数据数组
	DataArray m_DataArray;
	//是否使用行号作为行头
	BOOL isLineTop ;

	SET_COL_DATA Set_Col_Data;
public:
	//系统数据指针的指针
	class CGetPalData** pPal;
public:

	//下拉表编辑控件
	CsListCtrl *m_pList;
	//弹出菜单控制数据
	DWORD  m_popMenuFlags;

public:
	CsDataGrid();
	~CsDataGrid();
	VOID Set_Set_Col_Data(SET_COL_DATA data) { Set_Col_Data = data; };
	// 设置表头，Col 行数，p_ColArray 表头结构数组
	void SetColClass(int col, ColArray& p_ColArray, BOOL IsLineTop = TRUE);

	// 设置表数据，row 行数 Col 列数 ROWData 表数据数组
	void SetDataClass(int row, int col, DataArray& RowData);

	DataArray& GetDataClass();
	//保存修改数据，用于数组
	vector<UnDoRowData> mUndoArray;
	int UndoCount{ 0 };
	int UndoCtrl{ 0 };

	void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
protected:
	afx_msg LRESULT Edit_Format(WPARAM w_wparam, LPARAM w_lparam);

	//设置可见部分行的表数据
	void SetVisibleCellRangesData();

	// 读取表格一行数据
	int Get_DataRow(long n_Row);

	//虚拟表格标志位，采用虚拟表格方式
	//RowMarkArray m_RowMarkArray;

	// 设置表格一行数据
	int Set_DataRow(long row);
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
public:
	afx_msg void OnMenuToTop();
	afx_msg void OnMenuToRow();
	afx_msg void OnMenuToBottom();
	afx_msg void OnMenuRange(UINT);

private:
	CCellID m_ClickCell;
protected:
	afx_msg LRESULT OnGridCellCharge(WPARAM wParam, LPARAM lParam);
protected:
    virtual void OnEndEditCell(int nRow, int nCol, CString str);

	afx_msg LRESULT OnGridRowCharge(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
public:
	// 撤消修改
	VOID Undo();
	virtual void  EndEditing();

};

