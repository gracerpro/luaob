/**

*/
#include "global.h"
#include "obfuscator.h"

void printHelp();
int parseArguments(const int argc, char *argv[], std::string &globalExcludeFunctionFileName);

using namespace Obfuscator;

void printHelp() {
	printf("Lua obfuscator, Version 1.0\n");
	printf("[-t FILE.toc] ");
	printf("[--gef global_exclude_function_file_name] ");
	printf("[-a[ppend] FILE.lua]\n");
}

int parseArguments(const int argc, char *argv[], std::string &tocFileName,
	std::string &globalExcludeFunctionFileName,
	StringList &luaFiles)
{
	int count = 0;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "-t")) {
			if (++i < argc) {
				tocFileName = argv[i];
			}
		}
		else if (!strcmp(arg, "-gef")) {
			if (++i < argc) {
				globalExcludeFunctionFileName = argv[i];
			}
		}
		else if (!strcmp(arg, "-a")) {
			if (++i < argc) {
				luaFiles.push_back(argv[i]);
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

	if (argc <= 1) {
		printHelp();
		return 0;
	}

	parseArguments(argc, argv, tocFileName, globalExcludeFunctionFileName, luaFiles);

	ReadAddonGlobalExcludeFunctions(globalExcludeFunctionFileName.data(), excludeGlobalFunctions);

	ReadAddonTocFile(tocFileName.data(), luaFiles);

	if (luaFiles.empty()) {
		printf("No a files for an obfuscating\n");
		return -1;
	}

	stObfuscator options;

	// TODO: this is debug initialization
	options.bCreateBakFile = false;
	options.bCreateOneFile = false;
	options.linesBetweenFiles = 3;
	options.ObfuscateAddComment = false;
	options.ObfuscateFunctionGlobal = false;
	options.ObfuscateInteger = false;
	options.ObfuscateLocalVasAndParam = false;
	obfuscate(&options, luaFiles, excludeGlobalFunctions);

	return 0;
}