#include <nan.h>
#include <v8.h>
#include <assert.h>
#include <cctype>
#include "./png.h"

using namespace v8;
using namespace std;

static char *parsePNGArgs(Local<Value> arg, PngWriteClosure *pngargs) {
  if (arg->IsObject()) {
    Local<Object> obj = Nan::To<Object>(arg).ToLocalChecked();

    Local<Value> cLevel = Nan::Get(obj, Nan::New("compressionLevel").ToLocalChecked()).ToLocalChecked();
    if (cLevel->IsUint32()) {
      uint32_t val = Nan::To<uint32_t>(cLevel).FromMaybe(0);
      // See quote below from spec section 4.12.5.5.
      if (val <= 9) pngargs->compressionLevel = val;
    }

    Local<Value> rez = Nan::Get(obj, Nan::New("resolution").ToLocalChecked()).ToLocalChecked();
    if (rez->IsUint32()) {
      uint32_t val = Nan::To<uint32_t>(rez).FromMaybe(0);
      if (val > 0) pngargs->resolution = val;
    }

    Local<Value> filters = Nan::Get(obj, Nan::New("filters").ToLocalChecked()).ToLocalChecked();
    if (filters->IsUint32()) pngargs->filters = Nan::To<uint32_t>(filters).FromMaybe(0);

    Local<Value> palette = Nan::Get(obj, Nan::New("palette").ToLocalChecked()).ToLocalChecked();
    if (palette->IsUint8ClampedArray()) {
      Local<Uint8ClampedArray> palette_ta = palette.As<Uint8ClampedArray>();
      pngargs->nPaletteColors = palette_ta->Length();
      if (pngargs->nPaletteColors % 4 != 0) {
        return "Palette length must be a multiple of 4.";
      }
      pngargs->nPaletteColors /= 4;
      Nan::TypedArrayContents<uint8_t> _paletteColors(palette_ta);
      pngargs->palette = *_paletteColors;
      // Optional background color index:
      Local<Value> backgroundIndexVal = Nan::Get(obj, Nan::New("backgroundIndex").ToLocalChecked()).ToLocalChecked();
      if (backgroundIndexVal->IsUint32()) {
        pngargs->backgroundIndex = static_cast<uint8_t>(Nan::To<uint32_t>(backgroundIndexVal).FromMaybe(0));
      }
    }
  }
  
  return nullptr;
}

void to_png_buffer(uv_work_t *req) {
  auto closure = static_cast<PngWriteClosure*>(req->data);
  closure->status = write_png(closure);
}

void to_png_buffer_after(uv_work_t *req, int) {
  Nan::HandleScope scope;
  Nan::AsyncResource async("to_png_buffer_after");
  auto closure = static_cast<PngWriteClosure*>(req->data);
  delete req;

  if (closure->status) {
    Local<Value> argv[1] = { Exception::Error(Nan::New<String>(error_status_to_string(closure->status)).ToLocalChecked()) };
    closure->cb.Call(1, argv, &async);
  } else {
    Local<Object> buf = Nan::NewBuffer((char*)closure->output, closure->outputLength).ToLocalChecked();
    Local<Value> argv[2] = { Nan::Null(), buf };
    closure->cb.Call(2, argv, &async);
  }

  closure->dataRef.Reset();
  delete closure;
}

NAN_METHOD(encodePNG) {
  if (!info[0]->IsNumber() || !info[1]->IsNumber() || !node::Buffer::HasInstance(info[2]) || !info[4]->IsFunction()) {
    return Nan::ThrowTypeError("Invalid arguments");
  }

  auto closure = new PngWriteClosure();
  closure->width = Nan::To<uint32_t>(info[0]).FromMaybe(0);
  closure->height = Nan::To<uint32_t>(info[1]).FromMaybe(0);
  Local<Context> ctx = Nan::GetCurrentContext();
  auto length = node::Buffer::Length(info[2]);

  if (length != (closure->width * closure->height * 4)) {
    delete closure;
    return Nan::ThrowTypeError("Invalid buffer size");
  }

  auto error = parsePNGArgs(info[3], closure);

  if (error) {
    delete closure;
    return Nan::ThrowTypeError(error);
  }

  closure->data = (uint8_t*)node::Buffer::Data(info[2]);
  closure->dataRef.Reset(info[2]);
  closure->cb.Reset(info[4].As<Function>());

  uv_work_t* req = new uv_work_t;
  req->data = closure;
  uv_queue_work(uv_default_loop(), req, to_png_buffer, to_png_buffer_after);
}

void decode_png_buffer(uv_work_t *req) {
  auto closure = static_cast<PngReadClosure*>(req->data);
  closure->status = read_png(closure);
}

void decode_png_buffer_after(uv_work_t *req, int) {
  Nan::HandleScope scope;
  Nan::AsyncResource async("decode_png_buffer_after");
  auto closure = static_cast<PngReadClosure*>(req->data);
  delete req;

  if (closure->status) {
    Local<Value> argv[1] = { Exception::Error(Nan::New<String>(error_status_to_string(closure->status)).ToLocalChecked()) };
    closure->cb.Call(1, argv, &async);
  } else {
    Local<Object> buf = Nan::NewBuffer((char*)closure->buffer, closure->width * closure->height * 4).ToLocalChecked();
    Local<Value> argv[4] = { Nan::Null(), buf, Nan::New<v8::Int32>(closure->width), Nan::New<v8::Int32>(closure->height) };
    closure->cb.Call(4, argv, &async);
  }

  closure->dataRef.Reset();
  delete closure;
}

NAN_METHOD(decodePNG) {
  if (!node::Buffer::HasInstance(info[0]) || !info[1]->IsFunction()) {
    return Nan::ThrowTypeError("Invalid arguments");
  }

  auto closure = new PngReadClosure();
  closure->data = (uint8_t*)node::Buffer::Data(info[0]);
  closure->length = (size_t)node::Buffer::Length(info[0]);
  closure->dataRef.Reset(info[0]);
  closure->cb.Reset(info[1].As<Function>());

  uv_work_t* req = new uv_work_t;
  req->data = closure;
  uv_queue_work(uv_default_loop(), req, decode_png_buffer, decode_png_buffer_after);
}

void Initialize(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
  Nan::HandleScope scope;
  auto ctx = Nan::GetCurrentContext();

  Nan::Set(target, Nan::New("encodePNG").ToLocalChecked(), Nan::New<FunctionTemplate>(encodePNG)->GetFunction(ctx).ToLocalChecked());
  Nan::Set(target, Nan::New("decodePNG").ToLocalChecked(), Nan::New<FunctionTemplate>(decodePNG)->GetFunction(ctx).ToLocalChecked());

  Nan::Set(target, Nan::New("PNG_NO_FILTERS").ToLocalChecked(), Nan::New<Uint32>(PNG_NO_FILTERS));
  Nan::Set(target, Nan::New("PNG_FILTER_NONE").ToLocalChecked(), Nan::New<Uint32>(PNG_FILTER_NONE));
  Nan::Set(target, Nan::New("PNG_FILTER_SUB").ToLocalChecked(), Nan::New<Uint32>(PNG_FILTER_SUB));
  Nan::Set(target, Nan::New("PNG_FILTER_UP").ToLocalChecked(), Nan::New<Uint32>(PNG_FILTER_UP));
  Nan::Set(target, Nan::New("PNG_FILTER_AVG").ToLocalChecked(), Nan::New<Uint32>(PNG_FILTER_AVG));
  Nan::Set(target, Nan::New("PNG_FILTER_PAETH").ToLocalChecked(), Nan::New<Uint32>(PNG_FILTER_PAETH));
  Nan::Set(target, Nan::New("PNG_ALL_FILTERS").ToLocalChecked(), Nan::New<Uint32>(PNG_ALL_FILTERS));
}

NAN_MODULE_INIT(init) {
  Initialize(target);
}

NODE_MODULE(ag_images, init);
