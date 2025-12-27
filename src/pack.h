#ifndef PACK_H
#define PACK_H

#include "common.h"
#include <kvpp/KV1.h>

extern void parsePack(const kvpp::KV1<> kv, int child, std::string pathToKVDir, std::string outputPath);
extern bool pack(packList_s packList, std::string outputPath);

#endif // PACK_H