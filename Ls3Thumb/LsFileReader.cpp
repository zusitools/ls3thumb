#include "stdafx.h"
#include "LsFileReader.h"
#include <fstream>
#include <string>
#include <unordered_map>

#define INT64_TO_COLOR(colorVal) { colorVal & 0xFF, \
	(colorVal >> 8) & 0xFF, (colorVal >> 16) & 0xFF, 1 }

unique_ptr<Ls3File> LsFileReader::readLs3File(const LPCWSTR fileName)
{
	char fileNameChar[MAX_PATH];
	size_t i;
	wcstombs_s(&i, fileNameChar, MAX_PATH, fileName, MAX_PATH);

	unique_ptr<Ls3File> result(new Ls3File());

	// Store subsets by their color.
	unordered_map<int64_t, Ls3MeshSubset> subsets;

	ifstream file;
	string tmp;

	file.open(fileName);

	string zusiVersion;
	file >> zusiVersion;

	int numElements;
	file >> numElements;

	string linkedName;
	file >> linkedName;
	while (linkedName.compare("#")) {
		float x, y, z, phix, phiy, phiz;
		file >> x >> y >> z >> phix >> phiy >> phiz;
		file >> linkedName;
	}

	for (int elIndex = 0; elIndex < numElements; elIndex++) {
		int numPoints;
		file >> numPoints;

		if (numPoints == 0) {
			// Light source
			for (int i = 0; i < 10; i++) {
				file >> tmp;
			}
		}
		else {
			file >> tmp;

			vector<ZUSIVERTEX> vertices;
			for (int i = 0; i < numPoints; i++) {
				vertices.push_back(ZUSIVERTEX());
				ZUSIVERTEX &vertex = vertices.back();
				ZeroMemory(&vertex, sizeof(vertex));
				string x, y, z;
				file >> x >> y >> z;
				x.replace(x.find(','), 1, 1, '.');
				y.replace(y.find(','), 1, 1, '.');
				z.replace(z.find(','), 1, 1, '.');

				vertex.pos = { stof(x), stof(y), stof(z) };
				vertex.normal = { -1, -1, -1 };
			}

			int64_t color;
			file >> color;

			for (int i = 0; i < 6; i++) {
				file >> tmp;
			}

			if (subsets.find(color) == subsets.end()) {
				subsets.insert(std::make_pair(color, Ls3MeshSubset()));
			}
			Ls3MeshSubset &subset = subsets.find(color)->second;
			subset.ambientColor = INT64_TO_COLOR(color);
			subset.diffuseColor = subset.ambientColor;

			subset.renderFlags = { 0, 1, 1, 2, 0, 0,
			{ 4, 0, 2, 0, 1, 0, 0, 0, 3, 3, 1 },
			{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
			{ 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 } };

			int faceIndex = subset.vertices.size();
			for (int i = 0; i < vertices.size(); i++) {
				subset.vertices.push_back(ZUSIVERTEX(vertices[i]));
			}

			for (int i = 1; i < vertices.size() - 1; i++) {
				subset.faceIndices.push_back(faceIndex + i);
				subset.faceIndices.push_back(faceIndex + i + 1);
				subset.faceIndices.push_back(faceIndex);
			}
		}
	}

	for (auto &subset : subsets) {
		result->subsets.push_back(subset.second);
	}

	return result;
}

void LsFileReader::getIncludedFiles(const LPCWSTR fileName,
	vector<wstring> &results)
{
}