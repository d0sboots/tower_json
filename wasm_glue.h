#ifndef WASM_GLUE_H
#define WASM_GLUE_H

#include <stdint.h>

struct result {
  int32_t size;
  unsigned char* data;
};

struct result* format_json(const unsigned char* buf, int size, int pretty);
#endif /* WASM_GLUE_H */
