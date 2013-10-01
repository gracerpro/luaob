#include "obfuscator.h"
#include "global.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace Obfuscator
{
static const char FUNCTION[] = "function";
static const int FUNCTION_LEN = sizeof(FUNCTION) - 1;

// 26 + 1 + 26
static char const szCharsFunName[] = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int CHARS_FUN_NAME_LEN = sizeof(szCharsFunName) - 1;


static size_t deleteComments(char *szDataIn);
static size_t deleteNewLines(char *szDataIn);
static size_t deleteSpaces(char *szDataIn);

static int skipStringAndMove(char **pData, char **pDest);



inline bool isSpace(char c) {
	return (c == ' ' || c == '\t');
}

inline bool isNewLine(char c) {
	return (c == '\n');
}

inline bool isAlphaFun(char c) {
	return (isalnum(c) || c == '_');
}

/*
 *
 */
bool isEscapedQuote(char* p) {
	char *pCur = p - 1;

	while (*pCur && *pCur == '\\')
		--pCur;

	return (p - pCur) % 2 == 0;
}

char* skipSpace(char **p) {
	while (isSpace(**p))
		++(*p);

	return *p;
}

char* skipSpaceAndNewLine(char **p)
{
	while (isSpace(**p) || isNewLine(**p))
		++(*p);

	return *p;
}

/*
 * Delete the comments from a Lua's code
 * --[[ Multiline comments ]]
 * -- Single line comment
 */
size_t deleteComments(char *szDataIn) {
	size_t commentSize = 0;

	char *p     = szDataIn;
	char *pDest = szDataIn;

	while (*p) {
//		commentSize += TrancateComment(&p);
		char *pStart = p;

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
		commentSize += p - pStart;
		*pDest = *p;
		++p;
		++pDest;
	}
	*pDest = 0;

	return commentSize;
}

/*
 * Delete the new line chars.
 * TODO: each chunk must be ended ';'
 */
size_t deleteNewLines(char *szDataIn)
{
	if (!szDataIn || !*szDataIn)
		return 0;

	char *p     = szDataIn;
	char *pDest = szDataIn;

	while (*p && isNewLine(*p))
		++p;
	while (*p) {
		if (isNewLine(*p)) {
			char cPrev = *(p - 1);
			while (*p && isNewLine(*p))
				++p;
			if (cPrev != ';')
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
 * Skip the string in Lua's code and move a pointer
 * *pData -- pointer to "string"
*/
static int skipStringAndMove(char **pData, char **pDest)
{
	char *p = *pData + 1;
	while (*p) {
		if (*p == '"') {
			if (*(p - 1) == '\\') {
				if (isEscapedQuote(p)) {
					++p;
					continue;
				}
			}
			break;
		}
		++p;
	}
	int size = p - *pData;
	if (pDest) {
		memmove(*pDest, *pData, size + 1);
		*pDest += size;
	}
	*pData += size;

	return size;
}

void _DeleteSpacesBeforeAfter(char **p, char **pDest,
	const int nDelimerSize, const int nSpaceCount)
{
	// p -- poiter to first char of delimer
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
size_t deleteSpaces(char *szDataIn)
{
	char const *arrClearChar = "{}()[].,;+-*/^%<>~=#\n";
	int spaceCount = 0;

	char *pDest = szDataIn;
	char *p     = szDataIn;

	while (*p && isSpace(*p))
		++p;

	while (*p) {
		*pDest = *p;
		if (*p == '"')
			skipStringAndMove(&p, &pDest);

		if (strchr(arrClearChar, *p)) {
			_DeleteSpacesBeforeAfter(&p, &pDest, 1, spaceCount);
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

char* GenNewFileName(char *szFileNameNew, char const *szFileName)
{
	strcpy(szFileNameNew, szFileName);
	char *p = strrchr(szFileNameNew, '.');
	if (p)
		strcpy(p, "_ob.lua");
	else
		strcat(szFileNameNew, "_ob.lua");

	return szFileNameNew;
}

int ReadAddonTocFile(char const *szTocFileName, StringList &luaFiles)
{
	char buf[300];

	if (!szTocFileName || !szTocFileName[0])
		return -1;

	FILE *fileToc = fopen(szTocFileName, "rt");
	if (!fileToc)
	{
		print("Couldn't open the toc file %s\n", szTocFileName);
		return 0;
	}

	//files.clear();
	while (!feof(fileToc) && fgets(buf, 300, fileToc))
	{
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
 * Прочитать из файла szGlobalExcludeFunctionFileName
 * имена глобальных функций, которые не следует заменять.
 * На каждой строке название функции
 * после символа # идет комментарий
 */
int ReadAddonGlobalExcludeFunctions(const char *szGlobalExcludeFunctionFileName, StringList &FunctionsExclude)
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
int obfuscate(const stObfuscator *obf, StringList const &fileList, StringList const &excludeFunctions)
{
	FILE *fileNew, *fileOld;
	char szFileNameNew[FILENAME_MAX];
	int commentSize, spaceSize, newLineSize;
	int nLocalVars, nIntReplace;
	char *pDataIn, *pDataOut;

	if (fileList.empty())
		return 0;

	StringListConstIter iter = fileList.begin();
	while (iter != fileList.end())
	{
		GenNewFileName(szFileNameNew, iter->c_str());

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
		if (obf->bCreateBakFile) {
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

		commentSize = deleteComments(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->data(), "1_comment");
#endif

		spaceSize = deleteSpaces(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->data(), "2_spaces");
#endif

		newLineSize = deleteNewLines(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->data(), "3_endline");
#endif

		const int nWidth = 32;
		print("%-*s: %+d\n", nWidth, "Delete the Comment", commentSize);
		print("%-*s: %+d\n", nWidth, "Delete the \"New line\"", newLineSize);
		print("%-*s: %+d\n", nWidth, "Delete the spaces", spaceSize);
		print("%-*s: %+d\n", nWidth, "Obfuscate integers", nIntReplace);
		print("%-*s: %+d\n", nWidth, "Obfuscate local vars", nLocalVars);

		strcpy(pDataOut, pDataIn);

		fileNew = fopen(szFileNameNew, "wt");
		if (!fileNew) {
			print("Couldnt create file %s\n", szFileNameNew);
			fclose(fileOld);
			return -1;
		}
		fwrite(pDataOut, 1, strlen(pDataOut), fileNew);

		delete[] pDataInSource;
		delete[] pDataOutSource;

		fclose(fileOld);
		fclose(fileNew);

		if (obf->bCreateBakFile) {
			unlink(iter->c_str());
			rename(szFileNameNew, iter->c_str());
		}

		++iter;
	}

	return 0;
}

}
