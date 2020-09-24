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
  ];

  images.forEach(({ name, width, height }) => it(`decodes image (${name})`, async () => {
    const png = fs.readFileSync(path.join(__dirname, `${name}.png`));
    const image = await decodePNG(png);

    fs.writeFileSync(path.join(__dirname, `${name}.data`), image.data);

    assert(image);
    assert(image.data);
    assert.strictEqual(image.width, width);
    assert.strictEqual(image.height, height);

    const expected = fs.readFileSync(path.join(__dirname, `${name}.data`));
    assert.strictEqual(Buffer.compare(image.data, expected), 0);
  }));

  images.forEach(({ name, width, height }) => it(`encodes image (${name})`, async () => {
    const data = fs.readFileSync(path.join(__dirname, `${name}.data`));
    const buffer = await encodePNG(width, height, data, {});
    fs.writeFileSync(path.join(__dirname, `${name}.out.png`), buffer);
  }));

  it.skip(`read/write speed test`, async () => {
    const files = fs.readdirSync('E:\\Downloads\\test')
      .filter(f => /\.png$/.test(f))
      .map(f => fs.readFileSync(`E:\\Downloads\\test\\${f}`));
    const decoded = [];

    {
      const start = Date.now();
      for (const f of files) {
        const png = await decodePNG(f);
        decoded.push(png);
      }
      console.log(`read done in ${(Date.now() - start).toFixed(2)} ms`);
    }

    {
      const start = Date.now();
      let totalLength = 0;
      for (const f of decoded) {
        const buffer = await encodePNG(f.width, f.height, f.data, { compressionLevel: 2, filters: PNG_FILTER_NONE });
        totalLength += buffer.byteLength;
      }
      console.log(`write done in ${(Date.now() - start).toFixed(2)} ms, length: ${(totalLength / (1024 * 1024)).toFixed(1)} MB`);
    }
  })
});
