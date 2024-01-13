const fs = require('fs');
const { encodePNG, decodePNG, PNG_NO_FILTERS } = require('./index');

const args = process.argv.slice(2);
const pngFilePath = args[0] || './test/interlace.png'; // Default PNG file path

let iterations = parseInt(args[1] || 100, 10);

let lastRunMs = '';
let lastSize = '';

async function run() {
  for (let i = 0; i < 100000; i++) {
    const memory = process.memoryUsage();
    console.log(i.toString().padStart(4),
      (memory.rss / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.heapTotal / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.arrayBuffers / (1024 * 1024)).toFixed(0).padStart(5),
      (memory.external / (1024 * 1024)).toFixed(0).padStart(5),
      lastRunMs, lastSize);

    const promises = [];

    const start = Date.now();
    for (let j = 0; j < iterations; j++) {
      const buffer = fs.readFileSync(pngFilePath);
      promises.push(decodePNG(buffer)
        .then(async image => {
          const out = await encodePNG(image.width, image.height, image.data, { compressionLevel: 0, filters: PNG_NO_FILTERS });
          // fs.writeFileSync('/tmp/test.fpng.png', out);
          lastSize = (out.byteLength / (image.width * image.height * 4)).toFixed(2);
        }));
    }
    await Promise.all(promises);
    lastRunMs = `Took ${Date.now() - start}ms`;
    if (global.gc) {
      global.gc();
    }
  }
}

run();
