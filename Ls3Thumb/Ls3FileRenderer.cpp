#include "stdafx.h"
#include "Ls3FileRenderer.h"

#define ZUSIFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

#define TRY(action) if (FAILED(hr = action)) { return hr; }

#define LINE_INTERSECT_X(x1, y1, m1, x2, y2, m2) ((-x2*m2 + y2 - y1 + x1*m1) / (m1 - m2))
#define LINE_INTERSECT_Y(x1, y1, m1, xintersect) (m1 * (xintersect - x1) + y1)

#define FOV_Y (D3DX_PI / 4)
#define CAMERA_ANGLE_DEGREES 40.0f

HRESULT Ls3FileRenderer::RenderScene(Ls3File &file, SIZE &size, LPDIRECT3DDEVICE9 &d3ddev)
{
	HRESULT hr;

	BoundingBox boundingBox;
	CalculateBoundingBox(file, boundingBox);

	FLOAT aspectRatio = (float) size.cx / (float) size.cy;
	FLOAT cameraZ = (boundingBox.zmin + boundingBox.zmax) / 2.0f;
	FLOAT fovX = aspectRatio * FOV_Y;

	FLOAT cameraAngle = CAMERA_ANGLE_DEGREES / 180.0 * D3DX_PI;

	// Flip Y coordinates to convert coordinate system to the one
	// normally used in geometry
	double x1 = boundingBox.ymin;
	double y1 = -boundingBox.xmin;
	double x2 = boundingBox.ymax;
	double y2 = -boundingBox.xmax;	

	// Retrieve point at which the upper-left and lower-right point of the
	// bounding box (as seen from above) are visible.
	double m1 = tan(cameraAngle + FOV_Y / 2);
	double m2 = tan(cameraAngle - FOV_Y / 2);
	double xp1 = LINE_INTERSECT_X(x1, y1, m1, x2, y2, m2);
	double yp1 = LINE_INTERSECT_Y(x1, y1, m1, xp1);

	// Retrieve point at which the two lower-left points of the bounding box
	// (as seen from above) are visible.
	double m3 = tan(cameraAngle);
	double xlookat = LINE_INTERSECT_X(xp1, yp1, m3, x1, y2, -1 / m3);
	double ylookat = LINE_INTERSECT_Y(xp1, yp1, m3, xlookat);

	double minDist = (boundingBox.zmax - cameraZ) / tan(fovX / 2);
	double xp2 = xlookat - cos(cameraAngle) * minDist;
	double yp2 = ylookat - sin(cameraAngle) * minDist;

	double dist1 = sqrt((xp1 - xlookat) * (xp1 - xlookat) + (yp1 - ylookat) * (yp1 - ylookat));
	double xcam, ycam;

	if (dist1 >= minDist) {
		xcam = xp1;
		ycam = yp1;
	}
	else
	{
		xcam = xp2;
		ycam = yp2;
	}

	D3DXVECTOR3 cameraPosition(-ycam, xcam, cameraZ);
	D3DXVECTOR3 cameraTarget(-ylookat, xlookat, cameraZ);
	D3DXVECTOR3 cameraUp(0.0f, 0.0f, 1.0f);
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtRH(&viewMatrix, &cameraPosition, &cameraTarget, &cameraUp);

	TRY(d3ddev->SetTransform(D3DTS_VIEW, &viewMatrix));

	// Setup the projection matrix
	D3DXMATRIX projectionMatrix;
	D3DXMatrixPerspectiveFovRH(&projectionMatrix, FOV_Y, aspectRatio, 0.1f, 100.0f);

	TRY(d3ddev->SetTransform(D3DTS_PROJECTION, &projectionMatrix));

	// Turn on 3D lighting and Z buffering
	TRY(d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE));
	TRY(d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(150, 150, 150)));
	TRY(d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE));

	TRY(d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(116, 165, 210), 1.0f, 0));
	TRY(d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0));
	TRY(d3ddev->BeginScene());
	TRY(d3ddev->SetFVF(ZUSIFVF));

	VOID* pData;
	LPDIRECT3DVERTEXBUFFER9 pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 pIndexBuffer;
	LPDIRECT3DTEXTURE9 pTexture;

	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
	light.Direction = D3DXVECTOR3(-5.0f, -5.0f, -5.0f);

	d3ddev->SetLight(0, &light);
	d3ddev->LightEnable(0, TRUE);

	for (auto &subset : file.subsets)
	{
		const size_t numVertices = subset.vertices.size();
		const size_t numFaceIndices = subset.faceIndices.size();
		const size_t numFaces = numFaceIndices / 3;

		if (numFaces == 0) {
			continue;
		}

		// Create vertex buffer
		TRY(d3ddev->CreateVertexBuffer(
			numVertices * sizeof(ZUSIVERTEX),
			0, ZUSIFVF, D3DPOOL_MANAGED, &pVertexBuffer, NULL));

		pVertexBuffer->Lock(0, 0, (void**) &pData, 0);
		memcpy(pData, subset.vertices.data(), numVertices * sizeof(ZUSIVERTEX));
		pVertexBuffer->Unlock();

		// Create index buffer
		TRY(d3ddev->CreateIndexBuffer(
			numFaceIndices * sizeof(UINT32),
			0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &pIndexBuffer, NULL));

		pIndexBuffer->Lock(0, 0, (void**) &pData, 0);
		memcpy(pData, subset.faceIndices.data(), numFaceIndices * sizeof(UINT32));
		pIndexBuffer->Unlock();

		// Create and set the material
		D3DMATERIAL9 material;
		ZeroMemory(&material, sizeof(material));
		material.Ambient.r = subset.ambientColor.r / 255.0f;
		material.Ambient.g = subset.ambientColor.g / 255.0f;
		material.Ambient.b = subset.ambientColor.b / 255.0f;
		material.Ambient.a = subset.ambientColor.a / 255.0f;

		material.Diffuse.r = subset.diffuseColor.r / 255.0f;
		material.Diffuse.g = subset.diffuseColor.g / 255.0f;
		material.Diffuse.b = subset.diffuseColor.b / 255.0f;
		material.Diffuse.a = subset.diffuseColor.a / 255.0f;

		TRY(d3ddev->SetMaterial(&material));

		// Load and set the texture
		if (subset.textureFilenames.size() > 0)
		{
			D3DXCreateTextureFromFile(d3ddev,
				subset.textureFilenames[0].c_str(), &pTexture);
		}
		else {
			pTexture = NULL;
		}

		d3ddev->SetTexture(0, pTexture);

		// Set render flags
		SetTextureStageState(0, subset.renderFlags.stage1, d3ddev);
		SetTextureStageState(1, subset.renderFlags.stage2, d3ddev);
		SetTextureStageState(2, subset.renderFlags.stage3, d3ddev);
		d3ddev->SetRenderState(D3DRS_ALPHAREF, subset.renderFlags.ALPHAREF);
		d3ddev->SetRenderState(D3DRS_SRCBLEND, subset.renderFlags.SRCBLEND);
		d3ddev->SetRenderState(D3DRS_DESTBLEND, subset.renderFlags.DESTBLEND);
		d3ddev->SetRenderState(D3DRS_SHADEMODE, subset.renderFlags.SHADEMODE);

		// Draw the mesh
		TRY(d3ddev->SetIndices(pIndexBuffer));
		TRY(d3ddev->SetStreamSource(0, pVertexBuffer, 0, sizeof(ZUSIVERTEX)));
		TRY(d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numFaces));

		if (pTexture != NULL) {
			pTexture->Release();
		}
	}

	TRY(d3ddev->EndScene());
	TRY(d3ddev->Present(NULL, NULL, NULL, NULL));

	return S_OK;
}

void Ls3FileRenderer::CalculateBoundingBox(const Ls3File &file,
		BoundingBox &boundingBox) {
	ZeroMemory(&boundingBox, sizeof(boundingBox));

	for (auto &sit : file.subsets)
	{
		for (auto &vit : sit.vertices)
		{
			COORD3D p = vit.pos;
			if (p.x < boundingBox.xmin) boundingBox.xmin = p.x;
			if (p.y < boundingBox.ymin) boundingBox.ymin = p.y;
			if (p.z < boundingBox.zmin) boundingBox.zmin = p.z;
			if (p.x > boundingBox.xmax) boundingBox.xmax = p.x;
			if (p.y > boundingBox.ymax) boundingBox.ymax = p.y;
			if (p.z > boundingBox.zmax) boundingBox.zmax = p.z;
		}
	}
}

void Ls3FileRenderer::SetTextureStageState(const int stage,
	const TEXSTAGESETTING &settings, LPDIRECT3DDEVICE9 d3ddev)
{
	d3ddev->SetTextureStageState(stage, D3DTSS_COLOROP, settings.COLOROP);
	d3ddev->SetTextureStageState(stage, D3DTSS_COLORARG0, settings.COLORARG0);
	d3ddev->SetTextureStageState(stage, D3DTSS_COLORARG1, settings.COLORARG1);
	d3ddev->SetTextureStageState(stage, D3DTSS_COLORARG2, settings.COLORARG2);
	d3ddev->SetTextureStageState(stage, D3DTSS_ALPHAOP, settings.ALPHAOP);
	d3ddev->SetTextureStageState(stage, D3DTSS_ALPHAARG0, settings.ALPHAARG0);
	d3ddev->SetTextureStageState(stage, D3DTSS_ALPHAARG1, settings.ALPHAARG1);
	d3ddev->SetTextureStageState(stage, D3DTSS_ALPHAARG2, settings.ALPHAARG2);
	d3ddev->SetTextureStageState(stage, D3DTSS_RESULTARG, settings.RESULTARG);
}