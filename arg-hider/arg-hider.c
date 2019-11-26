#include <stdio.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <zconf.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (!prctl(PR_SET_MM, PR_SET_MM_ARG_START, argv[1], 0, 0)) {
        char *err = strerror(errno);
        const int BUFSIZE = getpagesize();
        unsigned char buffer[BUFSIZE];
        int fd = open("/proc/self/cmdline", O_RDONLY);
        int nbytesread = read(fd, buffer, BUFSIZE);
        unsigned char *end = buffer + nbytesread;
        for (unsigned char *p = buffer; p < end; /**/) {
            printf("%s ", p);
            while (*p++); // skip until start of next 0-terminated section
        }
        printf("\n");
        close(fd);
    } else {
        char *err = strerror(errno);
        printf("%s\n", err);
    }
}