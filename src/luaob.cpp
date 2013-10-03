//
#include "obfuscator.h"
#include "global.h"


//void printHelp();
//int parseArguments(int argc, char *argv[], std::string &globalExcludeFunctionFileName);




void printHelp() {
	printf("Lua obfuscator, Version 1.0");
	printf("[--gef global_exclude_function_file_name]");
	printf("[-a[ppend] FILE]");
}

int parseArguments(int argc, char *argv[], std::string &tocFileName,
	std::string &globalExcludeFunctionFileName,
	StringList &luaFiles)
{
	int count = 0;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "-t")) {
			if (++i < argc) {
				tocFileName = argv[i];
				++count;
			}
		}
		else if (!strcmp(arg, "-gef")) {
			if (++i < argc) {
				globalExcludeFunctionFileName = argv[i];
				++count;
			}
		}
		else if (!strcmp(arg, "-a")) {
			if (++i < argc) {
				luaFiles.push_back(argv[i]);
				++count;
			}
		}
	}

	return count;
}

int main(int argc, char *argv[]) {
	StringList excludeGlobalFunctions;
	StringList luaFiles;
	std::string globalExcludeFunctionFileName;
	std::string tocFileName;

	parseArguments(argc, argv, tocFileName, globalExcludeFunctionFileName, luaFiles);

	LuaObfuscator::readAddonGlobalExcludeFunctions(globalExcludeFunctionFileName.c_str(), excludeGlobalFunctions);

	LuaObfuscator::readAddonTocFile(tocFileName.c_str(), luaFiles);

	if (luaFiles.empty()) {
		printf("No a files for an obfuscating\n");
		return -1;
	}

	stObfuscatorSetting settings;

	// TODO: this is debug initialization
	settings.bCreateBakFile = false;
	settings.bCreateOneFile = false;
	settings.linesBetweenFiles = 3;
	settings.ObfuscateAddComment = false;
	settings.ObfuscateFunctionGlobal = true;
	settings.ObfuscateInteger = false;
	settings.ObfuscateLocalVasAndParam = false;

	LuaObfuscator obfuscator(luaFiles, excludeGlobalFunctions);

	obfuscator.obfuscate(settings);

	return 0;
}