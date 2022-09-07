#!/usr/bin/env python3
"""Merge templates together into a single HTML file"""

import argparse
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--html", type=argparse.FileType('rb'), required=True,
                    help="Filename of the html template")
parser.add_argument("--js", type=argparse.FileType('rb'), required=True,
                    help="Filename of the javascript template")
parser.add_argument("--wasm", type=argparse.FileType('rb'), required=True,
                    help="Filename of the wasm code")
parser.add_argument("--xor", type=int, default=0x6f,
                    help="Value to xor characters with, to avoid bad patterns")
args = parser.parse_args()

HTML_TAG = b'"""REPLACE_JS_HERE"""'
JS_TAG = b"'''REPLACE_WASM_HERE'''"

html = args.html.read()
html_tag_pos = html.find(HTML_TAG)
if html_tag_pos < 0:
    raise RuntimeError(f"Missing required tag {HTML_TAG.decode()} in {args.html.name}")
js = args.js.read()
js_tag_pos = js.find(JS_TAG)
if js_tag_pos < 0:
    raise RuntimeError(f"Missing required tag {JS_TAG.decode()} in {args.js.name}")

buf = sys.stdout.buffer
buf.write(html[:html_tag_pos])
buf.write(js[:js_tag_pos])

# We need to special case \0 because HTML doesn't like raw NULs, and it's too
# common to encode as \0. Adding one is a cheap way to get around this, but
# 0x7f is a common-ish value.
# By xor'ing with 0x40, we move 0x3f into the 0x7f slot, and it's very rarely
# used.
XOR = args.xor
trans = [chr(x^XOR).encode() for x in range(256)]
trans[ord("\0")^XOR] = rb"\0"
trans[ord("\n")^XOR] = rb"\n"
trans[ord("\r")^XOR] = rb"\r"
trans[ord("\\")^XOR] = rb"\\"
trans[ord('"')^XOR] = rb"\""

for chunk in iter((lambda:args.wasm.read(2048)), b''):
    buf.write(b''.join(trans[x] for x in chunk))

buf.write(js[js_tag_pos+len(JS_TAG):])
buf.write(html[html_tag_pos+len(HTML_TAG):])
