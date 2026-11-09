///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <locale.h>
#include <zmouse.h>
#include "UIWindows.h"
#include "UIManager.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzOpenData.h"
#include "ZzzScene.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "ZzzObject.h"
#include "ZzzCharacter.h"
#include "ZzzInterface.h"
#include "ZzzInventory.h"
#include "zzzLodTerrain.h"
#include "DSPlaySound.h"
#include "wsclientinline.h"
#include "Resource.h"
#include <imm.h>
#include "zzzpath.h"
#include "Nprotect.h"
#include "Local.h"
#include "PersonalShopTitleImp.h"
#include "./Utilities/Log/ErrorReport.h"
#include "UIMapName.h"		// rozy
#include "./ExternalObject/leaf/ExceptionHandler.h"
#include "./Utilities/Dump/CrashReporter.h"
#include "./Utilities/Log/muConsoleDebug.h"
#include "ProtocolSend.h"
#include "ProtectSysKey.h"

#include "CBTMessageBox.h"
#include "./ExternalObject/leaf/regkey.h"

#include "CSChaosCastle.h"
#include "GMHellas.h"
#include <io.h>
#include "Input.h"
#include "./Time/Timer.h"
#include "UIMng.h"

#ifdef MOVIE_DIRECTSHOW
#include <dshow.h>
#include "MovieScene.h"
#endif // MOVIE_DIRECTSHOW
#include "GameCensorship.h"
#include "w_MapHeaders.h"
#include "w_PetProcess.h"
#include <ThemidaInclude.h>
#include "MultiLanguage.h"
#include "CGMProtect.h"
#include "CAIController.h"

#include "ScaleForm.h"
#include "CGMFontLayer.h"

#include <tlhelp32.h>

#include <dbghelp.h>
#pragma comment(lib,"dbghelp.lib")
#include <stdarg.h>

#include <wzAudio.h>
#pragma comment(lib, "wzAudio.lib")
#ifdef IMPLEMENT_IMGUI_WIN32
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"
#endif // IMPLEMENT_IMGUI_WIN32
#include "ConnectVersionHex.h"
#include "CShaderGL.h"
#include "SystemCompatibility.h"

#include "TextClien.h"


// =====================================================================
// GLOBAIS EXTERNOS
// =====================================================================
extern BOOL g_bIMEBlock;
extern DWORD g_dwTopWindow;
extern int  LogIn;
extern char LogInID[];
extern int  TimeRemain;
extern bool EnableFastInput;
extern PATH* path;

void MainScene(HDC hDC);

// =====================================================================
// GLOBAIS DO MÓDULO
// =====================================================================
CUITextInputBox* g_pSingleTextInputBox = NULL;
CUITextInputBox* g_pSinglePasswdInputBox = NULL;
int g_iChatInputType = 1;

CChatRoomSocketList* g_pChatRoomSocketList = NULL;

CMultiLanguage* pMultiLanguage = NULL;

#ifdef MOVIE_DIRECTSHOW
CMovieScene* g_pMovieScene = NULL;
#endif // MOVIE_DIRECTSHOW

CUIManager* g_pUIManager = NULL;
CUIMapName* g_pUIMapName = NULL;		// rozy

int Time_Effect = 0;
bool ashies = false;
int weather = rand() % 3;
HDC       g_hDC = NULL;
HGLRC     g_hRC = NULL;
HFONT     g_hFont = NULL;
HFONT     g_hFontBold = NULL;
HFONT     g_hFontBig = NULL;
HFONT     g_hFixFont = NULL;

CTimer* g_pTimer = NULL;	// performance counter.
bool      Destroy = false;
bool      ActiveIME = false;

BYTE* RendomMemoryDump;
//CHARACTER* CharacterMemoryDump;

int       RandomTable[100];

char TextMu[] = "mu.exe";

CErrorReport g_ErrorReport;
static volatile LONG g_CrashCaptureInProgress = 0;

BOOL g_bMinimizedEnabled = FALSE;
int g_iScreenSaverOldValue = 60 * 15;

extern float g_fScreenRate_x;
extern float g_fScreenRate_y;

int g_iInactiveWarning = 0;
bool HangulDelete = false;
int Hangul = 0;
bool g_bEnterPressed = false;

float g_iMousePopPosition_x = 0;
float g_iMousePopPosition_y = 0;

char m_ID[6][11];
char m_Psz[6][11];
char m_Version[11];
char m_ExeVersion[11];
int  m_SaveAccount;
int  m_SoundOnOff;
int  m_MusicOnOff;
int  m_Resolution;
int targetIndexFPS;
int	m_nColorDepth;
int	g_iRenderTextType = 0;

char Mp3FileName[256];
std::string g_strSelectedML = "";
char g_aszMLSelection[MAX_LANGUAGE_NAME_LENGTH] = { '\0' };

HANDLE g_hMainExe = INVALID_HANDLE_VALUE;

char g_lpszCmdURL[50];
BOOL g_bInactiveTimeChecked = FALSE;

// =====================================================================
// IDIOMA / CONSTANTES DE UI (uso interno do arquivo)
// =====================================================================
namespace
{
	const char* kSupportedLangCodes[] = { "Eng", "Por", "Spn", "Chs", "Kor" };
	const int kSupportedLangCodeCount = sizeof(kSupportedLangCodes) / sizeof(kSupportedLangCodes[0]);

	// Qtd. de idiomas selecionáveis no diálogo de configuração (subconjunto de kSupportedLangCodes)
	const int kConfigDialogLangCount = 3;

	// Radio buttons do diálogo de FPS alvo (compartilhado entre WM_INITDIALOG e WM_COMMAND)
	const int kFpsRadioButtons[] = { IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4 };
	const int kFpsRadioButtonCount = sizeof(kFpsRadioButtons) / sizeof(kFpsRadioButtons[0]);

	int GetLangIndexFromCode(const char* code)
	{
		if (code == NULL || code[0] == 0)
		{
			return 0;
		}

		for (int i = 0; i < kSupportedLangCodeCount; ++i)
		{
			if (_stricmp(code, kSupportedLangCodes[i]) == 0)
			{
				return i;
			}
		}

		return 0;
	}

	const char* GetLangCodeFromIndex(int index)
	{
		if (index < 0 || index >= kSupportedLangCodeCount)
		{
			return kSupportedLangCodes[0];
		}

		return kSupportedLangCodes[index];
	}

	void NormalizeSelectedLanguageCode(char* outCode, size_t outCodeSize)
	{
		if (outCode == NULL || outCodeSize == 0)
		{
			return;
		}

		const char* normalizedCode = GetLangCodeFromIndex(GetLangIndexFromCode(outCode));
		strcpy_s(outCode, outCodeSize, normalizedCode);
	}
}

// =====================================================================
// ÁUDIO (MP3 DE FUNDO)
// =====================================================================
// Assumido (declarado em outro módulo):
//   extern char Mp3FileName[MAX_PATH];   // nome da faixa atual ('\0' = nenhuma)
//   extern BOOL m_MusicOnOff;            // música habilitada nas opções
//   extern BOOL Destroy;                 // aplicação em encerramento
//   wzAudioGetStreamOffsetRange();       // retorna o progresso da faixa em percentual (0..100).

static const int MP3_END_OFFSET = 100;

static inline bool IsMp3Loaded()
{
	return Mp3FileName[0] != '\0';
}

void StopMp3(const char* Name, BOOL bEnforce)
{
	if (Name == NULL)
		return;
	if (!m_MusicOnOff && !bEnforce)
		return;
	if (!IsMp3Loaded())
		return;
	if (_stricmp(Name, Mp3FileName) != 0)
		return;

	wzAudioStop();
	Mp3FileName[0] = '\0';
}

void PlayMp3(const char* Name, BOOL bEnforce)
{
	if (Destroy)
		return;
	if (Name == NULL || Name[0] == '\0')
		return;
	if (!m_MusicOnOff && !bEnforce)
		return;
	if (_stricmp(Name, Mp3FileName) == 0)   // já está tocando
		return;

	wzAudioPlay(Name, 1);

	// Atualiza o nome apenas depois de chamar o player, com terminador garantido.
	strncpy(Mp3FileName, Name, sizeof(Mp3FileName) - 1);
	Mp3FileName[sizeof(Mp3FileName) - 1] = '\0';
}

bool IsEndMp3()
{
	return wzAudioGetStreamOffsetRange() >= MP3_END_OFFSET;
}

int GetMp3PlayPosition()
{
	return wzAudioGetStreamOffsetRange();
}

void CheckHack(void)
{
	SendCheck();
}

// =====================================================================
// JANELA / CONTEXTO OPENGL
// =====================================================================
GLvoid KillGLWindow(GLvoid)
{
	if (g_hRC)
	{
		if (!wglMakeCurrent(NULL, NULL))
		{
			g_ErrorReport.Write("GL - Release Of DC And RC Failed\r\n");
			MessageBox(NULL, "Release Of DC And RC Failed.", "Error", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(g_hRC))
		{
			g_ErrorReport.Write("GL - Release Rendering Context Failed\r\n");
			MessageBox(NULL, "Release Rendering Context Failed.", "Error", MB_OK | MB_ICONINFORMATION);
		}

		g_hRC = NULL;
	}

	if (g_hDC && !ReleaseDC(gwinhandle->GethWnd(), g_hDC))
	{
		g_ErrorReport.Write("GL - OpenGL Release Error\r\n");
		MessageBox(NULL, "OpenGL Release Error.", "Error", MB_OK | MB_ICONINFORMATION);
		g_hDC = NULL;
	}

	if (!gwinhandle->CheckWndMode())
	{
		ChangeDisplaySettings(NULL, 0);
		ShowCursor(TRUE);
	}
}

namespace
{
	// Unifica os blocos repetidos de "falhou X, loga, mata janela GL, mostra MessageBox, retorna false"
	// que existiam em CreateOpenglWindow().
	bool FailOpenGLStep(const char* logFormat, const char* msgBoxCaption)
	{
		g_ErrorReport.Write(logFormat, GetLastError());
		KillGLWindow();
		MessageBox(NULL, GlobalText[4], msgBoxCaption, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
}

bool CreateOpenglWindow()
{
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;

	if (!(g_hDC = GetDC(gwinhandle->GethWnd())))
	{
		return FailOpenGLStep("OpenGL Get DC Error - ErrorCode : %d\r\n", "OpenGL Get DC Error.");
	}

	GLuint PixelFormat;

	if (!(PixelFormat = ChoosePixelFormat(g_hDC, &pfd)))
	{
		return FailOpenGLStep("OpenGL Choose Pixel Format Error - ErrorCode : %d\r\n", "OpenGL Choose Pixel Format Error.");
	}

	if (!DescribePixelFormat(g_hDC, PixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
	{
		return FailOpenGLStep("OpenGL Describe Pixel Format Error - ErrorCode : %d\r\n", "OpenGL Describe Pixel Format Error.");
	}

	if (!SetPixelFormat(g_hDC, PixelFormat, &pfd))
	{
		return FailOpenGLStep("OpenGL Set Pixel Format Error - ErrorCode : %d\r\n", "OpenGL Set Pixel Format Error.");
	}

	if (!(g_hRC = wglCreateContext(g_hDC)))
	{
		return FailOpenGLStep("OpenGL Create Context Error - ErrorCode : %d\r\n", "OpenGL Create Context Error.");
	}

	if (!wglMakeCurrent(g_hDC, g_hRC))
	{
		return FailOpenGLStep("OpenGL Make Current Error - ErrorCode : %d\r\n", "OpenGL Make Current Error.");
	}

	const GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK)
	{
		g_ErrorReport.Write("OpenGL extension loader warning: %s\r\n",
			reinterpret_cast<const char*>(glewGetErrorString(glewStatus)));
	}

	const GLubyte* glVendor = glGetString(GL_VENDOR);
	const GLubyte* glRenderer = glGetString(GL_RENDERER);
	const GLubyte* glVersion = glGetString(GL_VERSION);
	GLint maxTextureSize = 0;
	GLint maxTextureUnits = 1;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	if (GLEW_VERSION_1_3 || GLEW_ARB_multitexture)
		glGetIntegerv(GL_MAX_TEXTURE_UNITS, &maxTextureUnits);

	g_ErrorReport.Write("OpenGL GPU vendor: %s\r\n", glVendor ? reinterpret_cast<const char*>(glVendor) : "Unknown");
	g_ErrorReport.Write("OpenGL GPU renderer: %s\r\n", glRenderer ? reinterpret_cast<const char*>(glRenderer) : "Unknown");
	g_ErrorReport.Write("OpenGL version: %s | Max texture: %d | Texture units: %d\r\n",
		glVersion ? reinterpret_cast<const char*>(glVersion) : "Unknown",
		maxTextureSize, maxTextureUnits);

	glEnable(GL_MULTISAMPLE);

	ShowWindow(gwinhandle->GethWnd(), SW_SHOW);
	SetForegroundWindow(gwinhandle->GethWnd());
	SetFocus(gwinhandle->GethWnd());

	return true;
}

void CreateImGuiWindow()
{
#ifdef IMPLEMENT_IMGUI_WIN32
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(gwinhandle->GethWnd());
	ImGui_ImplOpenGL2_Init();
#endif // IMPLEMENT_IMGUI_WIN32
}

void DestroyImGuiWindow()
{
#ifdef IMPLEMENT_IMGUI_WIN32
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // IMPLEMENT_IMGUI_WIN32
}

void DestroyWindow()
{
	//. save volume level
	leaf::CRegKey regkey;
	regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
	regkey.WriteDword("VolumeLevel", g_pOption->GetVolumeLevel());

	CUIMng::Instance().Release();

#ifdef MOVIE_DIRECTSHOW
	if (g_pMovieScene)
	{
		g_pMovieScene->Destroy();
	}
#endif // MOVIE_DIRECTSHOW

	//. release font handle
	if (g_hFont)
		DeleteObject((HGDIOBJ)g_hFont);

	if (g_hFontBold)
		DeleteObject((HGDIOBJ)g_hFontBold);

	if (g_hFontBig)
		DeleteObject((HGDIOBJ)g_hFontBig);

	ReleaseCharacters();

	if (g_hFixFont)
		DeleteObject((HGDIOBJ)g_hFixFont);

	SAFE_DELETE_ARRAY(GateAttribute);
	SAFE_DELETE_ARRAY(SkillAttribute);
	SAFE_DELETE(CharacterMachine);

	DeleteWaterTerrain();

#ifdef MOVIE_DIRECTSHOW
	if (SceneFlag != MOVIE_SCENE)
#endif // MOVIE_DIRECTSHOW
	{
		gMapManager->DeleteObjects();

		// Object.
		for (int i = MODEL_LOGO; i < MAX_MODELS; i++)
		{
			gmClientModels->GetModel(i)->Release();
		}
		// Bitmap
		Bitmaps.UnloadAllImages();
	}

	SAFE_DELETE_ARRAY(RendomMemoryDump);

#ifdef DYNAMIC_FRUSTRUM
	DeleteAllFrustrum();
#endif //DYNAMIC_FRUSTRUM

	SAFE_DELETE(g_pSingleTextInputBox);
	SAFE_DELETE(g_pSinglePasswdInputBox);

	SAFE_DELETE(g_pChatRoomSocketList);
	SAFE_DELETE(g_pUIMapName);	// rozy
	SAFE_DELETE(g_pTimer);
	SAFE_DELETE(g_pUIManager);

#ifdef MOVIE_DIRECTSHOW
	SAFE_DELETE(g_pMovieScene);
#endif // MOVIE_DIRECTSHOW

	SAFE_DELETE(pMultiLanguage);
	BoostRest(g_BuffSystem);
	BoostRest(g_MapProcess);
	BoostRest(g_petProcess);

	g_ErrorReport.Write("Destroy");

	HWND shWnd = FindWindow(NULL, "MuPlayer");
	if (shWnd)
		SendMessage(shWnd, WM_DESTROY, 0, 0);
}

void DestroySound()
{
	for (int i = 0; i < MAX_BUFFER; i++)
		ReleaseBuffer(i);

	FreeDirectSound();
	wzAudioDestroy();
}

// =====================================================================
// ARQUIVOS / CAMINHOS
// =====================================================================
BOOL GetFileNameOfFilePath(char* lpszFile, char* lpszPath)
{
	if (lpszPath != NULL)
	{
		int iFind = (int)'\\';
		char* lpFound = lpszPath;
		char* lpOld = lpFound;
		while (lpFound)
		{
			lpOld = lpFound;
			lpFound = strchr(lpFound + 1, iFind);
		}

		if (strchr(lpszPath, iFind))
		{
			strcpy(lpszFile, lpOld + 1);
		}
		else
		{
			strcpy(lpszFile, lpOld);
		}

		BOOL bCheck = TRUE;
		for (char* lpTemp = lpszFile; bCheck; ++lpTemp)
		{
			switch (*lpTemp)
			{
			case '\"':
			case '\\':
			case '/':
			case ' ':
				*lpTemp = '\0';
			case '\0':
				bCheck = FALSE;
				break;
			}
		}
	}
	return (TRUE);
}

BOOL OpenMainExe(void)
{
#ifdef _DEBUG
	return (TRUE);
#endif
	char lpszFile[MAX_PATH];
	char* lpszCommandLine = GetCommandLine();
	GetFileNameOfFilePath(lpszFile, lpszCommandLine);

	g_hMainExe = CreateFile((char*)lpszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	return (INVALID_HANDLE_VALUE != g_hMainExe);
}

void CloseMainExe(void)
{
	CloseHandle(g_hMainExe);

#ifdef MAX_INSTANCE_GAME
	GMProtect->runtime_delete_mutex();
#endif // MAX_INSTANCE_GAME
}

BOOL GetFileVersion(char* lpszFileName, WORD* pwVersion)
{
	DWORD dwHandle = 1;
	DWORD dwLen = GetFileVersionInfoSize(lpszFileName, &dwHandle);

	if (dwLen == 0 || dwHandle == 0)
	{
		return (FALSE);
	}

	BYTE* pbyData = new BYTE[dwLen];

	if (GetFileVersionInfo(lpszFileName, dwHandle, dwLen, pbyData) == false)
	{
		delete[] pbyData;
		return (FALSE);
	}

	VS_FIXEDFILEINFO* pffi;
	UINT uLen;
	if (!VerQueryValue(pbyData, "\\", (LPVOID*)&pffi, &uLen))
	{
		delete[] pbyData;
		return (FALSE);
	}

	pwVersion[0] = HIWORD(pffi->dwFileVersionMS);
	pwVersion[1] = LOWORD(pffi->dwFileVersionMS);
	pwVersion[2] = HIWORD(pffi->dwFileVersionLS);
	pwVersion[3] = LOWORD(pffi->dwFileVersionLS);

	delete[] pbyData;

	return (TRUE);
}

BOOL UpdateFile(char* lpszOld, char* lpszNew)
{
	SetFileAttributes(lpszOld, FILE_ATTRIBUTE_NORMAL);
	SetFileAttributes(lpszNew, FILE_ATTRIBUTE_NORMAL);

	DWORD dwStartTickCount = ::GetTickCount();

	while (::GetTickCount() - dwStartTickCount < 5000)
	{
		if (CopyFile(lpszOld, lpszNew, FALSE))
		{	// ok
			DeleteFile(lpszOld);
			return (TRUE);
		}
	}
	g_ErrorReport.Write("%s to %s CopyFile Error : %d\r\n", lpszNew, lpszOld, GetLastError());
	return (FALSE);
}

BOOL KillExeProcess(char* lpszExe)
{
	HANDLE hProcessSnap = NULL;
	BOOL bRet = FALSE;
	PROCESSENTRY32 pe32 = { 0 };

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return (FALSE);

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcessSnap, &pe32))
	{
		do
		{
			if (stricmp(pe32.szExeFile, lpszExe) == 0)
			{
				HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

				if (process)
				{
					TerminateProcess(process, 0);
				}
			}
		} while (Process32Next(hProcessSnap, &pe32));
		bRet = TRUE;
	}
	else
		bRet = FALSE;

	CloseHandle(hProcessSnap);

	return bRet;
}

// =====================================================================
// CHECKSUM / ANTI-CHEAT
// =====================================================================
WORD DecryptCheckSumKey(WORD wSource)
{
	WORD wAcc = wSource ^ 0xB479;
	return ((wAcc >> 10) << 4) | (wAcc & 0xF);
}

DWORD GenerateCheckSum(BYTE* pbyBuffer, DWORD dwSize, WORD wKey)
{
	DWORD dwKey = (DWORD)wKey;
	DWORD dwResult = dwKey << 9;
	for (DWORD dwChecked = 0; dwChecked <= dwSize - 4; dwChecked += 4)
	{
		DWORD dwTemp;
		memcpy(&dwTemp, pbyBuffer + dwChecked, sizeof(DWORD));

		switch ((dwChecked / 4 + wKey) % 3)
		{
		case 0:
			dwResult ^= dwTemp;
			break;
		case 1:
			dwResult += dwTemp;
			break;
		case 2:
			dwResult <<= (dwTemp % 11);
			dwResult ^= dwTemp;
			break;
		}

		if (0 == (dwChecked % 4))
		{
			dwResult ^= ((dwKey + dwResult) >> ((dwChecked / 4) % 16 + 3));
		}
	}

	return (dwResult);
}

DWORD GetCheckSum(WORD wKey)
{
	wKey = DecryptCheckSumKey(wKey);

	const char* lpszFile = "data\\local\\Gameguard.csr";

	HANDLE hFile = SystemCompatibility::CreateSequentialReadHandleA(lpszFile, FILE_SHARE_READ);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return (0);
	}

	DWORD dwSize = GetFileSize(hFile, NULL);
	BYTE* pbyBuffer = new BYTE[dwSize];
	DWORD dwNumber = 0;

	// FIX: o handle era vazado quando ReadFile falhava (não entrava no antigo if).
	// Comportamento preservado: o checksum ainda é calculado mesmo que a leitura falhe,
	// exatamente como no código original.
	ReadFile(hFile, pbyBuffer, dwSize, &dwNumber, 0);
	CloseHandle(hFile);

	DWORD dwCheckSum = GenerateCheckSum(pbyBuffer, dwSize, wKey);
	delete[] pbyBuffer;

	return (dwCheckSum);
}

// =====================================================================
// CONFIG / LINHA DE COMANDO
// =====================================================================
namespace
{
	void RegReadIntOrDefault(HKEY hKey, const char* valueName, int& outValue, int defaultValue)
	{
		DWORD size = sizeof(int);
		if (RegQueryValueEx(hKey, valueName, 0, NULL, (LPBYTE)&outValue, &size) != ERROR_SUCCESS)
		{
			outValue = defaultValue;
		}
	}

	void RegReadStringOrDefault(HKEY hKey, const char* valueName, char* outBuffer, DWORD bufferSize, const char* defaultValue)
	{
		DWORD size = bufferSize;
		if (RegQueryValueEx(hKey, valueName, 0, NULL, (LPBYTE)outBuffer, &size) != ERROR_SUCCESS)
		{
			strcpy_s(outBuffer, bufferSize, defaultValue);
		}
	}
}

BOOL OpenInitFile()
{
	char szIniFilePath[256 + 20] = "";
	char szCurrentDir[256];

	GetCurrentDirectory(256, szCurrentDir);

	strcpy(szIniFilePath, szCurrentDir);
	if (szCurrentDir[strlen(szCurrentDir) - 1] == '\\')
		strcat(szIniFilePath, "config.ini");
	else
		strcat(szIniFilePath, "\\config.ini");

	GetPrivateProfileString("LOGIN", "Version", "", m_Version, 11, szIniFilePath);

	GMProtect->HookComplete();

	char* lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	if (GetFileNameOfFilePath(lpszFile, lpszCommandLine))
	{
		WORD wVersion[4];
		if (GetFileVersion(lpszFile, wVersion))
		{
			sprintf(m_ExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if (wVersion[2] > 0)
			{
				char lpszMinorVersion[3] = "a";
				if (wVersion[2] > 26)
				{
					lpszMinorVersion[0] = 'A';
					lpszMinorVersion[0] += (wVersion[2] - 27);
					lpszMinorVersion[1] = '+';
				}
				else
				{
					lpszMinorVersion[0] += (wVersion[2] - 1);
				}
				strcat(m_ExeVersion, lpszMinorVersion);
			}
		}
		else
		{
			strcpy(m_ExeVersion, m_Version);
		}
	}
	else
	{
		strcpy(m_ExeVersion, m_Version);
	}

	m_SoundOnOff = 1;
	m_MusicOnOff = 1;
	m_Resolution = 1;
	m_nColorDepth = 0;
	targetIndexFPS = 0;

	memset(m_ID, 0, sizeof(m_ID));
	memset(m_Psz, 0, sizeof(m_Psz));

	// FIX: hKey era usado não-inicializado em RegCloseKey() quando RegCreateKeyEx falhava.
	HKEY hKey = NULL;
	DWORD dwDisp;
	DWORD dwSize;
	int bUseWindowMode = TRUE;

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp))
	{
#ifdef SAVE_ACCOUNT_SYSTEM
		dwSize = sizeof(m_ID);
		if (RegQueryValueEx(hKey, "_LiwID", 0, REG_NONE, (LPBYTE)m_ID, &dwSize) == ERROR_SUCCESS)
		{
			dwSize = sizeof(m_Psz);
			RegQueryValueEx(hKey, "_LiwKey", 0, REG_NONE, (LPBYTE)m_Psz, &dwSize);
		}

		RegReadIntOrDefault(hKey, "SaveAccount", m_SaveAccount, true);
#endif

		dwSize = 11;
		RegQueryValueEx(hKey, "ID", 0, NULL, (LPBYTE)m_ID[MAX_ACCOUNT_REG], &dwSize);

		RegReadIntOrDefault(hKey, "SoundOnOff", m_SoundOnOff, true);
		RegReadIntOrDefault(hKey, "MusicOnOff", m_MusicOnOff, false);
		RegReadIntOrDefault(hKey, "Resolution", m_Resolution, 1);
		RegReadIntOrDefault(hKey, "ColorDepth", m_nColorDepth, 0);
		RegReadIntOrDefault(hKey, "TextOut", g_iRenderTextType, 0);
		RegReadIntOrDefault(hKey, "targetIndexFPS", targetIndexFPS, 0);

		g_iChatInputType = 1;

		RegReadIntOrDefault(hKey, "WindowMode", bUseWindowMode, TRUE);
		RegReadStringOrDefault(hKey, "LangSelection", g_aszMLSelection, sizeof(g_aszMLSelection), "Eng");

		NormalizeSelectedLanguageCode(g_aszMLSelection, sizeof(g_aszMLSelection));
		g_strSelectedML = g_aszMLSelection;
	}

	if (hKey)
	{
		RegCloseKey(hKey);
	}

	gwinhandle->SetWndMode(bUseWindowMode);
	gwinhandle->SetDisplayIndex(m_Resolution);

	return TRUE;
}

BOOL Util_CheckOption(char* lpszCommandLine, unsigned char cOption, char* lpszString)
{
	unsigned char cComp[2];
	cComp[0] = cOption; cComp[1] = cOption;
	if (islower((int)cOption))
	{
		cComp[1] = toupper((int)cOption);
	}
	else if (isupper((int)cOption))
	{
		cComp[1] = tolower((int)cOption);
	}

	int nFind = (int)'/';
	unsigned char* lpFound = (unsigned char*)lpszCommandLine;
	while (lpFound)
	{
		lpFound = (unsigned char*)strchr((char*)(lpFound + 1), nFind);
		if (lpFound && (*(lpFound + 1) == cComp[0] || *(lpFound + 1) == cComp[1]))
		{
			if (lpszString)
			{
				int nCount = 0;
				for (unsigned char* lpSeek = lpFound + 2; *lpSeek != ' ' && *lpSeek != '\0'; lpSeek++)
				{
					nCount++;
				}

				memcpy(lpszString, lpFound + 2, nCount);
				lpszString[nCount] = '\0';
			}
			return (TRUE);
		}
	}

	return (FALSE);
}

BOOL GetConnectServerInfo(PSTR szCmdLine, char* lpszURL, WORD* pwPort)
{
	char lpszTemp[256] = { 0, };
	if (Util_CheckOption(szCmdLine, 'y', lpszTemp))
	{
		BYTE bySuffle[] = { 0x0C, 0x07, 0x03, 0x13 };

		for (int i = 0; i < (int)strlen(lpszTemp); i++)
			lpszTemp[i] -= bySuffle[i % 4];
		strcpy(lpszURL, lpszTemp);

		if (Util_CheckOption(szCmdLine, 'z', lpszTemp))
		{
			for (int j = 0; j < (int)strlen(lpszTemp); j++)
				lpszTemp[j] -= bySuffle[j % 4];
			*pwPort = atoi(lpszTemp);
		}
		g_ErrorReport.Write("[Virtual Connection] Connect IP : %s, Port : %d\r\n", lpszURL, *pwPort);
		return (TRUE);
	}
	if (!Util_CheckOption(szCmdLine, 'u', lpszTemp))
	{
		return (FALSE);
	}
	strcpy(lpszURL, lpszTemp);
	if (!Util_CheckOption(szCmdLine, 'p', lpszTemp))
	{
		return (FALSE);
	}
	*pwPort = atoi(lpszTemp);

	return (TRUE);
}

// =====================================================================
// TRATAMENTO DE CRASH
// =====================================================================
namespace
{
	const char* GetExceptionCodeName(DWORD code)
	{
		switch (code)
		{
		case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "DATATYPE_MISALIGNMENT";
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return "FLT_DENORMAL_OPERAND";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INEXACT_RESULT:       return "FLT_INEXACT_RESULT";
		case EXCEPTION_FLT_INVALID_OPERATION:    return "FLT_INVALID_OPERATION";
		case EXCEPTION_FLT_OVERFLOW:             return "FLT_OVERFLOW";
		case EXCEPTION_FLT_STACK_CHECK:          return "FLT_STACK_CHECK";
		case EXCEPTION_FLT_UNDERFLOW:            return "FLT_UNDERFLOW";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR:            return "IN_PAGE_ERROR";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_INT_OVERFLOW:             return "INT_OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION:      return "INVALID_DISPOSITION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
		case EXCEPTION_PRIV_INSTRUCTION:         return "PRIV_INSTRUCTION";
		case EXCEPTION_SINGLE_STEP:              return "SINGLE_STEP";
		case EXCEPTION_STACK_OVERFLOW:           return "STACK_OVERFLOW";
		default:                                 return "UNKNOWN_EXCEPTION";
		}
	}

	void GetModuleFromAddress(void* address, char* outModule, size_t outModuleSize, DWORD_PTR* outOffset)
	{
		if (!address || !outModule || outModuleSize == 0 || !outOffset)
		{
			return;
		}

		HMODULE hModule = NULL;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)address, &hModule))
		{
			GetModuleFileNameA(hModule, outModule, (DWORD)outModuleSize);
			*outOffset = ((DWORD_PTR)address - (DWORD_PTR)hModule);
		}
		else
		{
			strcpy_s(outModule, outModuleSize, "UnknownModule");
			*outOffset = 0;
		}
	}

	// hFile valido -> escreve no arquivo de log do crash.
	// hFile == INVALID_HANDLE_VALUE -> escreve via g_ErrorReport.
	void WriteFormattedLine(HANDLE hFile, const char* fmt, ...)
	{
		char buffer[2048] = { 0 };
		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(buffer, fmt, ap);
		va_end(ap);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD written = 0;
			::WriteFile(hFile, buffer, (DWORD)strlen(buffer), &written, 0);
		}
		else
		{
			g_ErrorReport.Write("%s", buffer);
		}
	}

	// Unifica as antigas WriteCrashStack() / WriteCrashStackToMuError(), que eram
	// ~95% código duplicado (mesma caminhada de pilha, apenas destino do log diferente).
	// A saída textual gerada é idêntica à das duas funções originais.
	void DumpStackTrace(HANDLE hFile, CONTEXT* context)
	{
		if (context == 0)
		{
			return;
		}

		const bool toErrorReport = (hFile == INVALID_HANDLE_VALUE);
		const char* initFailTag = toErrorReport ? "[CrashStack] " : "[StackTrace] ";
		const char* perLineTag = toErrorReport ? "[CrashStack] " : "";
		const char* header = toErrorReport ? "[CrashStack] Begin\r\n" : "\r\n[StackTrace]\r\n";

		HANDLE hProcess = GetCurrentProcess();
		HANDLE hThread = GetCurrentThread();

		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		if (!SymInitialize(hProcess, 0, TRUE))
		{
			WriteFormattedLine(hFile, "%sSymInitialize failed. Error=%lu\r\n", initFailTag, GetLastError());
			return;
		}

		STACKFRAME64 stackFrame;
		memset(&stackFrame, 0, sizeof(stackFrame));

		DWORD machineType = 0;
#if defined(_M_IX86)
		machineType = IMAGE_FILE_MACHINE_I386;
		stackFrame.AddrPC.Offset = context->Eip;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = context->Ebp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = context->Esp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_X64)
		machineType = IMAGE_FILE_MACHINE_AMD64;
		stackFrame.AddrPC.Offset = context->Rip;
		stackFrame.AddrPC.Mode = AddrModeFlat;
		stackFrame.AddrFrame.Offset = context->Rsp;
		stackFrame.AddrFrame.Mode = AddrModeFlat;
		stackFrame.AddrStack.Offset = context->Rsp;
		stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

		WriteFormattedLine(hFile, "%s", header);

		for (int i = 0; i < 64; i++)
		{
			if (!StackWalk64(machineType, hProcess, hThread, &stackFrame, context, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
			{
				break;
			}

			if (stackFrame.AddrPC.Offset == 0)
			{
				break;
			}

			DWORD64 address = stackFrame.AddrPC.Offset;
			char symbolStorage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
			PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolStorage;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = MAX_SYM_NAME;

			DWORD64 displacement = 0;
			char stackModulePath[MAX_PATH] = { 0 };
			DWORD_PTR stackModuleOffset = 0;
			GetModuleFromAddress((void*)address, stackModulePath, MAX_PATH, &stackModuleOffset);

			IMAGEHLP_LINE64 lineInfo;
			memset(&lineInfo, 0, sizeof(lineInfo));
			lineInfo.SizeOfStruct = sizeof(lineInfo);
			DWORD lineDisplacement = 0;

			if (SymFromAddr(hProcess, address, &displacement, symbol))
			{
				if (SymGetLineFromAddr64(hProcess, address, &lineDisplacement, &lineInfo))
				{
					WriteFormattedLine(hFile, "%s#%02d 0x%p %s + 0x%I64X | %s:%lu\r\n", perLineTag, i, (void*)address, symbol->Name, displacement, lineInfo.FileName, lineInfo.LineNumber);
				}
				else
				{
					WriteFormattedLine(hFile, "%s#%02d 0x%p %s + 0x%I64X | %s + 0x%p\r\n", perLineTag, i, (void*)address, symbol->Name, displacement, stackModulePath, (void*)stackModuleOffset);
				}
			}
			else
			{
				WriteFormattedLine(hFile, "%s#%02d 0x%p | %s + 0x%p\r\n", perLineTag, i, (void*)address, stackModulePath, (void*)stackModuleOffset);
			}
		}

		if (toErrorReport)
		{
			WriteFormattedLine(hFile, "%sEnd\r\n", perLineTag);
		}

		SymCleanup(hProcess);
	}
}

bool ExceptionCallback(_EXCEPTION_POINTERS* pExceptionInfo)
{
	if (InterlockedCompareExchange(&g_CrashCaptureInProgress, 1, 0) != 0)
	{
		return true;
	}

	GAME_LOG_ERROR("Crash", "Unhandled exception captured in ExceptionCallback");

	char logPath[MAX_PATH];
	char reportDir[MAX_PATH] = { 0 };
	char exePath[MAX_PATH] = { 0 };
	char modulePath[MAX_PATH] = { 0 };
	DWORD_PTR moduleOffset = 0;

	// Renomeado de "path" para "dumpFilePath": o nome "path" colidia (shadowing)
	// com a global "extern PATH* path;" declarada no topo do arquivo.
	char dumpFilePath[MAX_PATH];
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	if (GetModuleFileNameA(0, exePath, MAX_PATH) > 0)
	{
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash)
		{
			*lastSlash = 0;
			wsprintf(reportDir, "%s\\CrashReports", exePath);
			CreateDirectoryA(reportDir, 0);
		}
	}

	if (reportDir[0] != 0)
	{
		wsprintf(dumpFilePath, "%s\\%04d-%02d-%02d_%02dh%02dm%02ds.dmp", reportDir, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
		wsprintf(logPath, "%s\\%04d-%02d-%02d_%02dh%02dm%02ds.crash.log", reportDir, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	}
	else
	{
		wsprintf(dumpFilePath, "%04d-%02d-%02d_%02dh%02dm%02ds.dmp", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
		wsprintf(logPath, "%04d-%02d-%02d_%02dh%02dm%02ds.crash.log", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	}

	if (pExceptionInfo && pExceptionInfo->ExceptionRecord)
	{
		GetModuleFromAddress(pExceptionInfo->ExceptionRecord->ExceptionAddress, modulePath, MAX_PATH, &moduleOffset);
		GAME_LOG_ERROR_F("Crash", "Code=0x%08X Address=0x%p Origin=%s+0x%p",
			pExceptionInfo->ExceptionRecord->ExceptionCode,
			pExceptionInfo->ExceptionRecord->ExceptionAddress,
			modulePath,
			(void*)moduleOffset);
	}

	HANDLE crashLogFile = CreateFile(logPath, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (crashLogFile != INVALID_HANDLE_VALUE && pExceptionInfo && pExceptionInfo->ExceptionRecord)
	{
		WriteFormattedLine(crashLogFile, "====== MAIN5.2 CRASH REPORT ======\r\n");
		WriteFormattedLine(crashLogFile, "DateTime: %04d-%02d-%02d %02d:%02d:%02d\r\n", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
		WriteFormattedLine(crashLogFile, "ThreadId: %lu\r\n", GetCurrentThreadId());
		WriteFormattedLine(crashLogFile, "ExceptionCode: 0x%08X (%s)\r\n", pExceptionInfo->ExceptionRecord->ExceptionCode, GetExceptionCodeName(pExceptionInfo->ExceptionRecord->ExceptionCode));
		WriteFormattedLine(crashLogFile, "ExceptionAddress: 0x%p\r\n", pExceptionInfo->ExceptionRecord->ExceptionAddress);
		WriteFormattedLine(crashLogFile, "OriginModule: %s + 0x%p\r\n", modulePath, (void*)moduleOffset);
		WriteFormattedLine(crashLogFile, "ExceptionFlags: 0x%08X\r\n", pExceptionInfo->ExceptionRecord->ExceptionFlags);
		WriteFormattedLine(crashLogFile, "Parameters: %lu\r\n", pExceptionInfo->ExceptionRecord->NumberParameters);
		for (DWORD i = 0; i < pExceptionInfo->ExceptionRecord->NumberParameters; i++)
		{
			WriteFormattedLine(crashLogFile, "  Param[%lu] = 0x%p\r\n", i, (void*)pExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
		}

#if defined(_M_IX86)
		if (pExceptionInfo->ContextRecord)
		{
			WriteFormattedLine(crashLogFile, "\r\n[Registers]\r\n");
			WriteFormattedLine(crashLogFile, "EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\r\n", pExceptionInfo->ContextRecord->Eax, pExceptionInfo->ContextRecord->Ebx, pExceptionInfo->ContextRecord->Ecx, pExceptionInfo->ContextRecord->Edx);
			WriteFormattedLine(crashLogFile, "ESI=0x%08X EDI=0x%08X EBP=0x%08X ESP=0x%08X\r\n", pExceptionInfo->ContextRecord->Esi, pExceptionInfo->ContextRecord->Edi, pExceptionInfo->ContextRecord->Ebp, pExceptionInfo->ContextRecord->Esp);
			WriteFormattedLine(crashLogFile, "EIP=0x%08X EFlags=0x%08X\r\n", pExceptionInfo->ContextRecord->Eip, pExceptionInfo->ContextRecord->EFlags);
		}
#elif defined(_M_X64)
		if (pExceptionInfo->ContextRecord)
		{
			WriteFormattedLine(crashLogFile, "\r\n[Registers]\r\n");
			WriteFormattedLine(crashLogFile, "RAX=0x%I64X RBX=0x%I64X RCX=0x%I64X RDX=0x%I64X\r\n", pExceptionInfo->ContextRecord->Rax, pExceptionInfo->ContextRecord->Rbx, pExceptionInfo->ContextRecord->Rcx, pExceptionInfo->ContextRecord->Rdx);
			WriteFormattedLine(crashLogFile, "RSI=0x%I64X RDI=0x%I64X RBP=0x%I64X RSP=0x%I64X\r\n", pExceptionInfo->ContextRecord->Rsi, pExceptionInfo->ContextRecord->Rdi, pExceptionInfo->ContextRecord->Rbp, pExceptionInfo->ContextRecord->Rsp);
			WriteFormattedLine(crashLogFile, "RIP=0x%I64X EFlags=0x%08X\r\n", pExceptionInfo->ContextRecord->Rip, pExceptionInfo->ContextRecord->EFlags);
		}
#endif

		DumpStackTrace(crashLogFile, pExceptionInfo->ContextRecord);
		WriteFormattedLine(crashLogFile, "\r\nDumpFile: %s\r\n", dumpFilePath);
		WriteFormattedLine(crashLogFile, "==================================\r\n");
		CloseHandle(crashLogFile);
	}

	g_ErrorReport.Write("[Crash] Type=%s Code=0x%08X Address=0x%p Origin=%s+0x%p Log=%s Dump=%s\r\n",
		(pExceptionInfo && pExceptionInfo->ExceptionRecord) ? GetExceptionCodeName(pExceptionInfo->ExceptionRecord->ExceptionCode) : "UNKNOWN_EXCEPTION",
		(pExceptionInfo && pExceptionInfo->ExceptionRecord) ? pExceptionInfo->ExceptionRecord->ExceptionCode : 0,
		(pExceptionInfo && pExceptionInfo->ExceptionRecord) ? pExceptionInfo->ExceptionRecord->ExceptionAddress : 0,
		modulePath,
		(void*)moduleOffset,
		logPath,
		dumpFilePath);
	g_ErrorReport.Write("\r\n");
	g_ErrorReport.Write("==================== CRASH START ====================\r\n");
	g_ErrorReport.Write("[CrashMarker] Date=%04d-%02d-%02d Time=%02d:%02d:%02d\r\n", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	if (pExceptionInfo && pExceptionInfo->ExceptionRecord)
	{
		g_ErrorReport.Write("[CrashDetail] Flags=0x%08X Parameters=%lu\r\n", pExceptionInfo->ExceptionRecord->ExceptionFlags, pExceptionInfo->ExceptionRecord->NumberParameters);
		for (DWORD i = 0; i < pExceptionInfo->ExceptionRecord->NumberParameters; i++)
		{
			g_ErrorReport.Write("[CrashDetail] Param[%lu]=0x%p\r\n", i, (void*)pExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
		}
	}

#if defined(_M_IX86)
	if (pExceptionInfo && pExceptionInfo->ContextRecord)
	{
		g_ErrorReport.Write("[CrashReg] EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\r\n", pExceptionInfo->ContextRecord->Eax, pExceptionInfo->ContextRecord->Ebx, pExceptionInfo->ContextRecord->Ecx, pExceptionInfo->ContextRecord->Edx);
		g_ErrorReport.Write("[CrashReg] ESI=0x%08X EDI=0x%08X EBP=0x%08X ESP=0x%08X\r\n", pExceptionInfo->ContextRecord->Esi, pExceptionInfo->ContextRecord->Edi, pExceptionInfo->ContextRecord->Ebp, pExceptionInfo->ContextRecord->Esp);
		g_ErrorReport.Write("[CrashReg] EIP=0x%08X EFlags=0x%08X\r\n", pExceptionInfo->ContextRecord->Eip, pExceptionInfo->ContextRecord->EFlags);
	}
#elif defined(_M_X64)
	if (pExceptionInfo && pExceptionInfo->ContextRecord)
	{
		g_ErrorReport.Write("[CrashReg] RAX=0x%I64X RBX=0x%I64X RCX=0x%I64X RDX=0x%I64X\r\n", pExceptionInfo->ContextRecord->Rax, pExceptionInfo->ContextRecord->Rbx, pExceptionInfo->ContextRecord->Rcx, pExceptionInfo->ContextRecord->Rdx);
		g_ErrorReport.Write("[CrashReg] RSI=0x%I64X RDI=0x%I64X RBP=0x%I64X RSP=0x%I64X\r\n", pExceptionInfo->ContextRecord->Rsi, pExceptionInfo->ContextRecord->Rdi, pExceptionInfo->ContextRecord->Rbp, pExceptionInfo->ContextRecord->Rsp);
		g_ErrorReport.Write("[CrashReg] RIP=0x%I64X EFlags=0x%08X\r\n", pExceptionInfo->ContextRecord->Rip, pExceptionInfo->ContextRecord->EFlags);
	}
#endif

	DumpStackTrace(INVALID_HANDLE_VALUE, pExceptionInfo ? pExceptionInfo->ContextRecord : 0);
	g_ErrorReport.Write("===================== CRASH END =====================\r\n");
	g_ErrorReport.Write("\r\n");
	g_ErrorReport.Flush();

	HANDLE file = CreateFile(dumpFilePath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pExceptionInfo;
		mdei.ClientPointers = FALSE;

		if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithIndirectlyReferencedMemory, &mdei, 0, 0))
		{
			CloseHandle(file);
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}

	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
	}

	InterlockedExchange(&g_CrashCaptureInProgress, 0);

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG WINAPI VectoredCrashHandler(_EXCEPTION_POINTERS* pExceptionInfo)
{
	if (pExceptionInfo == 0 || pExceptionInfo->ExceptionRecord == 0)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
	bool isCriticalCode =
		(code == EXCEPTION_ACCESS_VIOLATION) ||
		(code == EXCEPTION_IN_PAGE_ERROR) ||
		(code == EXCEPTION_STACK_OVERFLOW) ||
		(code == EXCEPTION_ILLEGAL_INSTRUCTION) ||
		(code == EXCEPTION_PRIV_INSTRUCTION) ||
		(code == EXCEPTION_INT_DIVIDE_BY_ZERO) ||
		(code == EXCEPTION_FLT_DIVIDE_BY_ZERO);

	if (isCriticalCode)
	{
		ExceptionCallback(pExceptionInfo);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void OnInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
{
	UNREFERENCED_PARAMETER(reserved);

	char expr[256] = { 0 };
	char func[256] = { 0 };
	char src[256] = { 0 };

	if (expression)
	{
		wcstombs(expr, expression, sizeof(expr) - 1);
	}
	if (function)
	{
		wcstombs(func, function, sizeof(func) - 1);
	}
	if (file)
	{
		wcstombs(src, file, sizeof(src) - 1);
	}

	g_ErrorReport.Write("[RuntimeFault] Invalid parameter. Expr=%s Function=%s File=%s Line=%u\r\n", expr, func, src, line);
	g_ErrorReport.Flush();

	RaiseException(STATUS_INVALID_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, 0);
}

void __cdecl OnPureCall()
{
	g_ErrorReport.Write("[RuntimeFault] Pure virtual function call detected\r\n");
	g_ErrorReport.Flush();
	RaiseException(EXCEPTION_NONCONTINUABLE_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, 0);
}

LONG WINAPI TopLevelExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
	if (ExceptionCallback(pExceptionInfo))
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

// =====================================================================
// THREADS AUXILIARES
// =====================================================================
void ThreadFreeVRAM(LPVOID lpThreadParameter)
{
	const DWORD sec_avg_max = 5000;

	HANDLE hThread = GetCurrentThread();

	SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);
	while (TRUE)
	{
		Sleep(sec_avg_max);
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	}
}

void memory_const_checked(LPVOID lpThreadParameter)
{
	const DWORD sec_avg_max = 60000;

	while (TRUE)
	{
		Sleep(sec_avg_max);
		GMProtect->runtime_checked_crc32();
	}
}

// =====================================================================
// EXPORTS / HINTS DE GPU DEDICADA
// =====================================================================
extern "C"
{
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

extern "C"
{
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

extern "C" __declspec(dllexport) void GetSharedValue(char* appgui, int Size)
{
	if (appgui)
	{
		strncpy(appgui, "{72494e4d-b660-4ec3-b431-f5366b514018}", Size);
	}
}

void EnablePermanentDep()
{
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");

	if (hNtdll == nullptr)
	{
		return;
	}

	lpNtSetInformationProcess NtSetInformationProcess = (lpNtSetInformationProcess)GetProcAddress(hNtdll, "NtSetInformationProcess");
	if (NtSetInformationProcess == nullptr)
	{
		return;
	}

	ULONG ExecuteFlags = 0x02;
	NTSTATUS Status = NtSetInformationProcess(NtCurrentProcess, 0x22, &ExecuteFlags, sizeof(ExecuteFlags));

	if (Status != 0)
	{
		return;
	}
}

// =====================================================================
// DIÁLOGO DE CONFIGURAÇÃO (FPS ALVO / IDIOMA)
// =====================================================================
BOOL CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hDlg, kFpsRadioButtons[targetIndexFPS], BST_CHECKED);

		HWND hLanguageCombo = GetDlgItem(hDlg, IDC_COMBO_LANGUAGE);
		for (int i = 0; i < kConfigDialogLangCount; ++i)
		{
			SendMessage(hLanguageCombo, CB_ADDSTRING, 0, (LPARAM)kSupportedLangCodes[i]);
		}

		int langIndex = GetLangIndexFromCode(g_aszMLSelection);
		if (langIndex >= kConfigDialogLangCount) langIndex = 0;
		SendMessage(hLanguageCombo, CB_SETCURSEL, langIndex, 0);
	}
	return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			for (int i = 0; i < kFpsRadioButtonCount; i++)
			{
				if (IsDlgButtonChecked(hDlg, kFpsRadioButtons[i]) == BST_CHECKED)
				{
					targetIndexFPS = i;
					break;
				}
			}
			leaf::CRegKey regkey;

			regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");
			regkey.WriteDword("targetIndexFPS", targetIndexFPS);

			HWND hLanguageCombo = GetDlgItem(hDlg, IDC_COMBO_LANGUAGE);
			int selectedLangIndex = (int)SendMessage(hLanguageCombo, CB_GETCURSEL, 0, 0);
			const char* selectedLangCode = GetLangCodeFromIndex(selectedLangIndex);
			strcpy_s(g_aszMLSelection, sizeof(g_aszMLSelection), selectedLangCode);
			g_strSelectedML = g_aszMLSelection;
			regkey.WriteString("LangSelection", g_aszMLSelection);

			switch (targetIndexFPS)
			{
			case 1:
				gmProtect->ajust_fps_render = 40;
				break;
			case 2:
				gmProtect->ajust_fps_render = 60;
				break;
			case 3:
				gmProtect->ajust_fps_render = 80;
				break;
			default:
				gmProtect->ajust_fps_render = 30;
				break;
			}

			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// =====================================================================
// PONTO DE ENTRADA
//
// OBS: o corpo abaixo foi mantido na MESMA ORDEM/ESTRUTURA do original.
// Ele contém blocos VM_START/VM_END usados pelo sistema de proteção/
// ofuscação (Themida/GMProtect). Extrair trechos para funções auxiliares
// poderia alterar o comportamento desses marcadores de forma imprevisível
// sem acesso ao gerador/empacotador usado no build real, então essa função
// não foi reestruturada — apenas comentada em seções para leitura.
// =====================================================================
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	GAME_LOG_INFO("Init", "Client bootstrap started");

	SetProcessDPIAware();

	if (GMProtect->ReadMainFile() == false)
	{
		GAME_LOG_ERROR("Init", "ReadMainFile failed");
		ExitProcess(0);
		return 0;
	}

	EnablePermanentDep();

	HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadFreeVRAM, 0, 0, 0);

	//if (GMProtect->IsWindows7OrLower() == false)
	//{
	//	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)memory_const_checked, 0, 0, 0);
	//}

	timeBeginPeriod(1);
	_set_invalid_parameter_handler(OnInvalidParameter);
	_set_purecall_handler(OnPureCall);
	AddVectoredExceptionHandler(1, VectoredCrashHandler);

	leaf::AttachExceptionHandler(ExceptionCallback);
	SetUnhandledExceptionFilter(TopLevelExceptionFilter);

	// --- Versão do executável (para log) ---
	char lpszExeVersion[256] = "unknown";

	char* lpszCommandLine = GetCommandLine();
	char lpszFile[MAX_PATH];
	WORD wVersion[4] = { 0, };
	if (GetFileNameOfFilePath(lpszFile, lpszCommandLine))
	{
		if (GetFileVersion(lpszFile, wVersion))
		{
			sprintf(lpszExeVersion, "%d.%02d", wVersion[0], wVersion[1]);
			if (wVersion[2] > 0)
			{
				char lpszMinorVersion[2] = "a";
				lpszMinorVersion[0] += (wVersion[2] - 1);
				strcat(lpszExeVersion, lpszMinorVersion);
			}
		}
	}

	g_ErrorReport.Write("\r\n");
	g_ErrorReport.WriteLogBegin();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.Write("Mu online %s (%s) executed. (%d.%d.%d.%d)\r\n", lpszExeVersion, "Eng", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ConsoleDebug->Write(MCD_NORMAL, "Mu Online (Version: %d.%d.%d.%d)", wVersion[0], wVersion[1], wVersion[2], wVersion[3]);

	g_ErrorReport.WriteCurrentTime();

	ER_SystemInfo si;

	ZeroMemory(&si, sizeof(ER_SystemInfo));
	GetSystemInfo(&si);

	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSystemInfo(&si);

	SystemCompatibility::SystemProfile systemProfile = {};
	if (SystemCompatibility::Initialize(systemProfile))
	{
		g_ErrorReport.Write("Compatibility: %s (build %lu)\r\n",
			SystemCompatibility::GetWindowsVersionName(systemProfile.windowsVersion),
			systemProfile.windowsBuild);
		g_ErrorReport.Write("CPU: %s | Vendor: %s | Cores: %lu | Threads: %lu\r\n",
			systemProfile.cpuBrand[0] ? systemProfile.cpuBrand : "Unknown",
			systemProfile.cpuVendor[0] ? systemProfile.cpuVendor : "Unknown",
			systemProfile.physicalCores, systemProfile.logicalProcessors);
		g_ErrorReport.Write("CPU features: SSE2=%s SSE4.1=%s AVX=%s AVX2=%s\r\n",
			systemProfile.sse2 ? "Yes" : "No",
			systemProfile.sse41 ? "Yes" : "No",
			systemProfile.avx ? "Yes" : "No",
			systemProfile.avx2 ? "Yes" : "No");
		g_ErrorReport.Write("Memory: total=%I64u MB | available=%I64u MB | virtual=%I64u MB\r\n",
			systemProfile.totalPhysicalMemory / (1024 * 1024),
			systemProfile.availablePhysicalMemory / (1024 * 1024),
			systemProfile.totalVirtualMemory / (1024 * 1024));
		g_ErrorReport.Write("Display adapter: %s\r\n",
			systemProfile.gpuName[0] ? systemProfile.gpuName : "Unknown");
		g_ErrorReport.Write("System drive: %s\r\n",
			SystemCompatibility::GetStorageTypeName(systemProfile.storageType));
	}

	g_ErrorReport.AddSeparator();

	// --- Conexão virtual (linha de comando) ---
	VM_START
		WORD wPortNumber;
	if (GetConnectServerInfo(szCmdLine, g_lpszCmdURL, &wPortNumber))
	{
		szServerIpAddress = g_lpszCmdURL;
		g_ServerPort = wPortNumber;
	}
	VM_END

		if (!OpenMainExe())
		{
			return false;
		}

	VM_START
		g_SimpleModulusCS.LoadEncryptionKey("Data\\Enc1.dat");
	g_SimpleModulusSC.LoadDecryptionKey("Data\\Dec2.dat");
	VM_END

		g_ErrorReport.Write("> To read config.ini.\r\n");

	if (OpenInitFile() == FALSE)
	{
		g_ErrorReport.Write("config.ini read error\r\n");
		GAME_LOG_ERROR("Init", "config.ini read error");
		return false;
	}

	GAME_LOG_INFO("Init", "config.ini loaded successfully");

	GMProtect->CheckPluginFile();

	pMultiLanguage = new CMultiLanguage(g_strSelectedML);

	//if (gmProtect->shutdown_popup)
	//{
	//	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG3), NULL, ConfigDialogProc);
	//}

	if (g_iChatInputType == 1)
		ShowCursor(FALSE);

	// --- Janela / OpenGL ---
	gwinhandle->Create(hInstance, WindowWidth, WindowHeight);

	g_ErrorReport.Write("> Start window success.\r\n");

#ifdef SHUTDOWN_SCALEFORM_INFO
	gfxinit->runtime_connection();
#endif // SHUTDOWN_SCALEFORM_INFO

	if (!CreateOpenglWindow())
	{
		return FALSE;
	}

#ifdef IMPLEMENT_IMGUI_WIN32
	CreateImGuiWindow();
#endif // IMPLEMENT_IMGUI_WIN32

	g_ErrorReport.Write("> OpenGL init success.\r\n");

	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteOpenGLInfo();
	g_ErrorReport.AddSeparator();
	g_ErrorReport.WriteSoundCardInfo();

	ShowWindow(gwinhandle->GethWnd(), nCmdShow);
	UpdateWindow(gwinhandle->GethWnd());

	//g_ErrorReport.WriteImeInfo( gwinhandle->GethWnd());
	g_ErrorReport.AddSeparator();

#ifdef V_SYNCRONIZE
	InitVSync();

	if (IsVSyncAvailable())
	{
		EnableVSync();
		//SetTargetFps(-1); // unlimited
	}
#endif // V_SINCRONIZE

#ifdef SHADER_VERSION_TEST
	gShaderGL->Init();
#endif // SHADER_VERSION_TEST

	setlocale(LC_ALL, "en_US");

	CInput::Instance().Create(gwinhandle->GethWnd(), WindowWidth, WindowHeight);

	g_pNewUISystem->Create();

	// --- Áudio ---
	if (m_MusicOnOff)
	{
		wzAudioCreate(gwinhandle->GethWnd());
		wzAudioOption(WZAOPT_STOPBEFOREPLAY, 1);
	}

	if (m_SoundOnOff)
	{
		DWORD value;

		InitDirectSound(gwinhandle->GethWnd());

		leaf::CRegKey regkey;
		regkey.SetKey(leaf::CRegKey::_HKEY_CURRENT_USER, "SOFTWARE\\Webzen\\Mu2\\Config");

		if (!regkey.ReadDword("VolumeLevel", value))
		{
			value = 5;	//. default setting
			regkey.WriteDword("VolumeLevel", value);
		}
		if (value < 0 || value >= 10)
			value = 5;

		g_pOption->SetVolumeLevel(int(value));
		SetEffectVolumeLevel(g_pOption->GetVolumeLevel());
	}

	SetTimer(gwinhandle->GethWnd(), HACK_TIMER, 20 * 1000, NULL);

	srand((unsigned)time(NULL));
	for (int i = 0; i < 100; i++)
	{
		RandomTable[i] = rand() % 360;
	}

	// --- Estado de jogo ---
	RendomMemoryDump = new BYTE[rand() % 100 + 1];
	GateAttribute = new GATE_ATTRIBUTE[MAX_GATES];
	SkillAttribute = new SKILL_ATTRIBUTE[MAX_SKILLS];

	static CGMCharacter gcCharacter;
	static CGMModelManager gcModel;

	CharacterMachine = new CHARACTER_MACHINE;

	memset(GateAttribute, 0, sizeof(GATE_ATTRIBUTE) * (MAX_GATES));
	memset(SkillAttribute, 0, sizeof(SKILL_ATTRIBUTE) * (MAX_SKILLS));
	memset(CharacterMachine, 0, sizeof(CHARACTER_MACHINE));

	CharacterAttribute = &CharacterMachine->Character;
	CharacterMachine->Init();

	Hero = gmCharacters->GetCharacter(0);

	if (g_iChatInputType == 1)
	{
		g_pSingleTextInputBox = new CUITextInputBox;
		g_pSinglePasswdInputBox = new CUITextInputBox;
	}

	g_pChatRoomSocketList = new CChatRoomSocketList;
	g_pUIManager = new CUIManager;
	g_pUIMapName = new CUIMapName;	// rozy
	g_pTimer = new CTimer();

#ifdef MOVIE_DIRECTSHOW
	g_pMovieScene = new CMovieScene;
#endif // MOVIE_DIRECTSHOW

	static CAIController pAIController(Hero);

	g_BuffSystem = BuffStateSystem::Make();
	g_MapProcess = MapProcess::Make();
	g_petProcess = PetProcess::Make();

	CUIMng::Instance().Create();

	if (g_iChatInputType == 1)
	{
		g_pSingleTextInputBox->Init(gwinhandle->GethWnd(), 200, 20);
		g_pSinglePasswdInputBox->Init(gwinhandle->GethWnd(), 200, 20, 9, TRUE);
		g_pSingleTextInputBox->SetState(UISTATE_HIDE);
		g_pSinglePasswdInputBox->SetState(UISTATE_HIDE);

		g_pSingleTextInputBox->SetFont(g_hFont);
		g_pSinglePasswdInputBox->SetFont(g_hFont);

		g_bIMEBlock = FALSE;
		HIMC  hIMC = ImmGetContext(gwinhandle->GethWnd());
		ImmSetConversionStatus(hIMC, IME_CMODE_ALPHANUMERIC, IME_SMODE_NONE);
		ImmReleaseContext(gwinhandle->GethWnd(), hIMC);
		SaveIMEStatus();
		g_bIMEBlock = TRUE;
	}

	if (!gwinhandle->CheckWndMode())
	{
		int nOldVal;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &nOldVal, 0);
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &g_iScreenSaverOldValue, 0);
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 300 * 60, NULL, 0);
	}

#ifdef SAVE_PACKET
	DeleteFile(PACKET_SAVE_FILE);
#endif

	gTextClien.Load();

	MSG msg = gwinhandle->winLoop();

	gwinhandle->Destroyer();

	if (hThread) CloseHandle(hThread);

	timeEndPeriod(1);

	return msg.wParam;
}