#ifndef VYP_FORMAT_H
#define VYP_FORMAT_H

#include <stdint.h>

/*
 * VYP2 archive format
 * ───────────────────
 * All file data blocks are compressed with Zstandard.
 * The index stores both compressed_size (bytes on disk) and
 * original_size (bytes after decompression).
 *
 * Layout:
 *   [VypHeader]
 *   [compressed data block 0]
 *   [compressed data block 1]
 *   ...
 *   [index: uint64_t count, (VypFileEntry + path_string) * count]
 */

#define VYP_MAGIC       "VYP2"     /* bumped from VYP1: incompatible format */
#define VYP_MAGIC_LEN   4
#define VYP_VERSION     2
#define MAX_FILES       1024
#define MAX_PATH_LEN    511
#define BUFFER_SIZE     65536

/* Zstandard compression level (1 = fastest … 22 = best).
 * Level 3 is the library default and a good all-round choice. */
#define ZSTD_COMPRESS_LEVEL 3

/* ── On-disk structures ─────────────────────────────────────────────────────
 * These are written directly to / read directly from the file.
 * Fields are native-endian (same as the original VYP1 format).
 */

typedef struct {
    char     magic[VYP_MAGIC_LEN]; /* "VYP2"                               */
    uint32_t version;              /* VYP_VERSION = 2                       */
    uint64_t index_offset;         /* byte offset of the index block        */
} VypHeader;

/*
 * Stored on disk as:   VypFileEntry  |  path_len bytes of path (no '\0')
 *
 *   path_len        — length of the path string that follows immediately
 *   offset          — byte offset of the compressed data block in the archive
 *   compressed_size — bytes stored on disk (output of ZSTD_compress)
 *   original_size   — bytes of the original file (input to ZSTD_compress)
 */
typedef struct {
    uint16_t path_len;
    uint64_t offset;
    uint64_t compressed_size;
    uint64_t original_size;
} VypFileEntry;

#endif /* VYP_FORMAT_H */
