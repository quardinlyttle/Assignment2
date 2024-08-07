/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    char data;
    off_t offset;
}BufferItem;

/*Circular Buffer Definition*/
typedef struct{
    BufferItem * const buffer;
    int read;
    int write; 
    const int size;
    pthread_mutex_t mutex;
}circBuffer;

int nIN,nOUT,bufSize;

// nanosleep for <10ms
int nsleep() {
    struct timespec tim = {0, 0};
    tim.tv_sec = 0;
    tim.tv_nsec = rand() % 10000000L;
    if(nanosleep(&tim , NULL) < 0 ) {
        fprintf(stderr, "Nano sleep system call failed \n");
        exit(1); //ERROR
    }
}

//Create and initialize buffer
circBuffer createBuffer(int bufSize){

    //allocate memory for buffer
    circBuffer *buffer=(circBuffer*)malloc(sizeof(circBuffer));
    if(!buffer){
        perror("Buffer not allocated in memory properly");
        return NULL;
    }

    //allocate memory for array of bufferitems
    buffer->buffer = (BufferItem*)malloc(bufSize* sizeof(BufferItem));
    if(!buffer->buffer){
        perror("Buffer not allocated in memory properly");
        free(buffer);
        return NULL;
    }

    buffer->read=0;
    buffer->write=0;
    buffer->size=bufSize;

    if(pthread_mutex_init(&buffer->mutex,NULL)!=0){
        perror("Mutex initilization failed");
        free(buffer->buffer);
        free(buffer);
    }

    return buffer;
}

//Free the allocated buffer memory as necesary. 
void freeBuffer(circBuffer* buffer){
    if (buffer) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->buffer); 
        free(buffer);             
        }
}



/*Declared thread functions*/
void* in(void*arg);
void* out(void*arg);

/*****************MAIN FUNCTION******************/
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


    /*Create and initialize Circular Buffer*/
    circBuffer* buffer = createBuffer(bufSize);
    if(!buffer){
        perror("Buffer not created!");
        return 1;
    }



    /*Open file to be copied*/
    FILE* ogFile;
    if(!(ogFile=fopen(fileIn, "r"))){
        perror("Could not open file to be copied");
        return 1;
    }

    /*Open file copying to*/
    FILE* copyFile;
    if(!(copyFile=fopen(fileOut, "w"))){
        perror("Could not open file to be copied");
        return 1;
    }

    /*Open Logfile*/
    FILE* logFile;
    if(!(logFile=fopen(Log, "w"))){
        perror("Could not open file to be copied");
        return 1;
    }



freeBuffer(buffer)
}