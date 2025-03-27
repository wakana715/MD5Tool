/*!
 * @file MD5Tool.cpp
 * @brief ディレクトリを再帰的に捜査してファイル一覧のMD5を算出してログファイルに出力する
 * @date 2025
 */

#include "GetDesktopDirectory.h"
#include "GetModuleDirectory.h"
#include "GetTemporaryFile.h"
#include "GetFileHashMD5.h"
#include "ReadFileInt.h"
#include "ReadFileMap.h"
#include "WriteFileInt.h"
#include "WriteFileStrU16.h"
#include <windows.h>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

#define LONG_PRE_PATH	L"\\\\?\\"
#define LONG_MAX_PATH	(32767)
#define LONG_MAX_CMD	(LONG_MAX_PATH*3+100)
#define BUF_SIZE	(1024*1024*256)

#if (LONG_MAX_PATH * 6 + LONG_MAX_CMD) > BUF_SIZE
#error "LONG_MAX_PATH > BUF_SIZE"
#endif

static int		s_len = 0;
static HANDLE	s_handle = INVALID_HANDLE_VALUE;	//ログファイルのハンドル <desktop>\hash_<DATE_TIME>.txt
static byte		s_buf[BUF_SIZE]{};
static bool		s_stop = false;
static std::wstring s_wsTempFileParameter;
static std::wstring s_wsTempFileStatus;
static std::wstring s_wsTempFileProgress;

/*!
 * @brief ワイド文字(UTF-16)からバイト文字(UTFF-8)へ変換する
 * @param[in]	_p_wcs ワイド文字(UTF-16)のポインタ
 * @param[in]	_size  ワイド文字(UTF-16)のバイト数
 * @param[out]	_p_str バイト文字(UTF-8)のポインタ
 * @return バイト文字のバイト数
 */
static int W2U(const wchar_t* _p_wcs, const int _size, char* _p_str)
{
	return WideCharToMultiByte(CP_UTF8, 0, _p_wcs, -1, _p_str, _size, NULL, NULL);
}

/*!
 * @brief テンポラリファイル名取得
 * @param[out]	_wsFileName	テンポラリファイル名
 * @return 0:成功, 1:GetTemporaryFile失敗
 */
static int GetTemporaryFileName(const int _type, std::wstring& _wsFileName)
{
	int ret = GetTemporaryFile(L"MD5", sizeof(s_buf), reinterpret_cast<wchar_t*>(s_buf));
	if (ret != 0)
	{
		DWORD dwError = ::GetLastError();
		std::wstringstream wss;
		wss << L"Failed GetTemporaryFileName(" << _type << L") = " << ret << L" ";
		wss << L" 0x" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<long>(dwError);
		(void)::MessageBoxW(NULL, wss.str().c_str(), L"Error", MB_ICONERROR | MB_OK);
		return 1;
	}
	_wsFileName = reinterpret_cast<wchar_t*>(s_buf);
	return 0;
}

/*!
 * @brief ファイルのパスとハッシュ値をログファイルへ出力する
 * @param[in]	_p_path ファイルパスのポインタ
 * @return なし
 */
static void GetHash(const wchar_t* _p_path)
{
	if (s_stop)
	{
		return;
	}
	int status;
	if (ReadFileInt(s_wsTempFileStatus.c_str(), &status) == 0)
	{
		if (status == 3)	// 3:中断要求
		{
			std::string strStop = "stop\x0d\x0a";
			(void)::WriteFile(s_handle, strStop.c_str(), static_cast<DWORD>(strStop.length()), NULL, NULL);
			(void)::FlushFileBuffers(s_handle);
			s_stop = true;
			return;
		}
	}
	unsigned char szHash[MD5_SIZE]{};
	int ret = GetFileHashMD5(LONG_MAX_PATH, _p_path, BUF_SIZE, reinterpret_cast<char*>(s_buf), MD5_SIZE, szHash);
	std::wstring wsHash;
	if (ret == 0)
	{
		std::wstringstream wss;
		for (int i = 0; i < MD5_SIZE; i++)
		{
			unsigned int uHash = static_cast<unsigned int>(szHash[i]);
			wss << std::setfill(L'0') << std::hex << std::setw(2) << uHash;
		}
		wss >> wsHash;
	}
	else
	{
		DWORD dwError = ::GetLastError();
		std::wstringstream wss;
		wss << L"error " << ret;
		wss << L" 0x" << std::hex << std::setfill(L'0') << std::setw(4) << static_cast<long>(dwError);
		wss >> wsHash;
	}
	wchar_t* pwide = reinterpret_cast<wchar_t*>(s_buf);
	(void)wcscpy_s(pwide, LONG_MAX_PATH, _p_path);
	WriteFileStrU16(s_wsTempFileProgress.c_str(), LONG_MAX_PATH, reinterpret_cast<wchar_t*>(s_buf));
	std::wstringstream wss;
	wss << wsHash << L"\t";
	wss << &pwide[s_len];
	wss << L"\x0d\x0a";
	char* pchar = reinterpret_cast<char*>(s_buf);
	int size = static_cast<int>(wss.str().size() * sizeof(wchar_t));
	int len = W2U(wss.str().c_str(), size, pchar);
	(void)::WriteFile(s_handle, pchar, len - 1, NULL, NULL);
	(void)::FlushFileBuffers(s_handle);
}

/*!
 * @brief ソート(std::sort)用の文字列比較関数(ignore case)
 * @param[in]	_ws1	文字列1
 * @param[in]	_ws2	文字列2
 * @return true:ws1<=ws2, false:ws1>ws2
 */
static bool ComparePath(std::wstring& _ws1, std::wstring& _ws2)
{
	std::wstring ws1low = _ws1;
	std::wstring ws2low = _ws2;
	std::transform(ws1low.begin(), ws1low.end(), ws1low.begin(), ::tolower);
	std::transform(ws2low.begin(), ws2low.end(), ws2low.begin(), ::tolower);
	return (ws1low.compare(ws2low) <= 0);
}

/*!
 * @brief ディレクトリを再帰的に捜査してファイルパスを取得する
 * @param[in]	_p_path ファイルパスのポインタ
 * @return true:成功, false:失敗
 */
static bool GetFile(const wchar_t* _p_path)
{
	std::wstring wcsPath = _p_path;
	wcsPath += L"\\*.*";
	WIN32_FIND_DATAW find;
	HANDLE handle = ::FindFirstFileW(wcsPath.c_str(), &find);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	std::vector<std::wstring> vct_dir;
	std::vector<std::wstring> vct_file;
	do
	{
		if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(find.cFileName, L".")  != 0 &&
				wcscmp(find.cFileName, L"..") != 0)
			{
				std::wstring wcsPathDir = _p_path;
				wcsPathDir += L"\\";
				wcsPathDir += find.cFileName;
				vct_dir.push_back(wcsPathDir.c_str());
			}
		}
		else
		{
			std::wstring wcsPathFile = _p_path;
			wcsPathFile += L"\\";
			wcsPathFile += find.cFileName;
			vct_file.push_back(wcsPathFile.c_str());
		}
	} while (::FindNextFileW(handle, &find));
	(void)::FindClose(handle);
	{
		std::sort(vct_file.begin(),vct_file.end(), ComparePath);
		for (std::vector<std::wstring>::iterator it = vct_file.begin(); it != vct_file.end(); ++it)
		{
			if (s_stop)
			{
				return false;
			}
			GetHash(it->c_str());
		}
	}
	{
		std::sort(vct_dir.begin(),vct_dir.end(), ComparePath);
		for (std::vector<std::wstring>::iterator it = vct_dir.begin(); it != vct_dir.end(); ++it)
		{
			if (s_stop)
			{
				return false;
			}
			if (!GetFile(it->c_str()))
			{
				break;
			}
		}
	}
	return true;
}

/*!
 * @brief 日時型(SYSTEMTIME)を文字列に変換する
 * @param[in]	_time 日時
 * @param[out]	_wstrDate 変換した文字列
 * @return なし
 */
static void GetStrDate(SYSTEMTIME _time, std::wstring& _wstrDate)
{
	std::wstringstream wss;
	wss << std::setfill(L'0') << std::setw(4) << _time.wYear;
	wss << std::setfill(L'0') << std::setw(2) << _time.wMonth;
	wss << std::setfill(L'0') << std::setw(2) << _time.wDay << L"_";
	wss << std::setfill(L'0') << std::setw(2) << _time.wHour;
	wss << std::setfill(L'0') << std::setw(2) << _time.wMinute;
	wss << std::setfill(L'0') << std::setw(2) << _time.wSecond;
	wss >> _wstrDate;
}

/*!
 * @brief 画面入力(FromPath)を取得する。処理時間をレポートファイルへ追記する。
 * @return なし
 */
static void MD5Tool(const wchar_t* _p_prg, const wchar_t* _p_dir)
{
	SYSTEMTIME start, end;
	(void)::GetLocalTime(&start);
	wchar_t* pwide = reinterpret_cast<wchar_t*>(s_buf);
	GetDesktopDirectory(sizeof(s_buf), pwide);
	std::wstring wcsDesktop = pwide;
	std::wstring wcsStart;
	GetStrDate(start, wcsStart);
	for (int i = 0; i < 1000; i++)
	{
		std::wstringstream wss;
		wss << LONG_PRE_PATH << wcsDesktop << L"\\hash_" + wcsStart;
		if (i != 0)
		{
			wss << L"_" << std::setfill(L'0') << std::setw(3) << i;
		}
		wss << L".txt";
		s_handle = ::CreateFileW(wss.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (s_handle != INVALID_HANDLE_VALUE)
		{
			break;
		}
	}
	if (s_handle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	if (!GetFile(_p_dir))
	{
		(void)::CloseHandle(s_handle);
		return;
	}
	(void)::CloseHandle(s_handle);
	(void)::GetLocalTime(&end);
	std::wstring wcsEnd;
	GetStrDate(end, wcsEnd);
	FILETIME fstart, fend;
	(void)::SystemTimeToFileTime(&start, &fstart);
	(void)::SystemTimeToFileTime(&end,   &fend);
	__int64 nstart = *((__int64*)&fstart);
	__int64 nend   = *((__int64*)&fend);
	__int64 nsec   = (nend - nstart) / 10000 / 1000;
	{
		{
			std::wstringstream wss;
			wss << LONG_PRE_PATH << wcsDesktop << L"\\report.txt";
			s_handle = ::CreateFileW(wss.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		if (s_handle != INVALID_HANDLE_VALUE)
		{
			(void)::SetFilePointer(s_handle, 0, NULL, FILE_END);
			std::wstringstream wss;
			wss << wcsStart				<< L"\t";
			wss << wcsEnd				<< L"\t";
			wss << std::setw(7) << nsec << L"\t";
			wss << _p_dir				<< L"\t";
			wss << s_stop				<< L"\x0d\x0a";
			char*  p = reinterpret_cast<char*>(s_buf);
			int size = static_cast<int>(wss.str().size() * sizeof(wchar_t));
			int len = W2U(wss.str().c_str(), size, p);
			(void)::WriteFile(  s_handle, p, len - 1, NULL, NULL);
			(void)::CloseHandle(s_handle);
		}
	}
}

/*!
 * @brief 画面を表示する。実行終了まで待機する。
 * @return 0固定
 */
int APIENTRY wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// モジュールファイル(exe)のディレクトリ名取得
	if (GetModuleDirectory(LONG_MAX_PATH, reinterpret_cast<wchar_t*>(s_buf)) != 0)
	{
		return 0;
	}
	std::wstring wsModuleDirectory = reinterpret_cast<wchar_t*>(s_buf);

	// テンポラリファイル名(パラメータ用)を取得する
	if (GetTemporaryFileName(1, s_wsTempFileParameter) != 0)
	{
		return 0;
	}

	// テンポラリファイル名(ステータス用)を取得する
	if (GetTemporaryFileName(2, s_wsTempFileStatus) != 0)
	{
		return 0;
	}

	// テンポラリファイル名(プログレス用)を取得する
	if (GetTemporaryFileName(3, s_wsTempFileProgress) != 0)
	{
		return 0;
	}

	WriteFileInt(s_wsTempFileStatus.c_str(), 0);	// 0:NOP

	// コマンドを作成する
	std::wstring wsCmd;
	{
		std::wstringstream wss;
		wss << L"powershell -NoProfile -ExecutionPolicy Unrestricted -WindowStyle Hidden \"";
		wss << wsModuleDirectory;
		wss << L"\\MD5Tool.ps1\" \"";
		wss << s_wsTempFileParameter;
		wss << L"\" \"";
		wss << s_wsTempFileStatus;
		wss << L"\" \"";
		wss << s_wsTempFileProgress;
		wss << L"\"";
		wsCmd = wss.str();
	}

	STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;

	DWORD dwRet;
	if (CreateProcessW(nullptr, &wsCmd[0], nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
	{
		do
		{
			dwRet = ::WaitForSingleObject(pi.hProcess, 500);
			if (dwRet == WAIT_TIMEOUT)
			{
				int status = 0;
				ReadFileInt(s_wsTempFileStatus.c_str(), &status);
				if (status == 1)										// 1:実行要求
				{
					std::map<std::wstring, std::wstring> map;
					ReadFileMap(s_wsTempFileParameter.c_str(), sizeof(s_buf), reinterpret_cast<char*>(s_buf), map);
					std::wstring wsKey = L"dir";
					if (map.count(wsKey) > 0)
					{
						std::wstring wsDir = map[wsKey];
						s_stop = false;
						WriteFileInt(s_wsTempFileStatus.c_str(), 2);	// 2:実行中
						MD5Tool(s_wsTempFileProgress.c_str(), wsDir.c_str());
					}
					WriteFileInt(s_wsTempFileStatus.c_str(), 0);		// 0:NOP
				}
			}
		} while (dwRet != WAIT_OBJECT_0);
	}
	(void)::DeleteFile(s_wsTempFileParameter.c_str());
	(void)::DeleteFile(s_wsTempFileStatus.c_str());
	(void)::DeleteFile(s_wsTempFileProgress.c_str());
	return 0;
}
