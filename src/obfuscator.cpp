#include "obfuscator.h"
#include "global.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>


static const char FUNCTION[] = "function";
static const int FUNCTION_LEN = sizeof(FUNCTION) - 1;

// 26 + 1 + 26
static char const szCharsFunName[] = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int CHARS_FUN_NAME_LEN = sizeof(szCharsFunName) - 1;


/*
 * Delete the comments from a Lua's code
 * --[[ Multiline comments ]]
 * -- Singleline comment
 */
LuaObfuscator::LuaObfuscator(const StringList &luaFiles, const StringList &excludeGlobalFunctions)
	: m_luaFiles(luaFiles), m_excludeFunctions(excludeGlobalFunctions)
{
}

LuaObfuscator::~LuaObfuscator() {

}

int LuaObfuscator::removeComments() {
	char *p     = m_buffer;
	char *pDest = m_buffer;

	while (*p) {
		if (isStringStart(p))
			skipStringAndMove(&p, &pDest);

		// TODO: endians
		if (*((uint32_t*)p) == 0x5B5B2D2D) { // 0x5B5B2D2D== '[[--'
			while (*p && *(uint16_t*)p != 0x5D5D) // 0x5D5D == ']]'
				++p;
			if (*p)
				p += 2;
		}
		else if (*((uint16_t*)p) == 0x2D2D) { // 0x == '--'
			p += 2;
			while (*p && !isNewLine(*p))
				++p;
		}
		*pDest = *p;
		++p;
		++pDest;
	}
	*pDest = 0;

	return pDest - p;
}

/*
 * Delete the new line chars.
 * TODO: each chunk must be ended ';'
 */
int LuaObfuscator::removeNewLines() {
	char *p = m_buffer;

	if (!p || !p[0])
		return 0;

	char *pDest = p;

	while (*p && isNewLine(*p))
		++p;
	while (*p) {
		if (isNewLine(*p)) {
			char cPrev = *(p - 1);
			while (*p && isNewLine(*p))
				++p;
			if (cPrev == '\\')
				*(pDest++) = 'n'; // within string here
			else if (cPrev != ';')
				*(pDest++) = ' ';
			continue;
		}
		else {
			*(pDest++) = *p;
		}
		++p;
	}
	if (isSpace(*(pDest - 1))) {
		--pDest;
	}

	*pDest = 0;

	return pDest - p;
}

/*
 * p -- poiter to first char of delimer
 */
void _deleteSpacesBeforeAfter(char **p, char **pDest, const int nDelimerSize,
	const int nSpaceCount) {

	*pDest -= nSpaceCount;
	char *pSpace = *p + nDelimerSize;
	while (isSpace(*pSpace))
		++pSpace;
	int nSpaceAfterCount = pSpace - *p - nDelimerSize;
	memmove(*pDest, *p, nDelimerSize);
	*p += nSpaceAfterCount + nDelimerSize;
	*pDest += nDelimerSize;
}

/*
 * Delete the spaces near some characters
 */
int LuaObfuscator::removeExtraWhitespace() {
	char const *arrClearChar = "{}()[].,;+-*/^%<>~=#\n";

	char *p = m_buffer;

	if (!p || !p[0])
		return 0;

	char *pDest = p;
	int spaceCount = 0;

	while (*p && isSpace(*p))
		++p;

	// delete dublicated spaces
	while (*p) {
		if (isStringStart(p))
			skipStringAndMove(&p, &pDest);
		if (!(isSpace(*p) && isSpace(*(p - 1)))) {
			*pDest = *p;
			++pDest;
		}
		++p;
	}
	*pDest = 0;

	pDest = m_buffer;
	p     = m_buffer;

	// delete spaces near chars
	while (*p) {
		*pDest = *p;
		if (isStringStart(p))
			skipStringAndMove(&p, &pDest);

		if (strchr(arrClearChar, *p)) {
			_deleteSpacesBeforeAfter(&p, &pDest, 1, spaceCount);
			spaceCount = 0;
			continue;
		}

		isSpace(*p) ? ++spaceCount : spaceCount = 0;
		++pDest;
		++p;
	}
	while (isSpace(*pDest))
		--pDest;
	*pDest = 0;

	return pDest - p;
}

char* generateNewFileName(char *szFileNameNew, char const *szFileName) {
	strcpy(szFileNameNew, szFileName);
	char *p = strrchr(szFileNameNew, '.');
	if (p)
		strcpy(p, "_ob.lua");
	else
		strcat(szFileNameNew, "_ob.lua");

	return szFileNameNew;
}

int LuaObfuscator::readAddonTocFile(char const *szTocFileName, StringList &luaFiles) {
	char buf[300];

	if (!szTocFileName || !szTocFileName[0])
		return -1;

	FILE *fileToc = fopen(szTocFileName, "rt");
	if (!fileToc) {
		print("Couldn't open the toc file %s\n", szTocFileName);
		return 0;
	}

	//files.clear();
	while (!feof(fileToc) && fgets(buf, 300, fileToc)) {
		if (!buf[0])
			continue;
		char *pFile = strtrim(buf);
		if (char *p = strchr(pFile, '\n'))
			*p = 0;
		if (pFile[0] && pFile[0] != '#') {
			luaFiles.push_back(pFile);
		}
	}
	fclose(fileToc);

	return luaFiles.size();
}

/*
 * Read from file <szGlobalExcludeFunctionFileName>
 * the names of global functions, which don't need to replace.
 * In this file are function's names on each string.
 * After '#' write comments
 */
int LuaObfuscator::readAddonGlobalExcludeFunctions(const char *szGlobalExcludeFunctionFileName, StringList &FunctionsExclude)
{
	const int N = 1024;
	int res = 0;
	char buf[N];

	if (!szGlobalExcludeFunctionFileName || !szGlobalExcludeFunctionFileName[0])
		return -1;

	FILE *file = fopen(szGlobalExcludeFunctionFileName, "rt");
	if (!file)
		return -1;

	FunctionsExclude.clear();

	std::string str;
	while (feof(file) && fgets(buf, N, file)) {
		char *pName = strtrim(buf);
		if (char *p = strchr(pName, '\n'))
			*p = 0;
		if (pName[0] && pName[0] != '#') {
			str.assign(pName);
			FunctionsExclude.push_back(str);
		}
	}

	fclose(file);

	return res;
}

char* getOperand(char *szData, int *pnLen, bool bLeftOperand) {
	char *p = szData;
	char *res;

	if (bLeftOperand) {
		--p;
		while (isSpace(*p))
			--p;
		char *pEnd = p;
		while (isalnum(*p))
			--p;
		res = p + 1;
		*pnLen = pEnd - p;
	}
	else {
		++p;
		while (isSpace(*p))
			++p;
		char *pBegin = p;
		while (isalnum(*p))
			++p;
		res = pBegin;
		*pnLen = p - pBegin;
	}

	return res;
}

/*
 * global function name(...)
 *      |
 *    *szData
 */
int getGlobalFunctionName(char **szData, char *szFun) {
	char *p = *szData - 1;
	bool bLocal = false;
	int len = 0;

	szFun[0] = 0;
	char *pOp = getOperand(p, &len, true);
	if (len && !strncmp(pOp, "local", len))
		return 1;

	while (isSpace(*p))
		--p;
	while (isAlphaFun(*p))
		--p;
	if (!strncmp(p+1, "local", 5))
		bLocal = true;

	*szData += 8; // strlen("function")
	p = *szData;

	while (*p && isSpace(*p))
		++p;

	if (*p) {
		char *pFunStart = p;
		while (*p && *p != '(')
			++p;
		while (*p && isSpace(*p))
			--p;
		size_t size = p - pFunStart;
		if (size)
			strncpy(szFun, pFunStart, size);
		szFun[size] = 0;
	}

	return p - *szData;
}

const char* generateFakeFunctionName()
{
	static char szFakeName[FAKE_FUNCTION_LEN + 2];

	for (int i = 0; i < FAKE_FUNCTION_LEN; ++i)
		szFakeName[i] = szCharsFunName[rand() % CHARS_FUN_NAME_LEN];
	szFakeName[FAKE_FUNCTION_LEN] = 0;

	return szFakeName;
}

int readGlobalFunctions(const char *szFileName, FakeFunctions &Functions)
{
	int count = 0;

	FILE *file = fopen(szFileName, "rt");
	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);

	char *data_s = new char[size + 10];
	memset(data_s, 0, 5);
	char *data = data_s + 5;

	rewind(file);
	int realSize = fread(data, 1, size, file);
	memset(data + realSize, 0, 5);
	char *p = data;
	while (*p) {
		if (isStringStart(p))
			skipStringAndMove(&p, NULL);

		if (*((unsigned long*)p) == 0x636E7566) { // 'cnuf' backview 'func'
			if (!strncmp(p, FUNCTION, FUNCTION_LEN)) {
				char szFun[200];
				int trancSpaces = getGlobalFunctionName(&p, szFun);
				if (trancSpaces && szFun[0]) {
					stFakeFunction fun;
					fun.name = szFun;
					fun.fake_name = generateFakeFunctionName();
					Functions[fun.name] = fun.fake_name;
					++count;
				}
				p += trancSpaces;
				continue;
			}
		}
		++p;
	}
	delete[] data_s;
	fclose(file);

	return count;
}

/*
 * pStart - pointer to first char of function name
 * pEnd1  - pointer to (last + 1) char of function name
 */
bool isFunctionNameInCode(char *pStart, char *pEnd1) {
	char *p;

	p = skipSpace(pStart - 1, false);
	if (*p == '(' || *p == ')')
		return true;

	p = skipSpace(pEnd1);

	if (*p == '(' || *p == ')')
		return true;

	return false;
}

/*
 * Scaning all text
 * Skip the "string"
 * Why do finds the function name:
 *   function call:
 *     foo(arg1, arg2, ...)
 *     a = foo(arg1, arg2, ...)
 *   when function send to other function
 *     trycall(foo)
 * ---------------------
 * therefore function name is near '(' and ')'
 */
int replaceGlobalFunctions(const char *szFileName, FakeFunctions &fakeFunctions,
	const StringList &excludeFunctions)
{
	if (!szFileName || !szFileName[0])
		return 0;

	FILE *file = fopen(szFileName, "rt");
	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char *pDataInSource = new char[size + 20]; // + 20 for additional 10_"data"_10
	memset(pDataInSource, 0, 10);
	char *pDataIn = pDataInSource + 10; // for delete/clear data
	char *pDataOutSource = new char[size + 20 + ADDITIONAL_MEMORY]; // for add data
	memset(pDataOutSource, 0, 10);
	char *pDataOut = pDataOutSource + 10;

	int realSize = fread(pDataIn, 1, size, file);
	memset(pDataIn + realSize, 0, 10);

	fclose(file);

	char *p = pDataIn;
	char *pDest = pDataOut;

	pDest = pDataOut;
#ifdef _DEBUG
	memset(pDest, 0, size + ADDITIONAL_MEMORY);
#endif
	while (*p) {
		*pDest = *p;
		if (isStringStart(p))
			skipStringAndMove(&p, &pDest);

		char *pStart = p;
		while (isAlphaFun(*p)) {
			++p;
		}

		size_t wordSize = p - pStart;
		if (wordSize) {
			if (isFunctionNameInCode(pStart, p)) {
				std::string str(pStart, wordSize);
				StringMapConstIter iter = fakeFunctions.find(str);
				if (iter != fakeFunctions.end()) {
					StringListConstIter IterFunExclude = std::find(excludeFunctions.begin(), excludeFunctions.end(), str);
					if (IterFunExclude == excludeFunctions.end()) {
						// replace
						memcpy(pDest, iter->second.c_str(), FAKE_FUNCTION_LEN);
						pDest += FAKE_FUNCTION_LEN;
						continue;
					}
				}
			}
			memcpy(pDest, pStart, wordSize + 1);
			pDest += wordSize;
		}

		++p;
		++pDest;
	}
	*pDest = 0;

	file = fopen(szFileName, "wt");
	size_t obfuscateSize = strlen(pDataOut);
	if (file) {
		fwrite(pDataOut, 1, obfuscateSize, file);
		fclose(file);
	}

	delete[] pDataInSource;
	delete[] pDataOutSource;

	print("%8d %s\n", obfuscateSize - realSize, szFileName);

	return obfuscateSize - realSize;
}

int LuaObfuscator::obfuscateGlobalFunctionNames() {
	FakeFunctions fakeFunctions;
	int size = 0;
	char szFileNameNew[MAX_PATH];

	// read global functions
	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		std::string sFileName = *iter;
		generateNewFileName(szFileNameNew, sFileName.c_str());
		readGlobalFunctions(szFileNameNew, fakeFunctions);
		++iter;
	}

	// replace global functions
	iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		std::string sFileName = *iter;
		generateNewFileName(szFileNameNew, sFileName.c_str());
		size += replaceGlobalFunctions(szFileNameNew, fakeFunctions, m_excludeFunctions);
		++iter;
	}

	return size;
}

#ifdef _DEBUG
void SaveToFile(char *data, char const *szFile, char *postfix)
{
	char buf[FILENAME_MAX];

	GetFileDir(szFile, buf);
	strcat(buf, "debug_ob_");
	strcat(buf, postfix);
	strcat(buf, ".lua");
	FILE *f = fopen(buf, "wt");
	if (f)
	{
		int len = strlen(data);
		len = fwrite(data, 1, len, f);
		fclose(f);
	}
}
#endif

/*
 * Obfuscate the Lua's code in the files in list <fileList>
 * <excludeFunctions> contatins name of global function, for which don't make obfuscating
 * <obf> contains the some options
 */
int LuaObfuscator::obfuscate(const stObfuscatorSetting &settings)
{
	FILE *fileNew, *fileOld;
	char szFileNameNew[FILENAME_MAX];
	int commentSize, spaceSize, newLineSize;
	int nLocalVars, nIntReplace;
	char *pDataIn, *pDataOut;

	if (m_luaFiles.empty())
		return 0;

	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end())
	{
		generateNewFileName(szFileNameNew, iter->c_str());

		print("\tObfuscate file %s\n", iter->c_str());

		fileOld = fopen(iter->data(), "rt");
		if (!fileOld) {
			print("ERROR: Open file fail\n");
			continue;
		}

		fseek(fileOld, 0, SEEK_END);
		long size = ftell(fileOld);
		rewind(fileOld);

		char *pDataInSource = new char[size + 20 + ADDITIONAL_MEMORY]; // + 20 for additional 10_"data"_10
		pDataIn = pDataInSource + 10; // for delete/clear data
		char *pDataOutSource = new char[size + 20 + ADDITIONAL_MEMORY]; // for add data
		pDataOut = pDataOutSource + 10;
		if (!pDataInSource) {
			fclose(fileOld);
			print("Try to alloc memory failed, size %d\n", size + 20);
			return 0;
		}
		if (!pDataOutSource) {
			fclose(fileOld);
			delete[] pDataInSource;
			print("Try to alloc memory failed, size %d\n", size + 20 + ADDITIONAL_MEMORY);
			return 0;
		}
		memset(pDataInSource, 0, 10);
		memset(pDataInSource + size + 10 + ADDITIONAL_MEMORY, 0, 10);
		size_t nSize = fread(pDataIn, 1, size, fileOld);
		pDataIn[nSize] = 0;
		pDataIn[nSize + 1] = 0;

		char szFileBakup[FILENAME_MAX];
		if (m_settings.bCreateBakFile) {
			strcpy(szFileBakup, iter->c_str());
			strcat(szFileBakup, ".bak");
			FILE *fileBakup = fopen(szFileBakup, "wt");
			if (fileBakup) {
				fwrite(pDataIn, 1, strlen(pDataIn), fileBakup);
				fclose(fileBakup);
			}
		}

		commentSize = 0;
		spaceSize = 0;
		newLineSize = 0;
		nIntReplace = 0;
		nLocalVars = 0;

		commentSize = removeComments();
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "1_comment");
#endif

		spaceSize = removeExtraWhitespace();
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "2_spaces");
#endif

		newLineSize = removeNewLines();
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "3_endline");
#endif

		const int nWidth = 32;
		print("%-*s: %+d\n", nWidth, "Remove the Comment", commentSize);
		print("%-*s: %+d\n", nWidth, "Remove the \"New line\"", newLineSize);
		print("%-*s: %+d\n", nWidth, "Remove the spaces", spaceSize);
		print("%-*s: %+d\n", nWidth, "Obfuscate integers", nIntReplace);
		print("%-*s: %+d\n", nWidth, "Obfuscate local vars", nLocalVars);

		fileNew = fopen(szFileNameNew, "wt");
		if (!fileNew) {
			print("Couldnt create file %s\n", szFileNameNew);
			fclose(fileOld);
			return -1;
		}
		fwrite(pDataIn, 1, strlen(pDataIn), fileNew);

		delete[] pDataInSource;
		delete[] pDataOutSource;

		fclose(fileOld);
		fclose(fileNew);

		if (m_settings.bCreateBakFile) {
			unlink(iter->c_str());
			rename(szFileNameNew, iter->c_str());
		}

		++iter;
	}

	if (m_settings.ObfuscateFunctionGlobal) {
		print("\nObfuscate global functions\n");
		int globalFunctionSize = obfuscateGlobalFunctionNames();
		print("size: %+d\n", globalFunctionSize);
	}

	return 0;
}
