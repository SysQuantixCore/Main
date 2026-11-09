// Local.cpp: implementation of the Local
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Local.h"
#include "MultiLanguage.h"
#include "Utilities/Log/muConsoleDebug.h"

#define g_pMultiLanguage (CMultiLanguage::GetSingletonPtr())

bool CheckSpecialText(char* pszText)
{
	for (unsigned char* lpszCheck = (unsigned char*)pszText; *lpszCheck; ++lpszCheck)
	{
		if (1 == _mbclen(lpszCheck))
		{
			if (!(48 <= *lpszCheck && *lpszCheck < 58) && !(65 <= *lpszCheck && *lpszCheck < 91) && !(97 <= *lpszCheck && *lpszCheck < 123))
			{
				return (true);
			}
		}
		else
		{
			return (true);
		}
	}

	return false;
}

bool CheckSpecialText(wchar_t* pszText)
{
	if (pszText == NULL)
	{
		return true;
	}

	for (wchar_t* p = pszText; *p; ++p)
	{
		wchar_t ch = *p;
		if (!((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z')))
		{
			return true;
		}
	}

	return false;
}


/**/