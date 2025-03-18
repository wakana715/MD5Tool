/*!
 * @file MD5Tool.cpp
 * @brief �f�B���N�g�����ċA�I�ɑ{�����ăt�@�C���ꗗ��MD5���Z�o���ă��O�t�@�C���ɏo�͂���
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
 * @brief ���C�h����(UTF-16)����o�C�g����(UTFF-8)�֕ϊ�����
 * @param[in]	_p_wcs ���C�h����(UTF-16)�̃|�C���^
 * @param[in]	_size  ���C�h����(UTF-16)�̃o�C�g��
 * @param[out]	_p_str �o�C�g����(UTF-8)�̃|�C���^
 * @return �o�C�g�����̃o�C�g��
 */
static int WideToStr(const wchar_t* _p_wcs, const int _size, char* _p_str)
{
	return WideCharToMultiByte(CP_UTF8, 0, _p_wcs, -1, _p_str, _size, NULL, NULL);
}

/*!
 * @brief ���W�X�g������p�X�������擾���āA�����O�t���p�X�`���ɕϊ�����
 * @param[in]	_p_name ���W�X�g�����̃|�C���^
 * @return true:����, false:���s
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
 * @brief ���W�X�g�����琔�l(DWORD)���擾����
 * @param[in]	_p_name ���W�X�g�����̃|�C���^
 * @param[out]	_p_dword ���l�̃|�C���^
 * @return true:����, false:���s
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
 * @brief ���W�X�g���L�[�쐬
 * @param[out]	_p_ret �������ʂ̃|�C���^ true:����, faise:���s
 * @return ���W�X�g���n���h��(HKEY)
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
 * @brief ���W�X�g���Ƀ��C�h����(UTF-16)��ݒ肷��
 * @param[in]	_p_name ���W�X�g�����̃|�C���^
 * @param[in]	_p_str  ���C�h�����̃|�C���^
 * @param[in]	_p_str  ���C�h�����̒���
 * @return �Ȃ�
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
 * @brief ���W�X�g���ɏ������̃t�@�C������ݒ肷��
 * @return �Ȃ�
 */
static void ShellRegSetPath()
{
	wchar_t* pwide = reinterpret_cast<wchar_t*>(g_buf);
	int len = static_cast<int>(wcsnlen_s(&pwide[g_len], LONG_PATH));
	ShellRegSetStr(L"ToPath", &pwide[g_len], len);
}

/*!
 * @brief ���W�X�g���ɐ��l(DWORD)��ݒ肷��
 * @param[in]	_p_name ���W�X�g�����̃|�C���^
 * @param[in]	_dword ���l
 * @return �Ȃ�
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
 * @brief �t�@�C���̃n�b�V���l(MD5)���Z�o����
 * @param[in]	_p_path �t�@�C���p�X�̃|�C���^
 * @param[in]	_hash �n�b�V���l
 * @return true:����, false:���s
 */
static bool GetFileHash(const wchar_t* _p_path, std::wstring& _hash)
{
	HCRYPTPROV hCryptProv;
	// TIPS:CRYPT_VERIFYCONTEXT : ���̃p�����[�^���w�肷��� Guest �A�J�E���g�œ��삵�Ȃ�
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
 * @brief �t�@�C���̃p�X�ƃn�b�V���l�����O�t�@�C���֏o�͂���
 * @param[in]	_p_path �t�@�C���p�X�̃|�C���^
 * @return �Ȃ�
 */
static void GetHash(const wchar_t* _p_path)
{
	if (g_stop)
	{
		return;
	}
	DWORD status = 1;
	ShellRegGetDWORD(L"Status", &status);
	if (status == 4)	// 4:���f�v��
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
 * @brief �f�B���N�g�����ċA�I�ɑ{�����ăt�@�C���p�X���擾����
 * @param[in]	_p_path �t�@�C���p�X�̃|�C���^
 * @return true:����, false:���s
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
 * @brief �����^(SYSTEMTIME)�𕶎���ɕϊ�����
 * @param[in]	_time ����
 * @param[out]	_wstrDate �ϊ�����������
 * @return �Ȃ�
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
 * @brief ��ʓ���(FromPath)���擾����B�������Ԃ����|�[�g�t�@�C���֒ǋL����B
 * @return �Ȃ�
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
 * @brief ��ʂ�\������B���s�I���܂őҋ@����B
 * @return 0�Œ�
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
				if (status == 1)						// 1:���s�v��
				{
					g_stop = false;
					ShellRegSetDWORD(L"Status", 2);		// 2:���s��
					MD5Tool();
					if (!g_stop)
					{
						ShellRegSetDWORD(L"Status", 3);	// 3:����I��
					}
				}
			}
		} while (dwRet != WAIT_OBJECT_0);
	}
	return 0;
}
