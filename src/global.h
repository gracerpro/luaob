#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include <stdint.h>

#if defined _WIN32
	#pragma warning(disable:4996)
#endif

const char* getExeDir();
const char* getExeFileName();
const char* getWorkDir();
char*       getFileDir(char const* szFile, char* szDir);

bool        isAbsoluteFilePath(const char *szFileName);
bool        fileExists(const char *szFileName);
bool        isPathSep(const char c);

char*       strtrim(char* sz);


void print(char const *format, ...);


#ifdef _WIN32
	#define SZ_EXE_FILE_NAME    "luaob.exe"
#else
	#define SZ_EXE_FILE_NAME    "luaob"
#endif
#define SZ_EXE_NAME         "luaob"

#if defined(_WIN32)
	#define PATH_SEPARATOR      "\\"
	#define PATH_SEPARATOR_CHAR '\\'
#else
	#define PATH_SEPARATOR      "/"
	#define PATH_SEPARATOR_CHAR '/'
#endif

/*
 * For obfuscator
 */
bool isSpace(const char c);
bool isNewLine(const char c);
bool isAlphaFun(const char c);
bool isStringStart(const char *p);
bool isSingleStringStart(const char *p);
bool isMultilineStringStart(const char *p);
bool isEscapedChar(const char* p);

size_t skipStringAndMove(char **pData, char **pDest);

char* skipSpace(char *str, bool bForward = true);
char* skipSpaceAndNewLine(char *str, bool bForward = true);

#endif
