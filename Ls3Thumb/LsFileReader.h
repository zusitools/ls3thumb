#pragma once

#include "Ls3File.h"
#include <memory>

using namespace std;

class LsFileReader
{
public:
	static unique_ptr<Ls3File> readLs3File(const LPCWSTR fileName);
	static void getIncludedFiles(const LPCWSTR fileName,
		vector<wstring> &results);
};

