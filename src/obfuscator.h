#ifndef _OBFUSCATE_H
#define _OBFUSCATE_H

#include <map>
#include <string>
#include <list>
#include <set>

struct stFakeFunction
{
	std::string name;
	std::string fake_name;
};

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


struct stObfuscatorSetting
{
	bool ObfuscateFunctionGlobal;
	//bool ObFunctionLocal;
	bool ObfuscateInteger;
	bool ObfuscateLocalVasAndParam;
	bool ObfuscateAddComment;
	bool bCreateBakFile;
	bool bCreateOneFile;
	int  linesBetweenFiles;
};

const int ADDITIONAL_MEMORY = 4096 * 2;
const int FAKE_FUNCTION_LEN = 10;
const int INT_OBFUSCATE_COUNT = 2; // *2 used in integer obfuscation


class LuaObfuscator {
public:
	LuaObfuscator(const StringList &luaFiles, const StringList &excludeGlobalFunctions);
	~LuaObfuscator();

	int obfuscate(const stObfuscatorSetting &settings);

	static int readAddonTocFile(char const *szTocFileName, StringList &luaFiles);
	static int readAddonGlobalExcludeFunctions(const char *szGlobalExcludeFunctionFileName,
		StringList &FunctionsExclude);

protected:
	int removeComments();
	int removeExtraWhitespace();
	int removeNewLines();

	int obfuscateConst(bool bInt, bool bFloat, bool bString);
	int obfuscateGlobalFunctionNames();
	int addFalseComment();
	int addFalseCode();

private:
	char *m_buffer;
	stObfuscatorSetting m_settings;
	const StringList m_luaFiles;
	const StringList m_excludeFunctions;
};

#endif