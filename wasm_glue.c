#include <string.h>
#include <emscripten/heap.h>

#include "zlib.h"

/* Use arena-allocation through sbrk() directly for a truly tiny malloc()
 * solution. Of course, this leaks all memory unless/until it is freed with
 * heap_reset() - it can only be used with an application that can reset the
 * heap after each "use" of WASM. */
extern size_t __heap_base;
static uintptr_t heap_current = (uintptr_t)&__heap_base;
#define ALIGNMENT (__alignof__(max_align_t))

void *malloc(size_t size) {
  uintptr_t increment = size;
  increment = (increment + (ALIGNMENT-1)) & ~(ALIGNMENT-1);
  uintptr_t old_brk = heap_current;
  uintptr_t new_brk = old_brk + increment;
  if (increment > 0 && (uint32_t)new_brk <= (uint32_t)old_brk) {
    return NULL;
  }
  if (new_brk > emscripten_get_heap_size()) {
    return NULL;
  }
  heap_current = new_brk;
  return (void*)old_brk;
}

inline void free(void *ptr) {}

extern size_t __heap_base;
void heap_reset() {
  heap_current = (uintptr_t)&__heap_base;
}

void* zalloc(void *unused, unsigned int count, unsigned int size) {
  return malloc(count * size);
}
void zfree(void *unused1, void *unused2) {}

struct result {
  int32_t size;
  Bytef data[0];
};

uintptr_t make_error(const char* str, size_t size) {
  struct result* res = malloc(sizeof(struct result) + size);
  res->size = -size;
  memcpy(res->data, str, size);
  // After an error, the heap will be reset.
  // The error value we return will still be "valid" until we call into WASM
  // again.
  heap_reset();
  return (uintptr_t)res;
}
#define MAKE_ERROR(str) make_error((str), sizeof(str))

#define SET_ERROR(result_p, str) do {(result_p)->size = -sizeof(str); memcpy((result_p)->data, (str), sizeof(str));} while(0)

uintptr_t do_deflate(uintptr_t buf_in, size_t size) {
  z_stream stream;

  stream.zalloc = zalloc;
  stream.zfree = zfree;
  int err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
  if (err != Z_OK) return MAKE_ERROR("Can't deflateInit2");

  stream.next_in = (z_const Bytef*)buf_in;
  stream.avail_in = size;
  stream.avail_out = deflateBound(&stream, size);

  struct result* res = malloc(sizeof(struct result) + stream.avail_out);
  stream.next_out = res->data;

  err = deflate(&stream, Z_FINISH);
  switch(err) {
    case Z_STREAM_END:
      res->size = stream.next_out - res->data; break;
    case Z_OK:
      SET_ERROR(res, "Didn't finish compressing"); break;
    case Z_STREAM_ERROR:
      SET_ERROR(res, "Z_STREAM_ERROR in deflate()"); break;
    case Z_BUF_ERROR:
      SET_ERROR(res, "Z_BUF_ERROR in deflate()"); break;
    default:
      SET_ERROR(res, "Unknown in deflate()!"); break;
  }
  // Reset the heap, which avoids needing any cleanup.
  // Our return value remains "valid" until we call into WASM again.
  heap_reset();
  return (uintptr_t)res;
}

uintptr_t do_inflate(uintptr_t buf_in, size_t size) {
  return 0;
}
