#include "LsFileReader.h"
#include <fstream>
#include <string>
#include <unordered_map>

#define INT64_TO_COLOR(colorVal) { static_cast<BYTE>(colorVal & 0xFF), \
	static_cast<BYTE>((colorVal >> 8) & 0xFF), static_cast<BYTE>((colorVal >> 16) & 0xFF), 1 }

// so we can use std::numeric_limits<>::max()
#undef max

#define SKIP_LINE(file) file.ignore(numeric_limits<streamsize>::max(), '\n')

unique_ptr<Ls3File> LsFileReader::readLs3File(const LPCWSTR fileName,
	const unsigned int maxElements)
{
	unique_ptr<Ls3File> result(new Ls3File());

	// Store subsets by their color.
	unordered_map<int64_t, Ls3MeshSubset> subsetsByColor;

	ifstream file;
	string tmp;

	file.open(fileName);

	SKIP_LINE(file); // Zusi version

	getline(file, tmp);
	int numElements = stoi(tmp);

	if (maxElements != 0 && numElements > maxElements)
	{
		numElements = maxElements;
	}

	string linkedName;
	getline(file, linkedName);
	while (linkedName.compare("#") && !file.eof()) {
		for (int i = 0; i < 6; i++) {
			SKIP_LINE(file);
		}

		getline(file, linkedName);
	}

	for (int elIndex = 0; elIndex < numElements; elIndex++) {
		getline(file, tmp);
		unsigned int numPoints = stoi(tmp);

		if (numPoints == 0) {
			// Light source
			for (int i = 0; i < 10; i++) {
				SKIP_LINE(file);
			}
		}
		else {
			SKIP_LINE(file);

			vector<ZUSIVERTEX> vertices;
			for (int i = 0; i < numPoints; i++) {
				vertices.push_back(ZUSIVERTEX());
				ZUSIVERTEX &vertex = vertices.back();
				ZeroMemory(&vertex, sizeof(vertex));

				string x, y, z;
				getline(file, x);
				getline(file, y);
				getline(file, z);

				if (x.find(',') != string::npos) {
					x.replace(x.find(','), 1, 1, '.');
				}
				if (y.find(',') != string::npos) {
					y.replace(y.find(','), 1, 1, '.');
				}
				if (z.find(',') != string::npos) {
					z.replace(z.find(','), 1, 1, '.');
				}

				vertex.pos = { stof(x), stof(y), stof(z) };
			}

			getline(file, tmp);
			int64_t color = stol(tmp);

			for (int i = 0; i < 6; i++) {
				SKIP_LINE(file);
			}

			if (subsetsByColor.find(color) == subsetsByColor.end()) {
				subsetsByColor.insert(std::make_pair(color, Ls3MeshSubset()));
				Ls3MeshSubset &subset = subsetsByColor.find(color)->second;

				subset.ambientColor = INT64_TO_COLOR(color);
				subset.diffuseColor = subset.ambientColor;

				// derived from Zusi 3 preset 1
				subset.renderFlags = { 0, 1, 1, 2, 0, 0,
					{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
					{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
					{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };
			}

			Ls3MeshSubset &subset = subsetsByColor.find(color)->second;

			int faceIndex = subset.vertices.size();
			for (int i = 0; i < vertices.size(); i++) {
				subset.vertices.push_back(vertices[i]);
			}

			for (int i = 1; i < vertices.size() - 1; i++) {
				// Reverse the order of the faces as Zusi 2 uses a LH
				// coordinate system
				subset.faceIndices.push_back(faceIndex);
				subset.faceIndices.push_back(faceIndex + i + 1);
				subset.faceIndices.push_back(faceIndex + i);
			}
		}
	}

	for (auto &colorAndSubset : subsetsByColor) {
		result->subsets.push_back(colorAndSubset.second);
	}

	return result;
}

void LsFileReader::getIncludedFiles(const LPCWSTR fileName,
	vector<wstring> &results)
{
}