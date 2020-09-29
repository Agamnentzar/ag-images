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

exports.decodePNG = function (buffer) {
  return new Promise((resolve, reject) => {
    bindings.decodePNG(buffer, (error, data, width, height) => {
      if (error) {
        reject(error);
      } else {
        resolve({ data, width, height });
      }
    })
  });
};
