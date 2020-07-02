# ag-images

Library for loading and saving PNG files

```
npm install ag-images
```

```ts
// main functions
export function encodePNG(width: number, height: number, data: Buffer, options?: PngConfig): Promise<Buffer>;
export function decodePNG(data: Buffer): Promise<DecodedImageData>;

export interface DecodedImageData {
	width: number;
	height: number;
	data: Buffer;
}

export interface PngConfig {
	compressionLevel?: 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9;
	filters?: number;
	palette?: Uint8ClampedArray;
	backgroundIndex?: number;
	resolution?: number;
}

// filters constants
export const PNG_NO_FILTERS: number;
export const PNG_ALL_FILTERS: number;
export const PNG_FILTER_NONE: number;
export const PNG_FILTER_SUB: number;
export const PNG_FILTER_UP: number;
export const PNG_FILTER_AVG: number;
export const PNG_FILTER_PAETH: number;
```
