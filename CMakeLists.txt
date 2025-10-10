cmake_minimum_required(VERSION 3.31)

project(solarpak
	VERSION "0.1.0"
	DESCRIPTION "A simple CLI program to pack a vpk from a kv file"
)
set(CMAKE_CXX_STANDARD 20)


set(SOURCEPP_LIBS_START_ENABLED OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_KVPP 			 ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VPKPP 		  	 ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_FSPP 			 ON CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VTFPP 			 OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_STEAMPP 		 OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_BSPPP 		     OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_MDLPP 			 OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_VCRYPTPP 		 OFF CACHE INTERNAL "" FORCE)
set(SOURCEPP_USE_DMXPP 		 	 OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/ext/sourcepp")

add_subdirectory("${CMAKE_SOURCE_DIR}/ext/argparse")

execute_process(
		COMMAND git rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_COMMIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file("${CMAKE_SOURCE_DIR}/src/config.h.in" "${CMAKE_SOURCE_DIR}/src/config.h")

add_executable(solarpak
	"${CMAKE_SOURCE_DIR}/src/solarpak.cpp"
	"${CMAKE_SOURCE_DIR}/src/config.h"
)

target_link_libraries(solarpak PRIVATE
	sourcepp::kvpp
	sourcepp::vpkpp
	sourcepp::fspp
	argparse
)