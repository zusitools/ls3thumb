#include "stdafx.h"
#include "Ls3FileReader.h"

unique_ptr<Ls3File> Ls3FileReader::readLs3File(LPCWSTR fileName)
{
	unique_ptr<Ls3File> result(new Ls3File());

	char fileNameChar[MAX_PATH];
	size_t i;
	wcstombs_s(&i, fileNameChar, MAX_PATH, fileName, MAX_PATH);

	file<> xmlFile(fileNameChar); // Default template is char
	xml_document<> doc;
	try {
		doc.parse<0>(xmlFile.data());
	} catch (...) {
		return result ;
	}

	xml_node<> *rootNode = doc.first_node("Zusi");
	if (!rootNode) {
		return result;
	}

	// Retrieve directory name from file name
	lstrcpyn(result->dir, fileName, MAX_PATH);
	PathRemoveFileSpec(result->dir);

	readZusiNode(*result, *rootNode);
	return result;
}

void Ls3FileReader::readZusiNode(Ls3File &file, xml_node<> &zusiNode)
{
	xml_node<> *landschaftNode = zusiNode.first_node("Landschaft");
	if (!landschaftNode) {
		return;
	}

	readLandschaftNode(file, *landschaftNode);
}

void Ls3FileReader::readLandschaftNode(Ls3File &file, xml_node<> &landschaftNode)
{
	bool useLsbFile = false;
	HANDLE lsbFile;

	xml_node<> *lsbNode = landschaftNode.first_node("lsb");
	if (lsbNode)
	{
		xml_attribute<> *fileNameAttribute =
			lsbNode->first_attribute("Dateiname");

		// TODO this sucks
		const char *fileName = fileNameAttribute->value();
		WCHAR fileNameWide[MAX_PATH];
		size_t i;
		mbstowcs_s(&i, fileNameWide, MAX_PATH, fileName, MAX_PATH);

		if (fileNameAttribute && GetFileByZusiPathSpec(fileNameWide, file.dir,
			lsbFile))
		{
			useLsbFile = true;
		}
	}

	for (xml_node<> *subsetNode = landschaftNode.first_node("SubSet");
		subsetNode; subsetNode = subsetNode->next_sibling("SubSet"))
	{
		file.subsets.push_back(Ls3MeshSubset());
		Ls3MeshSubset &subset = file.subsets.back();

		// Set default color values
		subset.ambientColor = 0;
		subset.diffuseColor = 0;

		xml_attribute<> *diffuseAttribute = subsetNode->first_attribute("C");
		if (diffuseAttribute)
		{
			long long colorVal = strtoll(diffuseAttribute->value(), (char**) NULL, 16);
			subset.diffuseColor = RGB(colorVal & 0xFF, (colorVal >> 8) & 0xFF,
				(colorVal >> 16) & 0xFF);
			// TODO alpha
		}

		xml_attribute<> *ambientAttribute = subsetNode->first_attribute("CA");
		if (ambientAttribute)
		{
			long long colorVal = strtoll(ambientAttribute->value(), (char**) NULL, 16);
			subset.ambientColor = RGB(colorVal & 0xFF, (colorVal >> 8) & 0xFF,
				(colorVal >> 16) & 0xFF);
			// TODO alpha
		}
		else
		{
			subset.ambientColor = subset.diffuseColor;
		}

		if (useLsbFile)
		{
			xml_attribute<> *meshVAttribute =
				subsetNode->first_attribute("MeshV");
			if (!meshVAttribute) continue;

			xml_attribute<> *meshIAttribute =
				subsetNode->first_attribute("MeshI");
			if (!meshIAttribute) continue;

			int numVertices = atoi(meshVAttribute->value());
			int numFaceIndices = atoi(meshIAttribute->value());

			// TODO read vertices and face indices from LSB file
		}
		else
		{
			for (xml_node<> *node = subsetNode->first_node(); node;
				node = node->next_sibling())
			{
				if (_stricmp(node->name(), "Vertex") == 0)
				{
					readVertexNode(subset, *node);
				}
				else if (_stricmp(node->name(), "Face") == 0)
				{
					readFaceNode(subset, *node);
				}
			}
		}
	}
}

void Ls3FileReader::readVertexNode(Ls3MeshSubset &subset, xml_node<> &vertexNode)
{
	subset.vertices.push_back(ZUSIVERTEX());
	ZUSIVERTEX& vertex = subset.vertices.back();
	ZeroMemory(&vertex, sizeof(ZUSIVERTEX));

	for (xml_node<> *vertexChildNode = vertexNode.first_node();
		vertexChildNode; vertexChildNode = vertexChildNode->next_sibling())
	{
		if (_stricmp(vertexChildNode->name(), "p") == 0)
		{
			read3DCoordinates(vertex.pos, *vertexChildNode);
		}
		else if (_stricmp(vertexChildNode->name(), "n") == 0)
		{
			read3DCoordinates(vertex.normal, *vertexChildNode);
		}
	}
}

void Ls3FileReader::readFaceNode(Ls3MeshSubset &subset, xml_node<> &faceNode)
{
	// The face indices are stored as semicolon-separated values
	// in the attribute "i"
	xml_attribute<> *indexAttr = faceNode.first_attribute("i");
	if (indexAttr)
	{
		UINT32 readIndices[3];
		unsigned char curIndex = 0;

		char *context = NULL;
		char *token = strtok_s(indexAttr->value(), ";", &context);
		while (curIndex < 3 && token != NULL)
		{
			readIndices[curIndex] = atoi(token);
			curIndex++;
			token = strtok_s(NULL, ";", &context);
		}

		if (curIndex == 3)
		{
			for (int i = 0; i < 3; i++) {
				// We seem to have to reverse the direction of the indices.
				subset.faceIndices.push_back(readIndices[2 - i]);
			}
		}
	}
}
	

void Ls3FileReader::read3DCoordinates(COORD3D &coords, xml_node<> &node)
{
	for (xml_attribute<> *attr = node.first_attribute();
		attr; attr = attr->next_attribute())
	{
		if (_stricmp(attr->name(), "x") == 0)
		{
			coords.x = atof(attr->value());
		}
		else if (_stricmp(attr->name(), "y") == 0)
		{
			coords.y = atof(attr->value());
		}
		else if (_stricmp(attr->name(), "z") == 0)
		{
			coords.z = atof(attr->value());
		}
	}
}

bool Ls3FileReader::GetFileByZusiPathSpec(LPCWSTR fileName,
	LPCWSTR parentFileDir, HANDLE &file)
{
	TCHAR sameDirName[MAX_PATH];
	PathCombine(sameDirName, parentFileDir, fileName);

	if (PathFileExists(sameDirName)) {
		file = CreateFile(sameDirName, GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		return true;
	}

	// TODO
	return false;
}
