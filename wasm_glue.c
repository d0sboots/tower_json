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
  Bytef* data;
};

#define RETURN_ERROR(str) do {\
  static const struct result _res = {1 - sizeof(str), (Bytef*)(str)};\
  heap_reset();\
  return (uintptr_t)&_res;\
} while(0)

uintptr_t do_deflate(uintptr_t buf_in, size_t size) {
  z_stream stream;

  stream.zalloc = zalloc;
  stream.zfree = zfree;
  int err = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
  if (err != Z_OK) RETURN_ERROR("Can't deflateInit2");

  stream.next_in = (z_const Bytef*)buf_in;
  stream.avail_in = size;
  stream.avail_out = deflateBound(&stream, size);

  struct result* res = malloc(sizeof(struct result) + stream.avail_out);
  res->data = ((Bytef*)res) + sizeof(*res);
  stream.next_out = res->data;

  err = deflate(&stream, Z_FINISH);
  if (stream.msg != NULL) {
    res->size = -strlen(stream.msg);
    res->data = (Bytef*)stream.msg;
  } else {
    if (err == Z_OK) {
      RETURN_ERROR("Z_OK: Buffer size issue in deflate()");
    }
    if (err != Z_STREAM_END) {
      RETURN_ERROR("Unknown error in deflate()!");
    }
    res->size = stream.next_out - res->data;
  }
  // Reset the heap, which avoids needing any cleanup.
  // Our return value remains "valid" until we call into WASM again.
  heap_reset();
  return (uintptr_t)res;
}

uintptr_t do_inflate(uintptr_t buf_in, size_t size) {
  z_stream stream;

  stream.zalloc = zalloc;
  stream.zfree = zfree;
  int err = inflateInit2(&stream, -15);
  if (err != Z_OK) RETURN_ERROR("Can't deflateInit2");

  stream.next_in = (z_const Bytef*)buf_in;
  stream.avail_in = size;
  // Provide a generous amount of room for inflate; in practice all payloads
  // ought to be less than 2000% compression ratio.
  stream.avail_out = size * 20 + 1024;

  struct result* res = malloc(sizeof(struct result) + stream.avail_out);
  res->data = ((Bytef*)res) + sizeof(*res);
  stream.next_out = res->data;

  err = inflate(&stream, Z_FINISH);
  if (stream.msg != NULL) {
    res->size = -strlen(stream.msg);
    res->data = (Bytef*)stream.msg;
  } else {
    if (err == Z_OK) {
      RETURN_ERROR("Z_OK: Buffer size issue in inflate()");
    }
    if (err == Z_BUF_ERROR) {
      RETURN_ERROR("Bad data in inflate()!");
    }
    if (err != Z_STREAM_END) {
      RETURN_ERROR("Unknown error in inflate()!");
    }
    if (stream.avail_in != 0) {
      RETURN_ERROR("Extra bytes after end-of-stream!");
    }
    res->size = stream.next_out - res->data;
  }
  // Reset the heap, which avoids needing any cleanup.
  // Our return value remains "valid" until we call into WASM again.
  heap_reset();
  return (uintptr_t)res;
}
