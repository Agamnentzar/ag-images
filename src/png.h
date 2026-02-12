#pragma once

#include <cmath> // round
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <pngconf.h>
#include <nan.h>
#include <stdint.h> // node < 7 uses libstdc++ on macOS which lacks complete c++11
#include "fpng.h"

#define USE_FPNG

#define WUFFS_IMPLEMENTATION

#define WUFFS_CONFIG__STATIC_FUNCTIONS

// Defining the WUFFS_CONFIG__MODULE* macros are optional, but it lets users of
// release/c/etc.c choose which parts of Wuffs to build. That file contains the
// entire Wuffs standard library, implementing a variety of codecs and file
// formats. Without this macro definition, an optimizing compiler or linker may
// very well discard Wuffs code for unused codecs, but listing the Wuffs
// modules we use makes that process explicit. Preprocessing means that such
// code simply isn't compiled.
#define WUFFS_CONFIG__MODULE__ADLER32
#define WUFFS_CONFIG__MODULE__AUX__BASE
#define WUFFS_CONFIG__MODULE__AUX__IMAGE
#define WUFFS_CONFIG__MODULE__BASE
#define WUFFS_CONFIG__MODULE__BMP
#define WUFFS_CONFIG__MODULE__CRC32
#define WUFFS_CONFIG__MODULE__DEFLATE
#define WUFFS_CONFIG__MODULE__GIF
#define WUFFS_CONFIG__MODULE__JPEG
#define WUFFS_CONFIG__MODULE__NETPBM
#define WUFFS_CONFIG__MODULE__NIE
#define WUFFS_CONFIG__MODULE__PNG
#define WUFFS_CONFIG__MODULE__TARGA
#define WUFFS_CONFIG__MODULE__VP8
#define WUFFS_CONFIG__MODULE__WBMP
#define WUFFS_CONFIG__MODULE__WEBP
#define WUFFS_CONFIG__MODULE__ZLIB
#include "wuffs-unsupported-snapshot.c"

enum error_status {
  ES_SUCCESS = 0,
  ES_NO_MEMORY,
  ES_WRITE_ERROR,
  ES_INVALID_SIGNATURE,
  ES_FAILED,
  ES_READING_PAST_END,
  ES_INVALID_FORMAT,
};

static const char* error_status_to_string(error_status status) {
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
  int32_t compressionLevel = 6;
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
  Nan::Persistent<v8::Value> dataRef;
  error_status status = ES_SUCCESS;

  // output for fpng (quality <= 0)
  std::unique_ptr<std::vector<uint8_t>> outputVector = 0;

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

class MyDecodeCallbacks : public wuffs_aux::DecodeImageCallbacks {
 public:
  MyDecodeCallbacks(bool _premultiplied) : m_fourcc(0), premultipled(_premultiplied) {}

  uint32_t m_fourcc;
  bool premultipled;

 private:
  wuffs_base__image_decoder::unique_ptr  //
  SelectDecoder(uint32_t fourcc,
                wuffs_base__slice_u8 prefix_data,
                bool prefix_closed) override {
    // Save the fourcc value (you can think of it as like a 'MIME type' but in
    // uint32_t form) before calling the superclass' implementation.
    //
    // The "if (m_fourcc == 0)" is because SelectDecoder can be called multiple
    // times. Files that are nominally BMP images can contain complete JPEG or
    // PNG images. This program prints the outer file format, the first one
    // encountered, not the inner one.
    if (m_fourcc == 0) {
      m_fourcc = fourcc;
    }
    return wuffs_aux::DecodeImageCallbacks::SelectDecoder(fourcc, prefix_data,
                                                          prefix_closed);
  }

  wuffs_base__pixel_format  //
  SelectPixfmt(const wuffs_base__image_config& image_config) override {
    auto format = this->premultipled ? WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL :
      WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL;

    return wuffs_base__make_pixel_format(format);
  }

  AllocPixbufResult  //
  AllocPixbuf(const wuffs_base__image_config& image_config,
              bool allow_uninitialized_memory) override {
    return wuffs_aux::DecodeImageCallbacks::AllocPixbuf(
        image_config, allow_uninitialized_memory);
  }
};

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
  error_status status = ES_SUCCESS;
  unsigned int width = closure->width;
  unsigned int height = closure->height;
  uint8_t *data = closure->data;
  if (width == 0 || height == 0) {
    status = ES_WRITE_ERROR;
    return status;
  }

  if (closure->compressionLevel <= 0) {
    int flags = fpng::FPNG_ENCODE_SLOWER;
    if (closure->compressionLevel == -1) {
      flags = 0;
    }

    closure->outputVector = std::make_unique<std::vector<uint8_t>>();
    auto fpng_status = fpng::fpng_encode_image_to_memory(closure->data, width, height, 4, *(closure->outputVector), flags);
    if (!fpng_status) {
      return ES_WRITE_ERROR;
    }
    return status;
  }
  png_bytep *volatile rows = (png_bytep *) malloc(height * sizeof (png_byte*));

  if (rows == NULL) {
    status = ES_NO_MEMORY;
    return status;
  }

  int stride = width * 4; // cairo_image_surface_get_stride(surface);
  for (unsigned int i = 0; i < height; i++) {
    rows[i] = (png_byte *) data + i * stride;
  }

  png_structp png = {};
  png_infop info = {};

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

  int bpc = 8;
  int png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;

  png_set_IHDR(png, info, width, height, bpc, png_color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  if (png_color_type != PNG_COLOR_TYPE_PALETTE) {
    png_color_16 white = {};
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
  Nan::Persistent<v8::Value> dataRef;
  Nan::Callback cb;
  error_status status = ES_SUCCESS;
  bool premultiplied;

  // output
  uint32_t width = 0;
  uint32_t height = 0;
  uint8_t *buffer = nullptr;
};

static error_status read_png(PngReadClosure *closure) {
  if (closure->length < 8 || !png_check_sig(closure->data, 8)) return ES_INVALID_SIGNATURE;

  MyDecodeCallbacks callbacks(closure->premultiplied);
  wuffs_aux::sync_io::MemoryInput input(closure->data, closure->length);
  wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(callbacks, input);
  if (!res.error_message.empty()) {
    return ES_FAILED;
  } else if (closure->premultiplied && res.pixbuf.pixcfg.pixel_format().repr !=
             WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL) {
    return ES_FAILED;
  }  else if (!closure->premultiplied && res.pixbuf.pixcfg.pixel_format().repr !=
             WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL) {
    return ES_FAILED;
  }

  uint32_t w = res.pixbuf.pixcfg.width();
  uint32_t h = res.pixbuf.pixcfg.height();
  closure->width = w;
  closure->height = h;

  // WARNING: this malloc needs to be free'd by the caller
  closure->buffer = (uint8_t*)res.pixbuf_mem_owner.release();

  return ES_SUCCESS;
}

// webp reading

struct WebpReadClosure {
  // input
  uint8_t *data;
  size_t length;
  Nan::Persistent<v8::Value> dataRef;
  Nan::Callback cb;
  error_status status = ES_SUCCESS;
  bool premultiplied;

  // output
  uint32_t width = 0;
  uint32_t height = 0;
  uint8_t *buffer = nullptr;
};

static bool is_webp(uint8_t* data, size_t len) {
  return len >= 12 &&
         data[0]=='R' && data[1]=='I' && data[2]=='F' && data[3]=='F' &&
         data[8]=='W' && data[9]=='E' && data[10]=='B' && data[11]=='P';
}

static error_status read_webp(WebpReadClosure *closure) {
  if (!is_webp(closure->data, closure->length)) return ES_INVALID_SIGNATURE;

  MyDecodeCallbacks callbacks(closure->premultiplied);
  wuffs_aux::sync_io::MemoryInput input(closure->data, closure->length);
  wuffs_aux::DecodeImageResult res = wuffs_aux::DecodeImage(callbacks, input);
  if (!res.error_message.empty()) {
    return ES_FAILED;
  } else if (closure->premultiplied && res.pixbuf.pixcfg.pixel_format().repr !=
             WUFFS_BASE__PIXEL_FORMAT__RGBA_PREMUL) {
    return ES_FAILED;
  } else if (!closure->premultiplied && res.pixbuf.pixcfg.pixel_format().repr !=
             WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL) {
    return ES_FAILED;
  }

  uint32_t w = res.pixbuf.pixcfg.width();
  uint32_t h = res.pixbuf.pixcfg.height();
  closure->width = w;
  closure->height = h;

  // WARNING: this malloc needs to be free'd by the caller
  closure->buffer = (uint8_t*)res.pixbuf_mem_owner.release();

  return ES_SUCCESS;
}
