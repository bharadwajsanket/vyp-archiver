#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include "vyp_format.h"
#include "archive.h"   /* for ArchiveIndex, FileRecord */

/* Read the index from an open archive into idx.
 * Returns number of entries on success, -1 on error. */
int  read_index(FILE *fp, const VypHeader *header, ArchiveIndex *idx);

/* Write idx to fp and update header->index_offset; rewinds to write header.
 * Returns 0 on success, -1 on error. */
int  write_index(FILE *fp, VypHeader *header, const ArchiveIndex *idx);

/* Return index of path in idx, or -1 if not found. */
int  find_file(const ArchiveIndex *idx, const char *path);

/* Validate the index against the known archive file size.
 * Returns 0 if all offsets and sizes are within bounds, -1 otherwise. */
int  validate_index(const ArchiveIndex *idx, uint64_t archive_size);

#endif /* INDEX_H */
