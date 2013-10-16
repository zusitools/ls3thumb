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
	static HRESULT RenderScene(Ls3File &file, SIZE &size, LPDIRECT3DDEVICE9 &d3ddev);

private:
	static void CalculateBoundingBox(const Ls3File &file,
		BoundingBox &boundingBox);
};
