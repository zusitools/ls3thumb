#pragma once

#include "Ls3File.h"

#include "rapidxml-1.13/rapidxml.hpp"
#include "rapidxml-1.13/rapidxml_utils.hpp"

using namespace rapidxml;

class Ls3FileReader
{
public:
	static Ls3File* readLs3File(const char *fileName);

private:
	static void readZusiNode(Ls3File &file, xml_node<> &zusiNode);
	static void readLandschaftNode(Ls3File &file, xml_node<> &landschaftNode);
	static void readVertexNode(Ls3MeshSubset &subset, xml_node<> &vertexNode);
	static void readFaceNode(Ls3MeshSubset &subset, xml_node<> &vertexNode);

	static void read3DCoordinates(COORD3D &coords, xml_node<> &node);
};
