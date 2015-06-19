########################################################################
# yase - Makefile                                                      #
########################################################################

# Include configuration
include config.mk

# Version information
VERSION_MAJOR=0
VERSION_MINOR=1
VERSION_PATCH=0

# List of source files to build
SOURCES=           \
	src/main.c     \
	src/popcnt.c   \
	src/presieve.c \
	src/seed.c     \
	src/sieve.c    \
	src/wheel.c

# List of objects
OBJECTS=$(patsubst src/%.c,build/%.o,$(SOURCES))

# Lists of common headers
#  - build/include/params.h:  autogenerated build parameter header
#  - build/include/version.h: autogenerated version information header
#  - src/yase.h:              header for entire project
HEADERS_SOURCES=include/yase.h
HEADERS_AUTOGEN=build/include/params.h build/include/version.h
HEADERS=$(HEADERS_SOURCES) $(HEADERS_AUTOGEN)

# Distribution name and files to include in source distribution
DIST_NAME=yase-$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
DIST_FILES=            \
	README.md          \
	CHANGELOG.md       \
	COPYING.md         \
	Makefile           \
	config.mk.default  \
	$(HEADERS_SOURCES) \
	$(SOURCES)

# `all' target = yase
.PHONY: all
all: yase

# yase: program binary
yase: $(OBJECTS) config.mk
	$(CC) $(CFLAGS) $(OBJECTS) -o yase

# build: directory for intermediate build files
build:
	mkdir build

# build/include: directory for autogenerated header files
build/include: | build
	mkdir build/include

# build/include/params.h: autogenerated parameters header
build/include/params.h: config.mk | build/include
	@echo "Generating build/include/params.h . . ."
	@echo "#ifndef PARAMS_H"                       >  build/include/params.h
	@echo "#define PARAMS_H"                       >> build/include/params.h
	@echo "#define SEGMENT_BYTES" $(SEGMENT_BYTES) >> build/include/params.h
	@echo "#define PRESIEVE_PRIMES" $(PRESIEVE_PRIMES) \
		>> build/include/params.h
	@echo "#endif"                                 >> build/include/params.h

# bulid/include/version.h: autogenerated version header
build/include/version.h: | build/include
	@echo "Generating build/include/version.h . . ."
	@echo "#ifndef VERSION_H"                      >  build/include/version.h
	@echo "#define VERSION_H"                      >> build/include/version.h
	@echo "#define VERSION_MAJOR" $(VERSION_MAJOR) >> build/include/version.h
	@echo "#define VERSION_MINOR" $(VERSION_MINOR) >> build/include/version.h
	@echo "#define VERSION_PATCH" $(VERSION_PATCH) >> build/include/version.h
	@echo "#endif"                                 >> build/include/version.h

# Pattern rule for all C sources
build/%.o: src/%.c $(HEADERS) config.mk | build
	$(CC) $(CFLAGS) -c -Iinclude -Ibuild/include $< -o $@

# `clean' target: remove all objects and final binary
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(HEADERS_AUTOGEN) yase
	rm -rf build

# Souce distribution archive
$(DIST_NAME).tar.gz: $(DIST_FILES)
	mkdir $(DIST_NAME)
	cp --parents $(DIST_FILES) $(DIST_NAME)
	tar -czf $(DIST_NAME).tar.gz $(DIST_NAME)
	rm -r $(DIST_NAME)

# `source-dist' target: bundle up for redistribution
.PHONY: source-dist
source-dist: $(DIST_NAME).tar.gz
