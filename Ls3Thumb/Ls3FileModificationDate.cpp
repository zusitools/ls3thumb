#include "stdafx.h"
#include "Ls3FileModificationDate.h"
#include "Ls3FileReader.h"

using namespace std;

void Ls3FileModificationDate::GetLastModificationDate(LPWSTR fileName,
	FILETIME *pResult)
{
	// Get the file's date stamp as the maximum of the date stamps of
	// all included files and the date stamp of the file itself.

	vector<wstring> includedFiles;
	Ls3FileReader::getIncludedFiles(fileName, includedFiles);

	HANDLE fileHandle = CreateFile(fileName, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	GetFileTime(fileHandle, NULL, NULL, pResult);
	CloseHandle(fileHandle);

	FILETIME includedFileDate;
	for (auto &includedFile : includedFiles) {
		fileHandle = CreateFile(includedFile.c_str(), GENERIC_READ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE) {
			continue;
		}

		GetFileTime(fileHandle, NULL, NULL, &includedFileDate);
		CloseHandle(fileHandle);

		if (CompareFileTime(&includedFileDate, pResult) > 0) {
			*pResult = FILETIME(includedFileDate);
		}
	}
}