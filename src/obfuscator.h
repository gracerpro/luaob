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

typedef std::list<std::string> StringList;
typedef std::list<std::string>::iterator StringListIter;
typedef std::list<std::string>::const_iterator StringListConstIter;

typedef std::set<std::string> FunctionSet;
typedef std::set<std::string>::iterator FunctionSetIter;
typedef std::set<std::string>::const_iterator FunctionSetConstIter;

typedef std::map<std::string, std::string> FakeFunctions;
typedef std::map<std::string, std::string>::iterator FakeFunctionsIter;
typedef std::map<std::string, std::string>::const_iterator FakeFunctionsIterConst;

//   key -- word
// value -- obfuscated word
typedef std::map<std::string, std::string> ObfuscatedItems;
typedef std::map<std::string, std::string>::iterator ObfuscatedItemsIter;
typedef std::map<std::string, std::string>::const_iterator ObfuscatedItemsConstIter;

namespace Obfuscator
{

struct stObfuscator
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

// Obfuscate each file in list by using options obf
int obfuscate(const stObfuscator *obf, const StringList &fileList, const StringList &excludeFunctions);

int ReadAddonTocFile(char const *szTocFileName, StringList &luaFiles);
int ReadAddonGlobalExcludeFunctions(const char *szGlobalExcludeFunctionFileName, StringList &FunctionsExclude);

}
#endif