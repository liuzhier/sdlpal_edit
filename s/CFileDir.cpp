//#define _AFXDLL
#include "stdafx.h"
#include "CFileDir.h"
#include <fstream>
#include <vector>

using namespace std;

const CHAR* PalFileName[] =
{
	"abc.mkf",
	"ball.mkf",
	"data.mkf",
	"f.mkf",
	"fbp.mkf",
	"fire.mkf",
	"gop.mkf",
	"map.mkf",
	"mgo.mkf",
	"rgm.mkf",
	"sss.mkf",
	"word.dat",
	"m.msg",
	//"DESC.DAT",
	nullptr,
};

BOOL CFileDir::IsCorrectDir(const CString& PalDir)
{
	BOOL b1 = 1;
	for (int i = 0; PalFileName[i]; i++)
	{
		CString dir = PalDir + PalFileName[i];
		b1 &= IsFileExist(dir);
	}
	if (!b1)
		return FALSE;
	//是正确的目录，往下进行
	return TRUE;
}

// 判断文件是否存在

BOOL CFileDir::IsFileExist(const CString& csFile)
{
	DWORD dwAttrib = GetFileAttributes(csFile.GetString());
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

// 判断文件夹是否存在

BOOL CFileDir::IsDirExist(const CString& csDir)
{
	DWORD dwAttrib = GetFileAttributes(csDir.GetString());
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 != (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL CFileDir::backupFile(const CString& csDir)
{
	{
		CString dirBak = csDir + "BAK\\";

		if (!IsDirExist(dirBak))
			CreateDirectory(dirBak, 0);
	}

	for (int i = 0; PalFileName[i]; i++)
	{
		CString dir = csDir + PalFileName[i];
		CString dirbak = csDir + "BAK\\";
		dirbak += PalFileName[i];
		if (!IsFileExist(dirbak))
			CopyFile(( dir),( dirbak), TRUE);
	}
	//删除.new文件,.Bak文件
	for (int i = 0; PalFileName[i]; i++)
	{
		CString newFile = csDir + PalFileName[i];
		newFile += L".new";
		if (IsFileExist(newFile))
		{
			remove(CStringA(newFile).GetString());
		}
		CString bakFile = csDir + PalFileName[i];
		bakFile += L".bak";
		if (IsFileExist(bakFile))
		{
			remove(CStringA(bakFile).GetString());
		}
	}
	//备份存档文件
	for (int n = 0; n < 10; n++)
	{
		CString s;
		s.Format(L"%d.rpg", n);
		CString dir = csDir + s;
		CString dirbak = csDir + "BAK\\";
		dirbak += s;
		if (IsFileExist(dir) && !IsFileExist(dirbak))
			CopyFile((dir),( dirbak), FALSE);
	}

	return 0;
}

BOOL CFileDir::restoreFile(const CString& csDir)
{
	for (int i = 0; PalFileName[i]; i++)
	{
		BOOL bi{ 0 };
		CString dir = csDir + PalFileName[i];
		CString dirbak = csDir + "BAK\\";
		dirbak += PalFileName[i];
		if (IsFileExist(dirbak))
		{
			DeleteFile(dir);
			bi = MoveFile(dirbak, dir);
		}
	}
	//存档文件
	for (int n = 0; n < 10; n++)
	{
		CString s;
		s.Format(L"%d.rpg", n);
		CString dir = csDir + s;
		CString dirbak = csDir + "BAK\\";
		dirbak += s;
		if (IsFileExist(dirbak))
		{
			DeleteFile(dir);
			MoveFile(dirbak, dir);
		}
	}
	CString olddir = csDir + "BAK\\";
	RemoveDirectory(olddir);
	
	return 0;
}

BOOL CFileDir::removeFile(const CString& csDir)
{
	for (int i = 0; PalFileName[i]; i++)
	{
		CString dirbak = csDir + "BAK\\";
		dirbak += PalFileName[i];
		DeleteFile(dirbak);
	}
	for (int n = 0; n < 10; n++)
	{
		CString s;
		s.Format(L"%d.rpg", n);
		CString dirbak = csDir + "BAK\\";
		dirbak += s;
		if (IsFileExist(dirbak))
			DeleteFile(dirbak);
	}
	CString olddir = csDir + "BAK\\";
	RemoveDirectory(olddir);
	return 0;
}

//查找符合条件的文件个数
INT CFileDir::getFileCount(CString& dirStr) 
{
	CFileFind finder{};
	int count{1};
	if (!finder.FindFile(dirStr.GetString()))
		return 0;

	while (finder.FindNextFile())
	{
		CString name = finder.GetFilePath();
		count++;
	}
	finder.Close();
	return count;
}

CString CFileDir::dirBuf;

int CFileDir::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:    //初始化消息
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPTSTR)(LPCTSTR)dirBuf.GetBuffer());   //  m_filePath 为类的静态变量
		break;
	case BFFM_SELCHANGED:    //选择路径变化，
	{
		TCHAR curr[MAX_PATH];
		SHGetPathFromIDList((LPCITEMIDLIST)lParam, curr);
		::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)curr);
	}
	break;
	default:
		break;
	}
	return 0;
}


//取得正确的目录 FALSE返回错,PalDir 返回目录取得值
INT CFileDir::GetCorrectDir(CString& PalDir)
{
	BROWSEINFO bi;
	CString name;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.pidlRoot = CSIDL_DESKTOP;
	bi.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
	bi.pszDisplayName = name.GetBuffer(MAX_PATH + 1);
	bi.lpszTitle = _T("选取PAL运行文件夹");
	bi.ulFlags =  BIF_RETURNFSANCESTORS;
	bi.lpfn = BrowseCallbackProc;        //设置CALLBACK函数
	dirBuf = PalDir;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (idl == NULL)
	{
		return -1;
	}
	//	CString strDirectoryPath;
	SHGetPathFromIDList(idl, name.GetBuffer(MAX_PATH));
	name.ReleaseBuffer();
	if (name.IsEmpty())
		return FALSE;
	if (name.Right(1) != "\\")
		name += "\\";
	PalDir = name;
	//变更工作目录
	SetCurrentDirectory(PalDir);

	if (!IsCorrectDir(name))
	{
		name += "Pal\\";
		if (!IsCorrectDir(name))
			return 0;
	}
	PalDir = name;
	//GetOpenFileName;
	return 1;
}
/*
//打开文件
INT CFileDir::GetFileName(CString& FontName)
{
	FONDFILELIST fontFileList;
	GetSystemFontFileList(fontFileList);
	return 0;
}
*/

/*
#define MAX_KAY_LEN 250
INT CFileDir::GetSystemFontFileList(FONDFILELIST & fontList)
{
	HKEY				hKey;
	LONG				result;
	{
		//获取 hKey
		static const WCHAR fontRegistryPath[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
		if(result != ERROR_SUCCESS) 
			return 0;
	}
	DWORD valueIndex = 0;
	WCHAR* valueName = new WCHAR[MAX_KAY_LEN];
	LPBYTE valueData = new BYTE[MAX_KAY_LEN];
	DWORD valueNameSize{}, valueDataSize{}, valueType{};
	// Look for a matching font name
	do {
		valueDataSize = 40;//比40长的键忽略
		valueNameSize = 25;//比0长的键忽略

		result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, valueData, &valueDataSize);
		valueIndex++;

		if (result != ERROR_SUCCESS || valueType != REG_SZ) {
			continue;
		}
		string  sValueName(CStringA((LPWSTR)valueName).GetString());//转换
		string  sValueData(CStringA((LPWSTR)valueData).GetString());//转换
		{
			char mExt[_MAX_EXT]{ 0 };
			_splitpath_s(sValueData.c_str(), NULL,0, NULL,0, NULL,0, mExt,_MAX_EXT);
			if (_strcmpi(".ttf", mExt))
				continue;;
		}
		CString* ps = new CString;
		ps->Format(L"%5.5d   %s    %s", valueIndex, valueName, (LPWSTR)valueData);
		PostMessage(AfxGetApp()->m_pMainWnd->m_hWnd, WM_POST_MESSAGE_CSTR, 0, (LPARAM)ps);
		fontList.push_back({ sValueName, sValueData });
	} while (result != ERROR_NO_MORE_ITEMS);
	delete [] valueName;
	delete [] valueData;
	return 0;//
	//DEFAULT_CHARSET;
}

*/





