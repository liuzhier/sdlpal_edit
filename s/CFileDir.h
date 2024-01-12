#pragma once
//#define _AFXDLL
#include <vector>
#include <string>
using namespace std;
typedef struct regFontStruct 
{
	string name;
	string value;
}FONTSTRUCT;
using FONDFILELIST = std::vector < FONTSTRUCT > ;

class CFileDir
{

public:
	BOOL IsCorrectDir(const CString& PalDir);

	//取得正确的目录 FALSE返回错,PalDir 返回目录取得值
	INT GetCorrectDir(CString& PalDir);

	//打开字库文件
	//INT GetFileName(CString& FontName);

	//INT GetSystemFontFileList(FONDFILELIST &fontList);

	// 判断文件是否存在
	BOOL IsFileExist(const CString& csFile);

	// 判断文件夹是否存在
	BOOL IsDirExist(const CString& csDir);

	//备份系统文件
	BOOL backupFile(const CString& csDir);
	//恢复系统文件
	BOOL restoreFile(const CString& csDir);

	BOOL removeFile(const CString& csDir);
	//查找符合条件的文件个数
	INT getFileCount(CString& dirStr);

	static  CString dirBuf;

private:
	static	int CALLBACK  BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

};

