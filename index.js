const bindings = require('./build/Release/ag_images.node');

exports.PNG_NO_FILTERS = bindings.PNG_NO_FILTERS;
exports.PNG_FILTER_NONE = bindings.PNG_FILTER_NONE;
exports.PNG_FILTER_SUB = bindings.PNG_FILTER_SUB;
exports.PNG_FILTER_UP = bindings.PNG_FILTER_UP;
exports.PNG_FILTER_AVG = bindings.PNG_FILTER_AVG;
exports.PNG_FILTER_PAETH = bindings.PNG_FILTER_PAETH;
exports.PNG_ALL_FILTERS = bindings.PNG_ALL_FILTERS;

exports.encodePNG = function (width, height, data, options) {
  return new Promise((resolve, reject) => {
    bindings.encodePNG(width, height, data, options, (error, result) => {
      if (error) {
        reject(error);
      } else {
        resolve(result);
      }
    })
  });
};

exports.decodePNG = function (buffer, options) {
  return new Promise((resolve, reject) => {
    const premultiplied = options?.premultiplied || false;
    bindings.decodePNG(buffer, premultiplied, (error, data, width, height) => {
      if (error) {
        reject(error);
      } else {
        resolve({ data, width, height, premultiplied });
      }
    })
  });
};

exports.decode = function (buffer, options) {
  if (buffer.length >= 8 && buffer[0] === 0x89 && buffer[1] === 0x50 && buffer[2] === 0x4e && buffer[3] === 0x47) {
    return exports.decodePNG(buffer, options);
  }
  if (buffer.length >= 12 && buffer[0] === 0x52 && buffer[1] === 0x49 && buffer[2] === 0x46 && buffer[3] === 0x46 &&
      buffer[8] === 0x57 && buffer[9] === 0x45 && buffer[10] === 0x42 && buffer[11] === 0x50) {
    return exports.decodeWebP(buffer, options);
  }
  return Promise.reject(new Error('Unsupported image format'));
};

exports.decodeWebP = function (buffer, options) {
  // VP8X (extended WebP: lossy + alpha) is not supported by the Wuffs decoder
  if (buffer && buffer.length >= 16 &&
      buffer[12] === 0x56 && buffer[13] === 0x50 && buffer[14] === 0x38 && buffer[15] === 0x58) {
    return Promise.reject(new Error('VP8X (extended WebP) is not supported. Use lossless WebP (VP8L) for alpha, or lossy WebP (VP8) without alpha.'));
  }
  return new Promise((resolve, reject) => {
    const premultiplied = options?.premultiplied || false;
    bindings.decodeWebP(buffer, premultiplied, (error, data, width, height) => {
      if (error) {
        reject(error);
      } else {
        resolve({ data, width, height, premultiplied });
      }
    })
  });
};
