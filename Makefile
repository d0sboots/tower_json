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
# The env setting is a Debian hack; you may not need it
CLOSURE_COMPILER=JAVA_JARPATH=. closure-compiler
MERGE_SCRIPT=./merge_templates.py

CFLAGS=-Os -Wall -sSTRICT -sSUPPORT_LONGJMP=0 -flto -DNO_GZIP
LDFLAGS=-sENVIRONMENT=web -sINCOMING_MODULE_JS_API=[] -sFILESYSTEM=0 -sEXPORTED_FUNCTIONS=[_malloc,_do_deflate,_do_inflate] -sDYNAMIC_EXECUTION=0 -sMINIMAL_RUNTIME=2 -sTOTAL_STACK=524288 -sMALLOC=none -sINITIAL_MEMORY=2097152 -sMODULARIZE=0
CLOSURE_ARGS=--language_in=ECMASCRIPT_2020 --compilation_level=ADVANCED_OPTIMIZATIONS

.PHONY: all configure clean
all: index.html

configure: zlib/zlib.pc

zlib/libz.a: zlib/zlib.pc
	cd zlib/ && $(EMMAKE) $(MAKE) libz.a

zlib/zlib.pc:
	cd zlib/ && CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(EMCONFIGURE) ./configure --static --solo

compiled.wasm: wasm_glue.c zlib/libz.a
	$(EMCC) $(CFLAGS) $(LDFLAGS) -v -Izlib/ -o compiled.js $^

minified.js: converter.js
	$(CLOSURE_COMPILER) $(CLOSURE_ARGS) --js $^ --js_output_file $@

index.html: index.html.in minified.js compiled.wasm merge_templates.py
	$(MERGE_SCRIPT) --html index.html.in --js minified.js --wasm compiled.wasm > $@

clean:
	cd zlib/ && $(MAKE) distclean
	rm -rf compiled.js compiled.wasm minified.js index.html
