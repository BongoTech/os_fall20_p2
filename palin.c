//*****************************************************
//Program Name: palin
//Author: Cory Mckiel
//Date Created: Oct 4, 2020
//Last Modified: Oct 5, 2020
//Program Description:
//      Assignment two for fall20 class 4760 at UMSL.
//
//      palin determines if a string from a file is 
//      a palindrome.
//To Compile:
//      Option 1: Using the supplied Makefile.
//          Type: make
//      Option 2: Using gcc.
//          Type: gcc -Wall -g palin.c -o palin
//*****************************************************

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

int strrev(char*);

int main(int argc, char *argv[])
{
    //Current version takes the palindrome by cmd ln.
    if (argc != 2) {
        fprintf(stderr, "%s: Error: Enter a string as argument.\n", argv[0]);
        return 1;
    }

    key_t key;
    int shmid;
    int *shmp;

    if ( (key = ftok("./", 876)) == -1 ) {
        fprintf(stderr, "%s: Error: ftok() failed to generated key.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( (shmid = shmget(key, sizeof(int), 0666)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to allocate shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    if ( (shmp = (int*)shmat(shmid, NULL, 0)) < 0 ) {
        fprintf(stderr, "%s: Error: Failed to attach to shared memory.\n%s\n", argv[0], strerror(errno));
        return 1;
    }

    printf("child: shmp: %d\n", *shmp);
    shmdt(shmp);
    
    //Make a copy of cmd ln string in temp.
    char temp[50];
    strncpy(temp, argv[1], 49);
    
    //Reverse temp.
    strrev(temp);

    //If the standard and reversed versions are
    //the same, it is a palindrome.
    if (strncmp(temp, argv[1], 49) == 0) {
        printf("true.\n");
    } else {
        printf("false.\n");
    }

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
