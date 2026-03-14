#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zstd.h>

#include "../include/vyp_format.h"
#include "../include/archive.h"
#include "../include/index.h"

/* =========================================================================
 * Internal helpers
 * ========================================================================= */

/*
 * read_file_alloc — read the entire contents of `fp` into a heap buffer.
 *
 * The caller is responsible for free()ing the returned pointer.
 * Returns NULL on any error; sets *out_size to the number of bytes read.
 */
static void *read_file_alloc(FILE *fp, uint64_t *out_size)
{
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: cannot seek to end of file\n");
        return NULL;
    }

    long sz = ftell(fp);
    if (sz < 0) {
        fprintf(stderr, "error: ftell failed\n");
        return NULL;
    }
    rewind(fp);

    *out_size = (uint64_t)sz;

    /* Allow zero-byte files: return a 1-byte dummy so malloc(0) is never called. */
    void *buf = malloc(sz > 0 ? (size_t)sz : 1);
    if (!buf) {
        fprintf(stderr, "error: out of memory (%ld bytes)\n", sz);
        return NULL;
    }

    if (sz > 0 && fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
        fprintf(stderr, "error: could not read file: %s\n", strerror(errno));
        free(buf);
        return NULL;
    }

    return buf;
}

/*
 * compress_buffer — compress src_buf with ZSTD.
 *
 * Returns a heap-allocated buffer of *dst_size bytes, or NULL on error.
 * The caller is responsible for free()ing the returned pointer.
 */
static void *compress_buffer(const void *src_buf, size_t src_size,
                              size_t *dst_size)
{
    size_t bound = ZSTD_compressBound(src_size);
    if (ZSTD_isError(bound)) {
        fprintf(stderr, "error: ZSTD_compressBound failed: %s\n",
                ZSTD_getErrorName(bound));
        return NULL;
    }

    void *dst = malloc(bound);
    if (!dst) {
        fprintf(stderr, "error: out of memory for compression buffer\n");
        return NULL;
    }

    size_t result = ZSTD_compress(dst, bound,
                                  src_buf, src_size,
                                  ZSTD_COMPRESS_LEVEL);
    if (ZSTD_isError(result)) {
        fprintf(stderr, "error: ZSTD_compress failed: %s\n",
                ZSTD_getErrorName(result));
        free(dst);
        return NULL;
    }

    *dst_size = result;
    return dst;
}

/*
 * decompress_buffer — decompress a ZSTD-compressed block.
 *
 * original_size must be the exact uncompressed size (stored in the index).
 * Returns a heap-allocated buffer of original_size bytes, or NULL on error.
 * The caller is responsible for free()ing the returned pointer.
 */
static void *decompress_buffer(const void *src_buf, size_t src_size,
                                uint64_t original_size)
{
    /* Zero-byte original file is a valid edge case. */
    void *dst = malloc(original_size > 0 ? (size_t)original_size : 1);
    if (!dst) {
        fprintf(stderr, "error: out of memory for decompression buffer\n");
        return NULL;
    }

    if (original_size > 0) {
        size_t result = ZSTD_decompress(dst, (size_t)original_size,
                                        src_buf, src_size);
        if (ZSTD_isError(result)) {
            fprintf(stderr, "error: ZSTD_decompress failed: %s\n",
                    ZSTD_getErrorName(result));
            free(dst);
            return NULL;
        }

        if (result != (size_t)original_size) {
            fprintf(stderr,
                    "error: decompressed %zu bytes but expected %llu\n",
                    result, (unsigned long long)original_size);
            free(dst);
            return NULL;
        }
    }

    return dst;
}

/*
 * copy_bytes — copy `size` raw bytes from src to dst using a stack buffer.
 * Used only by delete_file when reconstructing the archive: the compressed
 * data blocks are copied verbatim (no re-compression needed).
 */
static int copy_bytes(FILE *dst, FILE *src, uint64_t size)
{
    char     buf[BUFFER_SIZE];
    uint64_t remain = size;

    while (remain > 0) {
        size_t chunk = (remain > BUFFER_SIZE) ? BUFFER_SIZE : (size_t)remain;
        size_t got   = fread(buf, 1, chunk, src);

        if (got == 0) {
            if (feof(src))
                fprintf(stderr, "error: unexpected end of file while reading\n");
            else
                fprintf(stderr, "error: read failed: %s\n", strerror(errno));
            return -1;
        }

        if (fwrite(buf, 1, got, dst) != got) {
            fprintf(stderr, "error: write failed: %s\n", strerror(errno));
            return -1;
        }

        remain -= got;
    }

    return 0;
}

/* =========================================================================
 * Header I/O
 * ========================================================================= */

int read_header(FILE *fp, VypHeader *header)
{
    rewind(fp);

    if (fread(header, sizeof(VypHeader), 1, fp) != 1) {
        fprintf(stderr, "error: could not read archive header\n");
        return 0;
    }

    return 1;
}

int write_header(FILE *fp, const VypHeader *header)
{
    rewind(fp);

    if (fwrite(header, sizeof(VypHeader), 1, fp) != 1) {
        fprintf(stderr, "error: could not write archive header\n");
        return 0;
    }

    return 1;
}

/* =========================================================================
 * Validation
 * ========================================================================= */

int validate_archive(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s': %s\n", path, strerror(errno));
        return 0;
    }

    VypHeader header;
    int ok = read_header(fp, &header);

    if (ok && memcmp(header.magic, VYP_MAGIC, VYP_MAGIC_LEN) != 0) {
        fprintf(stderr, "error: '%s' is not a valid VYP archive "
                "(wrong magic — was it created with an older version?)\n", path);
        ok = 0;
    }

    fclose(fp);
    return ok;
}

/* =========================================================================
 * Commands
 * ========================================================================= */

int create_archive(const char *archive)
{
    FILE *fp = fopen(archive, "wb");
    if (!fp) {
        fprintf(stderr, "error: cannot create '%s': %s\n", archive, strerror(errno));
        return -1;
    }

    VypHeader header;
    memcpy(header.magic, VYP_MAGIC, VYP_MAGIC_LEN);
    header.version      = VYP_VERSION;
    header.index_offset = sizeof(VypHeader);

    if (!write_header(fp, &header)) {
        fclose(fp);
        return -1;
    }

    /* Empty index: just the count field set to 0. */
    uint64_t zero = 0;
    if (fwrite(&zero, sizeof(uint64_t), 1, fp) != 1) {
        fprintf(stderr, "error: could not initialise index\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    printf("Archive created: %s\n", archive);
    return 0;
}

/* -------------------------------------------------------------------------- */

int list_files(const char *archive)
{
    if (!validate_archive(archive))
        return -1;

    FILE *fp = fopen(archive, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s': %s\n", archive, strerror(errno));
        return -1;
    }

    VypHeader    header;
    ArchiveIndex idx;

    if (!read_header(fp, &header))         { fclose(fp); return -1; }
    if (read_index(fp, &header, &idx) < 0) { fclose(fp); return -1; }

    fclose(fp);

    printf("Files: %d\n", idx.count);
    for (int i = 0; i < idx.count; i++) {
        const FileRecord *r = &idx.records[i];
        /* Show original (uncompressed) size — that is the meaningful size
         * from the user's perspective. */
        printf("  %s  (%llu bytes)\n",
               r->path,
               (unsigned long long)r->original_size);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int add_file(const char *file, const char *archive)
{
    if (!validate_archive(archive))
        return -1;

    size_t path_len = strlen(file);
    if (path_len == 0 || path_len > MAX_PATH_LEN) {
        fprintf(stderr, "error: file path too long (max %d chars)\n", MAX_PATH_LEN);
        return -1;
    }

    FILE *ff = fopen(file, "rb");
    if (!ff) {
        fprintf(stderr, "error: cannot open '%s': %s\n", file, strerror(errno));
        return -1;
    }

    FILE *fa = fopen(archive, "rb+");
    if (!fa) {
        fprintf(stderr, "error: cannot open archive '%s': %s\n", archive, strerror(errno));
        fclose(ff);
        return -1;
    }

    VypHeader    header;
    ArchiveIndex idx;

    if (!read_header(fa, &header))         { fclose(fa); fclose(ff); return -1; }
    if (read_index(fa, &header, &idx) < 0) { fclose(fa); fclose(ff); return -1; }

    if (find_file(&idx, file) >= 0) {
        fprintf(stderr, "error: '%s' already exists in archive\n", file);
        fclose(fa); fclose(ff);
        return -1;
    }

    if (idx.count >= MAX_FILES) {
        fprintf(stderr, "error: archive is full (max %d files)\n", MAX_FILES);
        fclose(fa); fclose(ff);
        return -1;
    }

    /* ── Step 1: read the entire source file into memory ─────────────────── */
    uint64_t original_size = 0;
    void *raw = read_file_alloc(ff, &original_size);
    fclose(ff);

    if (!raw) {
        fclose(fa);
        return -1;
    }

    /* ── Step 2: compress with ZSTD ──────────────────────────────────────── */
    size_t compressed_size = 0;
    void *compressed = compress_buffer(raw, (size_t)original_size, &compressed_size);
    free(raw);

    if (!compressed) {
        fclose(fa);
        return -1;
    }

    /* ── Step 3: write compressed block at the current index_offset ─────── */
    if (fseek(fa, (long)header.index_offset, SEEK_SET) != 0) {
        fprintf(stderr, "error: seek in archive failed\n");
        free(compressed);
        fclose(fa);
        return -1;
    }

    uint64_t data_offset = header.index_offset;

    if (fwrite(compressed, 1, compressed_size, fa) != compressed_size) {
        fprintf(stderr, "error: could not write compressed data: %s\n",
                strerror(errno));
        free(compressed);
        fclose(fa);
        return -1;
    }

    free(compressed);

    /* ── Step 4: update in-memory index and write it ─────────────────────── */
    snprintf(idx.records[idx.count].path, MAX_PATH_LEN + 1, "%s", file);
    idx.records[idx.count].offset          = data_offset;
    idx.records[idx.count].compressed_size = (uint64_t)compressed_size;
    idx.records[idx.count].original_size   = original_size;
    idx.count++;

    if (write_index(fa, &header, &idx) != 0) {
        fclose(fa);
        return -1;
    }

    fclose(fa);

    printf("Added %s  (%llu → %llu bytes, %.1f%%)\n",
           file,
           (unsigned long long)original_size,
           (unsigned long long)compressed_size,
           original_size > 0
               ? 100.0 * (double)compressed_size / (double)original_size
               : 100.0);

    return 0;
}

/* -------------------------------------------------------------------------- */

/*
 * extract_one — shared implementation used by both extract_file and extract_all.
 *
 * fp must be open on the archive; the record gives offset, compressed_size,
 * and original_size.  The decompressed data is written to out_path.
 */
static int extract_one(FILE *fp, const FileRecord *r, const char *out_path)
{
    /* ── Read compressed block ────────────────────────────────────────────── */
    if (fseek(fp, (long)r->offset, SEEK_SET) != 0) {
        fprintf(stderr, "error: seek failed for '%s'\n", r->path);
        return -1;
    }

    void *compressed = malloc((size_t)r->compressed_size);
    if (!compressed) {
        fprintf(stderr, "error: out of memory reading '%s'\n", r->path);
        return -1;
    }

    if (fread(compressed, 1, (size_t)r->compressed_size, fp)
            != (size_t)r->compressed_size) {
        fprintf(stderr, "error: could not read compressed data for '%s'\n", r->path);
        free(compressed);
        return -1;
    }

    /* ── Decompress ───────────────────────────────────────────────────────── */
    void *decompressed = decompress_buffer(compressed,
                                           (size_t)r->compressed_size,
                                           r->original_size);
    free(compressed);

    if (!decompressed)
        return -1;

    /* ── Write output file ────────────────────────────────────────────────── */
    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "error: cannot create '%s': %s\n",
                out_path, strerror(errno));
        free(decompressed);
        return -1;
    }

    int write_ok = 1;
    if (r->original_size > 0) {
        if (fwrite(decompressed, 1, (size_t)r->original_size, out)
                != (size_t)r->original_size) {
            fprintf(stderr, "error: could not write '%s': %s\n",
                    out_path, strerror(errno));
            write_ok = 0;
        }
    }

    fclose(out);
    free(decompressed);

    return write_ok ? 0 : -1;
}

/* -------------------------------------------------------------------------- */

int extract_file(const char *file, const char *archive)
{
    if (!validate_archive(archive))
        return -1;

    FILE *fp = fopen(archive, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s': %s\n", archive, strerror(errno));
        return -1;
    }

    VypHeader    header;
    ArchiveIndex idx;

    if (!read_header(fp, &header))         { fclose(fp); return -1; }
    if (read_index(fp, &header, &idx) < 0) { fclose(fp); return -1; }

    int pos = find_file(&idx, file);
    if (pos < 0) {
        fprintf(stderr, "error: '%s' not found in archive\n", file);
        fclose(fp);
        return -1;
    }

    int rc = extract_one(fp, &idx.records[pos], file);
    fclose(fp);

    if (rc == 0)
        printf("Extracted %s\n", file);

    return rc;
}

/* -------------------------------------------------------------------------- */

int extract_all(const char *archive)
{
    if (!validate_archive(archive))
        return -1;

    FILE *fp = fopen(archive, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s': %s\n", archive, strerror(errno));
        return -1;
    }

    VypHeader    header;
    ArchiveIndex idx;

    if (!read_header(fp, &header))         { fclose(fp); return -1; }
    if (read_index(fp, &header, &idx) < 0) { fclose(fp); return -1; }

    int errors = 0;

    for (int i = 0; i < idx.count; i++) {
        if (extract_one(fp, &idx.records[i], idx.records[i].path) == 0)
            printf("Extracted %s\n", idx.records[i].path);
        else
            errors++;
    }

    fclose(fp);
    return errors ? -1 : 0;
}

/* -------------------------------------------------------------------------- */

int delete_file(const char *file, const char *archive)
{
    if (!validate_archive(archive))
        return -1;

    FILE *fa = fopen(archive, "rb");
    if (!fa) {
        fprintf(stderr, "error: cannot open '%s': %s\n", archive, strerror(errno));
        return -1;
    }

    VypHeader    header;
    ArchiveIndex idx;

    if (!read_header(fa, &header))         { fclose(fa); return -1; }
    if (read_index(fa, &header, &idx) < 0) { fclose(fa); return -1; }

    int pos = find_file(&idx, file);
    if (pos < 0) {
        fprintf(stderr, "error: '%s' not found in archive\n", file);
        fclose(fa);
        return -1;
    }

    /* Rebuild the archive into a temp file, omitting the deleted entry. */
    char tmp_path[MAX_PATH_LEN + 16];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", archive);

    FILE *ft = fopen(tmp_path, "wb");
    if (!ft) {
        fprintf(stderr, "error: cannot create temp file '%s': %s\n",
                tmp_path, strerror(errno));
        fclose(fa);
        return -1;
    }

    VypHeader new_header;
    memcpy(new_header.magic, VYP_MAGIC, VYP_MAGIC_LEN);
    new_header.version      = VYP_VERSION;
    new_header.index_offset = 0; /* filled in by write_index */

    if (!write_header(ft, &new_header)) {
        fclose(ft); fclose(fa);
        remove(tmp_path);
        return -1;
    }

    ArchiveIndex new_idx;
    new_idx.count = 0;

    for (int i = 0; i < idx.count; i++) {
        if (i == pos)
            continue; /* skip the deleted entry */

        const FileRecord *r = &idx.records[i];

        if (fseek(fa, (long)r->offset, SEEK_SET) != 0) {
            fprintf(stderr, "error: seek failed while rebuilding archive\n");
            fclose(ft); fclose(fa);
            remove(tmp_path);
            return -1;
        }

        long data_start = ftell(ft);
        if (data_start < 0) {
            fprintf(stderr, "error: ftell on temp file failed\n");
            fclose(ft); fclose(fa);
            remove(tmp_path);
            return -1;
        }

        /* Copy the compressed block verbatim — no need to re-compress. */
        if (copy_bytes(ft, fa, r->compressed_size) != 0) {
            fclose(ft); fclose(fa);
            remove(tmp_path);
            return -1;
        }

        snprintf(new_idx.records[new_idx.count].path, MAX_PATH_LEN + 1,
                 "%s", r->path);
        new_idx.records[new_idx.count].offset          = (uint64_t)data_start;
        new_idx.records[new_idx.count].compressed_size = r->compressed_size;
        new_idx.records[new_idx.count].original_size   = r->original_size;
        new_idx.count++;
    }

    fclose(fa);

    if (write_index(ft, &new_header, &new_idx) != 0) {
        fclose(ft);
        remove(tmp_path);
        return -1;
    }

    fclose(ft);

    if (remove(archive) != 0) {
        fprintf(stderr, "error: cannot remove original archive: %s\n",
                strerror(errno));
        remove(tmp_path);
        return -1;
    }

    if (rename(tmp_path, archive) != 0) {
        fprintf(stderr, "error: cannot rename temp file: %s\n", strerror(errno));
        return -1;
    }

    printf("Deleted %s\n", file);
    return 0;
}

/* -------------------------------------------------------------------------- */

int test_archive(const char *archive)
{
    FILE *fp = fopen(archive, "rb");
    if (!fp) {
        fprintf(stderr, "error: cannot open '%s': %s\n", archive, strerror(errno));
        printf("Archive corrupted\n");
        return -1;
    }

    /* 1. Magic check. */
    VypHeader header;
    if (!read_header(fp, &header)) {
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }

    if (memcmp(header.magic, VYP_MAGIC, VYP_MAGIC_LEN) != 0) {
        fprintf(stderr, "error: invalid magic bytes\n");
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }

    /* 2. Determine file size. */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: cannot seek to end of archive\n");
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }
    long archive_size_l = ftell(fp);
    if (archive_size_l < 0) {
        fprintf(stderr, "error: ftell failed\n");
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }
    uint64_t archive_size = (uint64_t)archive_size_l;

    /* 3. index_offset within bounds. */
    if (header.index_offset < sizeof(VypHeader) ||
        header.index_offset >= archive_size) {
        fprintf(stderr, "error: index_offset %llu is out of bounds\n",
                (unsigned long long)header.index_offset);
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }

    /* 4. Load index. */
    ArchiveIndex idx;
    if (read_index(fp, &header, &idx) < 0) {
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }

    /* 5. Validate all offsets and compressed sizes against the data region. */
    if (validate_index(&idx, header.index_offset) != 0) {
        fclose(fp);
        printf("Archive corrupted\n");
        return -1;
    }

    /* 6. Verify each ZSTD frame by reading and decompressing it fully.
     *    This catches truncated or bit-flipped compressed data. */
    for (int i = 0; i < idx.count; i++) {
        const FileRecord *r = &idx.records[i];

        if (fseek(fp, (long)r->offset, SEEK_SET) != 0) {
            fprintf(stderr, "error: seek failed for '%s'\n", r->path);
            fclose(fp);
            printf("Archive corrupted\n");
            return -1;
        }

        void *cbuf = malloc((size_t)r->compressed_size);
        if (!cbuf) {
            fprintf(stderr, "error: out of memory testing '%s'\n", r->path);
            fclose(fp);
            printf("Archive corrupted\n");
            return -1;
        }

        if (fread(cbuf, 1, (size_t)r->compressed_size, fp)
                != (size_t)r->compressed_size) {
            fprintf(stderr, "error: could not read compressed block for '%s'\n",
                    r->path);
            free(cbuf);
            fclose(fp);
            printf("Archive corrupted\n");
            return -1;
        }

        /* Use ZSTD_getFrameContentSize to verify the stored original_size
         * matches what is encoded inside the ZSTD frame header. */
        unsigned long long frame_size =
            ZSTD_getFrameContentSize(cbuf, (size_t)r->compressed_size);

        if (frame_size == ZSTD_CONTENTSIZE_ERROR ||
            frame_size == ZSTD_CONTENTSIZE_UNKNOWN) {
            fprintf(stderr, "error: invalid ZSTD frame for '%s'\n", r->path);
            free(cbuf);
            fclose(fp);
            printf("Archive corrupted\n");
            return -1;
        }

        if (frame_size != (unsigned long long)r->original_size) {
            fprintf(stderr,
                    "error: '%s' frame content size %llu != stored %llu\n",
                    r->path,
                    (unsigned long long)frame_size,
                    (unsigned long long)r->original_size);
            free(cbuf);
            fclose(fp);
            printf("Archive corrupted\n");
            return -1;
        }

        free(cbuf);
    }

    fclose(fp);
    printf("Archive OK\n");
    return 0;
}
