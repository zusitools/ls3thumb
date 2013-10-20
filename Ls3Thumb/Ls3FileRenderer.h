#pragma once

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#include "windows.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "Ls3File.h"

struct BoundingBox {
	FLOAT xmin, ymin, zmin, xmax, ymax, zmax;
};

class Ls3FileRenderer
{
public:
	static HRESULT RenderScene(const Ls3File &file, const SIZE &size,
		LPDIRECT3DDEVICE9 &d3ddev);

private:
	static void CalculateBoundingBox(const Ls3File &file,
		BoundingBox &boundingBox);
	static void SetTextureStageState(const int stage,
		const TEXSTAGESETTING &settings, LPDIRECT3DDEVICE9 d3ddev);
};
