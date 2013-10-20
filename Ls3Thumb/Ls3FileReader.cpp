#include "stdafx.h"
#include "Ls3FileReader.h"

#define LONG_LONG_TO_COLOR(colorVal) { colorVal & 0xFF, \
	(colorVal >> 8) & 0xFF, (colorVal >> 16) & 0xFF, (colorVal >> 24) & 0xFF }

unique_ptr<Ls3File> Ls3FileReader::readLs3File(LPCWSTR fileName)
{
	unique_ptr<Ls3File> result(new Ls3File());

	char fileNameChar[MAX_PATH];
	size_t i;
	wcstombs_s(&i, fileNameChar, MAX_PATH, fileName, MAX_PATH);

	xml_document<wchar_t> doc;

	try {
		file<wchar_t> xmlFile(fileNameChar);
		doc.parse<0>(xmlFile.data());
	
		xml_node<wchar_t> *rootNode = doc.first_node(L"Zusi");
		if (!rootNode) {
			return result;
		}

		// Retrieve directory name from file name
		lstrcpyn(result->dir, fileName, MAX_PATH);
		PathRemoveFileSpec(result->dir);

		readZusiNode(*result, *rootNode);
		return result;
	}
	catch (...) {
		return result;
	}
}

void Ls3FileReader::readZusiNode(Ls3File &file, xml_node<wchar_t> &zusiNode)
{
	xml_node<wchar_t> *landschaftNode = zusiNode.first_node(L"Landschaft");
	if (!landschaftNode) {
		return;
	}

	readLandschaftNode(file, *landschaftNode);
}

void Ls3FileReader::readLandschaftNode(Ls3File &file,
	xml_node<wchar_t> &landschaftNode)
{
	bool useLsbFile = false;
	HANDLE lsbFile;

	xml_node<wchar_t> *lsbNode = landschaftNode.first_node(L"lsb");
	if (lsbNode)
	{
		xml_attribute<wchar_t> *fileNameAttribute =
			lsbNode->first_attribute(L"Dateiname");

		if (fileNameAttribute)
		{
			wstring lsbPath = GetAbsoluteFilePath(fileNameAttribute->value(),
				file.dir);

			if (!lsbPath.empty())
			{
				lsbFile = CreateFile(lsbPath.c_str(), GENERIC_READ,
					FILE_SHARE_READ, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);

				if (lsbFile != INVALID_HANDLE_VALUE)
				{
					useLsbFile = true;
				}
			}
		}
	}

	for (xml_node<wchar_t> *childNode = landschaftNode.first_node();
		childNode; childNode = childNode->next_sibling())
	{
		if (wcsicmp(childNode->name(), L"SubSet") == 0) {
			readSubSetNode(file, useLsbFile, lsbFile, *childNode);
		}
		else if (wcsicmp(childNode->name(), L"Verknuepfte") == 0) {
			readVerknuepfteNode(file, *childNode);
		}
	}

	if (useLsbFile) {
		CloseHandle(lsbFile);
	}
}

void Ls3FileReader::readSubSetNode(Ls3File &file, bool useLsbFile,
	HANDLE lsbFile, xml_node<wchar_t> &subsetNode)
{
	file.subsets.push_back(Ls3MeshSubset());
	Ls3MeshSubset &subset = file.subsets.back();

	// Set default values
	ZeroMemory(&subset.ambientColor, sizeof(subset.ambientColor));
	ZeroMemory(&subset.diffuseColor, sizeof(subset.diffuseColor));
	ZeroMemory(&subset.renderFlags, sizeof(subset.renderFlags));

	xml_attribute<wchar_t> *diffuseAttribute = subsetNode.first_attribute(L"C");
	if (diffuseAttribute)
	{
		long long colorVal =
			wcstoll(diffuseAttribute->value(), (wchar_t**) NULL, 16);
		subset.diffuseColor = LONG_LONG_TO_COLOR(colorVal);
	}

	xml_attribute<wchar_t> *ambientAttribute = subsetNode.first_attribute(L"CA");
	if (ambientAttribute)
	{
		long long colorVal =
			wcstoll(ambientAttribute->value(), (wchar_t**) NULL, 16);
		subset.ambientColor = LONG_LONG_TO_COLOR(colorVal);
	}
	else
	{
		subset.ambientColor = subset.diffuseColor;
	}

	if (useLsbFile)
	{
		xml_attribute<wchar_t> *meshVAttribute =
			subsetNode.first_attribute(L"MeshV");
		if (!meshVAttribute) return;

		xml_attribute<wchar_t> *meshIAttribute =
			subsetNode.first_attribute(L"MeshI");
		if (!meshIAttribute) return;

		int numVertices = _wtoi(meshVAttribute->value());
		int numFaceIndices = _wtoi(meshIAttribute->value());

		// Insert code to read vertex and face data from LSB file here
	}

	for (xml_node<wchar_t> *node = subsetNode.first_node(); node;
		node = node->next_sibling())
	{
		if (wcsicmp(node->name(), L"Vertex") == 0)
		{
			readVertexNode(subset, *node);
		}
		else if (wcsicmp(node->name(), L"Face") == 0)
		{
			readFaceNode(subset, *node);
		}
		else if (wcsicmp(node->name(), L"Textur") == 0)
		{
			readTexturNode(subset, file, *node);
		}
		else if (wcsicmp(node->name(), L"RenderFlags") == 0)
		{
			readRenderFlagsNode(subset, *node);
		}
	}
}

void Ls3FileReader::readVerknuepfteNode(Ls3File &file,
	xml_node<wchar_t> &verknuepfteNode)
{
	xml_node<wchar_t> *dateiNode = verknuepfteNode.first_node(L"Datei");
	if (!dateiNode) {
		return;
	}

	xml_attribute<wchar_t> *fileNameAttribute =
		dateiNode->first_attribute(L"Dateiname");
	if (!fileNameAttribute)
	{
		return;
	}

	wstring filePath = GetAbsoluteFilePath(fileNameAttribute->value(),
			file.dir);
	if (filePath.empty())
	{
		return;
	}

	COORD3D pos, angle, scale;
	ZeroMemory(&pos, sizeof(pos));
	ZeroMemory(&angle, sizeof(angle));
	ZeroMemory(&scale, sizeof(scale));

	xml_node<wchar_t> *pNode = verknuepfteNode.first_node(L"p");
	if (pNode) {
		read3DCoordinates(pos, *pNode);
	}

	xml_node<wchar_t> *phiNode = verknuepfteNode.first_node(L"phi");
	if (phiNode) {
		read3DCoordinates(pos, *phiNode);
	}

	xml_node<wchar_t> *skNode = verknuepfteNode.first_node(L"sk");
	if (skNode) {
		read3DCoordinates(pos, *skNode);
	}

	unique_ptr<Ls3File> linkedFile = readLs3File(filePath.c_str());
	for (auto subset : linkedFile->subsets) {
		// TODO transform vertices
		file.subsets.push_back(subset);
	}
}

void Ls3FileReader::readVertexNode(Ls3MeshSubset &subset,
	xml_node<wchar_t> &vertexNode)
{
	subset.vertices.push_back(ZUSIVERTEX());
	ZUSIVERTEX& vertex = subset.vertices.back();
	ZeroMemory(&vertex, sizeof(ZUSIVERTEX));

	for (xml_node<wchar_t> *vertexChildNode = vertexNode.first_node();
		vertexChildNode; vertexChildNode = vertexChildNode->next_sibling())
	{
		if (wcsicmp(vertexChildNode->name(), L"p") == 0)
		{
			read3DCoordinates(vertex.pos, *vertexChildNode);
		}
		else if (wcsicmp(vertexChildNode->name(), L"n") == 0)
		{
			read3DCoordinates(vertex.normal, *vertexChildNode);
		}
	}
}

void Ls3FileReader::readFaceNode(Ls3MeshSubset &subset,
	xml_node<wchar_t> &faceNode)
{
	// The face indices are stored as semicolon-separated values
	// in the attribute "i"
	xml_attribute<wchar_t> *indexAttr = faceNode.first_attribute(L"i");
	if (indexAttr)
	{
		UINT32 readIndices[3];
		unsigned char curIndex = 0;

		wchar_t *context = NULL;
		wchar_t *token = wcstok_s(indexAttr->value(), L";", &context);
		while (curIndex < 3 && token != NULL)
		{
			readIndices[curIndex] = _wtoi(token);
			curIndex++;
			token = wcstok_s(NULL, L";", &context);
		}

		if (curIndex == 3)
		{
			for (int i = 0; i < 3; i++) {
				subset.faceIndices.push_back(readIndices[i]);
			}
		}
	}
}


void Ls3FileReader::readTexturNode(Ls3MeshSubset &subset, Ls3File &file,
	xml_node<wchar_t> &texturNode)
{
	xml_node<wchar_t> *dateiNode = texturNode.first_node(L"Datei");
	if (dateiNode) {
		xml_attribute<wchar_t> *dateinameAttribute = 
			dateiNode->first_attribute(L"Dateiname");
		if (dateinameAttribute) {
			wstring fileName = GetAbsoluteFilePath(dateinameAttribute->value(),
				file.dir);
			if (!fileName.empty()) {
				subset.textureFilenames.push_back(fileName);
			}
		}
	}
}


void Ls3FileReader::readRenderFlagsNode(Ls3MeshSubset &subset,
	xml_node<wchar_t> &renderFlagsNode)
{

}
	

void Ls3FileReader::read3DCoordinates(COORD3D &coords, xml_node<wchar_t> &node)
{
	for (xml_attribute<wchar_t> *attr = node.first_attribute();
		attr; attr = attr->next_attribute())
	{
		if (wcsicmp(attr->name(), L"x") == 0)
		{
			coords.x = _wtof(attr->value());
		}
		else if (wcsicmp(attr->name(), L"y") == 0)
		{
			coords.y = _wtof(attr->value());
		}
		else if (wcsicmp(attr->name(), L"z") == 0)
		{
			coords.z = _wtof(attr->value());
		}
	}
}

wstring Ls3FileReader::GetAbsoluteFilePath(LPCWSTR fileName,
	LPCWSTR parentFileDir)
{
	TCHAR result[MAX_PATH];

	// Relative to parent file directory
	PathCombine(result, parentFileDir, fileName);
	if (PathFileExists(result)) {
		return wstring(result);
	}

	// Relative to Zusi data path
	PathCombine(result, GetZusiDataPath().c_str(), fileName);
	if (PathFileExists(result)) {
		return wstring(result);
	}

	// Absolute path
	if (PathFileExists(fileName)) {
		return wstring(fileName);
	}

	return wstring();
}

wstring Ls3FileReader::GetZusiDataPath()
{
	LONG hr;
	HKEY hZusiKey;

	hr = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Zusi3", 0,
		KEY_READ | KEY_WOW64_32KEY, &hZusiKey);

	if (hr != ERROR_SUCCESS) {
		return wstring();
	}

	TCHAR result[MAX_PATH] = L"";
	DWORD dwType;
	DWORD dwDataSize = sizeof(result);
	
	hr = RegQueryValueEx(hZusiKey, L"DatenDir", NULL, NULL, (LPBYTE) result, &dwDataSize);
	if (hr != ERROR_SUCCESS)
	{
		hr = RegQueryValueEx(hZusiKey, L"DatenDirDemo", NULL, NULL, (LPBYTE) result, &dwDataSize);
	}

	RegCloseKey(hZusiKey);
	return wstring(result);
}
