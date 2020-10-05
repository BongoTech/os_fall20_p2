//*****************************************************
//Program Name: master
//Author: Cory Mckiel
//Date Created: Oct 4, 2020
//Last Modified: Oct 5, 2020
//Program Description:
//      Assignment two for operating systems class 4760
//      during fall20 semester.
//
//      master launches a child (palin) for each string
//      in a file to determine if it is a palindrome.
//      Once determined the result will be stored in a
//      file. master is responsible for processing the 
//      command arguments and keeping track of the
//      children.
//Compilation Instructions:
//      Option 1: Using the supplied Makefile.
//          Type: make
//      Option 2: Using gcc.
//          Type: gcc -Wall -g master.c -o master
//*****************************************************

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int help(char*);

int main(int argc, char *argv[])
{
    int max_lifetime_children = 4;
    int max_concurrent_children = 2;
    int max_run_time = 100;
    //FILE *fp = NULL;
    char file[51];

//BEGIN: Command line processing.
//*****************************************************
    int option;
    while ( (option = getopt(argc, argv, ":n:s:t:h")) != -1 ) {
        switch ( option ) {
            case 'h':
                help(argv[0]);
                return 0;
            case ':':
                fprintf(stderr, "%s: Error: Missing argument value.\n%s\n", argv[0], strerror(errno));
                return 1;
            case 'n':
                max_lifetime_children = atoi(optarg);
                break;
            case 's':
                max_concurrent_children = atoi(optarg);
                break;
            case 't':
                max_run_time = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "%s: Error: Unknown argument.\n%s\n", argv[0], strerror(errno));
                return 1;
        }
    }

    if ( max_concurrent_children < 1 || max_concurrent_children > 20 ) {
        fprintf(stderr, "%s: Error: s must satisfy 1 <= s <= 20.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( max_lifetime_children < 1 ) {
        fprintf(stderr, "%s: Error: n must be at least 1.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( max_run_time < 1 ) {
        fprintf(stderr, "%s: Error: t must be at least 1.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( optind < argc ) {
        strncpy(file, argv[optind], 50);
    } else {
        fprintf(stderr, "%s: Error: There must be a file name.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    printf("s: %d\n", max_concurrent_children);

    printf("n: %d\n", max_lifetime_children);

    printf("t: %d\n", max_run_time);

    printf("file: %s\n", file);

//END: Command line processing.
//*****************************************************

    return 0;
}

int help(char *prog_name)
{
    printf("%s: Usage: %s [-n x] [-s y] [-t z] input_file\n", prog_name, prog_name);
    printf("Where: x is number of children %s makes over lifetime.(Default 4)\n", prog_name);
    printf("y is max number of children existing at one time.(Default 2)\n");
    printf("z is max time the entire process takes. (Default 100.)\n");
    printf("input_file is some file containing a list of strings, one per line.\n");
    printf("input_file is not optional.\n");
    return 0;
}
