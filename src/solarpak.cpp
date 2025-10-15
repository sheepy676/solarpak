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

#define NAMEKEY				"$name"
#define PACKKEY				"$pack"
#define SINGLEVPKKEY		"$singlevpk"
#define VERSIONKEY			"$version"
#define FORMATKEY			"$type"
#define ZIPALIASKEY			"$ziptype"
#define COMPRESSIONLEVELKEY	"$compresslevel"
#define COMPRESSIONTYPEKEY	"$compresstype"

enum packType_e {
	vpk = 0,
	zip,
	pak,
};

enum zipAlias_e {
	none = 0,
	bmz,
	pk3,
	pk4,
};

struct packList_s {
	std::string name;
	std::vector<std::string> packFiles;
	std::vector<std::string> packFilesPath;
	packType_e type = packType_e::vpk;
	EntryCompressionType cType = EntryCompressionType::ZSTD;
	zipAlias_e zipAlias = zipAlias_e::none;
	int16_t cLevel = 5;
	bool singlevpk = false;
	int version = 1;
};

#define KVCOMMANDHELP \
		"Key:			Input:\n" \
		NAMEKEY "			[string]\n" \
		SINGLEVPKKEY "		[0/1]\n" \
		VERSIONKEY "		[1/2]\n" \
		PACKKEY "			[path]\n" \
		FORMATKEY "			[format]\n" \
		ZIPALIASKEY "		[zipformat]\n" \
		COMPRESSIONLEVELKEY "		[0-22]\n" \
		COMPRESSIONTYPEKEY "		[type]\n"

#define SUPPORTEDTYPES \
		"Supported formats:\n" \
		"vpk [default]\n" \
		"zip\n" \
		"pak\n" \
		"wad3\n"

#define SUPPORTEDZIPALIAS \
		"Supported ZIP formats:\n" \
		"zip [default]\n" \
		"bmz\n" \
		"pk3\n" \
		"pk4\n"

#define SUPPORTEDCOMPRESSIONTYPES \
		"Supported Comprsesion types:\n" \
		"none\n" \
		"zstd [default]\n" \
		"deflate\n" \
		"lzma\n" \
		"bzip2\n" \
		"xz\n"

static bool pack(packList_s packList, std::string outputPath)
{
	std::string packName;
	std::unique_ptr<PackFile> packFile;
	EntryOptions options;
	int i;


	switch (packList.type)
	{
	case packType_e::vpk: {
		packName = outputPath + packList.name + vpkpp::VPK_EXTENSION.data();
		packFile = VPK::create(packName, packList.version);
		options.vpk_saveToDirectory = packList.singlevpk;
		break;
	}
	case packType_e::zip: {
		// Change zip extension based on defined alias
		switch (packList.zipAlias) {
		case zipAlias_e::none: {
			packName = outputPath + packList.name + vpkpp::ZIP_EXTENSION.data();
			break;
		}
		case zipAlias_e::bmz: {
			packName = outputPath + packList.name + vpkpp::BMZ_EXTENSION.data();
			break;
		}
		case zipAlias_e::pk3: {
			packName = outputPath + packList.name + vpkpp::PK3_EXTENSION.data();
			break;
		}
		case zipAlias_e::pk4: {
			packName = outputPath + packList.name + vpkpp::PK4_EXTENSION.data();
			break;
		}
		}
		packFile = ZIP::create(packName);
		options.zip_compressionStrength = packList.cLevel;
		options.zip_compressionType = packList.cType;
		break;
	}
	case packType_e::pak: {
		packName = outputPath + packList.name + vpkpp::PAK_EXTENSION.data();
		packFile = PAK::create(packName, PAK::Type::PAK);
		break;
	}
	}

	// Error out if compression level is greater than 9 when not using ZSTD as it creates a broken zip
	if (packList.type = packType_e::zip)
		if (packList.cType != EntryCompressionType::ZSTD)
			if (packList.cLevel > 9) {
				printf("Error: Compression level is greater than 9. Please use ZSTD\n");
				return false;
			}


	printf("Creating: %s\n", packName.c_str());
	printf("Packing:\n");

	// Iterate through packFiles and packDirs and put them into the type specified
	i = 0;
	while (i < packList.packFiles.size())
	{
		if (std::filesystem::is_directory(packList.packFilesPath[i])) {
			packFile->addDirectory(packList.packFiles[i], packList.packFilesPath[i], options);
			std::cout << "- " << packList.packFiles[i] << std::endl;
		}
		else if (std::filesystem::is_regular_file(packList.packFilesPath[i]))
		{
			packFile->addEntry(packList.packFiles[i], packList.packFilesPath[i], options);
			std::cout << "- " << packList.packFiles[i] << std::endl;
		}
		else 
			return false;

		i++;
	}

	// If we are packing a zip set the type and strength
	if (packList.type == packType_e::zip)
	{
		packFile->bake("", {
			.zip_compressionTypeOverride = packList.cType,
			.zip_compressionStrength = packList.cLevel
			}, nullptr);

	}
	else
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

	program.add_epilog("KV Commands:\n" KVCOMMANDHELP "\n" SUPPORTEDTYPES "\n" SUPPORTEDZIPALIAS "\n" SUPPORTEDCOMPRESSIONTYPES);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err)
	{
		std::cerr << err.what() << std::endl;
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

		if (!strcmp(token.c_str(), NAMEKEY)) {
			packList.name = value;
			continue;
		}
		else if (!strcmp(token.c_str(), SINGLEVPKKEY))
		{
			packList.singlevpk = vpkKey.getValue<bool>();
			continue;
		}
		else if (!strcmp(token.c_str(), VERSIONKEY))
		{
			// Make sure $version is 1 or 2
			if (vpkKey.getValue<int>() > 2 || vpkKey.getValue<int>() < 1) {
				printf("Error: $version can't be greater than 2 and less than 1\n");
				exit(1);
			}

			packList.version = vpkKey.getValue<int>();
			continue;
		}
		else if (!strcmp(token.c_str(), FORMATKEY))
		{
			if (!strcmp(value.c_str(), "vpk"))
				packList.type = packType_e::vpk;
			else if (!strcmp(value.c_str(), "zip"))
				packList.type = packType_e::zip;
			else if (!strcmp(value.c_str(), "pak"))
				packList.type = packType_e::pak;
			else {
				printf("Error: %s is invalid! Must be a supported type.\n", value.c_str());
				printf(SUPPORTEDTYPES);
				exit(3);
			}
			continue;
		}
		else if (!strcmp(token.c_str(), COMPRESSIONLEVELKEY))
		{
			packList.cLevel = vpkKey.getValue<int>();
		}
		else if (!strcmp(token.c_str(), COMPRESSIONTYPEKEY))
		{
			if (!strcmp(value.c_str(), "none"))
				packList.cType = EntryCompressionType::NO_COMPRESS;
			else if (!strcmp(value.c_str(), "deflate"))
				packList.cType = EntryCompressionType::DEFLATE;
			else if (!strcmp(value.c_str(), "bzip2"))
				packList.cType = EntryCompressionType::BZIP2;
			else if (!strcmp(value.c_str(), "lzma"))
				packList.cType = EntryCompressionType::LZMA;
			else if (!strcmp(value.c_str(), "zstd"))
				packList.cType = EntryCompressionType::ZSTD;
			else if (!strcmp(value.c_str(), "xz"))
				packList.cType = EntryCompressionType::XZ;
			else {
				printf("Error: %s is invalid! Must be a supported type.\n", value.c_str());
				printf(SUPPORTEDCOMPRESSIONTYPES);
				exit(3);
			}
		}
		else if (!strcmp(token.c_str(), PACKKEY))
		{
			if (!std::filesystem::exists(pathToKVDir + value)) {
				printf("Error: '%s' does not exist!\n", value.c_str());
				exit(1);
			}


			if (std::filesystem::is_regular_file(pathToKVDir + value) || std::filesystem::is_directory(pathToKVDir + value))
			{
				auto pos = packList.packFilesPath.begin();
				auto pos2 = packList.packFiles.begin();
				packList.packFilesPath.insert(std::next(pos, 0),pathToKVDir + value);
				packList.packFiles.insert(std::next(pos2, 0),value);
				continue;
			}
			else
			{
				printf("Error: %s is not a file or directory!\n", value.c_str());
				exit(1);
			}
		}
		else if (!strcmp(token.c_str(), ZIPALIASKEY))
		{
			if (!strcmp(value.c_str(), "zip"))
				packList.zipAlias = zipAlias_e::none;
			else if (!strcmp(value.c_str(), "bmz"))
				packList.zipAlias = zipAlias_e::bmz;
			else if (!strcmp(value.c_str(), "pk3"))
				packList.zipAlias = zipAlias_e::pk3;
			else if (!strcmp(value.c_str(), "pk4"))
				packList.zipAlias = zipAlias_e::pk4;
			else {
				printf("Error: %s is invalid! Must be a supported type.\n", value.c_str());
				printf(SUPPORTEDZIPALIAS);
				exit(3);
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