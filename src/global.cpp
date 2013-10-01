#include "global.h"
#include <stdarg.h>
#include <stdio.h>
#ifdef _WIN32
	#include <io.h>
#endif

void print(char const *format, ...)
{
	const int N = 1024;
	char buf[N];
	va_list va;

	if (!format || !format[0])
		return;

	va_start(va, format);
	vsprintf(buf, format, va);
	va_end(va);

	printf(buf);
}

bool fileExists(const char *szFile) {
#ifdef WIN32
	return access(szFile, 0) == 0;
#else
	return std::iostream::good(szFile); // TODO: debug
#endif
}

char* GetFileDir(char const* szFile, char* szDir)
{
	char const *p = strrchr(szFile, '\\');
	if (!p)
		p = strrchr(szFile, '/');
	if (!p) {
		return strcpy(szDir, GetExeDir());
	}
	int size = p - szFile + 1;
	strncpy(szDir, szFile, size);
	szDir[size] = 0;

	return szDir;
}

char const* GetExeDir()
{
	static char buf[MAX_PATH];

	GetModuleFileName(NULL, buf, MAX_PATH);
	char *p = strrchr(buf, '\\');
	*(p + 1) = 0;

	return buf;
}

char const* GetExeName()
{
	static char buf[MAX_PATH];

	GetModuleFileName(NULL, buf, MAX_PATH);

	return buf;
}

char* strtrim(char* sz)
{
	if (!sz || !sz[0])
		return NULL;

	char *p = sz;
	while (*p && (*p == ' ' || *p == '\t'))
		++p;

	char *pEnd = &p[strlen(p) - 1];
	while (pEnd >= p && (*pEnd == ' ' || *pEnd == '\t'))
		*(pEnd--) = 0;

	return p;
}