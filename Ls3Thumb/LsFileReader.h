#pragma once

#include "Ls3File.h"
#include <memory>

using namespace std;

class LsFileReader
{
public:
	static unique_ptr<Ls3File> readLs3File(const LPCWSTR fileName,
		const unsigned int maxElements = 0);
	static void getIncludedFiles(const LPCWSTR fileName,
		vector<wstring> &results);
};

