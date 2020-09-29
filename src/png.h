#pragma once

#include <cmath> // round
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <pngconf.h>
#include <nan.h>
#include <stdint.h> // node < 7 uses libstdc++ on macOS which lacks complete c++11

enum error_status {
  ES_SUCCESS = 0,
  ES_NO_MEMORY,
  ES_WRITE_ERROR,
  ES_INVALID_SIGNATURE,
  ES_FAILED,
  ES_READING_PAST_END,
  ES_INVALID_FORMAT,
};

static char* error_status_to_string(error_status status) {
  switch (status) {
    case ES_SUCCESS: return "success";
    case ES_NO_MEMORY: return "no memory";
    case ES_WRITE_ERROR: return "write error";
    case ES_INVALID_SIGNATURE: return "invalid signature";
    case ES_FAILED: return "failed";
    case ES_READING_PAST_END: return "reading past end";
    case ES_INVALID_FORMAT: return "invalid format";
    default: return "invalid";
  }
}

// writing

struct PngWriteClosure {
  uint32_t compressionLevel = 6;
  uint32_t filters = PNG_ALL_FILTERS;
  uint32_t resolution = 0; // 0 = unspecified
  // Indexed PNGs:
  uint32_t nPaletteColors = 0;
  uint8_t* palette = nullptr;
  uint8_t backgroundIndex = 0;

  Nan::Callback cb;

  uint32_t width;
  uint32_t height;
  uint8_t *data;
  Nan::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>> dataRef;
  error_status status = ES_SUCCESS;

  // output
  uint8_t *output = 0;
  size_t outputLength = 0;
  size_t outputCapacity = 0;
};

static void flush_func(png_structp) {
}

#ifdef PNG_SETJMP_SUPPORTED
bool setjmp_wrapper(png_structp png) {
  return setjmp(png_jmpbuf(png));
}
#endif

#define INITIAL_SIZE 4096

static void write_func(png_structp png, png_bytep data, png_size_t size) {
  PngWriteClosure *closure = (PngWriteClosure *) png_get_io_ptr(png);

  if (!closure->output) {
    closure->output = (uint8_t*)malloc(INITIAL_SIZE);
    closure->outputCapacity = INITIAL_SIZE;
  }

  while ((closure->outputCapacity - closure->outputLength) < size) {
    closure->output = (uint8_t*)realloc(closure->output, closure->outputCapacity * 2);
    closure->outputCapacity = closure->outputCapacity * 2;
  }

  if (closure->output) {
    memcpy(closure->output + closure->outputLength, data, size);
    closure->outputLength += size;
  } else {
    closure->status = ES_NO_MEMORY;
    png_error(png, NULL);
  }
}

static error_status write_png(PngWriteClosure *closure) {
  unsigned int i;
  error_status status = ES_SUCCESS;
  png_structp png;
  png_infop info;
  png_color_16 white;
  int png_color_type;
  int bpc;
  unsigned int width = closure->width;
  unsigned int height = closure->height;
  uint8_t *data = closure->data;

  if (width == 0 || height == 0) {
    status = ES_WRITE_ERROR;
    return status;
  }

  png_bytep *volatile rows = (png_bytep *) malloc(height * sizeof (png_byte*));

  if (rows == NULL) {
    status = ES_NO_MEMORY;
    return status;
  }

  int stride = width * 4; // cairo_image_surface_get_stride(surface);
  for (i = 0; i < height; i++) {
    rows[i] = (png_byte *) data + i * stride;
  }

#ifdef PNG_USER_MEM_SUPPORTED
  png = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, NULL, NULL);
#else
  png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
#endif

  if (png == NULL) {
    status = ES_NO_MEMORY;
    free(rows);
    return status;
  }

  info = png_create_info_struct(png);
  if (info == NULL) {
    status = ES_NO_MEMORY;
    png_destroy_write_struct(&png, &info);
    free(rows);
    return status;
  }

#ifdef PNG_SETJMP_SUPPORTED
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    free(rows);
    return status;
  }
#endif

  png_set_write_fn(png, closure, write_func, flush_func);
  png_set_compression_level(png, closure->compressionLevel);
  png_set_filter(png, 0, closure->filters);

  if (closure->resolution != 0) {
    uint32_t res = static_cast<uint32_t>(round(static_cast<double>(closure->resolution) * 39.3701));
    png_set_pHYs(png, info, res, res, PNG_RESOLUTION_METER);
  }

  bpc = 8;
  png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;

  png_set_IHDR(png, info, width, height, bpc, png_color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  if (png_color_type != PNG_COLOR_TYPE_PALETTE) {
    white.gray = (1 << bpc) - 1;
    white.red = white.blue = white.green = white.gray;
    png_set_bKGD(png, info, &white);
  }

  png_write_info(png, info);
  png_write_image(png, rows);
  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  free(rows);
  return status;
}

// reading

struct PngReadClosure {
  // input
  uint8_t *data;
  size_t length;
  Nan::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>> dataRef;
  Nan::Callback cb;
  error_status status = ES_SUCCESS;

  // output
  uint32_t width;
  uint32_t height;
  uint8_t *buffer;
};

void read_func(png_structp png, png_bytep outBytes, png_size_t byteCountToRead) {
  auto closure = (PngReadClosure*)png_get_io_ptr(png);
  if (byteCountToRead <= closure->length) {
    memcpy(outBytes, closure->data, byteCountToRead);
    closure->data += byteCountToRead;
    closure->length -= byteCountToRead;
  } else {
    closure->status = ES_READING_PAST_END;
    png_error(png, NULL);
  }
}

// void error_func(png_structp, png_const_charp message) {
//   printf("PNG error: %s\n", message);
// }

static error_status read_png(PngReadClosure *closure) {
  if (closure->length < 8 || !png_check_sig(closure->data, 8)) return ES_INVALID_SIGNATURE;

  auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) return ES_NO_MEMORY;

  auto info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return ES_NO_MEMORY;
  }
  
  uint8_t *buffer = NULL;
  png_bytep *rowPointers = NULL;

#ifdef PNG_SETJMP_SUPPORTED
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    if (rowPointers) free(rowPointers);
    if (buffer) free(buffer);
    return ES_FAILED;
  }
#endif

  png_set_read_fn(png_ptr, closure, read_func);
  // png_set_error_fn(png_ptr, NULL, error_func, NULL);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 width = 0;
  png_uint_32 height = 0;
  int bitDepth = 0;
  int colorType = -1;
  int interlaceMethod = 0;
  int number_of_passes = 1;
  png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, &interlaceMethod, NULL, NULL);
  if (retval != 1) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return ES_FAILED;
  }

  buffer = (uint8_t*)malloc(width * height * 4);
  if (!buffer) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return ES_NO_MEMORY;
  }

  if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
  if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
  if (colorType != PNG_COLOR_TYPE_RGB_ALPHA) png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
  if (bitDepth == 16) png_set_strip_16(png_ptr);
  if (bitDepth < 8) png_set_packing(png_ptr);
  if (interlaceMethod == PNG_INTERLACE_ADAM7) number_of_passes = png_set_interlace_handling(png_ptr);

  rowPointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  if (!rowPointers) {
    free(buffer);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return ES_NO_MEMORY;
  }

  png_bytep rowData = (png_bytep)buffer;
  
  for (png_uint_32 row = 0; row < height; row++) {
    rowPointers[row] = rowData;
    rowData += width * 4;
  }

  png_read_image(png_ptr, rowPointers);

  free(rowPointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  closure->width = width;
  closure->height = height;
  closure->buffer = buffer;
  return ES_SUCCESS;
}
