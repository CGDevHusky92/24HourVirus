#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("tstWrappers [file ...]\n");
        exit(1);
    }

    // Open file from argv[1]
    int fd, i;
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        printf("Failed to open file\n");
        exit(errno);
    }

    // fstat file
    struct stat fstatInfo;
    if (fstat(fd, &fstatInfo) == -1) {
        printf("Configuration could not be read\n");
        close(fd);
        exit(errno);
    }

    // print out file size and first 8 bytes
    char buf;
    printf("%lld\n", fstatInfo.st_size);
    for (i = 0; i < 8; i++) {
        if (read(fd, &buf, 1) == -1) {
            printf("Error reading file\n");
            close(fd);
            exit(errno);
        }
        printf("%02X", buf);
    }
    printf("\n");

    // close the file
    close(fd);

    return 0;
}