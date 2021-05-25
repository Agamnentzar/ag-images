const fs = require('fs');
const { encodePNG, decodePNG } = require('./index');

async function run() {
  for (let i = 0; i < 100000; i++) {
    const memory = process.memoryUsage();
    console.log(i.toString().padStart(4),
      (memory.rss / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.heapTotal / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.arrayBuffers / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.external / (1024 * 1024)).toFixed(0).padStart(5));

    const promises = [];

    for (let j = 0; j < 100; j++) {
      const buffer = fs.readFileSync('./test/interlace.png');
      promises.push(decodePNG(buffer)
        .then(image => encodePNG(image.width, image.height, image.data, { compressionLevel: 1 })));
    }

    await Promise.all(promises);
  }
}

run();
