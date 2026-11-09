#include "stdafx.h"
#include "LoadingScene.h"

#include "Input.h"

namespace
{
	const int REF_WIDTH = 800;
	const int REF_HEIGHT = 600;

	const int TILE_WIDTH = REF_WIDTH / 2;
	const int TOP_HEIGHT = 512;
	const int BOTTOM_HEIGHT = REF_HEIGHT - TOP_HEIGHT;

	const int g_anTileWidth[LDS_BACK_MAX] = { TILE_WIDTH, TILE_WIDTH, TILE_WIDTH, TILE_WIDTH };
	const int g_anTileHeight[LDS_BACK_MAX] = { TOP_HEIGHT, TOP_HEIGHT, BOTTOM_HEIGHT, BOTTOM_HEIGHT };
}

CLoadingScene::CLoadingScene()
{
}

CLoadingScene::~CLoadingScene()
{
}

void CLoadingScene::Create()
{
	CInput& rInput = CInput::Instance();

	const float fScaleX = static_cast<float>(rInput.GetScreenWidth()) / REF_WIDTH;
	const float fScaleY = static_cast<float>(rInput.GetScreenHeight()) / REF_HEIGHT;

	for (int i = 0; i < LDS_BACK_MAX; ++i)
	{
		m_asprBack[i].Create(g_anTileWidth[i], g_anTileHeight[i], BITMAP_TITLE + i, 0, NULL, 0, 0, false,
			SPR_SIZING_DATUMS_LT, fScaleX, fScaleY);
		m_asprBack[i].Show(true);
	}

	m_asprBack[1].SetPosition(400, 0, X);
	m_asprBack[2].SetPosition(0, 512, Y);
	m_asprBack[3].SetPosition(400, 512);
}

void CLoadingScene::Release()
{
	for (int i = 0; i < LDS_BACK_MAX; ++i)
		m_asprBack[i].Release();
}

void CLoadingScene::Render()
{
	for (int i = 0; i < LDS_BACK_MAX; ++i)
		m_asprBack[i].Render();
}