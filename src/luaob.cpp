//
#include "global.h"
#include "obfuscator.h"
#include <time.h>

//void printHelp();
//int parseArguments(int argc, char *argv[], std::string &globalExcludeFunctionFileName);




void printHelp() {
	print("\tLua obfuscator, Version 1.0\n");
	print("[-gef global_exclude_function_file_name]\n");
	print("[-dir directory_name]\n");
	print("[-a FILE]\n");
}

int parseArguments(int argc, char *argv[], std::string &tocFileName, std::string &addonDir,
	std::string &globalExcludeFunctionFileName, StringList &luaFiles,
	stObfuscatorSetting &setting)
{
	int count = 0;

	// sets default state
	setting.bCreateOneFile = false;
	setting.ObfuscateAddFalseCode = false;
	setting.ObfuscateAddFalseComment = false;
	setting.ObfuscateConstFloat = false;
	setting.ObfuscateConstInt = false;
	setting.ObfuscateConstString = false;
	setting.ObfuscateGlobalFunctionName = false;
	setting.ObfuscateLocalFunctionName = false;
	setting.ObfuscateLocalVasAndParam = false;
	setting.bCreateBakFile = true;
	setting.linesBetweenFiles = 3;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strncmp(arg, "-opt-", 5)) {
			const char *opt = arg + 5;

			if (!strcmp(opt, "one_file")) {
				setting.bCreateOneFile = true;
			}
			else if (!strcmp(opt, "add_false_code")) {
				setting.ObfuscateAddFalseCode = true;
			}
			else if (!strcmp(opt, "add_false_comment")) {
				setting.ObfuscateAddFalseComment = true;
			}
			else if (!strcmp(opt, "const_float")) {
				setting.ObfuscateConstFloat = true;
			}
			else if (!strcmp(opt, "const_int")) {
				setting.ObfuscateConstInt = true;
			}
			else if (!strcmp(opt, "const_string")) {
				setting.ObfuscateConstString = true;
			}
			else if (!strcmp(opt, "global_function")) {
				setting.ObfuscateGlobalFunctionName = true;
			}
			else if (!strcmp(opt, "local_function")) {
				setting.ObfuscateLocalFunctionName = true;
			}
			else if (!strcmp(opt, "local_vars_args")) {
				setting.ObfuscateLocalVasAndParam = true;
			}
		}
		else if (!strcmp(arg, "-t")) {
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
		else if (!strcmp(arg, "-between_lines")) {
			if (++i < argc) {
				setting.linesBetweenFiles = atoi(argv[i]);
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
	stObfuscatorSetting settings;

	srand(static_cast<unsigned int>(time(NULL)));

	parseArguments(argc, argv, tocFileName, addonDir, globalExcludeFunctionFileName, luaFiles, settings);

	LuaObfuscator::readAddonGlobalExcludeFunctions(globalExcludeFunctionFileName.c_str(), excludeGlobalFunctions);

	LuaObfuscator::readAddonTocFile(tocFileName.c_str(), luaFiles);

	// add absolute path name, if need
	validateFileNames(luaFiles, addonDir.c_str());

	if (luaFiles.empty()) {
		printHelp();
		printf("\nNo a files for an obfuscating\n");
		return -1;
	}

	try {
		LuaObfuscator obfuscator(luaFiles, excludeGlobalFunctions);

		obfuscator.obfuscate(settings);
	}
	catch (std::exception) {
		print("ERROR: obfuscator creating fail\n");
	}

#ifdef _DEBUG
	getchar();
#endif

	return 0;
}