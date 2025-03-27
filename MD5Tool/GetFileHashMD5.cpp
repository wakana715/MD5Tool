/*!
 * @file GetFileHashMD5.cpp
 * @brief ファイルのハッシュ値(MD5)を取得
 * @date 2025
 */
#include "GetFileHashMD5.h"
#include <windows.h>
#include <bcrypt.h>
#include <vector>

#pragma comment (lib, "Bcrypt.lib")

int GetFileHashMD5(const size_t _size_path, const wchar_t* _p_path, const size_t _size_buf, char* _p_buf, const size_t _size_hash, unsigned char* _p_hash)
{
	bool is_d88 = false;
	size_t len = wcsnlen_s(_p_path, _size_path);
	if (len > 4)
	{
		if (_wcsnicmp(&_p_path[len - 4], L".d88", 5) == 0)
		{
			is_d88 = true;
		}
	}
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	DWORD dwObjectSize = 0, dwDataSize = 0;
	if (::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0) != 0)
	{
		return 1;
	}
	if (::BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&dwObjectSize), sizeof(DWORD), &dwDataSize, 0) != 0)
	{
		(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
		return 2;
	}
	std::vector<BYTE> vctHash(dwObjectSize, 0);
	if (::BCryptCreateHash(hAlg, &hHash, &vctHash[0], dwObjectSize, NULL, 0, 0) != 0)
	{
		(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
		return 3;
	}
	HANDLE handle = ::CreateFileW(_p_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
		return 4;
	}
	DWORD dwRead = 0;
	do
	{
		if (::ReadFile(handle, _p_buf, static_cast<DWORD>(_size_buf), &dwRead, NULL) == 0)
		{
			(void)::CloseHandle(handle);
			(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
			return 5;
		}
		if (is_d88)
		{
			is_d88 = false;
			memset(_p_buf, 0, 0x20);	// Clear Headder
		}
		if (dwRead != 0)
		{
		    if (::BCryptHashData(hHash, reinterpret_cast<PBYTE>(_p_buf), dwRead, 0) != 0)
			{
				(void)::CloseHandle(handle);
				(void)::BCryptDestroyHash(hHash);
				(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
				return 6;
			}
		}
	} while (dwRead != 0);
	(void)::CloseHandle(handle);
	if (::BCryptFinishHash(hHash, reinterpret_cast<PUCHAR>(_p_hash), static_cast<ULONG>(_size_hash), 0) != 0)
	{
		(void)::BCryptDestroyHash(hHash);
		(void)::BCryptCloseAlgorithmProvider(hAlg, 0);
		return 7;
	}
	(void)::BCryptDestroyHash(hHash);
    (void)::BCryptCloseAlgorithmProvider(hAlg, 0);
	return 0;
}
