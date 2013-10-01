#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <windows.h>
#include "resource.h"

#if defined _WIN32
	#pragma warning(disable:4996)
#endif

struct stOptions {
	bool chbFunctionGlobal;
	bool chbInteger;
	bool chbAddComment;
	bool chbOnewFile;
	bool chbLocalVars;
	bool chbBackup;

	int nLinesFiles;
};

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

#endif
