#include <iostream>
#include <stdlib.h>
#include <string>
#include <filesystem>
#include <pthread.h>
#include <chrono>

#include "pack.h"
#include "common.h"

#include <vpkpp/vpkpp.h>

void parsePack(const kvpp::KV1<> kv, int child, std::string pathToKVDir, std::string outputPath)
{

	std::string_view name = kv["solarpak"][child].getValue().data();
	packList_s packList;

	std::cout << "Packing: " << name << std::endl;
	packList.name = name;

	// Parse the $pak kv for its keys
	for (const auto& pakKey : kv["solarpak"][child].getChildren())
	{
		std::string token{pakKey.getKey()};
		std::string value{pakKey.getValue()};

		if (!strcmp(token.c_str(), SINGLEVPKKEY))
		{
			packList.singlevpk = pakKey.getValue<bool>();
			continue;
		}
		else if (!strcmp(token.c_str(), VERSIONKEY))
		{
		// Make sure $version is 1 or 2
		if (pakKey.getValue<int>() > 2 || pakKey.getValue<int>() < 1) {
			printf("Error: $version can't be 0 or greater than 2\n");
			return;
		}

			packList.version = pakKey.getValue<int>();
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
				return;
			}
			continue;
		}
		else if (!strcmp(token.c_str(), COMPRESSIONLEVELKEY))
		{
			packList.cLevel = pakKey.getValue<int>();
		}
		else if (!strcmp(token.c_str(), COMPRESSIONTYPEKEY))
		{
			if (!strcmp(value.c_str(), "none"))
				packList.cType = vpkpp::EntryCompressionType::NO_COMPRESS;
			else if (!strcmp(value.c_str(), "deflate"))
				packList.cType = vpkpp::EntryCompressionType::DEFLATE;
			else if (!strcmp(value.c_str(), "bzip2"))
				packList.cType = vpkpp::EntryCompressionType::BZIP2;
			else if (!strcmp(value.c_str(), "lzma"))
				packList.cType = vpkpp::EntryCompressionType::LZMA;
			else if (!strcmp(value.c_str(), "zstd"))
				packList.cType = vpkpp::EntryCompressionType::ZSTD;
			else if (!strcmp(value.c_str(), "xz"))
				packList.cType = vpkpp::EntryCompressionType::XZ;
			else {
				printf("Error: %s is invalid! Must be a supported type.\n", value.c_str());
				printf(SUPPORTEDCOMPRESSIONTYPES);
				return;
			}
		}
		else if (!strcmp(token.c_str(), PACKKEY))
		{
			if (!std::filesystem::exists(pathToKVDir + value)) {
				printf("Error: '%s' does not exist!\n", value.c_str());
				return;
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
				return;
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
				return;
			}
		}
		else
		{
			printf("Error: %s is not a supported command!\nAccepted commands are:\n", token.c_str());
			printf(KVCOMMANDHELP);
			return;
		}
	}

	///////////////

	// Make sure there is a name specified
	if (packList.name == "")
	{
		printf("Error: No name specified!\nPlease add the name to $pak like so: '$pak \"name\"'\n");
		printf(KVCOMMANDHELP);
		return;
	}

	// Warn the user if $singlevpk isn't specified when making a mutli pack vpk
	if (!packList.name.ends_with("_dir") && packList.singlevpk == false && packList.type == vpk)
		printf(
			"Warning(%s): multiple vpks being made without _dir suffix!\n\
			This may cause issues if the vpk is more than 4gb.\n", name.data()
		);

	

	// Pack the packList
	if (!pack(packList, outputPath))
	{
		printf("Error: Packing failed!");
		return;
	}

	return;
}

/*
 * Creates a package from a packList struct
*/
bool pack(packList_s packList, std::string outputPath)
{
	std::string packName;
	std::unique_ptr<vpkpp::PackFile> packFile;
	vpkpp::EntryOptions options;
	std::chrono::steady_clock::time_point begin, end;
	int i;


	switch (packList.type)
	{
	case packType_e::vpk: {
		packName = outputPath + packList.name + vpkpp::VPK_EXTENSION.data();
		packFile = vpkpp::VPK::create(packName, packList.version);
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
		packFile = vpkpp::ZIP::create(packName);
		options.zip_compressionStrength = packList.cLevel;
		options.zip_compressionType = packList.cType;
		break;
	}
	case packType_e::pak: {
		packName = outputPath + packList.name + vpkpp::PAK_EXTENSION.data();
		packFile = vpkpp::PAK::create(packName, vpkpp::PAK::Type::PAK);
		break;
	}
	}

	// Error out if compression level is greater than 9 when not using ZSTD as it creates a broken zip
	if (packList.type == packType_e::zip)
		if (packList.cType != vpkpp::EntryCompressionType::ZSTD)
			if (packList.cLevel > 9) {
				printf("Error(%s): Compression level is greater than 9. Please use ZSTD\n", packList.name.data());
				return false;
			}


	// printf("Creating: %s\n", packList.name.c_str());
	if (verboseMode)
		std::cout << "Files:" << std::endl;

	if (printTime)
		begin = std::chrono::steady_clock::now();

	// Iterate through packFiles and packDirs and put them into the type specified
	for (i = 0; i < packList.packFiles.size(); i++)
	{
		if (std::filesystem::is_directory(packList.packFilesPath[i])) {
			packFile->addDirectory(packList.packFiles[i], packList.packFilesPath[i], options);
			if (verboseMode)
				std::cout << "- " << packList.packFiles[i] << std::endl;
		}
		else if (std::filesystem::is_regular_file(packList.packFilesPath[i]))
		{
			packFile->addEntry(packList.packFiles[i], packList.packFilesPath[i], options);
			if (verboseMode)
				std::cout << "- " << packList.packFiles[i] << std::endl;
		}
		else 
			return false;
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

	if (printTime)
	{
		end = std::chrono::steady_clock::now();
		
		auto hours = std::chrono::duration_cast<std::chrono::hours>(end - begin).count();
		auto minutes = std::chrono::duration_cast<std::chrono::minutes>(end - begin).count();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
		
		std::cout << "Pack time: " << hours << ":" << minutes << ":" << seconds << "." << milliseconds << "s" << std::endl;
	}

	return true;
}