const { encodePNG, decodePNG } = require('../');
const fs = require('fs');
const path = require('path');
const assert = require('assert');

describe('encodePNG', () => {
  it('throws on invalid arguments', async () => {
    await assert.rejects(() => encodePNG());
    await assert.rejects(() => encodePNG('x'));
    await assert.rejects(() => encodePNG(1, 'f'));
  });

  it('encodes image', async () => {
    const data = Buffer.from([0, 0, 0, 0]);
    const buffer = await encodePNG(1, 1, data, {});
    assert.equal(buffer.toString('base64'), 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABmJLR0QA/wD/AP+gvaeTAAAAC0lEQVQImWNgAAIAAAUAAWJVMogAAAAASUVORK5CYII=');
  });
  
  it(`doesn't crash on bad file`, async () => {
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
  ];

  images.forEach(({ name, width, height }) => it(`decodes image (${name})`, async () => {
    const png = fs.readFileSync(path.join(__dirname, `${name}.png`));
    const image = await decodePNG(png);

    fs.writeFileSync(path.join(__dirname, `${name}.data`), image.data);

    assert(image);
    assert(image.data);
    assert.equal(image.width, width);
    assert.equal(image.height, height);

    const expected = fs.readFileSync(path.join(__dirname, `${name}.data`));
    assert.equal(Buffer.compare(image.data, expected), 0);
  }));

  images.forEach(({ name, width, height }) => it(`encodes image (${name})`, async () => {
    const data = fs.readFileSync(path.join(__dirname, `${name}.data`));
    const buffer = await encodePNG(width, height, data, {});
    fs.writeFileSync(path.join(__dirname, `${name}.out.png`), buffer);
  }));
});
