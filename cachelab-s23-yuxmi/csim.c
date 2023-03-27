#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void usage(void) {
    printf("Usage: ./csim -ref [-v] -s <s> -E <E> -b <b> -t <trace > ./csim "
           "-ref -h\n");
    printf("-h Print this help message and exit\n");
    printf("-v Verbose mode: report effects of each memory operation\n");
    printf("-s <s> Number of set index bits (there are 2**s sets)\n");
    printf("-b <b> Number of block bits (there are 2**b blocks)\n");
    printf("-E <E> Number of lines per set ( associativity )\n");
    printf("-t <trace> File name of the memory trace to process\n");
    printf("The -s, -b, -E, and -t options must be supplied for all "
           "simulations.\n");
}

int main(int argc, char **argv) {
    int ch;
    unsigned long sval, bval, eval;
    // bool v_flag = false;

    while ((ch = getopt(argc, argv, "h:v:s:b:E:t:")) != -1) {
        switch (ch) {
        case 'h':
            // Prints help message and exits without warning
            usage();
            exit(0);
        case 'v':
            // Verbose mode
            // v_flag = true;
            break;
        case 's':
            sval = strtoul(optarg, NULL, 10);
            if (sval < 0 || sval > 64) {
                // Exits with warning
                printf("Invalid input.\n");
                exit(1);
            } else {
                printf("s flag value: %lu\n", sval);
            }
            break;
        case 'b':
            bval = strtoul(optarg, NULL, 10);
            if (bval < 0 || bval > 64) {
                // Exits with warning
                printf("Invalid input.\n");
                exit(1);
            } else {
                printf("b flag value: %lu\n", bval);
            }
            break;
        case 'E':
            eval = strtoul(optarg, NULL, 10);
            if (eval < 0 || eval > 64) {
                // Exits with warning
                printf("Invalid input.\n");
                exit(1);
            } else {
                printf("e flag value: %lu\n", eval);
            }
            break;
        case 't':
            break;
        default:
            // Exits with warning
            printf("Mandatory arguments missing or zero.\n");
            usage();
            exit(1);
        }
    }
}
