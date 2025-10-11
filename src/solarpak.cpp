#include <cstdlib>
#include <iostream>
#include <istream>
#include <fstream>
#include <string>
#include <thread>
#include <algorithm>
#include <string_view>
#include <vector>

#include "config.h"

#include <vpkpp/vpkpp.h>
#include <kvpp/KV1.h>
#include <fspp/fspp.h>

#include <argparse/argparse.hpp>

using namespace sourcepp;
using namespace vpkpp;
using namespace kvpp;
using namespace fspp;

enum packType {
	vpk = 0,
	zip,
	pak,
};

struct packList_s {
	std::string name;
	std::vector<std::string> packFiles;
	std::vector<std::string> packFilesPath;
	std::vector<std::string> packDirs;
	std::vector<std::string> packDirsPath;
	packType type = vpk;
	bool singlevpk = false;
	int version = 1;
};

#define KVCOMMANDHELP \
		"Key:			Input:\n" \
		"$name			[string]\n" \
		"$singlevpk		[0/1]\n" \
		"$version		[1/2]\n" \
		"$pack			[path]\n" \
		"$type			[format]\n"

#define SUPPORTEDTYPES \
		"Supported types:\n" \
		"vpk\n" \
		"zip\n" \
		"pak\n"

static bool pack(packList_s packList, std::string outputPath)
{
	std::string packName;
	std::unique_ptr<PackFile> packFile;
	EntryOptions options;
	int i;


	switch (packList.type)
	{
	case vpk: {
		packName = outputPath + packList.name + vpkpp::VPK_EXTENSION.data();
		packFile = VPK::create(packName, packList.version);
		options.vpk_saveToDirectory = packList.singlevpk;
		break;
	}
	case zip: {
		packName = outputPath + packList.name + vpkpp::ZIP_EXTENSION.data();
		packFile = ZIP::create(packName);
		break;
	}
	case pak: {
		packName = outputPath + packList.name + vpkpp::PAK_EXTENSION.data();
		packFile = PAK::create(packName, PAK::Type::PAK);
		break;
	}
	}


	printf("Creating: %s\n", packName.c_str());
	printf("Packing Files:\n");

	// Iterate through packFiles and packDirs and put them into the type specified
	i = 0;
	while (i < packList.packFiles.size())
	{
		if (!std::filesystem::exists(packList.packFilesPath[i])) {
			printf("Error: '%s' Does not exist!\n", packList.packFiles[i].c_str());
			return false;
		}
		if (!std::filesystem::is_regular_file(packList.packFilesPath[i])) {
			printf("Error: '%s' Is not a file!\n", packList.packFiles[i].c_str());
			return false;
		}
		packFile->addEntry(packList.packFiles[i], packList.packFilesPath[i], options);
		std::cout << "- " << packList.packFiles[i] << std::endl;
		i++;
	}

	printf("Packing Directories:\n");

	i = 0;
	while (i < packList.packDirs.size())
	{
		if (!std::filesystem::exists(packList.packDirsPath[i])) {
			printf("Error: '%s' Does not exist!\n", packList.packDirs[i].c_str());
			return false;
		}
		if (!std::filesystem::is_directory(packList.packDirsPath[i])) {
			printf("Error: '%s' Is not a file!\n", packList.packDirs[i].c_str());
			return false;
		}
		packFile->addDirectory(packList.packDirs[i], packList.packDirsPath[i], options);
		std::cout << "- " << packList.packDirs[i] << std::endl;
		i++;
	}

	packFile->bake("", {}, nullptr);

	return true;
}

int main(int argc, char* argv[])
{
	packList_s packList;
	std::filesystem::path cwd;
	std::string outputPath;

	cwd = std::filesystem::current_path();

	argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);
	program.add_argument("KVPATH")
		.help("Path to kvfile");

	program.add_argument("-o", "--output")
		.metavar("PATH")
		.help("Specify directory to output VPK to");

	program.add_description(PROGRAM_DESC);

	program.add_epilog("KV Commands:\n" KVCOMMANDHELP "\n" SUPPORTEDTYPES);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	std::string kvFile{program.get("KVPATH")};

	// Get the absolute path to the kv file, normalize it and add a trailing slash
	std::filesystem::path pathToKV = std::filesystem::absolute(kvFile);
	std::string pathToKVDir = pathToKV.parent_path().string();
	sourcepp::string::normalizeSlashes(pathToKVDir);
	pathToKVDir.append("/");

	if (!std::filesystem::exists(kvFile)) {
		printf("Error: '%s' Does not exist!\n", kvFile.c_str());
		return 1;
	}
		

	KV1 kv{ fs::readFileText(kvFile)};
	if (!kv.hasChild("solarpak")) {
		printf("Error: `solarpak` root key not found!\n");
		return 1;
	}


	const auto& aPak = kv["solarpak"];
	
	// Get keys from kvfile
	for (const auto& vpkKey : aPak.getChildren())
	{
		std::string token{vpkKey.getKey()};
		std::string value{vpkKey.getValue()};

		if (!strcmp(token.c_str(), "$name")) {
			packList.name = value;
			continue;
		}
		else if (!strcmp(token.c_str(), "$singlevpk"))
		{
			packList.singlevpk = vpkKey.getValue<bool>();
			continue;
		}
		else if (!strcmp(token.c_str(), "$version"))
		{
			// Make sure $version is 1 or 2
			if (vpkKey.getValue<int>() > 2 || vpkKey.getValue<int>() < 1) {
				printf("Error: $version can't be greater than 2 and less than 1\n");
				exit(1);
			}

			packList.version = vpkKey.getValue<int>();
			continue;
		}
		else if (!strcmp(token.c_str(), "$type"))
		{
			if (!strcmp(value.c_str(), "zip"))
				packList.type = zip;
			else if (!strcmp(value.c_str(), "pak"))
				packList.type = pak;
			else {
				printf("Error: %s is invalid! Must be a supported type.\n", value.c_str());
				printf(SUPPORTEDTYPES);
				exit(2);
			}
			continue;
		}
		else if (!strcmp(token.c_str(), "$pack"))
		{
			if (std::filesystem::is_regular_file(pathToKVDir + value))
			{
				auto pos = packList.packFilesPath.begin();
				auto pos2 = packList.packFiles.begin();
				packList.packFilesPath.insert(std::next(pos, 0),pathToKVDir + value);
				packList.packFiles.insert(std::next(pos2, 0),value);
				continue;
			}
			else if (std::filesystem::is_directory(pathToKVDir + value))
			{
				auto pos = packList.packDirsPath.begin();
				auto pos2 = packList.packDirs.begin();
				packList.packDirsPath.insert(std::next(pos, 0), pathToKVDir + value);
				packList.packDirs.insert(std::next(pos2, 0), value);
				continue;
			}
			else
			{
				printf("Error: %s is not a file or directory!\n", value.c_str());
				exit(1);
			}
		}
		else
		{
			printf("Error: %s is not a supported command!\nAccepted commands are:\n", token.c_str());
			printf(KVCOMMANDHELP);
			exit(2);
		}
	}

	// Make sure there is a name specified
	if (packList.name == "")
	{
		printf("Error: No name specified!\nPlease add $name \"name\" to the kv file\n");
		printf(KVCOMMANDHELP);
		return 1;
	}

	if (!packList.name.ends_with("_dir") && packList.singlevpk == false && packList.type == vpk)
		printf(
			"Warning: multiple vpks being made without _dir suffix!\n\
			This may cause issues if the vpk is more than 4gb.\n"
		);

	outputPath = "";

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

	if (!pack(packList, outputPath))
	{
		printf("Error: Packing failed!");
		return 1;
	}

	return 0;
}