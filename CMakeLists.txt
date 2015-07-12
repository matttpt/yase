cmake_minimum_required(VERSION 2.8)
project(yase)

# Project metadata
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)

# Load configuration
if(EXISTS ${CMAKE_BINARY_DIR}/config.cmake)
	include(${CMAKE_BINARY_DIR}/config.cmake)
else()
	message(FATAL_ERROR "-- You need to create a config.cmake file! "
	        "Copy config.cmake.default in the source distribution to "
	        "start.")
endif()

# Use release build type if none specified
if(NOT CMAKE_BUILD_TYPE)
	message("-- No build type specified, assuming release.")
	set(CMAKE_BUILD_TYPE Release)
endif()

# C99 and compiler warnings
add_definitions("-std=c99 -Wall")

# Generate parameters and version headers
configure_file(include/params.h.in include/params.h)
configure_file(include/version.h.in include/version.h)

# If on Unix, we need to link with the math library
if(UNIX)
	set(MATH_LIBRARY -lm)
else()
	set(MATH_LIBRARY)
endif()

# Set include path to find headers
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

# yase source list
set(SOURCES
	src/args.c
	src/expr.c
	src/main.c
	src/popcnt.c
	src/presieve.c
	src/seed.c
	src/set.c
	src/sieve.c
	src/wheel.c)

# yase executable
add_executable(yase ${SOURCES})
target_link_libraries(yase ${MATH_LIBRARY})

# Installation information - just one binary to install
install(PROGRAMS ${CMAKE_BINARY_DIR}/yase DESTINATION bin)

# CPack information
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
	"Fast implementation of the Sieve of Eratosthenes")
set(CPACK_PACKAGE_CONTACT "Matthew Ingwersen <matttpt@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_SOURCE_PACKAGE_FILE_NAME
	"yase-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES
	".*~$;.*\\\\.swp$;"                                   # Editor backups
	".o$;.a$;.so$;.tar.*$;.zip$;Makefile$;/yase$"         # Assorted files
	"/\\\\.git;"                                          # .git, .gitignore
	"/config.cmake$"                                      # User config
	"/CMakeCache.txt$;/cmake_install.cmake$;/CMakeFiles/" # CMake stuff
	"CPack*;/_CPack_Packages;/install_manifest.txt$;"     # CPack stuff
	"/include/version.h$;/include/params.h$")             # Generated headers
set(CPACK_GENERATOR "TGZ;TBZ2;ZIP")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
include(CPack)
