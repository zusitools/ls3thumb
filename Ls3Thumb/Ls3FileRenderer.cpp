#include "stdafx.h"
#include "Ls3FileRenderer.h"

#define ZUSIFVF (D3DFVF_XYZ | D3DFVF_NORMAL /* | D3DFVF_TEX1 */)

#define TRY(action) if (FAILED(hr = action)) { return hr; }

HRESULT Ls3FileRenderer::RenderScene(Ls3File &file, SIZE &size, LPDIRECT3DDEVICE9 &d3ddev)
{
	HRESULT hr;

	D3DXVECTOR3 cameraPosition(-5.0f, -5.0f, 5.0f);
	D3DXVECTOR3 cameraTarget(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 cameraUp(0.0f, 0.0f, 1.0f);
	D3DXMATRIX viewMatrix;
	D3DXMatrixLookAtLH(&viewMatrix, &cameraPosition, &cameraTarget, &cameraUp);

	TRY(d3ddev->SetTransform(D3DTS_VIEW, &viewMatrix));

	// Setup the projection matrix
	D3DXMATRIX projectionMatrix;
	D3DXMatrixPerspectiveFovLH(&projectionMatrix, D3DX_PI / 4, (float) size.cx / (float) size.cy, 0.1f, 100.0f);

	TRY(d3ddev->SetTransform(D3DTS_PROJECTION, &projectionMatrix));

	// Turn on 3D lighting and Z buffering
	TRY(d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE));
	TRY(d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(150, 150, 150)));
	TRY(d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE));

	TRY(d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(rand() % 255, 165, 210), 1.0f, 0));
	TRY(d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0));
	TRY(d3ddev->BeginScene());
	TRY(d3ddev->SetFVF(ZUSIFVF));

	VOID* pData;
	LPDIRECT3DVERTEXBUFFER9 pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 pIndexBuffer;

	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(light));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(0.8f, 0.8f, 0.8f, 1.0f);
	light.Direction = D3DXVECTOR3(-5.0f, -5.0f, -5.0f);

	d3ddev->SetLight(0, &light);
	d3ddev->LightEnable(0, TRUE);

	for (auto it = file.subsets.begin(); it != file.subsets.end(); it++)
	{
		const Ls3MeshSubset subset = *it;
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
		material.Ambient.r = GetRValue(subset.ambientColor) / 255.0;
		material.Ambient.g = GetGValue(subset.ambientColor) / 255.0;
		material.Ambient.b = GetBValue(subset.ambientColor) / 255.0;
		material.Ambient.a = 1.0;

		material.Diffuse.r = GetRValue(subset.diffuseColor) / 255.0;
		material.Diffuse.g = GetGValue(subset.diffuseColor) / 255.0;
		material.Diffuse.b = GetBValue(subset.diffuseColor) / 255.0;
		material.Diffuse.a = 1.0;

		TRY(d3ddev->SetMaterial(&material));

		// Draw the mesh
		TRY(d3ddev->SetIndices(pIndexBuffer));
		TRY(d3ddev->SetStreamSource(0, pVertexBuffer, 0, sizeof(ZUSIVERTEX)));
		TRY(d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numFaces));
	}

	TRY(d3ddev->EndScene());
	TRY(d3ddev->Present(NULL, NULL, NULL, NULL));

	return S_OK;
}
