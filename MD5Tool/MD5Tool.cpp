/*!
 * @file MD5Tool.cpp
 * @brief ディレクトリを再帰的に捜査してファイル一覧のMD5を算出してログファイルに出力する
 * @author wakana
 * @date 2025
 */

#include <SDKDDKVer.h>
#include <windows.h>
#include <processthreadsapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>

#define LONG_PATH	(32767)
#define BUF_SIZE	(1024*1024*256)
#define	MD5_LEN		(16)

int     g_len = 0;
HANDLE  g_handle = INVALID_HANDLE_VALUE;
byte    g_buf[BUF_SIZE] = { 0 };
bool	g_stop = false;

/*!
 * @brief ワイド文字(UTF-16)からバイト文字(UTFF-8)へ変換する
 * @param[in]	_p_wcs ワイド文字(UTF-16)のポインタ
 * @param[in]	_size  ワイド文字(UTF-16)のバイト数
 * @param[out]	_p_str バイト文字(UTF-8)のポインタ
 * @return バイト文字のバイト数
 */
static int WideToStr(const wchar_t* _p_wcs, const int _size, char* _p_str)
{
	return WideCharToMultiByte(CP_UTF8, 0, _p_wcs, -1, _p_str, _size, NULL, NULL);
}

/*!
 * @brief レジストリからパス文字を取得して、ロングフルパス形式に変換する
 * @param[in]	_p_name レジストリ名のポインタ
 * @return true:成功, false:失敗
 */
static bool ShellRegGetPath(const wchar_t* _p_name)
{
	HKEY hKey;
	{
		LSTATUS stat = ::RegCreateKeyW(HKEY_CURRENT_USER, L"Software", &hKey);
		if (ERROR_SUCCESS != stat)
		{
			return false;
		}
	}
	DWORD size;
	char    *pchar = reinterpret_cast< char  *>(g_buf);
	wchar_t *pwide = reinterpret_cast<wchar_t*>(g_buf);
	{
		LSTATUS stat = ::RegGetValueW(hKey, L"MD5Tool", _p_name, RRF_RT_REG_SZ, 0, pchar, &size);
		(void)::RegCloseKey(hKey);
		if (ERROR_SUCCESS != stat)
		{
			pchar[0] = '\0';
			return false;
		}
	}
	pchar[LONG_PATH - 1] = '\0';
	if (size < LONG_PATH)
	{
		pchar[size] = '\0';
	}
	std::wstring wcsPath = pwide;
	std::wstring wcsPathLong = L"\\\\?\\" + wcsPath;
	DWORD ret = ::GetFullPathNameW(wcsPathLong.c_str(), LONG_PATH, pwide, 0);
	if (ret == 0)
	{
		return false;
	}
	return true;
}

/*!
 * @brief レジストリから数値(DWORD)を取得する
 * @param[in]	_p_name レジストリ名のポインタ
 * @param[out]	_p_dword 数値のポインタ
 * @return true:成功, false:失敗
 */
static void ShellRegGetDWORD(const wchar_t* _p_name, DWORD* _p_dword)
{
	HKEY hKey;
	{
		LSTATUS stat = ::RegCreateKeyW(HKEY_CURRENT_USER, L"Software", &hKey);
		if (ERROR_SUCCESS != stat)
		{
			return;
		}
	}
	DWORD size;
	{
		LSTATUS stat = ::RegGetValueW(hKey, L"MD5Tool", _p_name, RRF_RT_REG_DWORD, 0, _p_dword, &size);
		(void)::RegCloseKey(hKey);
		if (ERROR_SUCCESS != stat)
		{
			return;
		}
	}
}

/*!
 * @brief レジストリキー作成
 * @param[out]	_p_ret 処理結果のポインタ true:成功, faise:失敗
 * @return レジストリハンドル(HKEY)
 */
static HKEY ShellRegCreate(bool* _p_ret)
{
	*_p_ret = true;
	HKEY hKey1;
	{
		LSTATUS stat = ::RegCreateKeyW(HKEY_CURRENT_USER, L"Software", &hKey1);
		if (ERROR_SUCCESS != stat)
		{
			*_p_ret = false;
			return nullptr;
		}
	}
	HKEY hKey2;
	{
		LSTATUS stat = ::RegCreateKeyW(hKey1, L"MD5Tool", &hKey2);
		if (ERROR_SUCCESS != stat)
		{
			(void)::RegCloseKey(hKey1);
			*_p_ret = false;
			return nullptr;
		}
	}
	(void)::RegCloseKey(hKey1);
	return hKey2;
}

/*!
 * @brief レジストリにワイド文字(UTF-16)を設定する
 * @param[in]	_p_name レジストリ名のポインタ
 * @param[in]	_p_str  ワイド文字のポインタ
 * @param[in]	_p_str  ワイド文字の長さ
 * @return なし
 */
static void ShellRegSetStr(const wchar_t* _p_name, const wchar_t* _p_str, const int _len)
{
	bool ret;
	HKEY hKey = ShellRegCreate(&ret);
	if (!ret)
	{
		return;
	}
	DWORD size  =      static_cast<DWORD>(_len * sizeof(wchar_t));
	BYTE* pbyte = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(_p_str));
	LSTATUS stat = ::RegSetValueExW(hKey, _p_name, 0, REG_SZ, pbyte, size);
	(void)::RegCloseKey(hKey);
}

/*!
 * @brief レジストリに処理中のファイル名を設定する
 * @return なし
 */
static void ShellRegSetPath()
{
	wchar_t* pwide = reinterpret_cast<wchar_t*>(g_buf);
	int len = static_cast<int>(wcsnlen_s(&pwide[g_len], LONG_PATH));
	ShellRegSetStr(L"ToPath", &pwide[g_len], len);
}

/*!
 * @brief レジストリに数値(DWORD)を設定する
 * @param[in]	_p_name レジストリ名のポインタ
 * @param[in]	_dword 数値
 * @return なし
 */
static void ShellRegSetDWORD(const wchar_t* _p_name, const DWORD _dword)
{
	bool ret;
	HKEY hKey = ShellRegCreate(&ret);
	if (!ret)
	{
		return;
	}
	DWORD size = sizeof(DWORD);
	DWORD val = _dword;
	LSTATUS stat = ::RegSetValueExW(hKey, L"Status", 0, REG_DWORD, reinterpret_cast<BYTE*>(&val), size);
	(void)::RegCloseKey(hKey);
}

/*!
 * @brief ファイルのハッシュ値(MD5)を算出する
 * @param[in]	_p_path ファイルパスのポインタ
 * @param[in]	_hash ハッシュ値
 * @return true:成功, false:失敗
 */
static bool GetFileHash(const wchar_t* _p_path, std::wstring& _hash)
{
	HCRYPTPROV hCryptProv;
	// TIPS:CRYPT_VERIFYCONTEXT : 他のパラメータを指定すると Guest アカウントで動作しない
	if (::CryptAcquireContextW(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) != TRUE)
	{
		return false;
	}
	HCRYPTHASH hCryptHash;
	ALG_ID	algid = CALG_MD5;
	if (::CryptCreateHash(hCryptProv, algid, 0, 0, &hCryptHash) != TRUE)
	{
		return false;
	}
	HANDLE handle = ::CreateFileW(_p_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	bool is_d88 = false;
	size_t len = wcsnlen_s(_p_path, LONG_PATH);
	if (len > 4)
	{
		if (_wcsnicmp(&_p_path[len - 4], L".d88", 5) == 0)
		{
			is_d88 = true;
		}
	}
	DWORD dwRead = 0;
	do
	{
		if (::ReadFile(handle, g_buf, BUF_SIZE, &dwRead, NULL) == 0)
		{
			return false;
		}
		if (is_d88)
		{
			is_d88 = false;
			memset(g_buf, 0, 0x20);	// Clear Headder
		}
		if (::CryptHashData(hCryptHash, g_buf, dwRead, 0) != TRUE)
		{
			return false;
		}
	} while (dwRead != 0);
	DWORD dwHashLength = MD5_LEN;
	byte hash[MD5_LEN];
	if (::CryptGetHashParam(hCryptHash, HP_HASHVAL, hash, &dwHashLength, 0) != TRUE)
	{
		return false;
	}
	if (handle != INVALID_HANDLE_VALUE)
	{
		(void)::CloseHandle(handle);
	}
	if (hCryptHash)
	{
		(void)::CryptDestroyHash(hCryptHash);
	}
	if (hCryptProv)
	{
		(void)::CryptReleaseContext(hCryptProv, 0);
	}
	std::wstringstream wss;
	for (int i = 0; i < MD5_LEN; i++)
	{
		unsigned int val = static_cast<unsigned int>(hash[i]);
		wss << std::setfill(L'0') << std::hex << std::setw(2) << val;
	}
	wss >> _hash;
	return true;
}

/*!
 * @brief ファイルのパスとハッシュ値をログファイルへ出力する
 * @param[in]	_p_path ファイルパスのポインタ
 * @return なし
 */
static void GetHash(const wchar_t* _p_path)
{
	if (g_stop)
	{
		return;
	}
	DWORD status = 1;
	ShellRegGetDWORD(L"Status", &status);
	if (status == 4)	// 4:中断要求
	{
		(void)::CloseHandle(g_handle);
		g_stop = true;
	}
	std::wstring hash;
	if (!GetFileHash(_p_path, hash))
	{
		return;
	}
	wchar_t* pwide = reinterpret_cast<wchar_t*>(g_buf);
	(void)wcscpy_s(pwide, LONG_PATH, _p_path);
	ShellRegSetPath();
	std::wstringstream wss;
	wss << hash << L"\t";
	wss << &pwide[g_len];
	wss << L"\x0d\x0a";
	char* pchar = reinterpret_cast<char*>(g_buf);
	int size = static_cast<int>(wss.str().size() * sizeof(wchar_t));
	int len = WideToStr(wss.str().c_str(), size, pchar);
	(void)::WriteFile(g_handle, pchar, len - 1, NULL, NULL);
	(void)::FlushFileBuffers(g_handle);
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
		std::sort(vct_file.begin(),vct_file.end());
		for (std::vector<std::wstring>::iterator it = vct_file.begin(); it != vct_file.end(); ++it)
		{
			if (g_stop)
			{
				return false;
			}
			GetHash(it->c_str());
		}
	}
	{
		std::sort(vct_dir.begin(),vct_dir.end());
		for (std::vector<std::wstring>::iterator it = vct_dir.begin(); it != vct_dir.end(); ++it)
		{
			if (g_stop)
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
static void MD5Tool()
{
	SYSTEMTIME start, end;
	(void)::GetLocalTime(&start);
	if (!ShellRegGetPath(L"FromPath"))
	{
		return;
	}
	wchar_t* pwide = reinterpret_cast<wchar_t*>(g_buf);
	g_len = static_cast<int>(wcsnlen_s(pwide, LONG_PATH) + 1);
	std::wstring wcsPath = pwide;
	(void)::SHGetSpecialFolderPathW(NULL, pwide, CSIDL_DESKTOP, 0);
	std::wstring wcsDesktop = pwide;
	std::wstring wcsStart;
	GetStrDate(start, wcsStart);
	ShellRegSetStr(L"Start", wcsStart.c_str(), static_cast<int>(wcsStart.length()));
	for (int i = 0; i < 1000; i++)
	{
		std::wstringstream wss;
		wss << L"\\\\?\\" << wcsDesktop << L"\\hash_" + wcsStart;
		if (i != 0)
		{
			wss << L"_" << std::setfill(L'0') << std::setw(3) << i;
		}
		wss << L".txt";
		g_handle = ::CreateFileW(wss.str().c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (g_handle != INVALID_HANDLE_VALUE)
		{
			break;
		}
	}
	if (g_handle == INVALID_HANDLE_VALUE)
	{
		return;
	}
	if (!GetFile(wcsPath.c_str()))
	{
		(void)::CloseHandle(g_handle);
		return;
	}
	(void)::CloseHandle(g_handle);
	(void)::GetLocalTime(&end);
	std::wstring wcsEnd;
	GetStrDate(end, wcsEnd);
	ShellRegSetStr(L"End", wcsEnd.c_str(), static_cast<int>(wcsEnd.length()));
	FILETIME fstart, fend;
	(void)::SystemTimeToFileTime(&start, &fstart);
	(void)::SystemTimeToFileTime(&end,   &fend);
	__int64 nstart = *((__int64*)&fstart);
	__int64 nend   = *((__int64*)&fend);
	__int64 nsec   = (nend - nstart) / 10000 / 1000;
	{
		std::wstringstream wss;
		wss << L"\\\\?\\" << wcsDesktop << L"\\report.txt";
		g_handle = ::CreateFileW(wss.str().c_str(), FILE_SHARE_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (g_handle != INVALID_HANDLE_VALUE)
		{
			(void)::SetFilePointer(g_handle, 0, NULL, FILE_END);
			std::wstringstream wss;
			wss << wcsStart << L"\t";
			wss << wcsEnd   << L"\t";
			wss << std::setw(7) << nsec << L"\t";
			wss << wcsPath.substr(4)	<< L"\t";
			wss << g_stop	<< L"\x0d\x0a";
			char*  p = reinterpret_cast<char*>(g_buf);
			int size = static_cast<int>(wss.str().size() * sizeof(wchar_t));
			int len = WideToStr(wss.str().c_str(), size, p);
			(void)::WriteFile(  g_handle, p, len - 1, NULL, NULL);
			(void)::CloseHandle(g_handle);
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
	wchar_t	szCommand[] = L"powershell -NoProfile -ExecutionPolicy Unrestricted -WindowStyle Hidden ./MD5Tool.ps1";
	STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;

	DWORD dwRet;
	if (CreateProcessW(nullptr, szCommand, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
	{
		do
		{
			dwRet = ::WaitForSingleObject(pi.hProcess, 500);
			if (dwRet == WAIT_TIMEOUT)
			{
				DWORD status = 0xFFFFFFFF;
				ShellRegGetDWORD(L"Status", &status);
				if (status == 1)						// 1:実行要求
				{
					g_stop = false;
					ShellRegSetDWORD(L"Status", 2);		// 2:実行中
					MD5Tool();
					if (!g_stop)
					{
						ShellRegSetDWORD(L"Status", 3);	// 3:正常終了
					}
				}
			}
		} while (dwRet != WAIT_OBJECT_0);
	}
	return 0;
}
