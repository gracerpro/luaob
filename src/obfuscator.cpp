#include "obfuscator.h"
#include "global.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <time.h>

// 26 + 1 + 26
static char const szCharsFunName[] = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int CHARS_FUN_NAME_LEN = sizeof(szCharsFunName) - 1;


/*
 * Delete the comments from a Lua's code
 * --[[ Multiline comments ]]
 * -- Singleline comment
 */
LuaObfuscator::LuaObfuscator(const StringList &luaFiles, const StringList &excludeGlobalFunctions,
	std::string &sAddonDir)
	: m_luaFiles(luaFiles), m_excludeFunctions(excludeGlobalFunctions), m_sAddonDir(sAddonDir)
{
	memset(&m_statistic, 0, sizeof(m_statistic));
}

LuaObfuscator::~LuaObfuscator() {

}

const char* LuaObfuscator::getFileName(StringListConstIter iter) {
	static char file[MAX_PATH];

	strcpy(file, m_sAddonDir.c_str());

	return strcat(file, iter->c_str());
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
			++m_statistic.multilineCommentCount;
		}
		else if (*((uint16_t*)p) == 0x2D2D) { // 0x == '--'
			p += 2;
			while (*p && !isNewLine(*p))
				++p;
			++m_statistic.singleCommentCount;
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
	char const *arrClearChar = "{}()[].,;+-*/^%<>~=#\n";
	char *p = szLuaCode;

	if (!p || !p[0])
		return 0;

	char *pDest = p;

	while (*p && isNewLine(*p))
		++p;
	while (*p) {
		if (isSingleStringStart(p)) {
			size_t size = skipStringAndMove(&p, NULL); // &pDest
			char *end = p;
			p -= size;
			char cPrev = 0;
			for ( ; p < end; ++p) {
				if (isNewLine(*p) && cPrev == '\\') {
					*(pDest++) = 'n';
				}
				else {
					*(pDest++) = *p;
				}
				cPrev = *p;
			}
			continue;
		}
		if (isMultilineStringStart(p)) {
			skipStringAndMove(&p, &pDest);
		}
		if (!strncmp(p, "tostring(obj)", sizeof("tostring(obj)") - 1))
			p = p;

		if (isNewLine(*p)) {
			char cPrev = *(p - 1);
			while (*p && isNewLine(*p))
				++p;
			if (cPrev == '\\') {
				*(pDest++) = 'n'; // within string here
			}
			else if (cPrev == '"') { // TODO: hack to locale string
				*(pDest++) = ';';
			}
			else if (!strchr(arrClearChar, cPrev)) { //  cPrev != ';' && cPrev != ')'
				*(pDest++) = ' ';
			}
			if (isSpace(*p))
				++p;
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
 * delete dublicated spaces
 */
int LuaObfuscator::removeDumplicatedChars(char *szLuaCode) {
	char *pDest = szLuaCode;
	char *p     = szLuaCode;

	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			continue;
		}
		if (!(isSpace(*p) && isSpace(*(p - 1)) ||
			isNewLine(*p) && isNewLine(*(p - 1))))
		{
			*pDest = *p;
			++pDest;
		}
		++p;
	}
	*pDest = 0;

	return pDest - p;
}

/*
 * Delete the spaces near some characters
 */
int LuaObfuscator::removeExtraWhitespace(char *szLuaCode) {
	char const *arrClearChar = "{}()[].,;+-*/^%<>~=#\n";
	size_t spaceCount;
	char *p     = szLuaCode;
	char *pDest = szLuaCode;

	if (!p || !p[0])
		return 0;

	while (*p && isSpace(*p))
		++p;

	// delete spaces near chars
	spaceCount = 0;
	while (*p) {
		if (isStringStart(p)) {
			skipStringAndMove(&p, &pDest);
			spaceCount = 0;
			continue;
		}

		if (strchr(arrClearChar, *p)) {
			pDest -= spaceCount;
			*pDest++ = *p++;
			while (isSpace(*p))
				++p;
			spaceCount = 0;
			continue;
		}
		if (isSpace(*p))
			++spaceCount;
		else
			spaceCount = 0;
		*pDest = *p;
		++pDest;
		++p;
	}
/*	while (spaceCount && isSpace(*pDest)) { // TODO: die code?
		--pDest;
		--spaceCount;
	}*/
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

	luaFiles.clear();
	//files.str("");
	while (!feof(fileToc) && fgets(buf, 300, fileToc)) {
		if (!buf[0])
			continue;
		char *pFile = strtrim(buf);
		char *pFileExt = strrchr(pFile, '.');
		bool bLuaFile = false;
		if (pFileExt) {
			strlwr(pFileExt);
			bLuaFile = !strcmp(pFileExt, ".lua");
		}
		if (pFile[0] && pFile[0] != '#' &&  bLuaFile) {
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

int LuaObfuscator::readGlobalFunctions(const char *szFileName, FakeFunctions &globalFunctions)
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
			if (!strncmp(p, "function", 8)) {
				char szFun[200];
				int trancSpaces = getGlobalFunctionName(&p, szFun);
				if (trancSpaces && szFun[0]) {
					stFakeFunction fun;
					fun.name = szFun;
					fun.fake_name = LuaObfuscator::generateObfuscatedFunctionName();
					globalFunctions[fun.name] = fun.fake_name;
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
int replaceGlobalFunctions(const char *szFileName, FakeFunctions &globalFunctions,
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

	int realSize = fread(pDataIn, 1, size, file);
	memset(pDataIn + realSize, 0, 10);

	fclose(file);

	StringStream stream;
	char *p = pDataIn;

	while (*p) {
		if (isStringStart(p)) {
			size_t size = skipStringAndMove(&p, NULL);
			stream.write(p - size, size);
			continue;
		}

		if (!strncmp(p, "tostring", sizeof("tostring") - 1))
			*p = *p;

		char *pStart = p;
		while (isAlphaFun(*p)) {
			++p;
		}

		size_t wordSize = p - pStart;
		if (wordSize) {
			if (isFunctionNameInCode(pStart, p)) {
				std::string str(pStart, wordSize);
				StringMapConstIter iter = globalFunctions.find(str);
				if (iter != globalFunctions.end()) {
					StringListConstIter IterFunExclude = std::find(excludeFunctions.begin(), excludeFunctions.end(), str);
					if (IterFunExclude == excludeFunctions.end()) {
						// replace
						stream.write(iter->second.c_str(), FAKE_FUNCTION_LEN);
						continue;
					}
				}
			}
			stream.write(pStart, wordSize);
			if (!p[0]) {
				break;
			}
		}
		stream << *p;

		++p;
	}


	file = fopen(szFileName, "wt");
	const std::string& str = stream.str();
	size_t obfuscateSize = str.length();
	if (file) {
		fwrite(str.c_str(), 1, obfuscateSize, file);
		fclose(file);
	}

	delete[] pDataInSource;

	print("%8d %s\n", obfuscateSize - realSize, szFileName);

	return obfuscateSize - realSize;
}

int LuaObfuscator::obfuscateGlobalFunctionNames() {
	FakeFunctions globalFunctions;
	int size = 0;
	char szFileNameNew[MAX_PATH];

	// read global functions
	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		const char *szFileName = getFileName(iter);
		generateNewFileName(szFileNameNew, szFileName);
		readGlobalFunctions(szFileNameNew, globalFunctions);
		++iter;
	}

	// replace global functions
	iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		const char *szFileName = getFileName(iter);
		generateNewFileName(szFileNameNew, szFileName);
		size += replaceGlobalFunctions(szFileNameNew, globalFunctions, m_excludeFunctions);
		++iter;
	}

	return size;
}

// Find end of function
// p -- pointer to first byte of formal parameters
char* findFunctionEnd(char *pAgrumentsStart)
{
	int nEnd = 0;

	char *p = pAgrumentsStart;
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

const char* LuaObfuscator::generateObfuscatedLocalVariableName() {
	const int N = 7;
	static char str[100];

	for (int i = 0; i < N; ++i)
		str[i] = szCharsFunName[rand() % CHARS_FUN_NAME_LEN];
	str[N] = 0;
	uint16_t w = static_cast<uint16_t>(time(NULL));
	w ^= rand(); // xor 0xFFFF
	memset(&str[N], '0', 4);
	sprintf(&str[N], "%04X", w);
	//itoa(w, &str[N], 16);
	str[N + 4] = 0;

	return str;
}

/*
 * read and skipp all variables before '='
 * ..."local "...
 *            |_this is pointer p
 */
char* readAndSkipLocalVariables(char *p, StringStream &stream, ObfuscatedItems &vars)
{
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

	while (p < pEqual) {
		char *pStart = p;
		char *pEnd = strtok(p, ",=");
		if (!pEnd)
			pEnd = pEqual - 1;
		size_t len = pEnd - pStart - 1;
		if (len) {
			std::string str(pStart, len);
			StringMapConstIter item = vars.find(str);
			if (item == vars.end()) {
				char const *pObName = LuaObfuscator::generateObfuscatedLocalVariableName();
				vars[str] = pObName;
				stream << pObName << *pEnd;
			}
			else {
				stream << item->second.c_str() << *pEnd;
			}
		}
		p = pEnd + 1;
	}

	return pEqual + 1;
}

/*
 * params:
 *   pArgumentStart -- first byte of arguments
 * return:
 *   first byte of function's body
 */
char* readAndObfuscateFunctionArguments(char *pArgumentStart, LocalVars &vars,
	StringStream &stream)
{
	char *p = pArgumentStart;

	const char *pRightSk = strchr(p, ')');
	if (!pRightSk) {
		print("Parsing error. ')' not found near arguments\n");
		return p;
	}

	// Find agruments...
	do {
		const char *pStart = p;

		while (*p != ',' && p < pRightSk)
			++p;
		size_t size = p - pStart;
		if (size) {
			std::string str(pStart, size);
			if (str != "...") {
				const char *szObVarName = LuaObfuscator::generateObfuscatedLocalVariableName();
				vars[str] = szObVarName;
				stream << szObVarName << *p; // *p == ',' or ')'
			}
			else {
				stream << "..." << *p;
			}
		}
		++p;
	}
	while (p < pRightSk);

	return p;
}

/*
 * Read function name
 * p -- pointer to first byte after string "function"
 * "... function"
 *              |_ here
 */
char* readFunctionName(char *p, StringStream &stream)
{
	char *pFunNameStart = p + 8;

	stream << "function";
	if (isSpace(*pFunNameStart)) {
		stream << *pFunNameStart;
		++pFunNameStart;
	}
	p = pFunNameStart;
	// find function name
	while (*p && isAlphaFun(*p))
		++p;
	if (*p != '(') {
		printf("parse error, '(' not found near function name\n");
		return p;
	}
	size_t nameSize = p - pFunNameStart;
	if (nameSize) {
		stream.write(pFunNameStart, nameSize);
	}
	stream << '(';

	return p + 1;
}

/*
 * p -- pointer to ferst byte of local variable
 * example:
 *   locale var1, var2, var3;
 *          |_ this is first byte
 * Read the locale variables
 * and obfuscate them names
 * and push equal variables
 */
char* readAndObfuscateLocaleVariables(char *p, LocalVars &varsLocal, LocalVars &varsBlock,
	StringStream &stream)
{
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
		print("Parse error near locale variables\n");
		return p;
	}

	while (p < pEqual) {
		char *pStart = p;
		while (isAlphaFun(*p))
			++p;
		size_t len = p - pStart;
		if (len) {
			std::string str(pStart, len);
			StringMapConstIter item = varsLocal.find(str);
			if (item != varsLocal.end()) {
				// PUSH
				char const *pObName = LuaObfuscator::generateObfuscatedLocalVariableName();
				varsLocal[str] = pObName;
				stream << item->second.c_str() << *p;
			}
			else {
				char const *pObName = LuaObfuscator::generateObfuscatedLocalVariableName();
				varsBlock[str] = pObName;
				//varsLocal[str] = pObName;
				stream << pObName << *p;
			}
		}
		++p;
	}

	return pEqual + 1;
}

size_t popLocalVariables(LocalVars &varsLocal, LocalVars &varsBlock) {
	StringMapConstIter iter = varsBlock.begin();
	size_t count = 0;

/*	for ( ; iter != varsBlock.end(); ++iter) {
		StringMapConstIter iterFind = varsLocal.find(iter->first);
		if (iterFind != varsLocal.end()) {
			varsLocal[iter->first] = varsBlock[iter->first];
		}
		else
			varsLocal.erase(iter);
		++count;
	}*/

	return count;
}

/*
 * params:
 *   pBlockBodyStart -- pointer to first byte of block's body
 *   varsLocal -- list of agrument's names, level up variables
 * blocks:
 *   1) function(...) block end
 *   2) do block end
 *   3) for ... do block end
 *   4) while ... do block end
 *   5) if ... then block end
 *   6) function type, this is similar to function, see item 1)
 *  =============================
 *  obfuscate until not found "end" of current block
 *
 * return:
 *   first byte after block's body
 */
char* obfuscateLocalVarsInBlock(char *pBlockBodyStart, LocalVars &varsLocal,
	StringStream &stream)
{
	char *p = const_cast<char*>(pBlockBodyStart);
//	int blockLevel = 0; // current block
	size_t wordLen = 0;
	char wordBuffer[300];
	char *pWordBuffer = wordBuffer;
	LocalVars varsBlock;

	while (*p) {
		if (isStringStart(p)) {
			size_t size = skipStringAndMove(&p, NULL);
			stream.write(p - size, size);
			continue;
		}

		// skip local functions, functions in parameters
		if (*p == 'd') {
			if (!strncmp(p, "do", 2) && !isalnum(*(p - 1)) && !isalnum(*(p + 2))) {
				stream << "do" << *(p + 2);
				char *pBodyStart = p + 2 + 1;
				p = obfuscateLocalVarsInBlock(pBodyStart, varsLocal, stream);
			}
		}
		else if (*p == 'r') { // TODO: repeat block
			if (!strncmp(p, "repeat", 6) && !isalnum(*(p - 1)) && !isalnum(*(p + 6))) {
				stream << "repeat" << *(p + 6);
				char *pBodyStart = p + 6 + 1;
				p = obfuscateLocalVarsInBlock(pBodyStart, varsLocal, stream);
			}
		}
		else if (*p == 'f') {
			if (!strncmp(p, "function", 8) && !isalnum(*(p + 8))) {
				LocalVars vars;
				p = readFunctionName(p, stream);
				p = readAndObfuscateFunctionArguments(p, vars, stream);
				p = obfuscateLocalVarsInBlock(p, vars, stream);
			}
		}
		// skip the "while", "for" and "if" constructions
		else if (*p == 't') {
			if (!strncmp(p, "then", 4) && !isalnum(*(p-1)) && !isalnum(*(p + 4))) {
				stream << "then" << *(p + 4);
				char *pBodyStart = p + 4 + 1;
				p = obfuscateLocalVarsInBlock(pBodyStart, varsLocal, stream);
			}
		}
		else if (*p == 'e') {
			if (!strncmp(p, "end", 3) && !isalnum(*(p - 1)) && !isalnum(*(p + 3))) {
				stream << "end";
				// pop the equal variables...
				popLocalVariables(varsLocal, varsBlock);
				return p + 3;
			}
			if (!strncmp(p, "elseif", 3) && !isalnum(*(p - 1)) && !isalnum(*(p + 6))) {
				stream << "elseif";
				// pop the equal variables...
				popLocalVariables(varsLocal, varsBlock);
				return p + 6;
			}
		}
		else if (*p == 'u') {
			if (!strncmp(p, "until", 5) && !isalnum(*(p - 1)) && !isalnum(*(p + 5))) {
				stream << "until";
				// pop the equal variables...
				popLocalVariables(varsLocal, varsBlock);
				return p + 5;
			}
		}
		else if (*p == 'l') {
			if (!strncmp(p, "local", 5) && !isalnum(*(p - 1)) && !isalnum(*(p + 5))) {
				stream << "local" << *(p + 5);
				p += 6;
				// push(save) the equal variables...
				p = readAndObfuscateLocaleVariables(p, varsLocal, varsBlock, stream);
				// DEBUG...
			}
		}

		if (isAlphaFun(*p)) {
			++wordLen;
			*pWordBuffer = *p;
			++pWordBuffer;
			++p;
			continue;
		}
		else {
			*pWordBuffer = 0;
			char *pBefore = p - wordLen - 1;
			if (wordLen > 0 && (*pBefore != '.' || *(pBefore - 1) == '.')) {
				std::string str(wordBuffer);
				StringMapConstIter iter = varsBlock.find(str);
				if (iter != varsBlock.end()) {
					size_t len = iter->second.length();
					stream.write(iter->second.c_str(), len);
				}
				else {
					iter = varsLocal.find(str);
					if (iter != varsLocal.end()) {
						size_t len = iter->second.length();
						stream.write(iter->second.c_str(), len);
					}
					else {
						stream << wordBuffer;
					}
				}
			}
			wordLen = 0;
			pWordBuffer = wordBuffer;
		}

		stream << *p;
		++p;
	}

	// pop the equal variables...

	return p;
}

/*
 * pParamStart -- pointer to first char of formal parameters
 */
char* obfuscateLocalVars(const char *pParamStart, char const *pBodyEnd, StringStream &stream) {
	char *p = const_cast<char*>(pParamStart);
	ObfuscatedItems vars;
	std::string str;
	size_t wordLen = 0;
	size_t len;

	char *pRightSk = strchr(p, ')');
	if (!pRightSk) {
		print("')' not found\n");
		return NULL;
	}

	// Find agruments...
	do {
		const char *pStart = p;

		while (*p != ',' && p < pRightSk)
			++p;
		size_t size = p - pStart;
		if (size) {
			str.assign(pStart, size);
			if (str != "...") {
				const char *pObFunName = LuaObfuscator::generateObfuscatedLocalVariableName();
				vars[str] = pObFunName;
				stream << pObFunName << *p; // *p == ',' or ')'
			}
			else {
				stream << "..." << *p;
			}
		}
		++p;
	}
	while (p < pRightSk);

	// Parse local variables...
	/*
	in the block local variables difference!
	block:
		do <block> end
		for ... do <block> end
		while ... do <block> end
		if ... then <block> end
	*/
	p = pRightSk + 1; // first byte of function's body

	//stream << ')';

	int blockLevel = 0;
	char *pEnd = p;
	char *pWord = p;
	char wordBuffer[300];
	char *pWordBuffer = wordBuffer;
	while (p <= pBodyEnd) {
		if (isStringStart(p)) {
			size_t size = skipStringAndMove(&p, NULL);
			stream.write(p - size, size);
			continue;
		}

		if (!strncmp(p, "do", 2) && !isalpha(*(p - 1)) && !isalpha(*(p + 2))) {
			++blockLevel;
		}
		else if (!strncmp(p, "if", 2) && !isalpha(*(p - 1)) && !isalpha(*(p + 2))) {
			++blockLevel;
		}

	//	char szOp1[100];
	//	char szOp2[100];
	//	GetOperand(p, &len, true);

		if (*p == 'l') {
			if (!strncmp(p, "local", 5) && !isalnum(*(p - 1)) && isSpace(*(p + 5))) {
				stream << "local "; // *(p + 5) == ' ' or '\t'
				p += 6;
				p = readAndSkipLocalVariables(p, stream, vars);
				wordLen = 0;
				pWord = p;
				continue;
			}
		}

		if (isAlphaFun(*p)) {
			++wordLen;
			*pWordBuffer = *p;
			++pWordBuffer;
			++p;
			continue;
		}
		else {
			*pWordBuffer = 0;
			char *pBefore = p - wordLen - 1;
			if (wordLen > 0 && (*pBefore != '.' || *(pBefore - 1) == '.')) {
				str.assign(pWord, wordLen);
				StringMapConstIter iter = vars.find(str);
				if (iter != vars.end()) {
					len = iter->second.length();
					stream.write(iter->second.c_str(), len);
				}
				else {
					stream << wordBuffer;
				}
			}
			pWord = p + 1;
			wordLen = 0;
			pWordBuffer = wordBuffer;
		}

		stream << *p;
		++p;
	}

	return 0;
}

/*
 * Recurcive function
 * Obfuscated all local variables in code
 * Example:
 *   local varName = 123
 *   local abrakadabra = 123
 */
int LuaObfuscator::obfuscateLocalVarsAndParameters(const char *szLuaCode, StringStream &obfuscatedLuaCode) {
	LocalVars vars;
	char *p = const_cast<char*>(szLuaCode);

	obfuscatedLuaCode.str("");
	obfuscateLocalVarsInBlock(p, vars, obfuscatedLuaCode);

	return obfuscatedLuaCode.str().size() - strlen(szLuaCode);

/*	StringStream &stream = obfuscatedLuaCode;

	stream.str("");
	while (*p) {
		if (isStringStart(p)) {
			size_t size = skipStringAndMove(&p, NULL);
			stream.write(p - size, size);
			continue;
		}

		const int FUNCTION_LEN = sizeof("function") - 1;
		if (*((unsigned long*)p) == 0x636E7566) { // 'cnuf' backview 'func'
			if (!strncmp(p, "function", FUNCTION_LEN) && !isalnum(*(p + FUNCTION_LEN)))
			{
				//if (p > szLuaCode && !isalnum(*(p + FUNCTION_LEN))) {
					char *pFunStart = p;
					char *pFunEnd = p + FUNCTION_LEN;
					char *pFunNameStart = pFunEnd;

					stream << "function";
					if (isSpace(*pFunNameStart)) {
						++pFunNameStart;
						stream << " ";
					}
					p = pFunNameStart;
					// find function name
					while (*p && *p != '(')
						++p;
					size_t nameSize = p - pFunNameStart;
					//if (pFunEnd == p - 1 )
					//	continue;
					if (*p != '(') {
						print("Syntax error. '(' not found near \"function\"\n");
						return -1;
					}

					if (nameSize)
						stream.write(pFunNameStart, nameSize);
					stream << "(";
					++p;
					LocalVars vars;
					char *pParamStart = p; // + Formal parameters space
					p = readAndObfuscateFunctionArguments(pParamStart, vars, stream);
					//char *pBodyEnd = findFunctionEnd(p);
					//p = obfuscateLocalVars(pParamStart, pBodyEnd, stream);
					p = obfuscateLocalVarsInBlock(p, vars, stream);
					//p = pBodyEnd + 1;
					continue;
				//}
			}
		}
		stream << *p;
		++p;
	}

	return stream.str().size() - strlen(szLuaCode);
	*/
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
	const char *szEscapeSec = "abftvrn\"\'\\";

	while (*p && size) {
		bool bBackslash   = (*p == '\\' && *(p + 1) == '\\');
		bool bEscapeChars = (*p == '\\' && strchr(szEscapeSec, *(p + 1)));

		if (bBackslash || bEscapeChars) {
			++p;
			--size;
		}
		else if (*p == '\\' && size >= 4) {
			bool bDigit = isdigit(*(p + 1)) && isdigit(*(p + 2)) && isdigit(*(p + 3));
			if (bDigit) {
				stream.write(p, 4);
				p += 4;
				size -= 4;
				continue;
			}
		}
		unsigned int code = static_cast<unsigned int>(static_cast<unsigned char>(*p));
		stream << '\\' << code;
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
	const bool bInt, const bool bFloat, const bool bString)
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
			size_t size = skipStringAndMove(&p, NULL);
			if (bString) {
				stream << *(p - size);
				obfuscateSingleString(stream, p - size + 1, size - 2);
				stream << *(p - 1);
			}
			else {
				stream.write(p - size, size);
			}
			++m_statistic.stringCount;
		}

		stream << *p;

		++p;
	}

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
		const char *szFileName = getFileName(iter);
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
				size_t stringSize = skipStringAndMove(&p, NULL);
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

	getFileDir(szFile, buf);
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
	int commentSize, spaceSize, newLineSize, duplicateCharsSize;
	int localVarsSize, constIntSize, constFloatSize, constStringSize;

	if (m_luaFiles.empty())
		return 0;

	StringListConstIter iter = m_luaFiles.begin();
	while (iter != m_luaFiles.end()) {
		const char *szFileName = getFileName(iter);
		fileOld = fopen(szFileName, "rt");
		if (!fileOld) {
			print("ERROR: Open file fail\n");
			++iter;
			continue;
		}

		fseek(fileOld, 0, SEEK_END);
		long size = ftell(fileOld);
		rewind(fileOld);

		char *pDataInSource = new char[size + 5];
		char *pDataIn = pDataInSource + 2; // for delete/clear data
		if (!pDataInSource) {
			fclose(fileOld);
			print("Try to alloc memory failed, size %d\n", size + 5);
			return 0;
		}

		size_t realSize = fread(pDataIn, 1, size, fileOld);
		pDataInSource[0] = 0; // \0\0...............\0\0
		pDataInSource[1] = 0;
		pDataIn[realSize] = 0;
		pDataIn[realSize + 1] = 0;


		char szFileBakup[FILENAME_MAX];
		if (settings.bCreateBakFile) {
			strcpy(szFileBakup, szFileName);
			strcat(szFileBakup, ".bak");
			FILE *fileBakup = fopen(szFileBakup, "wt");
			if (fileBakup) {
				fwrite(pDataIn, 1, strlen(pDataIn), fileBakup);
				fclose(fileBakup);
			}
		}

		commentSize = 0;
		duplicateCharsSize = 0;
		spaceSize = 0;
		newLineSize = 0;
		constIntSize = 0;
		constFloatSize = 0;
		constStringSize = 0;
		localVarsSize = 0;

		commentSize = removeComments(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, szFileName, "1_comment");
#endif
		duplicateCharsSize = removeDumplicatedChars(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, szFileName, "2_dublicate");
#endif

		spaceSize = removeExtraWhitespace(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, szFileName, "3_spaces");
#endif

		newLineSize = removeNewLines(pDataIn);
#ifdef _DEBUG
		SaveToFile(pDataIn, szFileName, "4_endline");
#endif

		StringStream strIn, strOut;

		if (settings.ObfuscateConstInt || settings.ObfuscateConstFloat ||
			settings.ObfuscateConstString)
		{
			obfuscateConst(pDataIn, strOut, settings.ObfuscateConstInt, settings.ObfuscateConstFloat,
			settings.ObfuscateConstString);
#ifdef _DEBUG
			SaveToFile(strOut.str().c_str(), szFileName, "5_const");
#endif
		}
		else {
			strOut.str("");
			strOut << pDataIn;
		}

		strIn.str("");
		strIn << strOut.str();
		if (settings.ObfuscateLocalVasAndParam) {
			localVarsSize = obfuscateLocalVarsAndParameters(strIn.str().c_str(), strOut);
#ifdef _DEBUG
			SaveToFile(strOut.str().c_str(), szFileName, "6_local_vars");
#endif
		}

		printObfuscateResilt("Remove the comments", commentSize);
		printObfuscateResilt("Remove the duplicate chars", duplicateCharsSize);
		printObfuscateResilt("Remove the spaces", spaceSize);
		printObfuscateResilt("Remove the \"New line\"", newLineSize);
		printObfuscateResilt("Obfuscate constant integer numbers", constIntSize);
		printObfuscateResilt("Obfuscate constant float numbers", constFloatSize);
		printObfuscateResilt("Obfuscate constant strings", constStringSize);
		printObfuscateResilt("Obfuscate local vars", localVarsSize);

		generateNewFileName(szFileNameNew, szFileName);

		fileNew = fopen(szFileNameNew, "wt");
		if (!fileNew) {
			print("Couldn't create file %s\n", szFileNameNew);
			fclose(fileOld);
			return -1;
		}
		const std::string &sTmp = strOut.str();
		const char *p = sTmp.c_str(); // TODO: correct code
		size_t lenCstr = strlen(p);
		size_t lenStream = sTmp.length(); // BAG: length difference!
		fwrite(p, 1, lenCstr, fileNew);

		delete[] pDataInSource;

		fclose(fileOld);
		fclose(fileNew);

	/*	if (settings.bCreateBakFile) {
			unlink(szFileName);
			rename(szFileNameNew, szFileName);
		}*/

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
