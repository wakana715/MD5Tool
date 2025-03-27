/*!
 * @file GetDesktopDirectory.h
 * @brief デスクトップのディレクトリ名取得
 * @date 2025
 */
#include "GetDesktopDirectory.h"
#include <windows.h>
#include <shlobj_core.h>

int GetDesktopDirectory(const size_t _size, wchar_t* _p_dir)
{
	if (::SHGetSpecialFolderPathW(NULL, _p_dir, CSIDL_DESKTOP, 0) != TRUE)
	{
		return 1;
	}
	int len = static_cast<int>(_size / sizeof(wchar_t));
	_p_dir[len - 1] = L'\0';
	return 0;
}
