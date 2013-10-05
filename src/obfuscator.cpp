#include "obfuscator.h"
#include "global.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <time.h>

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

int LuaObfuscator::removeComments(char *szLuaCode) {
	char *p     = szLuaCode;
	char *pDest = szLuaCode;

	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}

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
int LuaObfuscator::removeNewLines(char *szLuaCode) {
	char *p = szLuaCode;

	if (!p || !p[0])
		return 0;

	char *pDest = p;

	while (*p && isNewLine(*p))
		++p;
	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}
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
		*(pDest++) = *p++;
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
void deleteSpacesBeforeAfter(char **pp, char **pDest) {
	char *p = *pp;

	if (isspace(*(p - 1)))
		--(*pDest);

	**pDest = *p++;
	++(*pDest);

	if (isSpace(*p))
		++p;
	**pDest = *p++;
	++(*pDest);

	*pp = p;
}

/*
 * Delete the spaces near some characters
 */
int LuaObfuscator::removeExtraWhitespace(char *szLuaCode) {
	char const *arrClearChar = "{}()[].,;+-*/^%<>~=#\n";

	char *p = szLuaCode;
	char *pDest = p;

	if (!p || !p[0])
		return 0;

	while (*p && isSpace(*p))
		++p;

	// delete dublicated spaces
	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}
		if (!(isSpace(*p) && isSpace(*(p - 1)))) {
			*pDest = *p;
			++pDest;
		}
		++p;
	}
	*pDest = 0;

	pDest = szLuaCode;
	p     = szLuaCode;

	// delete spaces near chars
	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}
		*pDest = *p;

		if (strchr(arrClearChar, *p)) {
			deleteSpacesBeforeAfter(&p, &pDest);
			continue;
		}

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

const char* LuaObfuscator::generateObfuscatedFunctionName()
{
	static char szFakeName[FAKE_FUNCTION_LEN + 2];

	for (int i = 0; i < FAKE_FUNCTION_LEN; ++i)
		szFakeName[i] = szCharsFunName[rand() % CHARS_FUN_NAME_LEN];
	szFakeName[FAKE_FUNCTION_LEN] = 0;

	return szFakeName;
}

int LuaObfuscator::readGlobalFunctions(const char *szFileName, FakeFunctions &Functions)
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
		if (isStringStart(p)) {
			skipStringAndMove(&p, NULL);
			continue;
		}

		if (*((unsigned long*)p) == 0x636E7566) { // 'cnuf' backview 'func'
			if (!strncmp(p, FUNCTION, FUNCTION_LEN)) {
				char szFun[200];
				int trancSpaces = getGlobalFunctionName(&p, szFun);
				if (trancSpaces && szFun[0]) {
					stFakeFunction fun;
					fun.name = szFun;
					fun.fake_name = LuaObfuscator::generateObfuscatedFunctionName();
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
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}

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

// Find end of function
// p -- pointer to first byte of formal parameters
char* findFunctionEnd(char *p)
{
	int nEnd = 0;

	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, NULL);
			continue;
		}
		// skip local functions, functions in parameters
		if (*p == 'f') {
			if (!strncmp(p, "for", 3) && !isalnum(*(p-1)) && !isalnum(*(p + 3)))
				++nEnd;
			else if (!strncmp(p, "function", 8) && !isalnum(*(p + 8)))
				++nEnd;
		}
		// skip the "while", "for" and "if" constructions
		else if (*p == 'w') {
			if (!strncmp(p, "while", 5) && !isalnum(*(p-1)) && !isalnum(*(p + 5)))
				++nEnd;
		}
		else if (*p == 'i') {
			if (!strncmp(p, "if", 2) && !isalnum(*(p-1)) && !isalnum(*(p + 2)))
				++nEnd;
		}
		else if (*p == 'e') {
			if (!strncmp(p, "end", 3) && !isalnum(*(p - 1)) && !isalnum(*(p + 3))) {
				--nEnd;
				if (nEnd < 0)
					return p - 1;
			}
		}

		++p;
	}

	return p;
}

char const* LuaObfuscator::generateObfuscatedLocalVariableName() {
	const int N = 7;
	static char str[100];

	for (int i = 0; i < N; ++i)
		str[i] = szCharsFunName[rand() % CHARS_FUN_NAME_LEN];
	str[N] = 0;
	long l = static_cast<long>(time(NULL));
	l ^= rand();
	memset(&str[N], '0', 4);
	ltoa(l, &str[N], 16);
	str[N + 4] = 0;

	return str;
}

// read and skipp all variables before '='
// p -- pointer to "local" string
char* readAndSkipLocalVariables(char *p, ObfuscatedItems &vars, char **ppDest)
{
	char *pDest = *ppDest;
	int len;
	char buf[100];
	char *pStart, *pWordTrim;

	pStart = p;
	p = p + 5; // skip "local"
	p = skipSpace(p);
	len = p - pStart;
	memcpy(pDest, pStart, len);
	pDest += len;
	char *pEqual = strchr(p, '=');
	char cEqual = '=';
	if (!pEqual) {
		pEqual = strchr(p, ';');
		cEqual = ';';
	}
	else if (!pEqual) {
		pEqual = strchr(p, '\n');
		cEqual = '\n';
	}
	else if (!pEqual) {
		print("Syntax error\n");
		return NULL;
	}
	*pEqual = 0;

	while (p && *p) // TODO: disapear spaces
	{
		pStart = p;
		p = skipSpace(p);
		len = p - pStart;
		memcpy(pDest, pStart, len);
		pDest += len;

		char *pTZ = strchr(p, ',');
		if (pTZ) {
			len = pTZ - p;
			strncpy(buf, p, len);
			buf[len] = 0;
			pWordTrim = strtrim(buf);
			p = pTZ + 1;
		}
		else {
			strcpy(buf, p);
			pWordTrim = strtrim(buf);
			p = NULL;
		}
		if (pWordTrim) {
			std::string str(pWordTrim);
			StringMapConstIter item = vars.find(str);
			if (item == vars.end()) {
				char const *pObName = LuaObfuscator::generateObfuscatedLocalVariableName();
				vars[str] = pObName;
				len = strlen(pObName);
				memcpy(pDest, pObName, len);
				pDest += len;
			}
			else {
				len = item->second.length();
				memcpy(pDest, item->second.c_str(), len);
				pDest += len;
			}
			if (p)
				*(pDest++) = ',';
			else
				*pDest = cEqual;
		}
	}
	*pEqual = cEqual;
	*ppDest = pDest + 1;

	return pEqual + 1;
}

/*
 * pParamStart -- pointer to first char of formal parameters
 */
char* _obfuscateLocalVars(char *pParamStart, char *pBodyEnd, char *pDest)
{
	char *p = pParamStart;
	ObfuscatedItems vars;
	std::string str;
	int nWordLen = 0;
	int len;
	char buf[200];
	char *pBuf;
	char const *pObFun;

	char *pRightSk = strchr(p, ')');
	if (!pRightSk) {
		print("')' not found\n");
		return NULL;
	}
	*pRightSk = 0;
	char *pSemicolon = strchr(p, ',');
	while (pSemicolon) {
		len = pSemicolon - p;
		strncpy(buf, p, len);
		buf[len] = 0;
		pBuf = strtrim(buf);
		str.assign(pBuf);

		pBuf = p;
		while (isSpace(*pBuf))
			++pBuf;
		len = pBuf - p;
		memcpy(pDest, p, len);
		pDest += len;

		if (str != "...") {
			pObFun = LuaObfuscator::generateObfuscatedLocalVariableName();
			vars[str] = pObFun;
			len = strlen(pObFun);
			memcpy(pDest, pObFun, len);
			pDest += len;
		}
		else {
			memcpy(pDest, "...", 3);
			pDest += 3;
		}

		p = pSemicolon + 1;
		pSemicolon = strchr(p, ',');
		*(pDest++) = ',';
	}
	if (p[0]) {
		pBuf = strtrim(p);
		str.assign(pBuf);

		pBuf = p;
		while (isSpace(*pBuf))
			++pBuf;
		len = pBuf - p;
		memcpy(pDest, p, len);
		pDest += len;

		if (str != "...") {
			pObFun = LuaObfuscator::generateObfuscatedLocalVariableName();
			vars[str] = pObFun;
			len = strlen(pObFun);
			memcpy(pDest, pObFun, len);
			pDest += len;
		}
		else {
			memcpy(pDest, "...", 3);
			pDest += 3;
		}
	}
	*(pDest++) = ')';
	*pRightSk = ')';
	p = pRightSk + 1;
	char *pEnd = p;
	skipSpaceAndNewLine(p);
	len = p - pEnd;
	memcpy(pDest, pEnd, len);
	pDest += len;

	char *pWord = p;
	while (p <= pBodyEnd) {
		*pDest = *p;
		if (*p == '"') {
			skipStringAndMove(&p, &pDest);
			continue;
		}

	//	char szOp1[100];
	//	char szOp2[100];
	//	GetOperand(p, &len, true);

		if (*p == 'l')
		{
			if (!strncmp(p, "local", 5) && !isalnum(*(p - 1)) && !isalnum(*(p + 5)))
			{
				p = readAndSkipLocalVariables(p, vars, &pDest);
				nWordLen = 0;
				pWord = p;
				continue;
			}
		}

		// in function parameters
		//
		if (isalnum(*p))
			++nWordLen;
		else
		{
			char *pBefore = p - nWordLen - 1;
			if (nWordLen > 0 && (*pBefore != '.' || *(pBefore - 1) == '.'))
			{
				str.assign(pWord, nWordLen);
				StringMapConstIter iter = vars.find(str);
				if (iter != vars.end())
				{
					len = iter->second.length();
					pDest -= nWordLen;
					memcpy(pDest, iter->second.c_str(), len); // TODO: -1
					pDest += len;
					*pDest = *p;
				}
			}
			pWord = p + 1;
			nWordLen = 0;
		}

		++pDest;
		++p;
	}

	return pDest - 1;
}

/*
 * function name(a, b, c)
 * >>>start<<< body {end}
 * end
 */
int LuaObfuscator::obfuscateLocalVarsAndParameters(const char *szLuaCode, char *szObfuscatedLuaCode) {
	char *p = const_cast<char*>(szLuaCode);
	char *pDest = szObfuscatedLuaCode;
	StringList LocalVars;
	int len;

	while (*p) {
		*pDest = *p;
		if (*p == '"') {
			skipStringAndMove(&p, &pDest);
			continue;
		}

		// if
		if (*((unsigned long*)p) == 0x636E7566) { // 'cnuf' backview 'func'
			if (!strncmp(p, FUNCTION, FUNCTION_LEN) && !isalnum(*(p - 1))
				&& !isalnum(*(p + FUNCTION_LEN)))
			{
				char *pFunStart = p;
				p = strchr(p, '(');
				if (!p) {
					print("Syntax error. '(' not found near \"function\"\n");
					return -1;
				}
				len = p - pFunStart + 1;
				memcpy(pDest, pFunStart, len);
				pDest += len;
				++p; // skip '('
				char *pParamStart = p; // + Formal parameters space
				char *pBodyEnd = findFunctionEnd(p);
				pDest = _obfuscateLocalVars(pParamStart, pBodyEnd, pDest);
				p = pBodyEnd;
			}
		}
		++pDest;
		++p;
	}
	*pDest = 0;

	return strlen(szObfuscatedLuaCode) - strlen(szLuaCode);
}

bool parseNumber(char** pp, char *szNumber) {
	const char *p = *pp;
	const char *pStart = p;

	while (isdigit(*p)) // \0 is non digit
		++p;
	if (*p == '.') { // then number is float...
		size_t sizeReal;

		while (isdigit(*p))
			++p;
		if (*p == 'e' || *p == 'E') {
			++p; // skip 'e';
			++p; // skip '+' || '-'
			while (isdigit(*p))
				++p;
		}
		sizeReal = p - pStart;
		strncpy(szNumber, pStart, sizeReal);
		szNumber[sizeReal] = 0;
		//double d = atof(szNumber); // atof(pStart)
		// size_t sizeObfuscate = obfuscateFloat ...
	}

	return false;
}

/*
 * Obfuscate the single string start with <p> and <size> bytes count
 * into <stream>
 */
void LuaObfuscator::obfuscateSingleString(StringStream &stream, const char *p, size_t size) {
	while (*p && size) {
		stream << '\\' << unsigned int(*p);
		--size;
		++p;
	}
}

/*
 * Obfuscate the integer number start with <p> and <size> bytes count
 * into <stream>
 */
void LuaObfuscator::obfuscateInt(StringStream &stream, const char *p, size_t size) {

}

/*
 * Obfuscate the float number start with <p> and <size> bytes count
 * into <stream>
 */
void LuaObfuscator::obfuscateFloat(StringStream &stream, const char *p, size_t size) {

}

/*
 * Pass <szLuaCode>
 * int:
 *   0, 123, -123
 *   1e-3, 1e+3, -1e-3, -1E+3
 *              |_unary minus
 * float:
 *   0.0, 1.0, -1.0
 *   1.01e-3, 1.01e+3, -1.01E+3, -1.01E-3
 * string:
 */
int LuaObfuscator::obfuscateConst(const char *szLuaCode,
	StringStream &obfuscatedLuaCode,
	bool bInt, bool bFloat, bool bString)
{
	int size = 0;
	char szNumber[100];

	StringStream &stream = obfuscatedLuaCode;

	char *p = const_cast<char*>(szLuaCode);
	while (*p) {
		// 0123456789+-Ee
	/*	if (*p == '-') {
			if (parseNumber(&p, szNumber)) {
				stream << '-(' << szNumber << ')';
				continue;
			}
			continue;
		}
		else if (isdigit(*p)) {
			if (parseNumber(&p, szNumber)) {
				stream << szNumber;
				continue;
			}
		}
		else */
		if (isSingleStringStart(p)) {
			int size = skipStringAndMove(&p, NULL);
			stream << *(p - size);
			obfuscateSingleString(stream, p - size + 1, size - 2);
			stream << *(p - 1);
		}

		stream << *p;

		++p;
	}
	stream << '\0';

	return size;
}

size_t LuaObfuscator::generateFalseComment(char *szAddComment) {
	size_t size = 2;
	szAddComment[0] = '-';
	szAddComment[1] = '-';
	szAddComment[2] = '[';
	szAddComment[3] = '[';

	char arr[] = "1a!b2c@f3y#u4i$5a%e6f^l7p&8ad9z(q0)_+~";
	const int N = sizeof(arr) - 1;

	for (size_t i = 4; i < 14; ++i)
		szAddComment[i] = arr[rand() % N];
	size += 11;
	szAddComment[++size] = ']';
	szAddComment[++size] = ']';
	szAddComment[++size] = 0;

	return size;
}

int LuaObfuscator::addFalseComment() {
	int size = 0;
	// The list of false comment...
	// or
	// generate the false comment...

	char szFileNameNew[MAX_PATH];

	// read global functions
	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		FILE *file = fopen(iter->c_str(), "rt");
		if (!file) {
			print("File not found: %s", iter->c_str());
			++iter;
			continue;
		}
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		rewind(file);
		char *pDataInSource = new char[fileSize + 20];
		char *pDataIn = pDataInSource + 10;
		memset(pDataInSource, 0, 10);
		int realSize = fread(pDataIn, sizeof(char), fileSize, file);
		memset(pDataIn + realSize, 0, 10);
		fclose(file);

		generateNewFileName(szFileNameNew, iter->c_str());

		file = fopen(szFileNameNew, "wt");
		char *p = pDataIn;
		while (*p) {
			if (isStringStart(p)) {
				char *pStart = p;
				int stringSize = skipStringAndMove(&p, NULL);
				if (stringSize) {
					fwrite(pStart, 1, stringSize, file);
					//obfuscateSize += len + 1;
					continue;
				}
				else {
					print("Start of line are found, but end of line not found\n");
					break;
				}
			}
			if (strchr(";,\n+*/%^", *p)) {
				char szAddComment[100];
				int len = generateFalseComment(szAddComment);
				fwrite(szAddComment, 1, len, file);
				fputc(*p, file);
				++p;
			//obfuscateSize += len + 1;
				continue;
			}
			fputc(*p, file);
			//++obfuscateSize;
			++p;
		}
		long obfuscateSize = ftell(file);
		fclose(file);
		delete[] pDataInSource;

		print("%8d %s\n", obfuscateSize - fileSize, szFileNameNew);
		size += obfuscateSize - fileSize;
		++iter;
	}

	return size;
}

int LuaObfuscator::addFalseCode() {
	int size = 0;
	// The list of false code...
	// or
	// generate the false code...

	return size;
}

#ifdef _DEBUG
void SaveToFile(const char *data, char const *szFile, char *postfix)
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

void printObfuscateResilt(const char *szResultName, const int commentSize) {
	const int width = 35;

	print("%-*s: %+d\n", width, szResultName, commentSize);
}

/*
 * Obfuscate the Lua's code in the files in list <fileList>
 * <excludeFunctions> contatins name of global function, for which don't make obfuscating
 * <obf> contains the some options
 */
int LuaObfuscator::obfuscate(const stObfuscatorSetting &settings) {
	FILE *fileNew, *fileOld;
	char szFileNameNew[FILENAME_MAX];
	int commentSize, spaceSize, newLineSize;
	int localVarsSize, constIntSize, constFloatSize, constStringSize;
	char *pDataIn, *pDataOut;

	if (m_luaFiles.empty())
		return 0;

	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end())
	{
		generateNewFileName(szFileNameNew, iter->c_str());

		print("%s\n", iter->c_str());

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

		size_t realSize = fread(pDataIn, 1, size, fileOld);
		pDataIn[realSize] = 0;
		pDataIn[realSize + 1] = 0;

		char szFileBakup[FILENAME_MAX];
		if (settings.bCreateBakFile) {
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
		constIntSize = 0;
		constFloatSize = 0;
		constStringSize = 0;
		localVarsSize = 0;

		commentSize = removeComments(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "1_comment");
#endif

		spaceSize = removeExtraWhitespace(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "2_spaces");
#endif

		newLineSize = removeNewLines(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, iter->c_str(), "3_endline");
#endif

		StringStream str;
		// DEBUG: HERE
		obfuscateConst(pDataIn, str,
			settings.ObfuscateConstInt, settings.ObfuscateConstFloat,
			settings.ObfuscateConstString);
#ifdef _DEBUG
		SaveToFile(str.str().c_str(), iter->c_str(), "7_const");
#endif
		if (settings.ObfuscateLocalVasAndParam) {
			localVarsSize = obfuscateLocalVarsAndParameters(pDataIn, pDataOut);
#ifdef _DEBUG
			SaveToFile(pDataOut, iter->c_str(), "7_local_vars");
#endif
		}

		printObfuscateResilt("Remove the Comment", commentSize);
		printObfuscateResilt("Remove the spaces", spaceSize);
		printObfuscateResilt("Remove the \"New line\"", newLineSize);
		printObfuscateResilt("Obfuscate constant integer numbers", constIntSize);
		printObfuscateResilt("Obfuscate constant strings", constStringSize);
		printObfuscateResilt("Obfuscate constant float numbers", constFloatSize);
		printObfuscateResilt("Obfuscate local vars", localVarsSize);

		fileNew = fopen(szFileNameNew, "wt");
		if (!fileNew) {
			print("Couldn't create file %s\n", szFileNameNew);
			fclose(fileOld);
			return -1;
		}
		fwrite(pDataIn, 1, strlen(pDataIn), fileNew);

		delete[] pDataInSource;
		delete[] pDataOutSource;

		fclose(fileOld);
		fclose(fileNew);

		if (settings.bCreateBakFile) {
			unlink(iter->c_str());
			rename(szFileNameNew, iter->c_str());
		}

		++iter;
	}

	if (settings.ObfuscateGlobalFunctionName) {
		print("\nObfuscate global functions\n");
		int globalFunctionSize = obfuscateGlobalFunctionNames();
		print("size: %+d\n", globalFunctionSize);
	}

	int addCommentSize = 0;
	// TODO: may be replace this code in loop of each file (see above, local)
	if (settings.ObfuscateAddFalseComment) {
		print("\nAdd comment\n");
		addCommentSize = addFalseComment();
		print("Total size: %d\n", addCommentSize);
	}

	return 0;
}
