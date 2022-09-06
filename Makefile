# Makefile for compiling the WASM and JS stuff
#
# Prereqs:
# 1. Install emscripten.
# 2. Download the latest version of zlib, and extract it to zlib/.
# 3. (Optional) If you are on a system like Debian that doesn't have an
#    up-to-date closure-compiler, download the latest jar and make a link to
#    it called "closure-compiler.jar", and then the JAVA_JARPATH arg will
#    cause it to be picked up.

EMCONFIGURE=emconfigure
EMMAKE=emmake
EMCC=emcc
# This is a Debian hack; you may not need it
CLOSURE_ARGS=--language_in=ECMASCRIPT_2020

CFLAGS=-Os -Wall -sSTRICT -sSUPPORT_LONGJMP=0 -flto -DNO_GZIP
LDFLAGS=-sENVIRONMENT=web -sINCOMING_MODULE_JS_API=[] -sFILESYSTEM=0 -sEXPORTED_FUNCTIONS=[_do_deflate] -sDYNAMIC_EXECUTION=0 -sTEXTDECODER=2 -sHTML5_SUPPORT_DEFERRING_USER_SENSITIVE_REQUESTS=0 -sMINIMAL_RUNTIME=2 -sTOTAL_STACK=524288 -sMALLOC=none -sINITIAL_MEMORY=2097152 -sINVOKE_RUN=0 --closure=1 -sSINGLE_FILE=0 -sMODULARIZE=0

.PHONY: all configure clean
all: compiled.js

configure: zlib/zlib.pc

zlib/libz.a: zlib/zlib.pc
	cd zlib/ && $(EMMAKE) $(MAKE) libz.a

zlib/zlib.pc:
	cd zlib/ && CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(EMCONFIGURE) ./configure --static --solo

compiled.js: converter.js wasm_glue.c converter.js zlib/libz.a
	JAVA_JARPATH=${PWD} EMCC_CLOSURE_ARGS=$(CLOSURE_ARGS) $(EMCC) $(CFLAGS) $(LDFLAGS) -v -Izlib/ -o compiled.js --pre-js $^

clean:
	cd zlib/ && $(MAKE) distclean
	rm -rf compiled.js compiled.wasm
