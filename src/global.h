#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include <stdint.h>


#if defined _WIN32
	#pragma warning(disable:4996)
#endif

char* GetFileDir(char const* szFile, char* szDir);

char const* GetExeDir();
char const* GetExeName();

bool fileExists(const char *szFileName);

char* strtrim(char* sz);

void print(char const *format, ...);

#ifdef _WIN32
	#define SZ_EXE_FILE_NAME    "luaob.exe"
#else
	#define SZ_EXE_FILE_NAME    "luaob"
#endif
#define SZ_EXE_NAME         "luaob"

/*
 * For obfuscator
 */
bool isSpace(const char c);
bool isNewLine(const char c);
bool isAlphaFun(const char c);
bool isStringStart(const char *p);
bool isEscapedChar(const char* p);

int skipStringAndMove(char **pData, char **pDest);

char* skipSpace(char *str, bool bForward = true);
char* skipSpaceAndNewLine(char *str, bool bForward = true);

#endif
