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
	
	// Make sure there is a name specified
	if (name == "") {
		Common::Error("Error: No name specified!\nPlease add the name to $pak like so: '$pak \"name\"'");
		goto error;
	}

	Common::Print("Packing: ", name);
	packList.name = name;

	// Parse the $pak kv for its keys
	for (const auto& pakKey : kv["solarpak"][child].getChildren())
	{
		std::string token{pakKey.getKey()};
		std::string value{pakKey.getValue()};

		if (token == SINGLEVPKKEY)
		{
			packList.singlevpk = pakKey.getValue<bool>();
			continue;
		}
		else if (token == VERSIONKEY)
		{
		// Make sure $version is 1 or 2
		if (pakKey.getValue<int>() > 2 || pakKey.getValue<int>() < 1) {
			Common::Error("Error(",name.data(),"): '$version' can't be 0 or greater than 2");
			goto error;
		}

			packList.version = pakKey.getValue<int>();
			continue;
		}
		else if (token == FORMATKEY)
		{
			if (value == "vpk")
				packList.type = packType_e::vpk;
			else if (value == "pak")
				packList.type = packType_e::pak;
			else if (value == "zip") {
				packList.type = packType_e::zip;
				packList.zipAlias = zipAlias_e::none;
			}
			else if (value == "bmz") {
				packList.type = packType_e::zip;
				packList.zipAlias = zipAlias_e::bmz;
			}
			else if (value == "pk2") {
				packList.type = packType_e::zip;
				packList.zipAlias = zipAlias_e::pk2;
			}
			else if (value == "pk3") {
				packList.type = packType_e::zip;
				packList.zipAlias = zipAlias_e::pk3;
			}
			else if (value == "pk4") {
				packList.type = packType_e::zip;
				packList.zipAlias = zipAlias_e::pk4;
			}
			else {
				Common::Error("Error(",name.data(),"): '",value.data(),"' is invalid! Must be a supported type");
				goto error;
			}
			continue;
		}
		else if (token == COMPRESSIONLEVELKEY)
		{
			packList.cLevel = pakKey.getValue<int>();
		}
		else if (token == COMPRESSIONTYPEKEY)
		{
			if (value == "none")
				packList.cType = vpkpp::EntryCompressionType::NO_COMPRESS;
			else if (value == "deflate")
				packList.cType = vpkpp::EntryCompressionType::DEFLATE;
			else if (value == "bzip2")
				packList.cType = vpkpp::EntryCompressionType::BZIP2;
			else if (value == "lzma")
				packList.cType = vpkpp::EntryCompressionType::LZMA;
			else if (value == "zstd")
				packList.cType = vpkpp::EntryCompressionType::ZSTD;
			else if (value == "xz")
				packList.cType = vpkpp::EntryCompressionType::XZ;
			else {
				Common::Error("Error(",name.data(),"): '",value.data(),"' is invalid! Must be a supported type");
				goto error;
			}
		}
		else if (token == PACKKEY)
		{
			if (!std::filesystem::exists(pathToKVDir + value)) {
				Common::Error("Error(",name.data(),"): '",value.data(),"' does not exist!");
				goto error;
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
				Common::Error("Error(",name.data(),"): '",value.data(),"' is not a file or directory!");
				goto error;
			}
		}
		else
		{
			Common::Error("Error(",name.data(),"): '",value.data(),"' is not a supported command!");
			goto error;
		}
	}

	///////////////

	// Warn the user if $singlevpk isn't specified when making a mutli pack vpk
	if (!packList.name.ends_with("_dir") && packList.singlevpk == false && packList.type == vpk) {
		Common::Print(
			"Warning(",name.data(),"): multiple vpks being made without _dir suffix!\n \
			This may cause issues if the vpk is more than 4gb."
		);
	}

	

	// Pack the packList
	if (!pack(packList, outputPath))
		goto error;

	
	return;
error:
	Common::Error("Error(",name.data(),"): Packing failed!");
	if(strictMode)
		exit(1);
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


	// What are we packing
	switch (packList.type)
	{
	case packType_e::vpk:
		packName = outputPath + packList.name + vpkpp::VPK_EXTENSION.data();
		packFile = vpkpp::VPK::create(packName, packList.version);
		options.vpk_saveToDirectory = packList.singlevpk;
		break;
	case packType_e::zip:
		// Change zip extension based on defined alias
		switch (packList.zipAlias) {
		case zipAlias_e::none:
			packName = outputPath + packList.name + vpkpp::ZIP_EXTENSION.data();
			break;
		case zipAlias_e::bmz:
			packName = outputPath + packList.name + vpkpp::BMZ_EXTENSION.data();
			break;
		case zipAlias_e::pk3:
			packName = outputPath + packList.name + vpkpp::PK3_EXTENSION.data();
			break;
		case zipAlias_e::pk4:
			packName = outputPath + packList.name + vpkpp::PK4_EXTENSION.data();
			break;
		case zipAlias_e::pk2:
			packName = outputPath + packList.name + ".pk2";
			break;
		}
		packFile = vpkpp::ZIP::create(packName);
		options.zip_compressionStrength = packList.cLevel;
		options.zip_compressionType = packList.cType;
		break;
	case packType_e::pak:
		packName = outputPath + packList.name + vpkpp::PAK_EXTENSION.data();
		packFile = vpkpp::PAK::create(packName, vpkpp::PAK::Type::PAK);
		break;
	}

	// Error out if compression level is greater than 9 when not using ZSTD as it creates a broken zip
	if (packList.type == packType_e::zip && 
		packList.cType != vpkpp::EntryCompressionType::ZSTD && 
		packList.cLevel > 9) {
		Common::Error("Error(",packList.name.data(),"): Compression level is greater than 9. Please use ZSTD or lower the compression level");
		return false;
	}

	if (verboseMode)
		Common::Print("Files:");

	if (printTime)
		begin = std::chrono::steady_clock::now();

	// Iterate through packFiles and packDirs and put them into the type specified
	for (i = 0; i < packList.packFiles.size(); i++)
	{
		if (verboseMode)
			Common::Print("- ", packList.packFiles[i]);

		if (std::filesystem::is_directory(packList.packFilesPath[i]))
			packFile->addDirectory(packList.packFiles[i], packList.packFilesPath[i], options);	
		else if (std::filesystem::is_regular_file(packList.packFilesPath[i]))
			packFile->addEntry(packList.packFiles[i], packList.packFilesPath[i], options);
		else 
			return false;
	}

	// If we are packing a zip set the type and strength
	if (packList.type == packType_e::zip) {
		packFile->bake("", {
			.zip_compressionTypeOverride = packList.cType,
			.zip_compressionStrength = packList.cLevel
			}, nullptr);
	}
	else
		packFile->bake("", {}, nullptr);

	// Print the time taken to pack
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