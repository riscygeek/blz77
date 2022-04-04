#ifndef FILE_BLZ77_H
#define FILE_BLZ77_H
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header is at the start of every blz77-compressed file.
 */
struct blz77_header {
    uint8_t magic[6];       // 0x7f, B, L, Z, 7, 7
    uint8_t version;        // currently: 0
    uint8_t reserved;       // reserved, must be 0
    uint32_t cap_sb;        // capacity of the S buffer
    uint8_t reserved2[4];   // reserved, must be 0
};

/*
 * Compress the `input` file and write the result to `output`.
 * Arguments:
 *   input:
 *     Input file, opened with a mode of "rb".
 *   output:
 *     Compressed output file, opened with a mode of "wb+".
 *   level:
 *     Compression level preset, a number from 0 (fast) to 9 (dense).
 * Return value:
 *   0 -> Success
 *  <0 -> Failure
 * Note:
 *   This function returns errors in errno.
 */
int blz77_compress (FILE *input,
                    FILE *output,
                    int level);

/*
 * Decompress the `input` file and write the result to `output`.
 * Arguments:
 *   input:
 *     Compressed input file, opened with a mode of "rb".
 *   output:
 *     Output file, opened with a mode of "wb+".
 * Return value:
 *   0 -> Success
 *  <0 -> Failure
 * Note:
 *   This function returns errors in errno.
 */
int blz77_decompress (FILE *input,
                      FILE *output);

/*
 * Read and validate the header of a compressed file.
 * Arguments:
 *   file:
 *     Compressed input file, opened with a mode of "rb".
 *   hdr:
 *     Pointer to the header struct, can be NULL.
 * Return values:
 *   0 = Success
 *  <0 = Failure
 */
int blz77_read_header (FILE *file, struct blz77_header *hdr);

#ifdef __cplusplus
}
#endif

#endif /* FILE_BLZ77_H */
