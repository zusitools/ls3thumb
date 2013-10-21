#pragma once

#include "Ls3File.h"
#include <memory>

#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"

using namespace rapidxml;
using namespace std;

class Ls3FileReader
{
public:
	static unique_ptr<Ls3File> readLs3File(const LPCWSTR fileName,
		const unsigned char lodMask = 0xF);
	static void getIncludedFiles(const LPCWSTR fileName,
		vector<wstring> &results);

private:
	static void readZusiNode(Ls3File &file, const xml_node<wchar_t> &zusiNode,
		const unsigned char lodMask);
	static void readLandschaftNode(Ls3File &file,
		const xml_node<wchar_t> &landschaftNode, const unsigned char lodMask);
	static void readSubSetNode(Ls3File &file, const bool useLsbFile,
		const HANDLE lsbFile, const xml_node<wchar_t> &subsetNode);
	static void readVerknuepfteNode(Ls3File &file,
		const xml_node<wchar_t> &verknuepfteNode, const unsigned char lodMask);
	static void readVertexNode(Ls3MeshSubset &subset,
		const xml_node<wchar_t> &vertexNode);
	static void readFaceNode(Ls3MeshSubset &subset,
		const xml_node<wchar_t> &vertexNode);
	static void readTexturNode(Ls3MeshSubset &subset, Ls3File &file,
		const xml_node<wchar_t> &texturNode);
	static void readRenderFlagsNode(Ls3MeshSubset &subset,
		const xml_node<wchar_t> &renderFlagsNode);
	static void readSubSetTexFlagsNode(TEXSTAGESETTING &texStageSetting,
		const xml_node<wchar_t> &texFlagsNode);

	static void read3DCoordinates(COORD3D &coords,
		const xml_node<wchar_t> &node);
	static wstring GetAbsoluteFilePath(const LPCWSTR fileName,
		const LPCWSTR parentFileDir);
	static wstring GetZusiDataPath();
};
