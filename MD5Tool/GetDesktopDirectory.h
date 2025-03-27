/*!
 * @file GetDesktopDirectory.h
 * @brief デスクトップのディレクトリ名取得
 * @date 2025
 */
#pragma once
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief デスクトップのディレクトリ名取得
 * @param[in]	_size		ディレクトリ名のサイズ(MAX_PATH以上を指定する)
 * @param[out]	_p_dir		ディレクトリ名
 * @return 0:成功, 1:SHGetSpecialFolderPathW
 */
extern int GetDesktopDirectory(const size_t _size, wchar_t* _p_dir);

#ifdef __cplusplus
}
#endif
