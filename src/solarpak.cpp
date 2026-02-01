#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "config.h"
#include "common.h"
#include "pack.h"

#include <kvpp/KV1.h>
#include <sourcepp/FS.h>

#include <argparse/argparse.hpp>

using namespace sourcepp;
using namespace vpkpp;
using namespace kvpp;


int main(int argc, char* argv[])
{
	std::filesystem::path cwd;
	std::string outputPath;
	int i;

	cwd = std::filesystem::current_path();

	argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);
	program.add_argument("KVPATH")
		.help("Path to kvfile");

	program.add_argument("-o", "--output")
		.metavar("PATH")
		.help("Specify directory to output VPK to");

	program.add_argument("-v", "--verbose")
		.flag()
		.store_into(verboseMode)
		.help("Verbose Mode");

	program.add_argument("-p","--print-time")
		.flag()
		.store_into(printTime)
		.help("Prints a time taken measurement after packing");

	program.add_argument("-s", "--strict")
		.flag()
		.store_into(strictMode)
		.help("Makes the program exit on error");

	program.add_description(PROGRAM_DESC);

	program.add_epilog("KV Commands:\n" KVCOMMANDHELP "\n" SUPPORTEDTYPES "\n" SUPPORTEDCOMPRESSIONTYPES);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err)
	{
		Common::Error(err.what(), "\n");
		std::cerr << program;
		exit(1);
	}

	std::string kvFile{program.get("KVPATH")};

	// Get the absolute path to the kv file, normalize it and add a trailing slash
	std::filesystem::path pathToKV = std::filesystem::absolute(kvFile);
	std::string pathToKVDir = pathToKV.parent_path().string();
	sourcepp::string::normalizeSlashes(pathToKVDir);
	pathToKVDir.append("/");

	if (!std::filesystem::exists(kvFile)) {
		Common::Error("Error: '",kvFile.c_str(),"' Does not exist!\n");
		return 1;
	}
		

	KV1 kv{ fs::readFileText(kvFile)};
	if (!kv.hasChild("solarpak")) {
		Common::Error("Error: 'solarpak' root key not found!\n");
		return 1;
	}


	outputPath = std::filesystem::current_path().string();
	outputPath.append("/");

	// Get and Set output path
	if (program.is_used("--output"))
	{
		outputPath = program.get("--output");
		if (!outputPath.ends_with("/")) {
			outputPath.append("/");
		}
		if (!std::filesystem::exists(outputPath)) {
			std::filesystem::create_directory(outputPath);
		}
		if (!std::filesystem::is_directory(outputPath)) {
			printf("Error: output must be a directory!\n");
			exit(1);
		}
	}
	
	// Get the $pak keys from the kvfile
	int count = kv["solarpak"].getChildCount("$pak");
	for (i = 0; i <= count - 1; i++)
	{
		parsePack(kv, i, pathToKVDir, outputPath);
	}

	return 0;
}