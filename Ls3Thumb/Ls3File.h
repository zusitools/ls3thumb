#pragma once

#include "windows.h"
#include <vector>

using namespace std;

struct COORD3D {
	FLOAT x, y, z;
};

struct COORD2D {
	FLOAT u, v;
};

struct COLOR {
	BYTE r, g, b, a;
};

struct ZUSIVERTEX {
	COORD3D pos;
	COORD3D normal;
	COORD2D tex1;
	COORD2D tex2;
};

struct TEXSTAGESETTING {
	DWORD COLOROP;
	DWORD COLORARG0;
	DWORD COLORARG1;
	DWORD COLORARG2;
	DWORD ALPHAOP;
	DWORD ALPHAARG0;
	DWORD ALPHAARG1;
	DWORD ALPHAARG2;
	DWORD MAGFILTER;
	DWORD MINFILTER;
	DWORD RESULTARG;
};


struct Ls3RenderFlags {
	DWORD ALPHAREF;
	DWORD SRCBLEND;
	DWORD DESTBLEND;
	DWORD SHADEMODE;
	DWORD ALPHABLENDENABLE;
	DWORD ALPHATESTENABLE;
	TEXSTAGESETTING stage1;
	TEXSTAGESETTING stage2;
	TEXSTAGESETTING stage3;
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
	COLOR ambientColor;

	/**
	 * The diffuse color of this subset.
	 */
	COLOR diffuseColor;

	/**
	 * The file names of the textures used in this subset.
	 */
	vector<wstring> textureFilenames;

	/**
	 * The render flags of this mesh subset.
	 */
	Ls3RenderFlags renderFlags;

	/**
	 * The Z bias (depth bias) for this subset.
	 */
	FLOAT zBias;

	/**
	 * The mask which specifies in which LOD levels this subset should be
	 * visible. Only the lower four bits are used, and (lodMask << i) set
	 * means that the subset is visible in LOD 3-i (for i in 0..3).
	 */
	BYTE lodMask;
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
