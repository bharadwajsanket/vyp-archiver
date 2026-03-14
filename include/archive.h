#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include "vyp_format.h"

/* ── In-memory representation of one file record ───────────────────────────
 *
 *   path             — null-terminated file path (as stored in the archive)
 *   offset           — byte offset of the compressed data block
 *   compressed_size  — bytes on disk
 *   original_size    — bytes after decompression (== original file size)
 */
typedef struct {
    char     path[MAX_PATH_LEN + 1];
    uint64_t offset;
    uint64_t compressed_size;
    uint64_t original_size;
} FileRecord;

/* In-memory index. */
typedef struct {
    FileRecord records[MAX_FILES];
    int        count;
} ArchiveIndex;

/* ── Header I/O ─────────────────────────────────────────────────────────── */
int  read_header(FILE *fp, VypHeader *header);
int  write_header(FILE *fp, const VypHeader *header);

/* ── Validation ─────────────────────────────────────────────────────────── */
int  validate_archive(const char *path);

/* ── Commands ───────────────────────────────────────────────────────────── */
int  create_archive(const char *archive);
int  list_files(const char *archive);
int  add_file(const char *file, const char *archive);
int  extract_file(const char *file, const char *archive);
int  extract_all(const char *archive);
int  delete_file(const char *file, const char *archive);
int  test_archive(const char *archive);

#endif /* ARCHIVE_H */
