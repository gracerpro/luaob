#ifndef _OBFUSCATE_H
#define _OBFUSCATE_H

#include <map>
#include <string>
#include <list>
#include <set>
#include "stack.h"

typedef std::list<std::string>                 StringList;
typedef std::list<std::string>::iterator       StringListIter;
typedef std::list<std::string>::const_iterator StringListConstIter;

typedef std::set<std::string>                  StringSet;
typedef std::set<std::string>::iterator        StringSetIter;
typedef std::set<std::string>::const_iterator  StringSetConstIter;

typedef std::map<std::string, std::string>                  StringMap;
typedef std::map<std::string, std::string>::iterator        StringMapIter;
typedef std::map<std::string, std::string>::const_iterator  StringMapConstIter;

typedef StringSet FunctionSet;
typedef StringMap FakeFunctions;
typedef StringMap ObfuscatedItems;
typedef StringMap LocalVars;

typedef std::stringstream StringStream;


struct stObfuscatorSetting {
	bool ObfuscateGlobalFunctionName;
	bool ObfuscateLocalFunctionName;
	bool ObfuscateConstInt;
	bool ObfuscateConstString;
	bool ObfuscateConstFloat;
	bool ObfuscateLocalVasAndParam;
	bool ObfuscateAddFalseComment;
	bool ObfuscateAddFalseCode;
	bool bCreateBakFile;
	bool bCreateOneFile;
	unsigned int linesBetweenFiles;
};

struct stObfuscatorStatistic {
	unsigned singleCommentCount;
	unsigned multilineCommentCount;
	unsigned newLineCount; // TODO: delete
	unsigned extraSpaceCount; // TODO: delete
	unsigned intNumberCount;
	unsigned floatNumberCount;
	unsigned stringCount;
	unsigned globalFunctionCount;
	unsigned localFunctionCount;
	unsigned localeVariableCount;
	unsigned formalParameterCount;

	unsigned addFalseCommentCount;
	unsigned addFalseCommentSize;
	unsigned addFalseCodeCount;
	unsigned addFalseCodeSize;
};

const int ADDITIONAL_MEMORY = 4096 * 2;
const int FAKE_FUNCTION_LEN = 10;
const int INT_OBFUSCATE_COUNT = 2; // *2 used in integer obfuscation

const int WORK_BLOCK_SIZE = 1024;

class LuaObfuscator {
public:
	LuaObfuscator(const StringList &luaFiles, const StringList &excludeGlobalFunctions,
		std::string &sAddonDir);
	~LuaObfuscator();

	int obfuscate(const stObfuscatorSetting &settings);

	static size_t readAddonTocFile(char const *szTocFileName, StringList &luaFiles);
	static int readAddonGlobalExcludeFunctions(const char *szGlobalExcludeFunctionFileName,
		StringList &FunctionsExclude);

	friend char* readAndSkipLocalVariables(char*, ObfuscatedItems&, char**);
	friend char* readAndSkipLocalVariables(char*, StringStream&, ObfuscatedItems&);
	friend char* obfuscateLocalVars(const char*, const char*, StringStream&);
//	friend char* obfuscateLocalVarsInBlock(char*, LocalVars&, StringStream&);
	friend char* readAndObfuscateFunctionArguments(char*, LocalVars&, LocalVarsStack&, StringStream&);
	friend char* readAndObfuscateLocaleVariables(char*, LocalVars&, LocalVars&, LocalVarsStack&, StringStream&);

	static const char* generateObfuscatedFunctionName();
	static const char* generateObfuscatedLocalVariableName(); // and arguments
	static size_t generateFalseComment(char *szAddComment);

protected:
	const char* getFileName(StringListConstIter iter);

	ptrdiff_t removeComments(char *szLuaCode);
	ptrdiff_t removeExtraWhitespace(char *szLuaCode);
	ptrdiff_t removeDumplicatedChars(char *szLuaCode); // ' ' and '\n' and '\t'
	ptrdiff_t removeNewLines(char *szLuaCode);

	void obfuscateInt(StringStream &stream, const char *p, size_t size);
	void obfuscateFloat(StringStream &stream, const char *p, size_t size);
	void obfuscateSingleString(StringStream &stream, const char *p, size_t size);
	//void obfuscateMultilineString(std::stringstream &stream, const char *p, size_t size);

	int obfuscateConst(const char *szLuaCode, StringStream &obfuscatedLuaCode, const bool bInt, const bool bFloat, const bool bStrin);
	ptrdiff_t obfuscateGlobalFunctionNames();
	ptrdiff_t obfuscateLocalVarsAndParameters(const char *szLuaCode, StringStream &obfuscatedLuaCode);
	int addFalseComment();
	int addFalseCode();

private:
	const StringList      &m_luaFiles;
	const StringList      &m_excludeFunctions;
	const std::string     &m_sAddonDir;
	stObfuscatorStatistic  m_statistic;

	int readGlobalFunctions(const char *szFileName, FakeFunctions &Functions);
};

#endif
