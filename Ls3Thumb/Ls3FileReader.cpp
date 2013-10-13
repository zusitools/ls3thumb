#include "stdafx.h"
#include "Ls3FileReader.h"

Ls3File* Ls3FileReader::readLs3File(const char *fileName)
{
	Ls3File *result = new Ls3File();

	file<> xmlFile(fileName); // Default template is char
	xml_document<> doc;
	doc.parse<0>(xmlFile.data());

	xml_node<> *rootNode = doc.first_node("Zusi");
	if (!rootNode) {
		return result;
	}

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
	for (xml_node<> *subsetNode = landschaftNode.first_node("SubSet");
		subsetNode; subsetNode = subsetNode->next_sibling("SubSet"))
	{
		file.subsets.push_back(Ls3MeshSubset());
		Ls3MeshSubset &subset = file.subsets.back();

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
				subset.faceIndices.push_back(readIndices[i]);
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
