const { decodeWebP, decodePNG, decode, encodePNG } = require('../');
const fs = require('fs');
const path = require('path');
const assert = require('assert');

describe('decodeWebP', () => {
  it('throws on invalid arguments', async () => {
    await assert.rejects(() => decodeWebP());
    await assert.rejects(() => decodeWebP('x'));
    await assert.rejects(() => decodeWebP(Buffer.from([0]), 'not bool'));
  });

  it('rejects bad file', async () => {
    await assert.rejects(() => decodeWebP(Buffer.from([0, 1, 2, 3])));
  });

  it('rejects non-webp file', async () => {
    const png = fs.readFileSync(path.join(__dirname, 'rgba.png'));
    await assert.rejects(() => decodeWebP(png));
  });

  it('rejects truncated webp', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'rgba.lossless.webp'));
    await assert.rejects(() => decodeWebP(webp.subarray(0, 20)));
  });

  it('rejects VP8X (extended WebP) with a clear error', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'vp8x.webp'));
    await assert.rejects(() => decodeWebP(webp), /VP8X/);
  });

  it('decodes lossy webp (VP8)', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'rgb.lossy.webp'));
    const image = await decodeWebP(webp);
    assert(image);
    assert(image.data);
    assert.strictEqual(image.width, 32);
    assert.strictEqual(image.height, 32);
    assert.strictEqual(image.data.length, 32 * 32 * 4);
    // Verify pixels are not all zero (actual image data was decoded)
    const sum = image.data.reduce((a, b) => a + b, 0);
    assert(sum > 0, 'decoded pixel data should not be all zeros');
  });

  it('decodes lossless webp (VP8L)', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'rgba.lossless.webp'));
    const image = await decodeWebP(webp);
    assert(image);
    assert(image.data);
    assert.strictEqual(image.width, 32);
    assert.strictEqual(image.height, 32);
    assert.strictEqual(image.data.length, 32 * 32 * 4);
  });

  it('decodes lossless webp with alpha', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'alpha.lossless.webp'));
    const image = await decodeWebP(webp);
    assert(image);
    assert(image.data);
    assert.strictEqual(image.width, 128);
    assert.strictEqual(image.height, 128);
    assert.strictEqual(image.data.length, 128 * 128 * 4);
  });

  it('lossless round-trips PNG pixel values exactly (where alpha > 0)', async () => {
    // Decode the gradient PNG to get reference pixels, then decode
    // the same image as lossless WebP and compare.
    // RGB values for fully transparent pixels (alpha=0) are undefined,
    // so we only compare pixels with alpha > 0.
    const png = fs.readFileSync(path.join(__dirname, 'alpha_gradient.png'));
    const webp = fs.readFileSync(path.join(__dirname, 'alpha_gradient.lossless.webp'));
    const fromPng = await decodePNG(png);
    const fromWebp = await decodeWebP(webp);
    assert.strictEqual(fromPng.width, fromWebp.width);
    assert.strictEqual(fromPng.height, fromWebp.height);
    const pixels = fromPng.width * fromPng.height;
    for (let i = 0; i < pixels; i++) {
      const off = i * 4;
      const a = fromPng.data[off + 3];
      assert.strictEqual(fromWebp.data[off + 3], a, `pixel ${i}: alpha mismatch`);
      if (a > 0) {
        assert.strictEqual(fromWebp.data[off], fromPng.data[off], `pixel ${i}: R mismatch`);
        assert.strictEqual(fromWebp.data[off + 1], fromPng.data[off + 1], `pixel ${i}: G mismatch`);
        assert.strictEqual(fromWebp.data[off + 2], fromPng.data[off + 2], `pixel ${i}: B mismatch`);
      }
    }
  });
});

describe('premultiplied alpha correctness', () => {
  // Use alpha_gradient image: 16x16, R=x*17, G=y*17, B=(x+y)*8, A=(15-y)*17
  // giving alpha values from 255 (top row) to 0 (bottom row)

  it('WebP: premul channels equal round(straight * alpha / 255)', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'alpha_gradient.lossless.webp'));
    const straight = await decodeWebP(webp, { premultiplied: false });
    const premul = await decodeWebP(webp, { premultiplied: true });

    assert.strictEqual(straight.width, premul.width);
    assert.strictEqual(straight.height, premul.height);

    const pixels = straight.width * straight.height;
    let checked = 0;
    for (let i = 0; i < pixels; i++) {
      const off = i * 4;
      const a = straight.data[off + 3];
      // Alpha channel itself must be identical in both modes
      assert.strictEqual(premul.data[off + 3], a, `pixel ${i}: alpha mismatch`);

      if (a === 0) {
        // Fully transparent: premul RGB should be 0
        assert.strictEqual(premul.data[off], 0, `pixel ${i}: premul R should be 0 for alpha=0`);
        assert.strictEqual(premul.data[off + 1], 0, `pixel ${i}: premul G should be 0 for alpha=0`);
        assert.strictEqual(premul.data[off + 2], 0, `pixel ${i}: premul B should be 0 for alpha=0`);
        checked++;
      } else if (a < 255) {
        // Semi-transparent: verify premul = round(straight * a / 255) within ±1
        for (let c = 0; c < 3; c++) {
          const expected = Math.round(straight.data[off + c] * a / 255);
          const actual = premul.data[off + c];
          assert(Math.abs(actual - expected) <= 1,
            `pixel ${i} channel ${c}: expected ~${expected}, got ${actual} (straight=${straight.data[off + c]}, alpha=${a})`);
        }
        checked++;
      } else {
        // Fully opaque: buffers should be identical
        assert.strictEqual(premul.data[off], straight.data[off]);
        assert.strictEqual(premul.data[off + 1], straight.data[off + 1]);
        assert.strictEqual(premul.data[off + 2], straight.data[off + 2]);
        checked++;
      }
    }
    assert.strictEqual(checked, pixels, 'all pixels should be checked');
  });

  it('PNG: premul channels equal round(straight * alpha / 255)', async () => {
    const png = fs.readFileSync(path.join(__dirname, 'alpha_gradient.png'));
    const straight = await decodePNG(png, { premultiplied: false });
    const premul = await decodePNG(png, { premultiplied: true });

    const pixels = straight.width * straight.height;
    let checked = 0;
    for (let i = 0; i < pixels; i++) {
      const off = i * 4;
      const a = straight.data[off + 3];
      assert.strictEqual(premul.data[off + 3], a, `pixel ${i}: alpha mismatch`);

      if (a === 0) {
        assert.strictEqual(premul.data[off], 0);
        assert.strictEqual(premul.data[off + 1], 0);
        assert.strictEqual(premul.data[off + 2], 0);
        checked++;
      } else if (a < 255) {
        for (let c = 0; c < 3; c++) {
          const expected = Math.round(straight.data[off + c] * a / 255);
          const actual = premul.data[off + c];
          assert(Math.abs(actual - expected) <= 1,
            `pixel ${i} channel ${c}: expected ~${expected}, got ${actual} (straight=${straight.data[off + c]}, alpha=${a})`);
        }
        checked++;
      } else {
        assert.strictEqual(premul.data[off], straight.data[off]);
        assert.strictEqual(premul.data[off + 1], straight.data[off + 1]);
        assert.strictEqual(premul.data[off + 2], straight.data[off + 2]);
        checked++;
      }
    }
    assert.strictEqual(checked, pixels);
  });

  it('PNG and WebP produce identical premul output for same image', async () => {
    const png = fs.readFileSync(path.join(__dirname, 'alpha_gradient.png'));
    const webp = fs.readFileSync(path.join(__dirname, 'alpha_gradient.lossless.webp'));
    const fromPng = await decodePNG(png, { premultiplied: true });
    const fromWebp = await decodeWebP(webp, { premultiplied: true });
    assert.strictEqual(Buffer.compare(fromPng.data, fromWebp.data), 0,
      'premul output should be identical across decoders for the same image');
  });
});

describe('decode (auto-detect)', () => {
  it('rejects unsupported format', async () => {
    await assert.rejects(() => decode(Buffer.from([0, 1, 2, 3])), /Unsupported image format/);
  });

  it('rejects empty buffer', async () => {
    await assert.rejects(() => decode(Buffer.alloc(0)), /Unsupported image format/);
  });

  it('rejects VP8X through auto-detect with a clear error', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'vp8x.webp'));
    await assert.rejects(() => decode(webp), /VP8X/);
  });

  it('auto-detects and decodes PNG', async () => {
    const png = fs.readFileSync(path.join(__dirname, 'rgba.png'));
    const image = await decode(png);
    assert(image);
    assert.strictEqual(image.width, 32);
    assert.strictEqual(image.height, 32);
  });

  it('auto-detects and decodes WebP', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'rgba.lossless.webp'));
    const image = await decode(webp);
    assert(image);
    assert.strictEqual(image.width, 32);
    assert.strictEqual(image.height, 32);
  });

  it('passes decode options through', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'alpha.lossless.webp'));
    const image = await decode(webp, { premultiplied: true });
    assert.strictEqual(image.premultiplied, true);
  });

  it('produces same result as calling decodePNG directly', async () => {
    const png = fs.readFileSync(path.join(__dirname, 'rgba.png'));
    const fromDecode = await decode(png);
    const fromDecodePNG = await decodePNG(png);
    assert.strictEqual(Buffer.compare(fromDecode.data, fromDecodePNG.data), 0);
  });

  it('produces same result as calling decodeWebP directly', async () => {
    const webp = fs.readFileSync(path.join(__dirname, 'rgba.lossless.webp'));
    const fromDecode = await decode(webp);
    const fromDecodeWebP = await decodeWebP(webp);
    assert.strictEqual(Buffer.compare(fromDecode.data, fromDecodeWebP.data), 0);
  });
});
