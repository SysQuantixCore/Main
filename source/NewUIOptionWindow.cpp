// NewUIOptionWindow.cpp: implementation of the CNewUIOptionWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NewUIOptionWindow.h"
#include "NewUISystem.h"
#include "ZzzTexture.h"
#include "DSPlaySound.h"
#include "Input.h"
#include "ZzzOpenData.h"
#include "ZzzInterface.h"
#include "pugixml.hpp"
#include "./ExternalObject/leaf/regkey.h"
#include "TextClien.h"
using namespace SEASON3B;

extern std::string g_strSelectedML;
extern char g_aszMLSelection[4];

namespace
{
	// Layout do menu de opcoes (coordenadas logicas).
	const float OPT_WINDOW_W = 260.f;
	const float OPT_WINDOW_H = 312.f;

	// Botao com a altura CHEIA do frame da textura (23 px) para as bordas
	// ficarem completamente visiveis; cada item ocupa 24 px (23 + 1 de gap).
	// Resolucao: 4 itens = 96 px; Fonte/Idioma: 3 itens = 72 px.
	const float OPT_LIST_X = 17.f;
	const float OPT_RES_LIST_Y = 38.f;
	const float OPT_FONT_LIST_Y = 140.f;
	const float OPT_LANG_LIST_Y = 224.f;

	const float OPT_RES_HEADER_Y = 30.f;
	const float OPT_FONT_HEADER_Y = 132.f;
	const float OPT_LANG_HEADER_Y = 216.f;

	// Coluna direita distribuida ao longo da janela.
	const float OPT_CHECK_X = 210.f;
	const float OPT_CHECK_Y = 34.f;
	const float OPT_CHECK_STEP = 18.f;

	const float OPT_VOLUME_Y = 210.f;
	const float OPT_EFFECT_Y = 254.f;

	// Largura/altura NATURAIS do frame da textura (54x23). Usar valores
	// menores faz o RenderImage amostrar apenas parte da textura e a borda
	// direita/inferior do botao desaparecer.
	const float OPT_LIST_BTN_W = 54.f;
	const float OPT_LIST_BTN_H = 23.f;
	// Gap maior para a barra de rolagem (15px) nao colar nos botoes.
	const float OPT_LIST_BTN_GAP = 8.f;

	// Fundo: 2px margem + 54 botao + 8 gap + 15 scrollbar + 3 margem = 82.
	const float OPT_TABLE_W = 82.f;

	// Botao de fechar (X) dentro da barra de titulo.
	const float OPT_CLOSE_X = OPT_WINDOW_W - 32.f;
	const float OPT_CLOSE_Y = 5.f;
	const float OPT_CLOSE_SIZE = 15.f;

	void RenderSectionSeparator(float x, float y, float width)
	{
		// Linha dourada discreta abaixo do cabecalho de cada secao.
		EnableAlphaTest();

		glColor4f(0.85f, 0.70f, 0.35f, 0.45f);
		RenderColor(x, y, width, 1.f, 0.f, 0);
		EndRenderColor();

		glColor4f(1.f, 1.f, 1.f, 1.f);
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SEASON3B::CNewUIOptionWindow::CNewUIOptionWindow()
{
	m_pNewUIMng = NULL;
	m_Pos.x = 0;
	m_Pos.y = 0;

	m_bAutoAttack = true;
	m_bWhisperSound = false;
	m_bSlideHelp = true;
	m_iVolumeLevel = 0;
	m_iRenderLevel = 4;

	m_RenderEffect = true;
	m_RenderEquipment = true;
	m_RenderTerrain = true;
	m_RenderObjects = true;
}

SEASON3B::CNewUIOptionWindow::~CNewUIOptionWindow()
{
	Release();
}

bool SEASON3B::CNewUIOptionWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
	if (NULL == pNewUIMng)
		return false;

	m_pNewUIMng = pNewUIMng;
	m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_OPTION, this);

	SetPos(x, y);

	LoadImages();

	SetButtonInfo();

	Show(false);

	return true;
}

void SEASON3B::CNewUIOptionWindow::SetButtonInfo()
{
	// O botao de fechar e desenhado manualmente (X pequeno) em RenderButtons:
	// a textura newui_button_close.tga e grande demais para esta janela compacta.

	resolutionList.Create(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_RES_LIST_Y, 4,
		BITMAP_btn_empty_very_small, (int)OPT_LIST_BTN_W, (int)OPT_LIST_BTN_H, (int)OPT_LIST_BTN_GAP, 23);

	fonttextList.Create(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_FONT_LIST_Y, 3,
		BITMAP_btn_empty_very_small, (int)OPT_LIST_BTN_W, (int)OPT_LIST_BTN_H, (int)OPT_LIST_BTN_GAP, 23);

	languageList.Create(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_LANG_LIST_Y, 3,
		BITMAP_btn_empty_very_small, (int)OPT_LIST_BTN_W, (int)OPT_LIST_BTN_H, (int)OPT_LIST_BTN_GAP, 23);

	LoadResolution("Data\\Resolutions.xml");
}

void SEASON3B::CNewUIOptionWindow::Release()
{
	UnloadImages();

	if (m_pNewUIMng)
	{
		m_pNewUIMng->RemoveUIObj(this);
		m_pNewUIMng = NULL;
	}
}

void SEASON3B::CNewUIOptionWindow::SetPos(int x, int y)
{
	float RenderFrameX = pos_center() + 190;
	float RenderFrameY = PositionY_In_The_Mid(0) + y;

	m_Pos.x = RenderFrameX;
	m_Pos.y = RenderFrameY;

	resolutionList.SetPosition(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_RES_LIST_Y);

	fonttextList.SetPosition(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_FONT_LIST_Y);

	languageList.SetPosition(m_Pos.x + OPT_LIST_X, m_Pos.y + OPT_LANG_LIST_Y);
}

bool SEASON3B::CNewUIOptionWindow::UpdateMouseEvent()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	if (SEASON3B::IsPress(VK_LBUTTON)
		&& CheckMouseIn(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y,
			OPT_CLOSE_SIZE, OPT_CLOSE_SIZE))
	{
		g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);
		PlayBuffer(SOUND_CLICK01);
		return false;
	}

	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y, 15, 15))
	{
		m_bAutoAttack = !m_bAutoAttack;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP, 15, 15))
	{
		m_bWhisperSound = !m_bWhisperSound;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 2.f, 15, 15))
	{
		m_bSlideHelp = !m_bSlideHelp;
	}

	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 3.f, 15, 15))
	{
		m_RenderEffect = !m_RenderEffect;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 4.f, 15, 15))
	{
		m_RenderEquipment = !m_RenderEquipment;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 5.f, 15, 15))
	{
		m_RenderTerrain = !m_RenderTerrain;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 6.f, 15, 15))
	{
		m_RenderObjects = !m_RenderObjects;
	}
	if (SEASON3B::IsPress(VK_LBUTTON) && CheckMouseIn(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 7.f, 15, 15))
	{
		double backupWidth = gwinhandle->GetScreenX();
		double backupHight = gwinhandle->GetScreenY();

		gwinhandle->ToggleWindowMode();

		CameraFactorPtr->Init();
		OpenFont();
		ClearInput(TRUE);

		CInput& rInput = CInput::Instance();
		rInput.Create(gwinhandle->GethWnd(), WindowWidth, WindowHeight);

		g_pMoveCommandWindow->SetPos(1, 1);
		this->SetPos(0, 50);
		g_pNewUISystem->RenderFrameUpdate(backupWidth, backupHight);
		g_pNewUI3DRenderMng->Reload3DEffectObject(WindowWidth, WindowHeight);

		LoadResolution("Data\\Resolutions.xml");
	}

	if (CheckMouseIn(RenderFrameX + 100, RenderFrameY + OPT_VOLUME_Y, 141, 16))
	{
		int iOldValue = m_iVolumeLevel;
		if (MouseWheel > 0)
		{
			MouseWheel = 0;
			m_iVolumeLevel++;
			if (m_iVolumeLevel > 10)
			{
				m_iVolumeLevel = 10;
			}
		}
		else if (MouseWheel < 0)
		{
			MouseWheel = 0;
			m_iVolumeLevel--;
			if (m_iVolumeLevel < 0)
			{
				m_iVolumeLevel = 0;
			}
		}
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (RenderFrameX + 100);
			if (x < 0)
			{
				m_iVolumeLevel = 0;
			}
			else
			{
				float fValue = (10.f * x) / 141.f;
				m_iVolumeLevel = (int)fValue + 1;
			}
		}

		if (iOldValue != m_iVolumeLevel)
		{
			SetEffectVolumeLevel(m_iVolumeLevel);
		}
	}
	if (CheckMouseIn(RenderFrameX + 100, RenderFrameY + OPT_EFFECT_Y, 141, 29))
	{
		if (SEASON3B::IsRepeat(VK_LBUTTON))
		{
			int x = MouseX - (RenderFrameX + 85);
			float fValue = (5.f * x) / 141.f;
			m_iRenderLevel = (int)fValue;
		}
	}

	if (resolutionList.UpdateMouseEvent())
	{
		change_resolution();
	}

	if (fonttextList.UpdateMouseEvent())
	{
		change_fontsize();
	}

	if (languageList.UpdateMouseEvent())
	{
		change_language();
	}

	return !CheckMouseIn(RenderFrameX, RenderFrameY, OPT_WINDOW_W, OPT_WINDOW_H);
}

bool SEASON3B::CNewUIOptionWindow::UpdateKeyEvent()
{
	if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_OPTION) == true)
	{
		if (SEASON3B::IsPress(VK_ESCAPE) == true)
		{
			g_pNewUISystem->Hide(SEASON3B::INTERFACE_OPTION);
			PlayBuffer(SOUND_CLICK01);
			return false;
		}
	}

	return true;
}

bool SEASON3B::CNewUIOptionWindow::Update()
{
	return true;
}

bool SEASON3B::CNewUIOptionWindow::Render()
{
	EnableAlphaTest();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	RenderFrame();

	RenderContents();

	RenderButtons();

	DisableAlphaBlend();

	return true;
}

float SEASON3B::CNewUIOptionWindow::GetLayerDepth()	//. 10.5f
{
	return 10.5f;
}

float SEASON3B::CNewUIOptionWindow::GetKeyEventOrder()	// 10.f;
{
	return 10.0f;
}

void SEASON3B::CNewUIOptionWindow::OpenningProcess()
{

}

void SEASON3B::CNewUIOptionWindow::ClosingProcess()
{
	SaveOptions();
}

void SEASON3B::CNewUIOptionWindow::SetGameOptions(BYTE GameOption)
{
	if ((GameOption & AUTOATTACK_ON) == AUTOATTACK_ON)
	{
		this->SetAutoAttack(true);
	}
	else
	{
		this->SetAutoAttack(false);
	}

	if ((GameOption & WHISPER_SOUND_ON) == WHISPER_SOUND_ON)
	{
		this->SetWhisperSound(true);
	}
	else
	{
		this->SetWhisperSound(false);
	}

	if ((GameOption & SLIDE_HELP_ON) == SLIDE_HELP_ON)
	{
		this->SetSlideHelp(true);
	}
	else
	{
		this->SetSlideHelp(false);
	}

	if ((GameOption & RENDER_EFFECT_ON) == RENDER_EFFECT_ON)
	{
		this->SetRenderEffect(true);
	}
	else
	{
		this->SetRenderEffect(false);
	}

	if ((GameOption & RENDER_EQUIPMENT_ON) == RENDER_EQUIPMENT_ON)
	{
		this->SetRenderEquipment(true);
	}
	else
	{
		this->SetRenderEquipment(false);
	}

	if ((GameOption & RENDER_TERRAIN_ON) == RENDER_TERRAIN_ON)
	{
		this->SetRenderTerrain(true);
	}
	else
	{
		this->SetRenderTerrain(false);
	}

	if ((GameOption & RENDER_OBJECTS_ON) == RENDER_OBJECTS_ON)
	{
		this->SetRenderObjects(true);
	}
	else
	{
		this->SetRenderObjects(false);
	}
}

void SEASON3B::CNewUIOptionWindow::LoadImages()
{
	LoadBitmap("Interface\\newui_button_close.tga", IMAGE_OPTION_BTN_CLOSE, GL_LINEAR);
	LoadBitmap("Interface\\newui_msgbox_back.jpg", IMAGE_OPTION_FRAME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_item_back03.tga", IMAGE_OPTION_FRAME_DOWN, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_top.tga", IMAGE_OPTION_FRAME_UP, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(L).tga", IMAGE_OPTION_FRAME_LEFT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_back06(R).tga", IMAGE_OPTION_FRAME_RIGHT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_line.jpg", IMAGE_OPTION_LINE, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_point.tga", IMAGE_OPTION_POINT, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_check.tga", IMAGE_OPTION_BTN_CHECK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect03.tga", IMAGE_OPTION_EFFECT_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_effect04.tga", IMAGE_OPTION_EFFECT_COLOR, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume01.tga", IMAGE_OPTION_VOLUME_BACK, GL_LINEAR);
	LoadBitmap("Interface\\newui_option_volume02.tga", IMAGE_OPTION_VOLUME_COLOR, GL_LINEAR);

	LoadBitmap("Interface\\HUD\\checkbox.tga", IMAGE_CHECK_LIVE);
	LoadBitmap("Interface\\HUD\\uncheckbox.tga", IMAGE_UNCHECK_LIVE);
}

void SEASON3B::CNewUIOptionWindow::UnloadImages()
{
	DeleteBitmap(IMAGE_OPTION_BTN_CLOSE);
	DeleteBitmap(IMAGE_OPTION_FRAME_BACK);
	DeleteBitmap(IMAGE_OPTION_FRAME_DOWN);
	DeleteBitmap(IMAGE_OPTION_FRAME_UP);
	DeleteBitmap(IMAGE_OPTION_FRAME_LEFT);
	DeleteBitmap(IMAGE_OPTION_FRAME_RIGHT);
	DeleteBitmap(IMAGE_OPTION_LINE);
	DeleteBitmap(IMAGE_OPTION_POINT);
	DeleteBitmap(IMAGE_OPTION_BTN_CHECK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_BACK);
	DeleteBitmap(IMAGE_OPTION_EFFECT_COLOR);
	DeleteBitmap(IMAGE_OPTION_VOLUME_BACK);
	DeleteBitmap(IMAGE_OPTION_VOLUME_COLOR);
}

void SEASON3B::CNewUIOptionWindow::RenderFrame()
{
	// Moldura classica do MU (a textura do Menu do Sistema tem largura fixa
	// de 213px e distorce se esticada para 260px).
	window_backmsg(m_Pos.x, m_Pos.y, OPT_WINDOW_W, OPT_WINDOW_H);

	int Bitmap_point_draw = CNewUICursedTempleSystem::IMAGE_CURSEDTEMPLESYSTEM_MINIMAPICON_HOLYITEM_PC;

	RenderBitmap(Bitmap_point_draw, ((float)m_Pos.x + OPT_LIST_X - 3.f), (float)m_Pos.y + OPT_RES_HEADER_Y, 8.f, 8.f, 0.f, 0.f, 14.f / 16.f, 14.f / 16.f);

	RenderBitmap(Bitmap_point_draw, ((float)m_Pos.x + OPT_LIST_X - 3.f), (float)m_Pos.y + OPT_FONT_HEADER_Y, 8.f, 8.f, 0.f, 0.f, 14.f / 16.f, 14.f / 16.f);

	RenderBitmap(Bitmap_point_draw, ((float)m_Pos.x + OPT_LIST_X - 3.f), (float)m_Pos.y + OPT_LANG_HEADER_Y, 8.f, 8.f, 0.f, 0.f, 14.f / 16.f, 14.f / 16.f);

	resolutionList.Render();

	fonttextList.Render();

	languageList.Render();
}

void SEASON3B::CNewUIOptionWindow::RenderContents()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	// Titulo sem fundo, um pouco mais acima, na fonte branca do MuOnline.
	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(255, 255, 255, 255);
	g_pRenderText->RenderText(RenderFrameX + 16.f, RenderFrameY + 5.f,
		GlobalText[3450], OPT_WINDOW_W - 48.f, 16.f, RT3_SORT_CENTER);

	// Rotulos curtos e intuitivos para a coluna lateral.
	g_pRenderText->RenderText(RenderFrameX + OPT_LIST_X + 12.f, RenderFrameY + OPT_RES_HEADER_Y, "Resolução");

	g_pRenderText->RenderText(RenderFrameX + OPT_LIST_X + 12.f, RenderFrameY + OPT_FONT_HEADER_Y, "Fonte");

	g_pRenderText->SetFont(g_hFont);

	g_pRenderText->SetTextColor(255, 255, 255, 255);

	g_pRenderText->SetBgColor(0);

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y, GlobalText[386], 0, 15); //-- Automatic Attack

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP, GlobalText[387], 0, 15); //-- Beep sound for whispering

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 2.f, GlobalText[919], 0, 15); //-- Slide Help

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 3.f, gTextClien.TextClien_Khac[6], 0, 15); //-- Effect

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 4.f, gTextClien.TextClien_Khac[7], 0, 15); //-- Equipment

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 5.f, gTextClien.TextClien_Khac[8], 0, 15); //-- Terrain

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 6.f, gTextClien.TextClien_Khac[9], 0, 15); //-- Objects

	const char* pszWindowMode = "Fullscreen";

	if (gwinhandle->GetWindowMode() == WINDOW_MODE_WINDOWED)
		pszWindowMode = "Window Mode";
	else if (gwinhandle->GetWindowMode() == WINDOW_MODE_BORDERLESS)
		pszWindowMode = "Borderless";

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 7.f, pszWindowMode, 0, 15); //-- Window Mode

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_VOLUME_Y - 8.f, GlobalText[389]); //-- Volume

	g_pRenderText->RenderText(RenderFrameX + 100, RenderFrameY + OPT_EFFECT_Y - 8.f, GlobalText[1840]); //-- +Effect limitation

	g_pRenderText->SetFont(g_hFontBold);

	g_pRenderText->SetTextColor(255, 255, 255, 255);

	g_pRenderText->RenderText(RenderFrameX + OPT_LIST_X + 12.f, RenderFrameY + OPT_LANG_HEADER_Y, "Idioma");

	g_pRenderText->SetFont(g_hFont);

	g_pRenderText->SetTextColor(255, 255, 255, 255);

}

void SEASON3B::CNewUIOptionWindow::RenderChecked(float RenderFrameX, float RenderFrameY, bool bEnable)
{
	if (bEnable)
		RenderImage(IMAGE_CHECK_LIVE, RenderFrameX, RenderFrameY, 15.0, 15.0, 0.0, 0.0, 0.75, 0.75);
	else
		RenderImage(IMAGE_UNCHECK_LIVE, RenderFrameX, RenderFrameY, 15.0, 15.0, 0.0, 0.0, 0.75, 0.75);
}

void SEASON3B::CNewUIOptionWindow::RenderButtons()
{
	float RenderFrameX = m_Pos.x;
	float RenderFrameY = m_Pos.y;

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y, m_bAutoAttack);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP, m_bWhisperSound);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 2.f, m_bSlideHelp);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 3.f, m_RenderEffect);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 4.f, m_RenderEquipment);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 5.f, m_RenderTerrain);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 6.f, m_RenderObjects);

	RenderChecked(RenderFrameX + OPT_CHECK_X, RenderFrameY + OPT_CHECK_Y + OPT_CHECK_STEP * 7.f, gwinhandle->CheckWndMode());


	RenderImage(IMAGE_OPTION_VOLUME_BACK, RenderFrameX + 108, RenderFrameY + OPT_VOLUME_Y, 124.f, 16.f);

	if (m_iVolumeLevel > 0)
	{
		RenderImage(IMAGE_OPTION_VOLUME_COLOR, RenderFrameX + 108, RenderFrameY + OPT_VOLUME_Y, 124.f * 0.1f * (m_iVolumeLevel), 16.f);
	}

	RenderImage(IMAGE_OPTION_EFFECT_BACK, RenderFrameX + 100, RenderFrameY + OPT_EFFECT_Y, 141.f, 29.f);

	if (m_iRenderLevel >= 0)
	{
		RenderImage(IMAGE_OPTION_EFFECT_COLOR, RenderFrameX + 100, RenderFrameY + OPT_EFFECT_Y, 141.f * 0.2f * (m_iRenderLevel + 1), 29.f);
	}

	// X de fechar pequeno, desenhado dentro da barra de titulo.
	EnableAlphaTest();

	glColor4f(0.10f, 0.10f, 0.12f, 0.90f);
	RenderColor(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y,
		OPT_CLOSE_SIZE, OPT_CLOSE_SIZE, 0.f, 0);

	glColor4f(1.f, 1.f, 1.f, 0.80f);
	RenderColor(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y,
		OPT_CLOSE_SIZE, 1.f, 0.f, 0);
	RenderColor(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y + OPT_CLOSE_SIZE - 1.f,
		OPT_CLOSE_SIZE, 1.f, 0.f, 0);
	RenderColor(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y,
		1.f, OPT_CLOSE_SIZE, 0.f, 0);
	RenderColor(RenderFrameX + OPT_CLOSE_X + OPT_CLOSE_SIZE - 1.f, RenderFrameY + OPT_CLOSE_Y,
		1.f, OPT_CLOSE_SIZE, 0.f, 0);

	EndRenderColor();

	g_pRenderText->SetBgColor(0);
	g_pRenderText->SetFont(g_hFontBold);
	g_pRenderText->SetTextColor(255, 255, 255, 255);
	g_pRenderText->RenderText(RenderFrameX + OPT_CLOSE_X, RenderFrameY + OPT_CLOSE_Y + 1.f,
		"X", OPT_CLOSE_SIZE, OPT_CLOSE_SIZE, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIOptionWindow::RenderTable(float x, float y, float width, float height)
{
	// Painel limpo: fundo escuro translucido + borda fina dourada discreta.
	// Substitui as ornacoes de 14px que deixavam a coluna lateral carregada.
	EnableAlphaTest();

	glColor4f(0.05f, 0.06f, 0.08f, 0.72f);
	RenderColor(x, y, width, height, 0.0f, 0);

	glColor4f(0.62f, 0.50f, 0.28f, 0.55f);
	RenderColor(x, y, width, 1.f, 0.0f, 0);
	RenderColor(x, y + height - 1.f, width, 1.f, 0.0f, 0);
	RenderColor(x, y, 1.f, height, 0.0f, 0);
	RenderColor(x + width - 1.f, y, 1.f, height, 0.0f, 0);

	EndRenderColor();
}

void SEASON3B::CNewUIOptionWindow::SetAutoAttack(bool bAuto)
{
	m_bAutoAttack = bAuto;
}

bool SEASON3B::CNewUIOptionWindow::IsAutoAttack()
{
	return m_bAutoAttack;
}

void SEASON3B::CNewUIOptionWindow::SetWhisperSound(bool bSound)
{
	m_bWhisperSound = bSound;
}

bool SEASON3B::CNewUIOptionWindow::IsWhisperSound()
{
	return m_bWhisperSound;
}

void SEASON3B::CNewUIOptionWindow::SetSlideHelp(bool bHelp)
{
	m_bSlideHelp = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::IsSlideHelp()
{
	return m_bSlideHelp;
}

void SEASON3B::CNewUIOptionWindow::SetVolumeLevel(int iVolume)
{
	m_iVolumeLevel = iVolume;
}

int SEASON3B::CNewUIOptionWindow::GetVolumeLevel()
{
	return m_iVolumeLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRenderLevel(int iRender)
{
	m_iRenderLevel = iRender;
}

int SEASON3B::CNewUIOptionWindow::GetRenderLevel()
{
	return m_iRenderLevel;
}

void SEASON3B::CNewUIOptionWindow::SetRenderEffect(bool bHelp)
{
	m_RenderEffect = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderEffect()
{
	return m_RenderEffect;
}

void SEASON3B::CNewUIOptionWindow::SetRenderEquipment(bool bHelp)
{
	m_RenderEquipment = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderEquipment()
{
	return m_RenderEquipment;
}

void SEASON3B::CNewUIOptionWindow::SetRenderTerrain(bool bHelp)
{
	m_RenderTerrain = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderTerrain()
{
	return m_RenderTerrain;
}

void SEASON3B::CNewUIOptionWindow::SetRenderObjects(bool bHelp)
{
	m_RenderObjects = bHelp;
}

bool SEASON3B::CNewUIOptionWindow::GetRenderObjects()
{
	return m_RenderObjects;
}

void SEASON3B::CNewUIOptionWindow::change_resolution()
{
	int index = gwinhandle->GetDisplayIndex(resolutionList.as_string());

	if (m_Resolution != index)
	{
		leaf::CRegKey regkey;

		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");

		if (regkey.WriteDword("Resolution", index))
		{
			m_Resolution = index;

			double backupWidth = gwinhandle->GetScreenX();

			double backupHight = gwinhandle->GetScreenY();

			gwinhandle->SetDisplayIndex(index, false);

			CameraFactorPtr->Init();

			OpenFont();

			ClearInput(TRUE);

			CInput& rInput = CInput::Instance();

			rInput.Create(gwinhandle->GethWnd(), WindowWidth, WindowHeight);

			g_pMoveCommandWindow->SetPos(1, 1);

			this->SetPos(0, 50);

			g_pNewUISystem->RenderFrameUpdate(backupWidth, backupHight);

			g_pNewUI3DRenderMng->Reload3DEffectObject(WindowWidth, WindowHeight);
		}
	}
}

void SEASON3B::CNewUIOptionWindow::change_fontsize()
{
	int fontsize = fonttextList.as_interget();

	if (fontsize != FontHeight)
	{
		gwinhandle->SetFontSize(fontsize);

		pugi::xml_document file;
		pugi::xml_parse_result res = file.load_file("Data\\Resolutions.xml");

		if (res.status == pugi::status_ok)
		{
			pugi::xml_node root = file.child("ResolutionsInfo");

			for (pugi::xml_node child = root.child("Resolution"); child; child = child.next_sibling())
			{
				std::string text_name = child.attribute("name").as_string();

				resolutionList.push_back(text_name);

				if (gwinhandle->GetDisplayIndex() == child.attribute("index").as_int())
				{
					child.attribute("font_size").set_value(fontsize);

					file.save_file("Data\\Resolutions.xml");
					return;
				}
			}
		}
	}
}

void SEASON3B::CNewUIOptionWindow::LoadResolution(const char* filename)
{
	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(filename);

	if (res.status != pugi::status_ok)
	{
		return;
	}

	resolutionList.clear();
	fonttextList.clear();
	languageList.clear();

	int fontSize = 13;
	int fontindex = 0;
	int resolutionListIndex = 0;
	int currentListIndex = 0;

	pugi::xml_node root = file.child("ResolutionsInfo");

	for (pugi::xml_node child_io = root.child("Resolution"); child_io; child_io = child_io.next_sibling())
	{
		std::string text_name = child_io.attribute("name").as_string();

		if (!CSettings::IsWindowModeResolution(text_name))
			continue;

		resolutionList.push_back(text_name);

		if (gwinhandle->GetDisplayIndex() == child_io.attribute("index").as_int())
		{
			fontSize = child_io.attribute("font_size").as_int();
			currentListIndex = resolutionListIndex;
		}

		resolutionListIndex++;
	}

	root = file.child("FontSize");

	int i = 0;

	for (pugi::xml_node child_io = root.child("size"); child_io; child_io = child_io.next_sibling(), i++)
	{
		std::string text_name = child_io.attribute("fontsize").as_string();

		fonttextList.push_back(text_name);

		if (fontSize == child_io.attribute("fontsize").as_int())
		{
			fontindex = i;
		}
	}

	fonttextList.SetCurrent(fontindex);

	resolutionList.SetCurrent(currentListIndex);

	static const char* langNames[] = { "English", "Portugues", "Espanol", "Chinese", "Korean" };
	static const char* langCodes[] = { "Eng", "Por", "Spn", "Chs", "Kor" };
	int langIndex = 0;
	for (int i = 0; i < 5; i++)
	{
		languageList.push_back(langNames[i]);
		if (_stricmp(g_strSelectedML.c_str(), langCodes[i]) == 0)
			langIndex = i;
	}
	languageList.SetCurrent(langIndex);
}

void SEASON3B::CNewUIOptionWindow::change_language()
{
	static const char* langCodes[] = { "Eng", "Por", "Spn", "Chs", "Kor" };
	int index = languageList.GetCurrent();
	if (index >= 0 && index < 5)
	{
		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
		regkey.WriteString("LangSelection", langCodes[index]);
		g_aszMLSelection[0] = '\0';
		strcpy_s(g_aszMLSelection, sizeof(g_aszMLSelection), langCodes[index]);
		g_strSelectedML = g_aszMLSelection;
	}
}
