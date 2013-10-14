#pragma once

#include "minwindef.h"
#include <vector>

struct COORD3D {
	FLOAT x, y, z;
};

struct ZUSIVERTEX {
	COORD3D pos;
	COORD3D normal;
	// FLOAT U1, U2;
};

/**
 * A mesh subset.
 */
struct Ls3MeshSubset
{
	/**
	 * The vertices of this subset.
	 */
	std::vector<ZUSIVERTEX> vertices;

	/**
	 * The vertex indices of the faces. Each 3 indices form a face.
	 */
	std::vector<UINT32> faceIndices;

	/**
	* The ambient color of this subset.
	*/
	COLORREF ambientColor;

	/**
	 * The diffuse color of this subset.
	 */
	COLORREF diffuseColor;

	// TODO renderFlags
	// TODO textures
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
