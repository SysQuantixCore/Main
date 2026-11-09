#ifndef __WINMAIN_H__
#define __WINMAIN_H__

//#define FOR_WORK
/*--------------------------------------------------------------------------------------*/

#define LANGUAGE_KOREAN			( 0 )
#define LANGUAGE_ENGLISH		( 1 )
#define LANGUAGE_TAIWANESE		( 2 )
#define LANGUAGE_CHINESE		( 3 )
#define LANGUAGE_JAPANESE		( 4 )
#define LANGUAGE_THAILAND		( 5 )
#define LANGUAGE_PHILIPPINES	( 6 )
#define LANGUAGE_VIETNAMESE		( 7 )
#define NUM_LANGUAGE			( 8 )

#define SELECTED_LANGUAGE	( LANGUAGE_ENGLISH )


#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

// Define NTAPI if not already defined
#ifndef NTAPI
#define NTAPI __stdcall
#endif

// Define current process handle
#ifndef NtCurrentProcess
#define NtCurrentProcess ((HANDLE)(LONG_PTR)-1)
#endif

// Typedefs for NtSetInformationProcess and NtAllocateVirtualMemory
typedef NTSTATUS(NTAPI* lpNtSetInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);
typedef NTSTATUS(NTAPI* lpNtAllocateVirtualMemory)(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);


#ifdef _DEBUG
#define ENABLE_EDIT
#define ENABLE_EDIT2
//#define DEBUG_BITMAP_CACHE
#endif // _DEBUG

#ifdef FOR_WORK
#ifdef _DEBUG

#define CSK_LH_DEBUG_CONSOLE
#ifdef CSK_LH_DEBUG_CONSOLE
#define CONSOLE_DEBUG
#endif // CSK_LH_DEBUG_CONSOLE

#define CSK_DEBUG_MAP_ATTRIBUTE

#define CSK_DEBUG_RENDER_BOUNDINGBOX

#define CSK_DEBUG_MAP_PATHFINDING

#endif // _DEBUG
#endif // FOR_WORK

#define WINDOWMODE						// (2006-07-18)
//#define ENABLE_FULLSCREEN

#if defined USER_WINDOW_MODE || (defined WINDOWMODE)
#define ACTIVE_FOCUS_OUT
#endif	// USER_WINDOW_MODE

//#define DEVIAS_XMAS_EVENT  //more snow in devias
//#define GUILD_WAR_EVENT
#define DUEL_SYSTEM
#define PROTECT_SYSTEMKEY   //alow alt+tab on fullscreen mode
//#define CAMERA_TEST

#define WM_USER_MEMORYHACK	( WM_USER + 1)
#define WM_NPROTECT_EXIT_TWO  (WM_USER + 10001)

#if MAIN_UPDATE > 303
#define MAX_ACCOUNT_REG				5
#else
#define MAX_ACCOUNT_REG				4
#endif

// =====================================================================
// Forward declarations (evita incluir headers pesados em WinMain.h)
// =====================================================================
class CUITextInputBox;
class CChatRoomSocketList;
class CMultiLanguage;
class CUIManager;
class CUIMapName;
class CTimer;
#ifdef MOVIE_DIRECTSHOW
class CMovieScene;
#endif // MOVIE_DIRECTSHOW

// =====================================================================
// Globais originais
// =====================================================================
extern bool ashies;
extern int weather;
extern int Time_Effect;
extern HDC       g_hDC;
extern HGLRC     g_hRC;
extern HFONT     g_hFont;
extern HFONT     g_hFontBold;
extern HFONT     g_hFontBig;
extern HFONT     g_hFixFont;
extern bool      Destroy;
extern int       RandomTable[];
extern char m_ID[6][11];
extern char m_Psz[6][11];
extern char m_Version[];
extern int  m_SaveAccount;
extern int  m_SoundOnOff;
extern int  m_MusicOnOff;
extern int  m_Resolution;
extern int m_nColorDepth;

// =====================================================================
// Globais adicionais expostas em WinMain.cpp
// =====================================================================

// Versão / configuração
extern char m_ExeVersion[];
extern int  targetIndexFPS;
extern int  g_iRenderTextType;

// Áudio (mp3 de fundo)
extern char Mp3FileName[256];

// Idioma selecionado
extern std::string g_strSelectedML;
extern char        g_aszMLSelection[]; // tamanho = MAX_LANGUAGE_NAME_LENGTH (ver MultiLanguage.h)
extern CMultiLanguage* pMultiLanguage;

// Chat / UI
extern int                   g_iChatInputType;
extern CUITextInputBox* g_pSingleTextInputBox;
extern CUITextInputBox* g_pSinglePasswdInputBox;
extern CChatRoomSocketList* g_pChatRoomSocketList;
extern CUIManager* g_pUIManager;
extern CUIMapName* g_pUIMapName;	// rozy
extern CTimer* g_pTimer;

#ifdef MOVIE_DIRECTSHOW
extern CMovieScene* g_pMovieScene;
#endif // MOVIE_DIRECTSHOW

// Processo principal / anti-cheat
extern HANDLE g_hMainExe;
extern BYTE* RendomMemoryDump;
extern char   TextMu[];

// Comando de conexão (servidor virtual)
extern char g_lpszCmdURL[50];
extern BOOL g_bInactiveTimeChecked;

// Janela / sistema
extern BOOL g_bMinimizedEnabled;
extern int  g_iScreenSaverOldValue;
extern int  g_iInactiveWarning;

// IME / input
extern bool HangulDelete;
extern int  Hangul;
extern bool g_bEnterPressed;
extern float g_iMousePopPosition_x;
extern float g_iMousePopPosition_y;

// =====================================================================
// Funções originais
// =====================================================================
extern void CheckHack(void);
extern DWORD GetCheckSum(WORD wKey);
extern void StopMp3(const char* Name, BOOL bEnforce = false);
extern void PlayMp3(const char* Name, BOOL bEnforce = false);
extern bool IsEndMp3();
extern int GetMp3PlayPosition();
extern unsigned int GenID();

extern void CloseMainExe(void);
extern GLvoid KillGLWindow(GLvoid);
extern void DestroyWindow();
extern void DestroySound();

// =====================================================================
// Funções adicionais expostas em WinMain.cpp
// =====================================================================

// Inicialização / finalização de janela e contexto OpenGL
extern bool CreateOpenglWindow();
extern void CreateImGuiWindow();
extern void DestroyImGuiWindow();

// Configuração / linha de comando
extern BOOL OpenInitFile();
extern BOOL Util_CheckOption(char* lpszCommandLine, unsigned char cOption, char* lpszString);
extern BOOL UpdateFile(char* lpszOld, char* lpszNew);
extern BOOL GetFileNameOfFilePath(char* lpszFile, char* lpszPath);
extern BOOL GetFileVersion(char* lpszFileName, WORD* pwVersion);
extern BOOL OpenMainExe(void);
extern BOOL GetConnectServerInfo(PSTR szCmdLine, char* lpszURL, WORD* pwPort);
extern BOOL KillExeProcess(char* lpszExe);

// Checksum / proteção
extern WORD  DecryptCheckSumKey(WORD wSource);
extern DWORD GenerateCheckSum(BYTE* pbyBuffer, DWORD dwSize, WORD wKey);
extern void  EnablePermanentDep();

// Crash handling
extern bool ExceptionCallback(_EXCEPTION_POINTERS* pExceptionInfo);
extern LONG WINAPI VectoredCrashHandler(_EXCEPTION_POINTERS* pExceptionInfo);
extern LONG WINAPI TopLevelExceptionFilter(_EXCEPTION_POINTERS* pExceptionInfo);
extern void OnInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved);
extern void __cdecl OnPureCall();

// Threads auxiliares
extern void ThreadFreeVRAM(LPVOID lpThreadParameter);
extern void memory_const_checked(LPVOID lpThreadParameter);

// Diálogo de configuração (FPS / idioma)
extern BOOL CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//#if defined _DEBUG || defined PBG_LOG_PACKET_WINSOCKERROR
//	#include "./Utilities/Log/DebugAngel.h"
//#define ExecutionLog	DebugAngel_Write
//#else
//#define ExecutionLog	{}
//#endif //_DEBUG

#define FAKE_CODE( pos)\
	_asm { jmp pos };\
	_asm { __emit 0xFF };\
	_asm { __emit 0x15 }


#endif