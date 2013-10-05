//
#include "global.h"
#include "obfuscator.h"

//void printHelp();
//int parseArguments(int argc, char *argv[], std::string &globalExcludeFunctionFileName);




void printHelp() {
	print("\tLua obfuscator, Version 1.0\n");
	print("[-gef global_exclude_function_file_name]\n");
	print("[-dir directory_name]\n");
	print("[-a FILE]\n");
}

int parseArguments(int argc, char *argv[], std::string &tocFileName, std::string &addonDir,
	std::string &globalExcludeFunctionFileName, StringList &luaFiles)
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
		else if (!strcmp(arg, "-dir")) {
			if (++i < argc) {
				addonDir = argv[i];
				if (addonDir[addonDir.length() - 1] != PATH_SEPARATOR_CHAR)
					addonDir += PATH_SEPARATOR_CHAR;
				++count;
			}
		}
	}

	return count;
}

void validateFileNames(StringList &luaFiles, const char *szAddonDir = NULL) {
	StringListIter iter = luaFiles.begin();

	if (!szAddonDir)
		szAddonDir = GetExeDir();

	while (iter != luaFiles.end()) {
		std::string &str = *iter;
		if (!isAbsoluteFilePath(str.c_str())) {
			str = szAddonDir + str;
		}
		++iter;
	}
}

int main(int argc, char *argv[]) {
	StringList excludeGlobalFunctions;
	StringList luaFiles;
	std::string globalExcludeFunctionFileName;
	std::string tocFileName;
	std::string addonDir;

	parseArguments(argc, argv, tocFileName, addonDir, globalExcludeFunctionFileName, luaFiles);

	LuaObfuscator::readAddonGlobalExcludeFunctions(globalExcludeFunctionFileName.c_str(), excludeGlobalFunctions);

	LuaObfuscator::readAddonTocFile(tocFileName.c_str(), luaFiles);

	// add absolute path name, if need
	validateFileNames(luaFiles, addonDir.c_str());

	if (luaFiles.empty()) {
		printHelp();
		printf("\nNo a files for an obfuscating\n");
		return -1;
	}

	stObfuscatorSetting settings;

	// TODO: this is debug initialization
	settings.bCreateBakFile = false;
	settings.bCreateOneFile = false;
	settings.linesBetweenFiles = 3;
	settings.ObfuscateAddFalseComment = false;
	settings.ObfuscateGlobalFunctionName = true;
	settings.ObfuscateLocalFunctionName = true;
	settings.ObfuscateConstInt = false;
	settings.ObfuscateConstFloat = false;
	settings.ObfuscateConstString = false;
	settings.ObfuscateLocalVasAndParam = false;

	try {
		LuaObfuscator obfuscator(luaFiles, excludeGlobalFunctions);

		obfuscator.obfuscate(settings);
	}
	catch (std::exception) {
		print("ERROR: obfuscator creating fail\n");
	}

	return 0;
}