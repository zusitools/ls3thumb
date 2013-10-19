#pragma once

#include "minwindef.h"
#include <vector>

using namespace std;

struct COORD3D {
	FLOAT x, y, z;
};

struct COORD2D {
	FLOAT u, v;
};

struct ZUSIVERTEX {
	COORD3D pos;
	COORD3D normal;
	COORD2D tex1;
};

/**
 * A mesh subset.
 */
struct Ls3MeshSubset
{
	/**
	 * The vertices of this subset.
	 */
	vector<ZUSIVERTEX> vertices;

	/**
	 * The vertex indices of the faces. Each 3 indices form a face.
	 */
	vector<UINT32> faceIndices;

	/**
	* The ambient color of this subset.
	*/
	COLORREF ambientColor;

	/**
	 * The diffuse color of this subset.
	 */
	COLORREF diffuseColor;

	/**
	 * The file names of the textures used in this subset.
	 */
	vector<wstring> textureFilenames;

	// TODO renderFlags
};

/**
 * A LS3 file containing one or more mesh subsets
 * and optional linked files.
 */
struct Ls3File
{
	/**
	 * The mesh subsets of this file.
	 */
	std::vector<Ls3MeshSubset> subsets;

	/**
	 * The directory this file lies in.
	 */
	WCHAR dir[MAX_PATH];
};
