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

/*
 * For obfuscator
 */
bool isStringStart(const char *p) {
	return (*p == '"' || *p == '\'' || (*p == '[' && *(p + 1) == '['));
}

bool isSpace(const char c) {
	return (c == ' ' || c == '\t');
}

bool isNewLine(const char c) {
	return (c == '\n');
}

bool isAlphaFun(const char c) {
	return (isalnum(c) || c == '_');
}

/*
 * Skip the string in Lua's code and move a pointer
 * *pData -- pointer to "string"
*/
int skipStringAndMove(char **pData, char **pDest) {
	char *p = *pData;

	if (*p == '"' || *p == '\'') {
		const char cStart = *(p++);
		while (*p) {
			if (*p == cStart) {
				if (*(p - 1) == '\\') {
					if (isEscapedChar(p)) {
						++p;
						continue;
					}
				}
				break;
			}
			++p;
		}
	}
/*	else {
		while (*p && !(*p == ']' && *(p + 1) == ']')) {
			++p;
		}
		if (*p)
			++p;
		if (*p)
			++p;
	}*/
	int size = p - *pData;
	if (pDest) {
		memmove(*pDest, *pData, size + 1);
		*pDest += size;
	}
	*pData += size;

	return size;
}

/*
 * Return true if quote is escaped, i. e. \" or \\\"
 * Return true if quote is non escaped, i. e. \\" or \\\\" or _"
 */
bool isEscapedChar(const char* p) {
	const char *pCur = p - 1;

	while (*pCur && *pCur == '\\')
		--pCur;

	return (p - pCur) % 2 == 0;
}

/*
 * Skip the whitespaces forward or back direction
 */
char* skipSpace(char *str, bool bForward) {
	char *p = str;

	if (bForward) {
		while (isSpace(*p))
			++p;
	}
	else {
		while (isSpace(*p))
			--p;
	}

	return p;
}

char* skipSpaceAndNewLine(char *str, bool bForward) {
	char *p = str;

	if (bForward) {
		while (isSpace(*p) || isNewLine(*p))
			++p;
	}
	else {
		while (isSpace(*p) || isNewLine(*p))
			--p;
	}

	return p;
}

/* END */
