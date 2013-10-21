#pragma once

#include "windows.h"

class Ls3FileModificationDate
{
public:
	static void GetLastModificationDate(LPWSTR fileName, FILETIME *pResult);
};

