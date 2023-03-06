#include "cachelab.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

void usage(void) {
    printf("Usage: ./csim -ref [-v] -s <s> -E <E> -b <b> -t <trace > ./csim -ref -h\n");
    printf("-h Print this help message and exit\n");
    printf("-v Verbose mode: report effects of each memory operation\n");
    printf("-s <s> Number of set index bits (there are 2**s sets)\n");
    printf("-b <b> Number of block bits (there are 2**b blocks)\n");
    printf("-E <E> Number of lines per set ( associativity )\n");
    printf("-t <trace> File name of the memory trace to process\n");
    printf("The -s, -b, -E, and -t options must be supplied for all simulations.\n");
}

int main(int argc, char **argv) {
    int ch;
    unsigned long S,B,E;
    bool v_flag = false;

    while ((ch = getopt(argc, argv, "h:v:s:b:E:t:")) != -1) {
        switch(ch) {
            case 'h':
                usage();
                exit(0);
            case 'v':
                // Verbose mode
                v_flag = true;
                break;
            case 's':
                S = strtoul(optarg, NULL, 10);
                if (S < 0 || S > 64) {
                    printf("Invalid input.\n");
                    exit(1);
                } else {
                    printf("s flag value: %lu\n", S);
                }
                break;
            case 'b':
                B = strtoul(optarg, NULL, 10);
                if (B < 0 || B > 64) {
                    printf("Invalid input.\n");
                    exit(1);
                } else {
                    printf("b flag value: %lu\n", B);
                }
                break;
            case 'E':
                E = strtoul(optarg, NULL, 10);
                if (E < 0 || E > 64) {
                    printf("Invalid input.\n");
                    exit(1);
                } else {
                    printf("e flag value: %lu\n", E);
                }
                break;
            case 't':
                break;
            default:
                printf("Mandatory arguments missing or zero.\n");
                usage();
                exit(1);
        }
    }
}
