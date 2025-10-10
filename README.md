# SolarPak
A CLI tool to create a VPK from a kv file.

This was made to easily and quickly package a VPK for easy distribution for a small Alien Swarm: Reactive Drop workshop campaign I am working on. I decided to relase it in-case others find it useful.

## Credits
- [sourcepp](https://github.com/craftablescience/sourcepp)
- [argparse](https://github.com/p-ranav/argparse)

## Usage
See [example.kv](example.kv1) for an example kv file.

Example usage command:
```shell
solarpak ./example.kv
```
A list of Keys accepted:
|Key | input|
|----|------|
|$name |      [string]|
|$singlevpk | [0/1]|
|$version |   [1/2]|
|$pack | 	  [path]|

