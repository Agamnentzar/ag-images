const bindings = require('./build/Release/ag_images.node');

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
