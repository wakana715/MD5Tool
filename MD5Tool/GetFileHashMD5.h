/*!
 * @file GetFileHashMD5.h
 * @brief ファイルのハッシュ値(MD5)を取得
 * @date 2025
 */
#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	MD5_SIZE	(16)

/*!
 * @brief ファイルのハッシュ値(MD5)を取得
 * @param[in]	_size_path	ファイルパスのバイト数(MAX_PATH以上をメモリ確保する)
 * @param[in]	_p_path		ファイルパス
 * @param[in]	_size_buf	読込バッファのバイト数(32バイト以上をメモリ確保する)
 * @param[out]	_p_buf		読込バッファ
 * @param[in]	_size_hash	ハッシュ値のバイト数(MD5_SIZE以上をメモリ確保する)
 * @param[out]	_p_hash		ハッシュ値
 * @return 0:成功, 1:CryptAcquireContextW失敗, 2:CryptCreateHash失敗, 3:CreateFileW失敗, 4:ReadFile失敗, 5:CryptHashData失敗, 6:CryptGetHashParam失敗
 */
extern int GetFileHashMD5(const size_t _size_path, const wchar_t* _p_path, const size_t _size_buf, char* _p_buf, const size_t _size_hash, unsigned char* _p_hash);

#ifdef __cplusplus
}
#endif
