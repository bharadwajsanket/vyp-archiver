#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../include/vyp_format.h"
#include "../include/archive.h"
#include "../include/index.h"

/* =========================================================================
 * read_index
 * ========================================================================= */

int read_index(FILE *fp, const VypHeader *header, ArchiveIndex *idx)
{
    idx->count = 0;

    if (fseek(fp, (long)header->index_offset, SEEK_SET) != 0) {
        fprintf(stderr, "error: could not seek to index\n");
        return -1;
    }

    uint64_t count = 0;
    if (fread(&count, sizeof(uint64_t), 1, fp) != 1) {
        fprintf(stderr, "error: could not read file count from index\n");
        return -1;
    }

    if (count > MAX_FILES) {
        fprintf(stderr, "error: archive index exceeds maximum file limit (%d)\n",
                MAX_FILES);
        return -1;
    }

    for (uint64_t i = 0; i < count; i++) {
        VypFileEntry entry;

        if (fread(&entry, sizeof(VypFileEntry), 1, fp) != 1) {
            fprintf(stderr, "error: could not read index entry %llu\n",
                    (unsigned long long)i);
            return -1;
        }

        if (entry.path_len == 0 || entry.path_len > MAX_PATH_LEN) {
            fprintf(stderr, "error: invalid path length in entry %llu\n",
                    (unsigned long long)i);
            return -1;
        }

        if (fread(idx->records[i].path, 1, entry.path_len, fp) != entry.path_len) {
            fprintf(stderr, "error: could not read path for entry %llu\n",
                    (unsigned long long)i);
            return -1;
        }

        idx->records[i].path[entry.path_len] = '\0';
        idx->records[i].offset          = entry.offset;
        idx->records[i].compressed_size = entry.compressed_size;
        idx->records[i].original_size   = entry.original_size;
    }

    idx->count = (int)count;
    return idx->count;
}

/* =========================================================================
 * write_index
 * ========================================================================= */

int write_index(FILE *fp, VypHeader *header, const ArchiveIndex *idx)
{
    long index_pos = ftell(fp);
    if (index_pos < 0) {
        fprintf(stderr, "error: ftell failed: %s\n", strerror(errno));
        return -1;
    }

    header->index_offset = (uint64_t)index_pos;

    uint64_t count = (uint64_t)idx->count;
    if (fwrite(&count, sizeof(uint64_t), 1, fp) != 1) {
        fprintf(stderr, "error: could not write index count\n");
        return -1;
    }

    for (int i = 0; i < idx->count; i++) {
        VypFileEntry entry;
        entry.path_len        = (uint16_t)strlen(idx->records[i].path);
        entry.offset          = idx->records[i].offset;
        entry.compressed_size = idx->records[i].compressed_size;
        entry.original_size   = idx->records[i].original_size;

        if (fwrite(&entry, sizeof(VypFileEntry), 1, fp) != 1) {
            fprintf(stderr, "error: could not write index entry %d\n", i);
            return -1;
        }

        if (fwrite(idx->records[i].path, 1, entry.path_len, fp) != entry.path_len) {
            fprintf(stderr, "error: could not write path for entry %d\n", i);
            return -1;
        }
    }

    /* Flush updated index_offset back into the header at byte 0. */
    rewind(fp);
    if (fwrite(header, sizeof(VypHeader), 1, fp) != 1) {
        fprintf(stderr, "error: could not update archive header\n");
        return -1;
    }

    return 0;
}

/* =========================================================================
 * find_file
 * ========================================================================= */

int find_file(const ArchiveIndex *idx, const char *path)
{
    for (int i = 0; i < idx->count; i++)
        if (strcmp(idx->records[i].path, path) == 0)
            return i;
    return -1;
}

/* =========================================================================
 * validate_index
 *
 * archive_size is passed as header->index_offset so that each entry's
 * compressed data block is validated to fit entirely before the index.
 * ========================================================================= */

int validate_index(const ArchiveIndex *idx, uint64_t archive_size)
{
    for (int i = 0; i < idx->count; i++) {
        const FileRecord *r = &idx->records[i];

        if (r->path[0] == '\0') {
            fprintf(stderr, "error: index entry %d has empty path\n", i);
            return -1;
        }

        if (r->offset < sizeof(VypHeader)) {
            fprintf(stderr, "error: entry '%s' has invalid offset %llu\n",
                    r->path, (unsigned long long)r->offset);
            return -1;
        }

        /* The compressed block must fit inside the data region
         * (before the index). */
        if (r->offset + r->compressed_size > archive_size) {
            fprintf(stderr,
                    "error: entry '%s' compressed data extends beyond archive bounds\n",
                    r->path);
            return -1;
        }

        /* Sanity: original size must be >= 0 (always true for uint64_t),
         * and compressed size must be > 0 for a valid ZSTD frame. */
        if (r->compressed_size == 0) {
            fprintf(stderr, "error: entry '%s' has zero compressed size\n",
                    r->path);
            return -1;
        }
    }

    return 0;
}
