#include "StdAfx.h"
#include "TextClien.h"
#include "Util.h"
#include <stdlib.h>
#include "windows.h"
//#include "atlstr.h"
#include "Interface.h"
#include "stdio.h"
#include "./Utilities/Log/muConsoleDebug.h"

#include "zzzinventory.h"
#include "dsplaysound.h"
#include "CSItemOption.h"

cTextClien gTextClien;

void cTextClien::Load()
{
	// Caminho absoluto baseado no executavel: o cliente pode ser iniciado pelo
	// launcher com outro diretorio de trabalho, e ai o caminho relativo antigo
	// (./Data/...) nao encontrava o ini e todos os textos viravam "Null".
	static const char* szKhacDefault[] =
	{
		"", "", "", "",
		"Resolution", "Font Size", "Effect", "Equipment Effect",
		"Terrain Effect", "Object Effect"
	};

	const std::string strBase = GetExecutablePath("Data\\Custom\\TextClien\\");

	std::string strFileName = strBase + "TextClien_" + g_strSelectedML + ".ini";

	if (GetFileAttributesA(strFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		strFileName = strBase + "TextClien_Eng.ini";

		if (GetFileAttributesA(strFileName.c_str()) == INVALID_FILE_ATTRIBUTES)
			strFileName = strBase + "TextClien.ini";
	}

	char GetFotText[35] = { 0 };
	for (int st = 0; st < 50; st++)
	{
		wsprintfA(GetFotText, "Mess%d", st);

		GetPrivateProfileStringA("ThongTin", GetFotText, "Null", TextClien_ThongTin[st], sizeof(TextClien_ThongTin[st]), strFileName.c_str());

		const char* pszKhacDefault = "Null";
		if (st >= 0 && st < (int)(sizeof(szKhacDefault) / sizeof(szKhacDefault[0]))
			&& szKhacDefault[st][0] != '\0')
		{
			pszKhacDefault = szKhacDefault[st];
		}

		GetPrivateProfileStringA("Khac", GetFotText, pszKhacDefault, TextClien_Khac[st], sizeof(TextClien_Khac[st]), strFileName.c_str());
		GetPrivateProfileStringA("Rgig", GetFotText, "Null", TextClien_Rgig[st], sizeof(TextClien_Rgig[st]), strFileName.c_str());
		g_ConsoleDebug->Write(5, "[Load text clien: %d: %s]", st, TextClien_ThongTin[st]);
	}

}