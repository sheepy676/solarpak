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

struct packList_s {
	std::string name;
	std::vector<std::string> packFiles;
	std::vector<std::string> packFilesPath;
	std::vector<std::string> packDirs;
	std::vector<std::string> packDirsPath;
	bool singlevpk = false;
	int version = 1;
};

int main(int argc, char* argv[])
{
	packList_s packList;
	std::filesystem::path cwd;
	int i;

	cwd = std::filesystem::current_path();

	argparse::ArgumentParser program(PROGRAM_NAME, PROGRAM_VERSION);
	program.add_argument("KVPATH")
		.help("Path to kvfile");

	program.add_description(PROGRAM_DESC);

	program.add_epilog(
		"KV Commands:\n"
		"Command:		Input:\n"	
		"$name			[string]\n"
		"$singlevpk		[true/false]\n"
		"$version		[1/2]\n"
		"$packdir		[path]\n"
		"$packfile		[path]\n"	
	);

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
				printf("Error: $version can't be greater than 2 and less than 1");
				exit(1);
			}

			packList.version = vpkKey.getValue<int>();
			continue;
		}
		else if (!strcmp(token.c_str(), "$packdir"))
		{
			auto pos = packList.packDirsPath.begin();
			auto pos2 = packList.packDirs.begin();
			packList.packDirsPath.insert(std::next(pos, 0), pathToKVDir + value);
			packList.packDirs.insert(std::next(pos2, 0), value);
			continue;
		}
		else if (!strcmp(token.c_str(), "$packfile"))
		{
			auto pos = packList.packFilesPath.begin();
			auto pos2 = packList.packFiles.begin();
			packList.packFilesPath.insert(std::next(pos, 0),pathToKVDir + value);
			packList.packFiles.insert(std::next(pos2, 0),value);
			continue;
		}
	}

	// Make sure there is a name specified
	if (packList.name == "")
	{
		printf("No vpkname specified!\nPlease add $name \"name\" to the kv file");
		return 1;
	}

	if (!packList.name.ends_with("_dir") && packList.singlevpk == false)
		printf(
			"Warning: multiple vpks being made without _dir suffix!\n\
			This may cause issues if the vpk is more than 4gb.\n"
		);

	std::string packName = packList.name + ".vpk";
	printf("Creating: %s\n", packName.c_str());
	std::unique_ptr<PackFile> vpkFile = VPK::create(packName, packList.version);

	printf("Packing Files:\n");

	EntryOptions options;
	options.vpk_saveToDirectory = packList.singlevpk;

	// Iterate through packFiles and packDirs and put them into a vpk
	i = 0;
	while (i < packList.packFiles.size())
	{
		if (!std::filesystem::exists(packList.packFilesPath[i])) {
			printf("Error: '%s' Does not exist!\n", packList.packFiles[i].c_str());
			exit(1);
		}
		if (!std::filesystem::is_regular_file(packList.packFilesPath[i])) {
			printf("Error: '%s' Is not a file!\n", packList.packFiles[i].c_str());
			exit(1);
		}
		vpkFile->addEntry(packList.packFiles[i], packList.packFilesPath[i], options);
		std::cout << "- " << packList.packFiles[i] << std::endl;
		i++;
	}

	printf("Packing Directories:\n");

	i = 0;
	while (i < packList.packDirs.size())
	{
		if (!std::filesystem::exists(packList.packDirsPath[i])) {
			printf("Error: '%s' Does not exist!\n", packList.packDirs[i].c_str());
			exit(1);
		}
		if (!std::filesystem::is_directory(packList.packDirsPath[i])) {
			printf("Error: '%s' Is not a file!\n", packList.packDirs[i].c_str());
			exit(1);
		}
		vpkFile->addDirectory(packList.packDirs[i], packList.packDirsPath[i], options);
		std::cout << "- " << packList.packDirs[i] << std::endl;
		i++;
	}

	vpkFile->bake("", {}, nullptr);

	return 0;
}