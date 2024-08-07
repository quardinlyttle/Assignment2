/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    char data;
    off_t offset;
}BufferItem;



int nIN,nOUT,bufSize;


int main(int argc, char* argv[]){
    
    /*Check for arguments and ensure none are missing*/
    if(argc<7){
        perror("Please check to ensure you have all the required arguments");
        return 1;
    }

    /*Assigning number of threads and file names based on command line inputs*/
    nIN = atoi(argv[1]);
    nOUT = atoi(argv[2]);
    bufSize atoi(argv[5]);

    //original file
    size_t length = strlen(argv[3]+1);
    char *fileIn = malloc(length);
    if (fileIn == NULL){
        perror("Memory allocation for string failed!");
        return 1;
    }
    strcpy(fileIn,argv[3]);

    //copy file
    length = strlen(argv[4]+1);
    char *fileOut = malloc(length);
    if (fileOut == NULL){
        perror("Memory allocation for string failed!");
        return 1;
    }
    strcpy(fileOut,argv[4]);

    //Log file
    length = strlen(argv[6]+1);
    char *Log = malloc(length);
    if (Log == NULL){
        perror("Memory allocation for string failed!");
        return 1;
    }
    strcpy(Log,argv[6]);






}