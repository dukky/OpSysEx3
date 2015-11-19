#include <stdio.h>

#define BUFFERLENGTH 256

int main(int argc, char **argv) {
        if(argc < 2) {
                printf("Usage: %s <L|W <filename>>\n", argv[0]);
        } else {
                char option = *argv[1];
                switch(option) {
                        case 'L':
                                break;
                        case 'W':
                                break;
                        default:
                                break;
                }
        }

        return 0;
}
