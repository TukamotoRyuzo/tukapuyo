#include "debug.h"
#include "rkiss.h"
#include <iostream>

void MyOutputDebugString(LPCSTR pszFormat, ...)
{
	va_list	argp;
	char pszBuf[256];
	va_start(argp, pszFormat);
	vsprintf_s(pszBuf, sizeof(pszBuf), pszFormat, argp);
	va_end(argp);
	OutputDebugString(pszBuf);
}

