"use strict";

// Define some stuff for minification purposes
const fromCharCode = String.fromCharCode;
const U8Array = Uint8Array;

const base64_box = document.getElementById("base64");
const json_box = document.getElementById("json");
const error_box = document.getElementById("error-box");
const pretty = document.getElementById("pretty");
// Names for error reporting.
const state_names = [
  "start"       ,  /* GO */
  "ok"          ,  /* OK */
  "object"      ,  /* OB */
  "key"         ,  /* KE */
  "colon"       ,  /* CO */
  "value"       ,  /* VA */
  "array"       ,  /* AR */
  "string"      ,  /* ST */
  "escape"      ,  /* ES */
  "u1"          ,  /* U1 */
  "u2"          ,  /* U2 */
  "u3"          ,  /* U3 */
  "u4"          ,  /* U4 */
  "minus"       ,  /* MI */
  "zero"        ,  /* ZE */
  "integer"     ,  /* IN */
  "fraction_dot",  /* FR */
  "fraction"    ,  /* FS */
  "e"           ,  /* E1 */
  "ex"          ,  /* E2 */
  "exp"         ,  /* E3 */
  "tr"          ,  /* T1 */
  "tru"         ,  /* T2 */
  "true"        ,  /* T3 */
  "fa"          ,  /* F1 */
  "fal"         ,  /* F2 */
  "fals"        ,  /* F3 */
  "false"       ,  /* F4 */
  "nu"          ,  /* N1 */
  "nul"         ,  /* N2 */
  "null"        ,  /* N3 */
];
const mode_names = ["array", "done", "key", "object"];

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
  pretty.onchange = decode;
  base64_box.onfocus = select_all;
  json_box.onfocus = select_all;
});

function result2array(result_ptr) {
  const size = HEAP32[result_ptr/4];
  const data_pos = HEAP32[result_ptr/4 + 1];
  if (size == -1) {
    var bad_char = HEAPU8[data_pos+4];
    bad_char = bad_char >= 32 ? String.fromCharCode(bad_char) : "\\x" + bad_char.toString(16);
    throw new Error(`Invalid character '${bad_char}' processing state '${state_names[HEAPU8[data_pos+5]]}' at position ${HEAP32[data_pos/4]}, in mode ${mode_names[HEAPU8[data_pos+6]]}`);
  }
  const view = new U8Array(buffer, data_pos, size < 0 ? -size : size);
  if (size < 0) {
    throw new Error(fromCharCode(...view));
  }
  return view;
}

/** @this {HTMLTextAreaElement} */
function select_all() {
  this.setSelectionRange(0, this.value.length);
}

function decode() {
  try {
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
    // result_ptr points to a struct which is a 32-bit length followed by a
    // ptr to that much data. The data has already been "freed", but it will
    // still be here to be copied.
    const result_ptr = _do_inflate(bin_buf_ptr, bin_str.length, pretty.checked | 0);
    const utf8_buf = result2array(result_ptr);
    var out_value = new TextDecoder("utf-8", {fatal: true}).decode(utf8_buf);
    json_box.value = out_value;
  } catch (err) {
    json_box.value = err + "";
    throw err;
  }
}

function encode() {
  try {
    var value = json_box.value;
    const utf8_buf_ptr = _malloc(value.length * 2);
    const arr = new U8Array(buffer, utf8_buf_ptr, value.length * 2);
    const encode_result = new TextEncoder().encodeInto(value, arr);
    if (encode_result.read < value.length) {
      throw new Error("Couldn't encode string as UTF-8");
    }
    // result_ptr points to a struct which is a 32-bit length followed by a
    // ptr to that much data. The data has already been "freed", but it will
    // still be here to be copied.
    const result_ptr = _do_deflate(utf8_buf_ptr, encode_result.written, pretty.checked | 0);
    const bin_str = fromCharCode(...result2array(result_ptr));
    const base_str = btoa(bin_str);
    base64_box.value = base_str;
  } catch (err) {
    base64_box.value = err + "";
    throw err;
  }
}
