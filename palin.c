//*****************************************************
//Program Name: palin
//Author: Cory Mckiel
//Date Created: Oct 4, 2020
//Last Modified: Oct 7, 2020
//Program Description:
//      Assignment two for fall20 class 4760 at UMSL.
//
//      palin determines if a string from the command line is 
//      a palindrome. palin then enters a critical 
//      section to write the string to it's correct
//      file. palin.out if it is a palindrome and
//      nopalin.out otherwise. palin writes to a log.out
//      to include logging info. Inside critical 
//      section palin waits [0-2] seconds to simulate
//      varying computation times. palin then exits
//      critical section.
//To Compile:
//      Option 1: Using the supplied Makefile.
//          Type: make
//      Option 2: Using gcc.
//          Type: gcc -Wall -g palin.c -o palin
//*****************************************************

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

//Function returns 1 (true) if argument is palindrome,
//0 (false) otherwise.
int is_palindrome(char*);

//Each child has a state. state is used
//to solve critical section problem.
enum state {idle, want_in, in_cs};

int main(int argc, char *argv[])
{
//*****************************************************
//BEGIN: Command line processing.
    
    //Current version takes the palindrome by cmd ln.
    //argv[1] must be string.
    //argv[2] is the number of children that will be 
    //created. (used for attaching to shared memory
    //and during the critical section code.)
    //argv[3] is the child's logical id.
    if (argc != 4) {
        fprintf(stderr, "%s: Error: Enter a string as argument.\n", argv[0]);
        return 1;
    }
    
    int num_children = atoi(argv[2]);
    int child_id = atoi(argv[3]);

//END: Command line processing.
//*****************************************************
//BEGIN: Attaching to shared memory.

    key_t key;
    int shmid;
    enum state *shmp;
    //Size is the number of children plus one for whos turn.
    int shm_size = sizeof(enum state) * (num_children + 1);

    //Generate the key the same way as master.
    if ( (key = ftok("./", 876)) == -1 ) {
        fprintf(stderr, "%s: Error: ftok() failed to generated key.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Get the shared mem id.
    if ( (shmid = shmget(key, shm_size, 0666)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to allocate shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    //Attach to shared memory.
    if ( (shmp = (enum state*)shmat(shmid, NULL, 0)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to attach to shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

//END: Attaching to shared memory.
//*****************************************************
//BEGIN: Determine if palindrome.

    //palin stores 1 if argv[1] is a palindrome,
    //0 otherwise.
    int palin = is_palindrome(argv[1]);

//END: Determine if palidrome.
//*****************************************************
//BEGIN: Critical section code.

    //General purpose variable
    int j;

    do {
        //Raise my flag.
        *(shmp + child_id) = want_in;
        //Set local var to the current turn.
        j = *shmp;
        //Wait for my turn.
        while (j != child_id) {
            if (*(shmp + j) != idle) {
                j = *shmp;
            } else {
                if ( (j + 1) > num_children )
                    j = 1;
                else
                    j = j + 1;
            }
        }

        //Declare intention to enter critical section.
        *(shmp + child_id) = in_cs;

        //Check no one else is in the critical section.
        for (j = 1; j <= num_children; j++)
            if ( (j != child_id) && (*(shmp + j) != idle) )
                break;
    } while ( (j <= num_children) || ( *shmp != child_id && *(shmp + *shmp) != idle ) );

    //Get the current time.
    time_t now = time(0);
    struct tm *ctime = localtime(&now);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer)-1, "%X", ctime);

    //Notify stderr of intentions to enter critical section.
    fprintf(stderr, "%s: Child %d entering the critical section.\n", time_buffer, child_id);
    *shmp = child_id;
    
    //CRITICAL SECTION
    int r = rand() % 3;
    sleep(r);

    FILE *fp = NULL;
    if ( palin ) {
        fp = fopen("palin.out", "a");
        fprintf(fp, "%s\n", argv[1]);
    } else {
        fp = fopen("nopalin.out", "a");
        fprintf(fp, "%s\n", argv[1]);
    }
    fclose(fp);

    fp = fopen("log.out", "a");
    fprintf(fp, "PID: %d, Logical ID: %d, String: %s\n", getpid(), child_id, argv[1]);
    fclose(fp);   
    fp = NULL;

    //EXIT CRITICAL SECTION.
    //Figure out who's turn it is next.
    if ( (*shmp + 1) > num_children ) {
        j = 1;
    } else {
        j = *shmp + 1;
    }
    while (*(shmp + j) == idle) {
        if ( (j + 1) > num_children ) {
            j = 1;
        } else {
            j = j + 1;
        }
    }

    //Get the current time.
    now = time(0);
    ctime = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer)-1, "%X", ctime);

    //Notify stderr of exit of critical section.
    fprintf(stderr, "%s: Child %d exiting the critical section.\n", time_buffer, child_id);

    //Assign turn to next waiting process. Go to idle.
    *shmp = j; *(shmp + child_id) = idle;

//END: Critical Section Code.
//*****************************************************
//BEGIN: Finishing up.

    //Detach from shared memory.
    shmdt(shmp);

    return 0;
}

//Reverses the string that is passed to it
//by reference.
int strrev(char *str)
{
    //Will contain length of str.
    int length = 0;

    //Get length of str.
    while (str[length] != '\0')
        length++;

    char temp[50];

    //Construct reverse string in temp.
    int i, j = 0;
    for (i = length-1; i >= 0; i--) {
         temp[j++] = str[i];
    }
    temp[length] = '\0';
    
    //Put reversed string into str.
    strncpy(str, temp, length);

   return 0;
}

//Determines if a given string is a palindrome.
//Returns 1 if true, 0 if false.
int is_palindrome(char *str)
{
    //Make a copy of str in temp.
    char temp[50];
    strncpy(temp, str, 49);
    
    //Reverse temp.
    strrev(temp);

    //If the standard and reversed versions are
    //the same, it is a palindrome.
    if (strncmp(temp, str, 49) == 0) {
        return 1;
    } else {
        return 0;
    }
}
