#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/vyp_format.h"

#define MAX_FILES 1024

typedef struct {
    char path[512];
    uint64_t offset;
    uint64_t size;
} FileRecord;

FileRecord records[MAX_FILES];

void usage() {
    printf("vypack commands:\n");
    printf("  init <archive.vyp>\n");
    printf("  list <archive.vyp>\n");
    printf("  add <file> <archive.vyp>\n");
    printf("  extract <file> <archive.vyp>\n");
    printf("  extractall <archive.vyp>\n");
}

int read_header(FILE *fp, VypHeader *header) {

    fread(header, sizeof(VypHeader), 1, fp);

    if (memcmp(header->magic, VYP_MAGIC, 4) != 0)
        return 0;

    return 1;
}

int load_index(FILE *fp, VypHeader *header) {

    fseek(fp, header->index_offset, SEEK_SET);

    uint64_t count;
    fread(&count, sizeof(uint64_t), 1, fp);

    for (uint64_t i = 0; i < count; i++) {

        VypFileEntry entry;
        fread(&entry, sizeof(entry), 1, fp);

        fread(records[i].path, 1, entry.path_len, fp);
        records[i].path[entry.path_len] = '\0';

        records[i].offset = entry.offset;
        records[i].size = entry.size;
    }

    return count;
}

void write_index(FILE *fp, VypHeader *header, int count) {

    header->index_offset = ftell(fp);

    fwrite(&count, sizeof(uint64_t), 1, fp);

    for (int i = 0; i < count; i++) {

        VypFileEntry entry;

        entry.path_len = strlen(records[i].path);
        entry.offset = records[i].offset;
        entry.size = records[i].size;

        fwrite(&entry, sizeof(entry), 1, fp);
        fwrite(records[i].path, 1, entry.path_len, fp);
    }

    fseek(fp, 0, SEEK_SET);
    fwrite(header, sizeof(VypHeader), 1, fp);
}

void init_archive(const char *archive) {

    FILE *fp = fopen(archive, "wb");

    if (!fp) return;

    VypHeader header;

    memcpy(header.magic, VYP_MAGIC, 4);
    header.version = 1;
    header.index_offset = sizeof(VypHeader);

    fwrite(&header, sizeof(header), 1, fp);

    uint64_t zero = 0;
    fwrite(&zero, sizeof(uint64_t), 1, fp);

    fclose(fp);

    printf("Archive created\n");
}

void list_archive(const char *archive) {

    FILE *fp = fopen(archive, "rb");

    if (!fp) return;

    VypHeader header;

    if (!read_header(fp, &header)) {
        printf("Invalid archive\n");
        return;
    }

    int count = load_index(fp, &header);

    printf("Files: %d\n", count);

    for (int i = 0; i < count; i++)
        printf("%s\n", records[i].path);

    fclose(fp);
}

void add_file(const char *file, const char *archive) {

    FILE *fa = fopen(archive, "rb+");
    FILE *ff = fopen(file, "rb");

    if (!fa || !ff) {
        printf("File error\n");
        return;
    }

    VypHeader header;
    read_header(fa, &header);

    int count = load_index(fa, &header);

    for (int i = 0; i < count; i++) {
        if (!strcmp(records[i].path, file)) {
            printf("File already exists\n");
            return;
        }
    }

    fseek(fa, header.index_offset, SEEK_SET);

    uint64_t offset = header.index_offset;

    fseek(ff, 0, SEEK_END);
    uint64_t size = ftell(ff);
    fseek(ff, 0, SEEK_SET);

    char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer,1,4096,ff)) > 0)
        fwrite(buffer,1,bytes,fa);

    fclose(ff);

    strcpy(records[count].path, file);
    records[count].offset = offset;
    records[count].size = size;

    count++;

    write_index(fa, &header, count);

    fclose(fa);

    printf("Added %s\n", file);
}

void extract_file(const char *file, const char *archive) {

    FILE *fp = fopen(archive, "rb");

    if (!fp) return;

    VypHeader header;
    read_header(fp, &header);

    int count = load_index(fp, &header);

    for (int i = 0; i < count; i++) {

        if (!strcmp(records[i].path, file)) {

            FILE *out = fopen(file, "wb");

            fseek(fp, records[i].offset, SEEK_SET);

            char buffer[4096];
            uint64_t remain = records[i].size;

            while (remain > 0) {

                size_t chunk = remain > 4096 ? 4096 : remain;

                fread(buffer,1,chunk,fp);
                fwrite(buffer,1,chunk,out);

                remain -= chunk;
            }

            fclose(out);

            printf("Extracted %s\n", file);

            fclose(fp);
            return;
        }
    }

    printf("File not found\n");
}

void extract_all(const char *archive) {

    FILE *fp = fopen(archive, "rb");

    if (!fp) return;

    VypHeader header;
    read_header(fp, &header);

    int count = load_index(fp, &header);

    for (int i = 0; i < count; i++) {

        FILE *out = fopen(records[i].path, "wb");

        fseek(fp, records[i].offset, SEEK_SET);

        char buffer[4096];
        uint64_t remain = records[i].size;

        while (remain > 0) {

            size_t chunk = remain > 4096 ? 4096 : remain;

            fread(buffer,1,chunk,fp);
            fwrite(buffer,1,chunk,out);

            remain -= chunk;
        }

        fclose(out);

        printf("Extracted %s\n", records[i].path);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        usage();
        return 0;
    }

    if (!strcmp(argv[1], "init") && argc == 3)
        init_archive(argv[2]);

    else if (!strcmp(argv[1], "list") && argc == 3)
        list_archive(argv[2]);

    else if (!strcmp(argv[1], "add") && argc == 4)
        add_file(argv[2], argv[3]);

    else if (!strcmp(argv[1], "extract") && argc == 4)
        extract_file(argv[2], argv[3]);

    else if (!strcmp(argv[1], "extractall") && argc == 3)
        extract_all(argv[2]);

    else
        usage();

    return 0;
}