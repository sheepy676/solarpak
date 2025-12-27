#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <string>
#include <vpkpp/vpkpp.h>

#define NAMEKEY				"$name"
#define PACKKEY				"$pack"
#define SINGLEVPKKEY		"$singlevpk"
#define VERSIONKEY			"$version"
#define FORMATKEY			"$type"
#define ZIPALIASKEY			"$ziptype"
#define COMPRESSIONLEVELKEY	"$compresslevel"
#define COMPRESSIONTYPEKEY	"$compresstype"

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
		"pak\n"

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
	vpkpp::EntryCompressionType cType = vpkpp::EntryCompressionType::ZSTD;
	zipAlias_e zipAlias = zipAlias_e::none;
	int16_t cLevel = 5;
	bool singlevpk = false;
	int version = 1;
};

inline bool verboseMode = false;
inline bool printTime = false;

#endif // COMMON_H