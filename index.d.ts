/** Constant used in PNG encoding methods. */
export const PNG_NO_FILTERS: number;
/** Constant used in PNG encoding methods. */
export const PNG_ALL_FILTERS: number;
/** Constant used in PNG encoding methods. */
export const PNG_FILTER_NONE: number;
/** Constant used in PNG encoding methods. */
export const PNG_FILTER_SUB: number;
/** Constant used in PNG encoding methods. */
export const PNG_FILTER_UP: number;
/** Constant used in PNG encoding methods. */
export const PNG_FILTER_AVG: number;
/** Constant used in PNG encoding methods. */
export const PNG_FILTER_PAETH: number;

export interface PngConfig {
	/** Specifies the ZLIB compression level. Defaults to 6. */
	compressionLevel?: 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9;
	/**
	 * Any bitwise combination of `PNG_FILTER_NONE`, `PNG_FITLER_SUB`,
	 * `PNG_FILTER_UP`, `PNG_FILTER_AVG` and `PNG_FILTER_PATETH`; or one of
	 * `PNG_ALL_FILTERS` or `PNG_NO_FILTERS`.
	 * These specify which filters *may* be used by libpng. During
	 * encoding, libpng will select the best filter from this list of allowed
	 * filters. Defaults to `PNG_ALL_FITLERS`.
	 */
	filters?: number;
	/**
	 * _For creating indexed PNGs._ The palette of colors. Entries should be in
	 * RGBA order.
	 */
	palette?: Uint8ClampedArray;
	/**
	 * _For creating indexed PNGs._ The index of the background color. Defaults
	 * to 0.
	 */
	backgroundIndex?: number;
	/** pixels per inch */
	resolution?: number;
}

export interface DecodedImageData {
	width: number;
	height: number;
	data: Buffer;
}

export interface DecodeOptions {
	premultipled: boolean;
}

export function encodePNG(width: number, height: number, data: Buffer, options?: PngConfig): Promise<Buffer>;
export function decodePNG(data: Buffer, options?: DecodeOptions): Promise<DecodedImageData>;
