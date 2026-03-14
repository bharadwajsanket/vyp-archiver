#include <stdio.h>
#include <string.h>

#include "../include/cli.h"
#include "../include/archive.h"

void print_usage(void)
{
    printf("Usage:\n");
    printf("  vypack init       <archive.vyp>\n");
    printf("  vypack add        <file> <archive.vyp>\n");
    printf("  vypack list       <archive.vyp>\n");
    printf("  vypack extract    <file> <archive.vyp>\n");
    printf("  vypack extractall <archive.vyp>\n");
    printf("  vypack delete     <file> <archive.vyp>\n");
    printf("  vypack test       <archive.vyp>\n");
}

int run_command(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        if (argc != 3) { print_usage(); return 1; }
        return create_archive(argv[2]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "add") == 0) {
        if (argc != 4) { print_usage(); return 1; }
        return add_file(argv[2], argv[3]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "list") == 0) {
        if (argc != 3) { print_usage(); return 1; }
        return list_files(argv[2]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "extract") == 0) {
        if (argc != 4) { print_usage(); return 1; }
        return extract_file(argv[2], argv[3]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "extractall") == 0) {
        if (argc != 3) { print_usage(); return 1; }
        return extract_all(argv[2]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "delete") == 0) {
        if (argc != 4) { print_usage(); return 1; }
        return delete_file(argv[2], argv[3]) == 0 ? 0 : 1;
    }

    if (strcmp(cmd, "test") == 0) {
        if (argc != 3) { print_usage(); return 1; }
        return test_archive(argv[2]) == 0 ? 0 : 1;
    }

    fprintf(stderr, "error: unknown command '%s'\n\n", cmd);
    print_usage();
    return 1;
}
