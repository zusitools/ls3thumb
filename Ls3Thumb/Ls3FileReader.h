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
	static unique_ptr<Ls3File> readLs3File(LPCWSTR fileName);

private:
	static void readZusiNode(Ls3File &file, xml_node<wchar_t> &zusiNode);
	static void readLandschaftNode(Ls3File &file,
		xml_node<wchar_t> &landschaftNode);
	static void readVertexNode(Ls3MeshSubset &subset,
		xml_node<wchar_t> &vertexNode);
	static void readFaceNode(Ls3MeshSubset &subset,
		xml_node<wchar_t> &vertexNode);

	static void read3DCoordinates(COORD3D &coords, xml_node<wchar_t> &node);
	static wstring GetAbsoluteFilePath(LPCWSTR fileName,
		LPCWSTR parentFileDir);
	static wstring GetZusiDataPath();
};
