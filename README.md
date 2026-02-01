# SolarPak
A CLI tool to create a VPK from a kv file.

This was made to easily and quickly package a VPK for easy distribution for a small Alien Swarm: Reactive Drop workshop campaign I am working on. I decided to relase it in-case others find it useful.

---
### Supported formats
* vpk
* zip
  * BMZ
  * PK2
  * PK3
  * PK4
* pak
  * Quake
  
## Dependencies
- [sourcepp](https://github.com/craftablescience/sourcepp)
- [argparse](https://github.com/p-ranav/argparse)

## Usage
### Example KV file
This example would create a zip archive named example using ZSTD compression and packs in the `materials` and `models` directories and `image.png`.
```
solarpak
{
  $pak "example"
  {
    $type zip
    $compresslevel 22
    $compresstype ZSTD
    $pack "materials"
    $pack "models"
    $pack "image.png"
  }
}
```

### Example command usage
Basic Usage:
```bash
solarpak ./example.kv
```
Specify VPK output directory:
```bash
solarpak -o ./output ./example.kv
```

### A list of Keys accepted:
|Key | input| description|
|----|------|------------|
|$singlevpk | [0/1]|Pack 1 VPK instead|
|$version |   [1/2]|Sets the version of a VPK|
|$pack | 	  [path]|File/DIR to pack|
|$type |	  [format]|Sets the type of pack to make|
|$compresslevel |	  [0-22]|Sets the level of compression of a ZIP|
|$compresstype |	  [type]|Sets the compression method to use for a ZIP|

## CLI Help:
```
Usage: solarpak [--help] [--version] [--output PATH] [--verbose] [--print-time] [--strict] KVPATH

A CLI tool to create a Package from a kv file.

Positional arguments:
  KVPATH             Path to kvfile 

Optional arguments:
  -h, --help         shows help message and exits 
  -v, --version      prints version information and exits 
  -o, --output PATH  Specify directory to output VPK to 
  -v, --verbose      Verbose Mode 
  -p, --print-time   Prints a time taken measurement after packing 
  -s, --strict       Makes the program exit on error 

KV Commands:
  Key:                    Input:
  $singlevpk              [0/1]
  $version                [1/2]
  $pack                   [path]
  $type                   [format]
  $compresslevel          [0-22]
  $compresstype           [type]

Supported formats:
  vpk [default]
  zip
  pak
  bmz
  pk2
  pk3
  pk4

Supported Comprsesion types:
  none
  zstd [default]
  deflate
  lzma
  bzip2
  xz
```