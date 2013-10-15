//
#include "global.h"
#include "obfuscator.h"
#include <time.h>

//void printHelp();
//int parseArguments(int argc, char *argv[], std::string &globalExcludeFunctionFileName);




void printHelp() {
	print("=============== Lua obfuscator, Version 1.0\n");
	print("===== Copyright (C) 2013, Gracer <gracerpro@gmail.com>\n");
	print("[-t FILE.toc]\n");
	print("[-gef global_exclude_function_file_name]\n");
	print("[-a FILE]\n");
	print("[-dir DIRECTORY]\n");
	print("[-between_lines line_count]\n");
	print("[-opt-one_file]\n");
	print("[-opt-add_false_code]\n");
	print("[-opt-add_false_comment]\n");
	print("[-opt-const_float]\n");
	print("[-opt-const_int]\n");
	print("[-opt-const_string]\n");
	print("[-opt-global_function]\n");
	print("[-opt-local_function]\n");
	print("[-opt-local_vars_args]\n");
}

int parseArguments(int argc, char *argv[], std::string &tocFileName, std::string &addonDir,
	std::string &globalExcludeFunctionFileName, StringList &luaFiles,
	stObfuscatorSetting &setting)
{
	int count = 0;

	// sets default state
	setting.bCreateOneFile              = false;
	setting.ObfuscateAddFalseCode       = false;
	setting.ObfuscateAddFalseComment    = false;
	setting.ObfuscateConstFloat         = false;
	setting.ObfuscateConstInt           = false;
	setting.ObfuscateConstString        = false;
	setting.ObfuscateGlobalFunctionName = false;
	setting.ObfuscateLocalFunctionName  = false;
	setting.ObfuscateLocalVasAndParam   = false;
	setting.bCreateBakFile              = true;
	setting.linesBetweenFiles           = 3;

//	addonDir = "e:/Software/Games/World of Warcraft/Interface/AddOns/chardumps/";

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
				if (addonDir == ".") {
					addonDir = getWorkDir();
				}
				char c = addonDir[addonDir.length() - 1];
				if (!isPathSep(c))
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

	if (!tocFileName.empty() && !isAbsoluteFilePath(tocFileName.c_str())) {
		if (!addonDir.empty())
			tocFileName = addonDir + tocFileName;
		else
			tocFileName = getWorkDir() + tocFileName;
	}

	return count;
}

void validateFileNames(StringList &luaFiles, const char *szAddonDir = NULL) {
	StringListIter iter = luaFiles.begin();

	if (!szAddonDir || !szAddonDir[0])
		szAddonDir = getWorkDir();

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

	print("Addon dir: %s\n", addonDir.c_str());
	print("Work dir: %s\n", getWorkDir());

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
		LuaObfuscator obfuscator(luaFiles, excludeGlobalFunctions, addonDir);

		print("create bak file:        %d\n", settings.bCreateBakFile);
		print("create one file:        %d\n", settings.bCreateOneFile);
		print("obfuscate const int:    %d\n", settings.ObfuscateConstInt);
		print("obfuscate const float:  %d\n", settings.ObfuscateConstFloat);
		print("obfuscate const string: %d\n", settings.ObfuscateConstString);
		print("obfuscate local vars:   %d\n", settings.ObfuscateLocalVasAndParam);
		print("obfuscate global fucntions: %d\n", settings.ObfuscateGlobalFunctionName);

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