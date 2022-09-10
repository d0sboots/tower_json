# tower_json
A utility website for decoding tower blueprint strings

See it in action at https://d0sboots.github.io/tower_json/!

This is a small utility for working with Tower Blueprint strings, from the game ***The Perfect Tower II***. The format of these is a JSON object,
serialized and DEFLATE'd, and the based64-encoded. However, the DEFLATE part is a ***raw*** deflate stream, not a gzip or zlib file, which makes
it annoying to handle with regular utilities.

Using the site requires WASM, which is supported by all recent browsers.

## Building

See the Makefile for building instructions. Building requires `emscripten`, `python3` and `closure-compiler`. Some of the steps may be a little hacky,
so the build process may not be fully portable (especially to Windows).
