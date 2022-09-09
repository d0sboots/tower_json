"use strict";

// Define some stuff for minification purposes
const fromCharCode = String.fromCharCode;
const U8Array = Uint8Array;

const base64_box = document.getElementById("base64");
const json_box = document.getElementById("json");

const wasm = U8Array.from("'''REPLACE_WASM_HERE'''", x => x.charCodeAt(0) ^ 0x6f);
var _malloc, _do_deflate, _do_inflate, buffer, HEAPU8, HEAP32;
WebAssembly.instantiate(wasm, {}).then(out => {
  let exports = out.instance.exports;
  _malloc = exports["c"];
  _do_deflate = exports["d"];
  _do_inflate = exports["e"];
  buffer = exports["a"].buffer;
  HEAPU8 = new U8Array(buffer);
  HEAP32 = new Int32Array(buffer);
  base64_box.oninput = decode;
  json_box.oninput = encode;
});

function result2array(result_ptr) {
  const size = HEAP32[result_ptr/4];
  const view = new U8Array(buffer, HEAP32[result_ptr/4 + 1], size < 0 ? -size : size);
  if (size < 0) {
    throw new Error(fromCharCode(...view));
  }
  return view;
}

function decode() {
  const value = base64_box.value;
  if (value === "") {
    // Special-case this: It's valid, but inflate will choke.
    json_box.value = "";
    return;
  }
  const bin_str = atob(value);
  const bin_buf_ptr = _malloc(bin_str.length);
  for (var i = 0; i < bin_str.length; ++i) {
    HEAPU8[bin_buf_ptr+i] = bin_str.charCodeAt(i);
  }
  // result_ptr points to a struct which is a 32-bit length followed by that much data
  // The data has already been "freed", but it will still be here to be copied
  const result_ptr = _do_inflate(bin_buf_ptr, bin_str.length);
  const utf8_buf = result2array(result_ptr);
  const out_value = new TextDecoder("utf-8", {fatal: true}).decode(utf8_buf);
  json_box.value = out_value;
}

function encode() {
  const value = json_box.value;
  const utf8_buf_ptr = _malloc(value.length * 2);
  const arr = new U8Array(buffer, utf8_buf_ptr, value.length * 2);
  const encode_result = new TextEncoder().encodeInto(value, arr);
  if (encode_result.read < value.length) {
    throw new Error("Couldn't encode string as UTF-8");
  }
  // result_ptr points to a struct which is a 32-bit length followed by that much data
  // The data has already been "freed", but it will still be here to be copied
  const result_ptr = _do_deflate(utf8_buf_ptr, encode_result.written);
  const bin_str = fromCharCode(...result2array(result_ptr));
  const base_str = btoa(bin_str);
  base64_box.value = base_str;
}
