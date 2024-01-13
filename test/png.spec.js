const { encodePNG, decodePNG, PNG_FILTER_NONE } = require('../');
const fs = require('fs');
const path = require('path');
const assert = require('assert');

describe('encode / decode', () => {
  it('throws on invalid arguments', async () => {
    await assert.rejects(() => encodePNG());
    await assert.rejects(() => encodePNG('x'));
    await assert.rejects(() => encodePNG(1, 'f'));
  });

  it('encodes image', async () => {
    const data = Buffer.from([0, 0, 0, 0]);
    const buffer = await encodePNG(1, 1, data, {});
    assert.strictEqual(buffer.toString('base64'), 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABmJLR0QA/wD/AP+gvaeTAAAAC0lEQVQImWNgAAIAAAUAAWJVMogAAAAASUVORK5CYII=');
  });

  it('encodes image with fastest encoder', async () => {
    const data = Buffer.from([0, 0, 0, 0]);
    const buffer = await encodePNG(1, 1, data, { compressionLevel: 0 });
    assert.strictEqual(buffer.toString('base64'), 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABWZkRUNSJJPjAOWrYpkAAAAQSURBVHgBAQUA+v8AAAAAAAAFAAFkeJU4AAAAAElFTkSuQmCC');
  });

  it(`doesn't crash on bad file`, async () => {
    // this prints "libpng error: undefined" in console
    const png = fs.readFileSync(path.join(__dirname, `fail.png`));
    await assert.rejects(() => decodePNG(png));
  })

  const images = [
    { name: 'rgba', width: 32, height: 32 },
    { name: 'rgb', width: 32, height: 32 },
    { name: 'gray', width: 32, height: 32 },
    { name: 'gray_alpha', width: 32, height: 32 },
    { name: 'pal', width: 32, height: 32 },
    { name: '1bit', width: 300, height: 225 },
    { name: 'interlace', width: 1024, height: 768 },
    { name: 'shino', width: 200, height: 200 },
    { name: 'semitransparent', width: 128, height: 128, premul: true },
    { name: 'semitransparent', width: 128, height: 128, premul: false },
  ];

  images.forEach(({ name, width, height, premul }) => it(`decodes image (${name})`, async () => {
    const png = fs.readFileSync(path.join(__dirname, `${name}.png`));
    const _image = await decodePNG(png);
    const _encoded = await encodePNG(_image.width, _image.height, _image.data, { compressionLevel: 0 })
    const image = await decodePNG(_encoded, { premultiplied: premul })

    fs.writeFileSync(path.join(__dirname, `${name}.data`), image.data);

    assert(image);
    assert(image.data);
    assert.strictEqual(image.width, width);
    assert.strictEqual(image.height, height);

    const expectedName = typeof premul !== 'undefined' ?
      `${name}.${premul ? 'premul' : 'nonpremul'}.data`  :
      `${name}.data`;

    console.log(expectedName);
    const expected = fs.readFileSync(path.join(__dirname, expectedName));

    assert.strictEqual(Buffer.compare(image.data, expected), 0);
  }));

  images.forEach(({ name, width, height }) => it(`encodes image (${name})`, async () => {
    const data = fs.readFileSync(path.join(__dirname, `${name}.data`));
    const buffer = await encodePNG(width, height, data, {});
    fs.writeFileSync(path.join(__dirname, `${name}.out.png`), buffer);
  }));

  if (process.env.SPEED_TEST_DIR) {
    const speedTestDir = process.env.SPEED_TEST_DIR;
    it(`read/write speed test`, async () => {
      const files = fs.readdirSync(speedTestDir)
        .filter(f => /\.png$/.test(f))
        .map(f => fs.readFileSync(path.join(speedTestDir,f)));
      const decoded = [];

      {
        const start = Date.now();
        await Promise.all(files.map(async (f) => {
          const png = await decodePNG(f);
          decoded.push(png);
        }));
        console.log(`read done in ${(Date.now() - start).toFixed(2)} ms`);
      }

      {
        const start = Date.now();
        let totalLength = 0;

        await Promise.all(decoded.map(async (f) => {
          const buffer = await encodePNG(f.width, f.height, f.data, { compressionLevel: 0, filters: PNG_FILTER_NONE });
          totalLength += buffer.byteLength;
        }));
        console.log(`write done in ${(Date.now() - start).toFixed(2)} ms, length: ${(totalLength / (1024 * 1024)).toFixed(1)} MB`);
      }
    })
  }
});
