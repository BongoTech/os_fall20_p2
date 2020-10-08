//*****************************************************
//Program Name: master
//Author: Cory Mckiel
//Date Created: Oct 4, 2020
//Last Modified: Oct 7, 2020
//Program Description:
//      Assignment two for operating systems class 4760
//      during fall20 semester.
//
//      master launches children (palin) for each string
//      in a file to determine if it is a palindrome.
//      Once determined all palindromes will be stored
//      in palin.out. All non palindromes will be stored
//      in nopalin.out. log.out will contain all of the
//      children created and their associated strings.
//      master is responsible for processing the 
//      command line arguments and keeping track of the
//      children.
//Compilation Instructions:
//      Option 1: Using the supplied Makefile.
//          Type: make
//      Option 2: Using gcc.
//          Type: gcc -Wall -g master.c -o master
//*****************************************************

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

//The size of the string in to be tested.
#define PALINSIZE 64
//The max file name length.
#define FILENAMESIZE 64

//Help function definition.
int help(char*);

//done_flag lets master know when either the
//timer is up or ctrl-c has been invoked.
//Volatile so that compiler knows to check
//every time rather than optimize. sig_atomic_t to ensure
//it only gets changed by one thing at a time.
static volatile sig_atomic_t done_flag = 0;

//Sets a flag to indicate when program should exit gracefully.
static void setdoneflag(int s)
{
    fprintf(stderr, "\nInterrupt Received.\n");
    done_flag = 1;
}

//Set the signal handler to listen to the timer
//and ctrl-c.
static int setupinterrupt()
{
    struct sigaction act;
    act.sa_handler = setdoneflag;
    act.sa_flags = 0;
    return (sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL) || sigaction(SIGINT, &act, NULL));
}

//Set up the interrupt timer for the time specified.
static int setuptimer(int time)
{
    struct itimerval value;
    if ( time <= 1000 ) {
        value.it_interval.tv_sec = time;
        value.it_interval.tv_usec = 0;
    } else {
        return -1;
    }
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_PROF, &value, NULL));
}

//state is used to solve the critical section
//problem. In the context of this code, it is
//only used to generate a size for the array
//in shared memory.
enum state {idle, want_in, in_cs};

//*****************************************************
///////////////////////MAIN////////////////////////////
//*****************************************************
int main(int argc, char *argv[])
{
    //Number of children this process will ever create.
    int max_lifetime_children = 4;
    //Number of children in system at a given time.
    int max_concurrent_children = 2;
    //The number of seconds this program is allowed to run.
    //Cannot be greater than 1000.
    int max_run_time = 100;
    //File info.
    FILE *fp = NULL;
    char file[FILENAMESIZE];

//*****************************************************
//BEGIN: Command line processing.

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

    //Max concurrent children must not exceed 20.
    if ( max_concurrent_children < 1 || max_concurrent_children > 20 ) {
        fprintf(stderr, "%s: Error: s must satisfy 1 <= s <= 20.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Max lifetime children must be at least one.
    if ( max_lifetime_children < 1 ) {
        fprintf(stderr, "%s: Error: n must be at least 1.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Max run time must be at least one second.
    if ( max_run_time < 1 ) {
        fprintf(stderr, "%s: Error: t must be at least 1.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //if true, there is an additional nonoption argument.
    if ( optind < argc ) {
        //copy it to file as the file name to work with.
        strncpy(file, argv[optind], FILENAMESIZE-1);
    } else {
        fprintf(stderr, "%s: Error: There must be a file name.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Open the file.
    fp = fopen(file, "r");

//END: Command line processing.
//*****************************************************
//BEGIN: Interrupt setup.

    if ( setupinterrupt() == -1 ) {
        fprintf(stderr, "%s: Error: Failed to set up interrupt.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( setuptimer(max_run_time) == -1 ) {
        fprintf(stderr, "%s: Error: Failed to set up timer.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

//END: Interrupt setup.
//*****************************************************
//BEGIN: Setting up shared memory.

    key_t key;
    int shmid;
    enum state *shmp;

    //Generate key deterministically so that children
    //can do the same and attach to shared memory.
    if ( (key = ftok("./", 876)) == -1 ) {
        fprintf(stderr, "%s: Error: ftok() failed to generated key.\n%s\n", argv[0], strerror(errno));
        return 1;
    }
    
    //Calculate the size of shared memory.
    //The first slot is whos turn it is for
    //accessing the file.
    //The rest is an array of states for all
    //children.
    int shm_size = sizeof(enum state) * (max_lifetime_children + 1);

    //Create and get the id of the shared memory segment.
    if ( (shmid = shmget(key, shm_size, IPC_CREAT|0666)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to allocate shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Attach to shared memory.
    if ( (shmp = (enum state*)shmat(shmid, NULL, 0)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to attach to shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Initialize turn to the first child.
    *shmp = 1;
    //Initialize every child's state to idle.
    int i;
    for ( i = 1; i <= max_lifetime_children; i++) {
        *(shmp+i) = idle;
    }

//END: Setting up shared memory.
//*****************************************************
//BEGIN: Creating children.

    //Array containing pids of all children.
    //Used for killing them after interrupt.
    pid_t childpid[max_lifetime_children];
    //The number of children at a given time.
    int child_count = 0;
    //The number of children created so far.
    int child_count_total = 0;
    //The logical id given to a child.
    int child_id = 1;
    //A buffer to hold strings read from file.
    char buffer[64];

    do {
        //If an interrupt occured, break.
        if ( done_flag ) {
            break;
        }

        //If there are less children right now than the
        //simultaneous max,
        if ( child_count < max_concurrent_children ) {
            //Read a string from file.
            if ( fgets(buffer, PALINSIZE-1, fp) != NULL ) {        
                //Create a child.
                if ( (childpid[child_count_total] = fork()) < 0 ) {
                    fprintf(stderr, "%s: Error: fork() failed to create child.\n%s\n", argv[0], strerror(errno));
                    return 1;
                } else if ( childpid[child_count_total] == 0 ) {
                    //Inside child,
                    //Build the argv.
                    char arg1[PALINSIZE];
                    char arg2[16];
                    char arg3[16];
                    sscanf(buffer, "%s", arg1);
                    sprintf(arg2, "%d", max_lifetime_children);
                    sprintf(arg3, "%d", child_id);
                    char *arg_vector[] = {"./palin", arg1, arg2, arg3, NULL};
                    //exec.
                    execv(arg_vector[0], arg_vector);
                } else {
                    //Inside parent,
                    //increment the simultaneous child count.
                    child_count++;
                    //increment the total child count.
                    child_count_total++;
                    //increment the logical child_id.
                    child_id++;
                }
            } else {
                //There was nothing read from file.
                break;
            }
        }

        //See if a child finished.
        if ( waitpid(-1, NULL, WNOHANG) > 0 ) {
            child_count--;
        }

    } while ( (child_count_total < max_lifetime_children) );

//END: Creating children.
//*****************************************************
//BEGIN: Finishing up.

    //If an interrupt occured, kill the children.
    if ( done_flag ) {
        for ( i = 0; i < child_count_total; i++ ) {
            kill(childpid[i], SIGINT);
        }
    }

    //wait for all children.
    while ( wait(NULL) > 0 );

    //Clean up shared memory.
    shmdt(shmp);
    shmctl(shmid, IPC_RMID, 0);
    
    //Close file.
    fclose(fp);
    fp = NULL;

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
