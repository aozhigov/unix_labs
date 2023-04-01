#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

#define BLOCK_SIZE_DEFAULT 4096
#define MISSING_ARG_ERROR "Missing arg: file path"
#define ALLOC_BUFF_ERROR "Error allocate buffer"

int get_block_size(int argc, char *argv[]) {
    int argument;
    int block_size = BLOCK_SIZE_DEFAULT;
    while ((argument = getopt(argc, argv, ":b:")) != -1) {
        if (argument == 'b') {
            block_size = atoi(optarg);
        }
    }
    return block_size;
}

int main(int argc, char *argv[]) {
    int block_size = get_block_size(argc, argv);

    char *in_file_path = NULL;
    if (optind < argc) {
        in_file_path = argv[optind++];
    }
    else {
        printf(MISSING_ARG_ERROR);
        return -1;
    }

    int in_fd = 0;
    int out_fd = 0;
    char *out_file_path = NULL;
    out_file_path = argv[optind++];
    if (out_file_path == NULL) {
        in_fd = STDIN_FILENO;
        out_fd = open(in_file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    }
    else {
        in_fd = open(in_file_path, O_RDONLY, 0);
        out_fd = open(out_file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    }

    char *buffer = (char *)malloc(block_size);
    if (buffer == NULL) {
        printf(ALLOC_BUFF_ERROR);
        return -1;
    }

    int zero_blocks_count = 0;
    int read_count;
    while ((read_count = read(in_fd, buffer, block_size)) != 0) {
        bool is_zero = true;
        for (int i = 0; i < read_count; i++) {
            if (buffer[i] != 0) {
                is_zero = false;
                break;
            }
        }
        if (is_zero) {
            zero_blocks_count++;
            continue;
        }

        if (zero_blocks_count != 0) {
            lseek(out_fd, zero_blocks_count * block_size, SEEK_CUR);
            zero_blocks_count = 0;
        }

        write(out_fd, buffer, read_count);
    }

    if (zero_blocks_count != 0) lseek(out_fd, zero_blocks_count * block_size, SEEK_CUR);

    if (in_fd != 0) close(in_fd);
    
    return close(out_fd);
}