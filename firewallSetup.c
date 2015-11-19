#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERLENGTH 2048

int processFile(char *filePath, char *buf) {
        char tmpBuf[BUFFERLENGTH];
        FILE *file = fopen(filePath, "r");
        if(file == NULL) {
                printf("ERROR: Unable to open file: %s\n", filePath);
                return 1;
        }
        while(fgets(tmpBuf, BUFFERLENGTH, file)) {
                int port;
                char *path;
                int res;
                printf("%s", tmpBuf);
                res = sscanf(tmpBuf, "%d %ms\n", &port, &path);
                if(res == 2) {
                        // Line has an int and a string, check if the string
                        // refers to an executable file
                        struct stat sb;
                        if(stat(path, &sb) == 0 && sb.st_mode & S_IXUSR) {
                                // File is executable, check if port is valid
                                if(port > 0 && port < 65335) {
                                        // File is executable and port is valid,
                                        // Add rule to list.
                                        printf("%s\n", "Accepting rule.");
                                        strncat(buf, tmpBuf, BUFFERLENGTH);
                                }
                        } else {
                                printf("%s\n", "ERROR: Cannot execute file");
                                exit(1);
                        }

                } else {
                        // Wrong number of arguments on this line
                        printf("ERROR: Ill-formed file\n");
                        exit(1);
                        return 1;
                }
        }
        return 0;
}

int sendList() {
        int res;
        FILE *procFile = fopen("/proc/firewallExtension", w);
        res = fwrite('L', sizeof(char), 1, procFile);
        return res;
}

int sendRules(char *rules) {

}

int main(int argc, char **argv) {
        char buffer[BUFFERLENGTH * 50];
        int res;
        if(argc < 2) {
                printf("Usage: %s <L|W <filename>>\n", argv[0]);
        } else {
                char option = *argv[1];
                switch(option) {
                        case 'L':

                                break;
                        case 'W':
                                res = processFile(argv[2], buffer);
                                if(res == 0) {
                                        puts("");
                                        puts(buffer);
                                }
                                break;
                        default:
                                printf("Usage: %s <L|W <filename>>\n", argv[0]);
                                break;
                }
        }

        return 0;
}
