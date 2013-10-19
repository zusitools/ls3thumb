#include "stdafx.h"
#include "Ls3FileReader.h"

#include "d3dx9.h" // TODO get rid of that

#define LONG_LONG_TO_COLOR(colorVal) { colorVal & 0xFF, \
	(colorVal >> 8) & 0xFF, (colorVal >> 16) & 0xFF, (colorVal >> 24) & 0xFF }

unique_ptr<Ls3File> Ls3FileReader::readLs3File(const LPCWSTR fileName,
	const unsigned char lodMask)
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

		readZusiNode(*result, *rootNode, lodMask);
		return result;
	}
	catch (...) {
		return result;
	}
}

void Ls3FileReader::getIncludedFiles(const LPCWSTR fileName,
	vector<wstring> &results)
{
	// Included files are collected from the following nodes:
	// Zusi - Landschaft - lsb
	// Zusi - Landschaft - Verknuepfte - Datei (recursive)
	// Zusi - Landschaft - SubSet - Textur - Datei
	
	// Retrieve directory name from file name
	wchar_t fileDir[MAX_PATH];
	lstrcpyn(fileDir, fileName, MAX_PATH);
	PathRemoveFileSpec(fileDir);

	char fileNameChar[MAX_PATH];
	size_t i;
	wcstombs_s(&i, fileNameChar, MAX_PATH, fileName, MAX_PATH);

	xml_document<wchar_t> doc;

	try {
		file<wchar_t> xmlFile(fileNameChar);
		doc.parse<0>(xmlFile.data());

		xml_node<wchar_t> *rootNode = doc.first_node(L"Zusi");
		if (!rootNode) {
			return;
		}

		xml_node<wchar_t> *landschaftNode =
			rootNode->first_node(L"Landschaft");
		if (!landschaftNode) {
			return;
		}

		// Zusi - Landschaft - lsb
		for (auto node = landschaftNode->first_node(L"lsb");
			node; node = node->next_sibling(L"lsb"))
		{
			xml_attribute<wchar_t> *dateinameAttribute =
				node->first_attribute(L"Dateiname");
			if (!dateinameAttribute)
			{
				continue;
			}

			wstring includedFilePath = GetAbsoluteFilePath(
				dateinameAttribute->value(), fileDir);
			if (PathFileExists(includedFilePath.c_str())) {
				results.push_back(wstring(includedFilePath));
			}
		}

		// Zusi - Landschaft - Verknuepfte - Datei
		for (auto node = landschaftNode->first_node(L"Verknuepfte");
			node; node = node->next_sibling(L"Verknuepfte"))
		{
			xml_node<wchar_t> *dateiNode = node->first_node(L"Datei");
			if (!dateiNode)
			{
				continue;
			}

			xml_attribute<wchar_t> *dateinameAttribute =
				dateiNode->first_attribute(L"Dateiname");
			if (dateinameAttribute)
			{
				wstring includedFilePath = GetAbsoluteFilePath(
					dateinameAttribute->value(), fileDir);
				if (PathFileExists(includedFilePath.c_str())) {
					results.push_back(wstring(includedFilePath));

					// TODO Prevent recursive inclusion
					Ls3FileReader::getIncludedFiles(includedFilePath.c_str(),
						results);
				}
			}
		}

		// Zusi - Landschaft - SubSet - Textur - Datei
		for (auto node = landschaftNode->first_node(L"SubSet");
			node; node = node->next_sibling(L"SubSet"))
		{
			for (auto texturNode = node->first_node(L"Textur");
				texturNode; texturNode = texturNode->next_sibling(L"Textur"))
			{
				xml_node<wchar_t> *dateiNode = texturNode->first_node(L"Datei");
				if (!dateiNode)
				{
					continue;
				}

				xml_attribute<wchar_t> *dateinameAttribute =
					dateiNode->first_attribute(L"Dateiname");
				if (dateinameAttribute)
				{
					wstring includedFilePath = GetAbsoluteFilePath(
						dateinameAttribute->value(), fileDir);
					if (PathFileExists(includedFilePath.c_str())) {
						results.push_back(wstring(includedFilePath));
					}
				}
			}
		}
	}
	catch (...) {
		return;
	}
}

void Ls3FileReader::readZusiNode(Ls3File &file,
	const xml_node<wchar_t> &zusiNode, const unsigned char lodMask)
{
	xml_node<wchar_t> *landschaftNode = zusiNode.first_node(L"Landschaft");
	if (!landschaftNode) {
		return;
	}

	readLandschaftNode(file, *landschaftNode, lodMask);
}

void Ls3FileReader::readLandschaftNode(Ls3File &file,
	const xml_node<wchar_t> &landschaftNode, const unsigned char lodMask)
{
	bool useLsbFile = false;
	HANDLE lsbFile = INVALID_HANDLE_VALUE;

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
		if (wcscmp(childNode->name(), L"SubSet") == 0) {
			readSubSetNode(file, useLsbFile, lsbFile, *childNode);
		}
		else if (wcscmp(childNode->name(), L"Verknuepfte") == 0) {
			readVerknuepfteNode(file, *childNode, lodMask);
		}
	}

	if (useLsbFile) {
		CloseHandle(lsbFile);
	}
}

void Ls3FileReader::readSubSetNode(Ls3File &file, const bool useLsbFile,
	const HANDLE lsbFile, const xml_node<wchar_t> &subsetNode)
{
	file.subsets.push_back(Ls3MeshSubset());
	Ls3MeshSubset &subset = file.subsets.back();

	// Set default values
	ZeroMemory(&subset.ambientColor, sizeof(subset.ambientColor));
	ZeroMemory(&subset.diffuseColor, sizeof(subset.diffuseColor));
	ZeroMemory(&subset.renderFlags, sizeof(subset.renderFlags));
	subset.lodMask = 0x0F; // visible in all LODs
	subset.zBias = 0.0f;

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

	xml_attribute<wchar_t> *zBiasAttribute = subsetNode.first_attribute(L"zBias");
	if (zBiasAttribute)
	{
		subset.zBias = _wtof(zBiasAttribute->value());
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

		FLOAT vertexData[10];
		DWORD numBytesRead;

		for (int i = 0; i < numVertices; i++)
		{
			ReadFile(lsbFile, vertexData, sizeof(vertexData),
				&numBytesRead, NULL);

			subset.vertices.push_back(ZUSIVERTEX());
			ZUSIVERTEX& vertex = subset.vertices.back();
			ZeroMemory(&vertex, sizeof(ZUSIVERTEX));

			vertex.pos.x = vertexData[0];
			vertex.pos.y = vertexData[1];
			vertex.pos.z = vertexData[2];
			vertex.normal.x = vertexData[3];
			vertex.normal.y = vertexData[4];
			vertex.normal.z = vertexData[5];
			vertex.tex1.u = vertexData[6];
			vertex.tex1.v = vertexData[7];
		}

		UINT16 faceIndices[3];

		for (int i = 0; i < numFaceIndices / 3; i++)
		{
			ReadFile(lsbFile, faceIndices, sizeof(faceIndices),
				&numBytesRead, NULL);

			subset.faceIndices.push_back(faceIndices[2]);
			subset.faceIndices.push_back(faceIndices[1]);
			subset.faceIndices.push_back(faceIndices[0]);
		}
	}

	for (xml_node<wchar_t> *node = subsetNode.first_node(); node;
		node = node->next_sibling())
	{
		if (wcscmp(node->name(), L"Vertex") == 0)
		{
			readVertexNode(subset, *node);
		}
		else if (wcscmp(node->name(), L"Face") == 0)
		{
			readFaceNode(subset, *node);
		}
		else if (wcscmp(node->name(), L"Textur") == 0)
		{
			readTexturNode(subset, file, *node);
		}
		else if (wcscmp(node->name(), L"RenderFlags") == 0)
		{
			readRenderFlagsNode(subset, *node);
		}
	}
}

void Ls3FileReader::readVerknuepfteNode(Ls3File &file,
	const xml_node<wchar_t> &verknuepfteNode, const unsigned char lodMask)
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

	BYTE linkedLodMask = 0x0F;
	xml_attribute<wchar_t> *lodAttribute =
		verknuepfteNode.first_attribute(L"LODbit");
	if (lodAttribute) {
		linkedLodMask = (BYTE) _wtoi(lodAttribute->value());
	}

	if ((lodMask & linkedLodMask) == 0) {
		return;
	}

	COORD3D pos, angle, scale;
	ZeroMemory(&pos, sizeof(pos));
	ZeroMemory(&angle, sizeof(angle));
	scale = { 1.0f, 1.0f, 1.0f };

	xml_node<wchar_t> *pNode = verknuepfteNode.first_node(L"p");
	if (pNode) {
		read3DCoordinates(pos, *pNode);
	}

	xml_node<wchar_t> *phiNode = verknuepfteNode.first_node(L"phi");
	if (phiNode) {
		read3DCoordinates(angle, *phiNode);
	}

	xml_node<wchar_t> *skNode = verknuepfteNode.first_node(L"sk");
	if (skNode) {
		read3DCoordinates(scale, *skNode);
	}

	unique_ptr<Ls3File> linkedFile = readLs3File(filePath.c_str(), lodMask);
	for (auto &subset : linkedFile->subsets) {
		subset.lodMask &= linkedLodMask;

		for (auto &vertex : subset.vertices)
		{
			// Scale
			vertex.pos.x *= scale.x;
			vertex.pos.y *= scale.y;
			vertex.pos.z *= scale.z;

			// Rotate
			// TODO Hack, this should not depend on D3DX and is generally ugly.
			if (angle.x != 0 || angle.y != 0 || angle.z != 0)
			{
				D3DXVECTOR3 vtmp = { vertex.pos.x, vertex.pos.y,
					vertex.pos.z };
				D3DXVECTOR3 ntmp = { vertex.normal.x, vertex.normal.y,
					vertex.normal.z };
				D3DXVECTOR3 vresult, nresult;

				D3DXMATRIX rotX, rotY, rotZ, rot, tmp, normrot;
				D3DXMatrixRotationX(&rotX, angle.x);
				D3DXMatrixRotationY(&rotY, angle.y);
				D3DXMatrixRotationZ(&rotZ, angle.z);
				rot = rotX * rotY * rotZ;

				D3DXMatrixTranspose(&tmp, &rot);
				D3DXMatrixInverse(&normrot, NULL, &tmp);

				D3DXVec3TransformCoord(&vresult, &vtmp, &rot);
				D3DXVec3TransformNormal(&nresult, &ntmp, &normrot);

				vertex.pos = { vresult.x, vresult.y, vresult.z };
				vertex.normal = { nresult.x, nresult.y, nresult.z };
			}

			// Translate
			vertex.pos.x += pos.x;
			vertex.pos.y += pos.y;
			vertex.pos.z += pos.z;
		}

		file.subsets.push_back(subset);
	}
}

void Ls3FileReader::readVertexNode(Ls3MeshSubset &subset,
	const xml_node<wchar_t> &vertexNode)
{
	subset.vertices.push_back(ZUSIVERTEX());
	ZUSIVERTEX& vertex = subset.vertices.back();
	ZeroMemory(&vertex, sizeof(ZUSIVERTEX));

	for (xml_node<wchar_t> *vertexChildNode = vertexNode.first_node();
		vertexChildNode; vertexChildNode = vertexChildNode->next_sibling())
	{
		if (wcscmp(vertexChildNode->name(), L"p") == 0)
		{
			read3DCoordinates(vertex.pos, *vertexChildNode);
		}
		else if (wcscmp(vertexChildNode->name(), L"n") == 0)
		{
			read3DCoordinates(vertex.normal, *vertexChildNode);
		}
	}

	for (xml_attribute<wchar_t> *attr = vertexNode.first_attribute();
		attr; attr = attr->next_attribute())
	{
		if (wcscmp(attr->name(), L"U1") == 0) {
			vertex.tex1.u = _wtof(attr->value());
		}
		else if (wcscmp(attr->name(), L"V1") == 0) {
			vertex.tex1.v = _wtof(attr->value());
		}
		else if (wcscmp(attr->name(), L"U2") == 0) {
			vertex.tex2.u = _wtof(attr->value());
		}
		else if (wcscmp(attr->name(), L"V2") == 0) {
			vertex.tex2.v = _wtof(attr->value());
		}
	}
}

void Ls3FileReader::readFaceNode(Ls3MeshSubset &subset,
	const xml_node<wchar_t> &faceNode)
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
	const xml_node<wchar_t> &texturNode)
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
	const xml_node<wchar_t> &renderFlagsNode)
{
	Ls3RenderFlags Ls3TexturePresets [] =
	{
		// Preset 1
		{ 0, 1, 1, 2, 0, 0,
		{ 4, 0, 2, 0, 1, 0, 0, 0, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }},

		// Preset 2
		{ 1, 5, 6, 2, 0, 1,
		{ 4, 0, 2, 0, 2, 0, 2, 0, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 3
		{ 0, 1, 1, 2, 0, 0,
		{ 4, 0, 2, 0, 1, 0, 0, 0, 3, 3, 5 },
		{ 4, 0, 2, 0, 2, 0, 2, 0, 3, 3, 1 },
		{ 16, 0, 1, 5, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 4
		{ 1, 5, 6, 2, 1, 1,
		{ 4, 0, 2, 0, 2, 0, 2, 0, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 5
		{ 1, 1, 1, 2, 0, 1,
		{ 4, 0, 2, 0, 2, 0, 2, 0, 3, 3, 1 },
		{ 13, 0, 2, 0, 1, 0, 0, 0, 3, 3, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 6
		{ 1, 6, 3, 2, 1, 1,
		{ 4, 0, 2, 0, 14, 0, 2, 0, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 7
		{ 1, 5, 6, 2, 1, 1,
		{ 4, 0, 2, 0, 2, 0, 2, 3, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 8
		{ 100, 5, 6, 2, 1, 1,
		{ 4, 0, 2, 0, 4, 0, 2, 0, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },

		// Preset 9
		{ 1, 5, 6, 2, 1, 1,
		{ 4, 0, 2, 0, 2, 0, 2, 3, 3, 3, 1 },
		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
		{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } },
	};

	unsigned char texPreset = 0;
	xml_attribute<wchar_t> *texPresetAttribute =
		renderFlagsNode.first_attribute(L"TexVoreinstellung");
	if (texPresetAttribute) {
		texPreset = _wtoi(texPresetAttribute->value());
	}

	if (texPreset > 0) {
		if (texPreset <= 9) {
			subset.renderFlags = Ls3TexturePresets[texPreset - 1];
		}
		return;
	}

	for (xml_attribute<wchar_t> *attr = renderFlagsNode.first_attribute();
		attr; attr = attr->next_attribute())
	{
		DWORD value = _wtoi(attr->value());
		if (wcscmp(attr->name(), L"ALPHAREF") == 0) {
			subset.renderFlags.ALPHAREF = value;
		}
		else if (wcscmp(attr->name(), L"ALPHABLENDENABLE") == 0) {
			subset.renderFlags.ALPHABLENDENABLE = value;
		}
		else if (wcscmp(attr->name(), L"ALPHATESTENABLE") == 0) {
			subset.renderFlags.ALPHATESTENABLE = value;
		}
		else if (wcscmp(attr->name(), L"SRCBLEND") == 0) {
			subset.renderFlags.SRCBLEND = value;
		}
		else if (wcscmp(attr->name(), L"DESTBLEND") == 0) {
			subset.renderFlags.DESTBLEND = value;
		}
		else if (wcscmp(attr->name(), L"SHADEMODE") == 0) {
			subset.renderFlags.SHADEMODE = value;
		}
	}

	for (xml_node<wchar_t> *node = renderFlagsNode.first_node();
		node; node = node->next_sibling())
	{
		if (wcscmp(node->name(), L"SubSetTexFlags") == 0) {
			readSubSetTexFlagsNode(subset.renderFlags.stage1, *node);
		}
		else if (wcscmp(node->name(), L"SubSetTexFlags2") == 0) {
			readSubSetTexFlagsNode(subset.renderFlags.stage2, *node);
		}
		else if (wcscmp(node->name(), L"SubSetTexFlags3") == 0) {
			readSubSetTexFlagsNode(subset.renderFlags.stage3, *node);
		}
	}
}


void Ls3FileReader::readSubSetTexFlagsNode(TEXSTAGESETTING &texStageSetting,
	const xml_node<wchar_t> &texFlagsNode)
{
	for (xml_attribute<wchar_t> *attr = texFlagsNode.first_attribute();
		attr; attr = attr->next_attribute())
	{
		DWORD value = _wtoi(attr->value());
		if (wcscmp(attr->name(), L"COLOROP") == 0) {
			texStageSetting.COLOROP = value;
		}
		else if (wcscmp(attr->name(), L"COLORARG0") == 0) {
			texStageSetting.COLORARG0 = value;
		}
		else if (wcscmp(attr->name(), L"COLORARG1") == 0) {
			texStageSetting.COLORARG1 = value;
		}
		else if (wcscmp(attr->name(), L"COLORARG2") == 0) {
			texStageSetting.COLORARG2 = value;
		}
		else if (wcscmp(attr->name(), L"ALPHAOP") == 0) {
			texStageSetting.ALPHAOP = value;
		}
		else if (wcscmp(attr->name(), L"ALPHAARG0") == 0) {
			texStageSetting.ALPHAARG0 = value;
		}
		else if (wcscmp(attr->name(), L"ALPHAARG1") == 0) {
			texStageSetting.ALPHAARG1 = value;
		}
		else if (wcscmp(attr->name(), L"ALPHAARG2") == 0) {
			texStageSetting.ALPHAARG2 = value;
		}
		else if (wcscmp(attr->name(), L"MAGFILTER") == 0) {
			texStageSetting.MAGFILTER = value;
		}
		else if (wcscmp(attr->name(), L"MINFILTER") == 0) {
			texStageSetting.MINFILTER = value;
		}
		else if (wcscmp(attr->name(), L"RESULTARG") == 0) {
			texStageSetting.RESULTARG = value;
		}
	}
}
	

void Ls3FileReader::read3DCoordinates(COORD3D &coords,
	const xml_node<wchar_t> &node)
{
	for (xml_attribute<wchar_t> *attr = node.first_attribute();
		attr; attr = attr->next_attribute())
	{
		if (wcscmp(attr->name(), L"X") == 0)
		{
			coords.x = _wtof(attr->value());
		}
		else if (wcscmp(attr->name(), L"Y") == 0)
		{
			coords.y = _wtof(attr->value());
		}
		else if (wcscmp(attr->name(), L"Z") == 0)
		{
			coords.z = _wtof(attr->value());
		}
	}
}

wstring Ls3FileReader::GetAbsoluteFilePath(const LPCWSTR fileName,
	const LPCWSTR parentFileDir)
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
	TCHAR result[MAX_PATH] = L"";
	DWORD dwDataSize = sizeof(result);

	// Try to read from VirtualStore first. (As this is a 64 bit plugin,
	// it will not get redirected to VirtualStore.)
	hr = RegOpenKeyEx(HKEY_CURRENT_USER,
		L"Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\Wow6432Node\\Zusi3",
		0, KEY_READ | KEY_WOW64_32KEY, &hZusiKey);
	if (hr == ERROR_SUCCESS)
	{
		hr = RegQueryValueEx(hZusiKey, L"DatenDir", NULL, NULL,
			(LPBYTE) result, &dwDataSize);
		if (hr != ERROR_SUCCESS)
		{
			dwDataSize = sizeof(result);
			hr = RegQueryValueEx(hZusiKey, L"DatenDirDemo", NULL, NULL,
				(LPBYTE) result, &dwDataSize);
		}

		if (hr == ERROR_SUCCESS)
		{
			return wstring(result);
		}
	}

	hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Zusi3", 0,
		KEY_READ | KEY_WOW64_32KEY, &hZusiKey);

	if (hr != ERROR_SUCCESS) {
		return wstring();
	}
	
	hr = RegQueryValueEx(hZusiKey, L"DatenDir", NULL, NULL,
		(LPBYTE) result, &dwDataSize);
	if (hr != ERROR_SUCCESS)
	{
		dwDataSize = sizeof(result);
		hr = RegQueryValueEx(hZusiKey, L"DatenDirDemo", NULL, NULL,
			(LPBYTE) result, &dwDataSize);
	}

	RegCloseKey(hZusiKey);
	return wstring(result);
}
