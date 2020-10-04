//*****************************************************
//Program Name: master
//Author: Cory Mckiel
//Date Created: Oct 4, 2020
//Last Modified: Oct 4, 2020
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

#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("Hello from master.\n");
    return 0;
}
