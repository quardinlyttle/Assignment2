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


typedef struct{
    int ID;
    pthread_t tid;
    BufferItem item;
}tData;

/*Global Variables*/
int nIN,nOUT,bufSize;
circBuffer* buffer;
FILE* ogFile, copyFile, logFile;
pthread_mutex_t readMutex,writeMutex,logMutex; 

//Intilalize global files and variables
void initFiles(char* fileIn, char* fileOut, char* log, ){
    
    /*Open file to be copied*/
    if(!(ogFile=fopen(fileIn, "r"))){
        perror("Could not open file to be copied");
        exit(1);
    }

    /*Open file copying to*/
    if(!(copyFile=fopen(fileOut, "w"))){
        perror("Could not open file to be copied");
        exit(1);
    }

    /*Open Logfile*/
    if(!(logFile=fopen(Log, "w"))){
        perror("Could not open file to be copied");
        exit(1);
    }

    //Intialize mutexs
    if(pthread_mutex_init(&readMutex,NULL)!=0){
        perror("Reader Mutex initilization failed");
    }
    if(pthread_mutex_init(&writeMutex,NULL)!=0){
        perror("Writer Mutex initilization failed");
    }
    if(pthread_mutex_init(&logMutex,NULL)!=0){
        perror("Log Mutex initilization failed");
    }



}

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
        exit(1);
    }

    //allocate memory for array of bufferitems
    buffer->buffer = (BufferItem*)malloc(bufSize* sizeof(BufferItem));
    if(!buffer->buffer){
        perror("Buffer not allocated in memory properly");
        free(buffer);
        exit(1);
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

//Read from file.
void read_byte(int thread, BufferItem *item){
    pthread_mutex_lock(&readMutex); //Engage reader mutex lock
    item->offset=ftell(ogFile);
    item->data=fgetc(ogFile);

    if((item->offset=lseek(ogFile,0,SEEK_CUR))<0){
        pthread_mutex_unlock(&readMutex);
        perror("Cannot seek output file!")
        exit(1);
    }
    if( read(fin, &(item->data), 1) < 1) {
        printf("read_byte PT%d EOF pthread_exit(0)\n", thread);
        pthread_mutex_unlock(&readMutex); /* Release read mutex lock */
        pthread_exit(0); //EOF
    }    
    toLog("read_byte", thread, *item, -1);
    pthread_mutex_unlock(&readMutex); //Release reader mutex lock

}

void toLog(char* info, int ID, BufferItem *item, int index){
    pthread_mutex_lock(&logMutex);
    

}


/*Declared thread functions*/
void* inThread(void*arg);
void* outThread(void*arg);

/*****************MAIN FUNCTION******************/
int main(int argc, char* argv[]){
    
    /*Check for arguments and ensure none are missing*/
    if(argc<7){
        perror("Please check to ensure you have all the required arguments");
        exit(1);
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
        exit(1);
    }
    strcpy(fileIn,argv[3]);

    //copy file
    length = strlen(argv[4]+1);
    char *fileOut = malloc(length);
    if (fileOut == NULL){
        perror("Memory allocation for string failed!");
        exit(1);
    }
    strcpy(fileOut,argv[4]);

    //Log file
    length = strlen(argv[6]+1);
    char *Log = malloc(length);
    if (Log == NULL){
        perror("Memory allocation for string failed!");
        exit(1);
    }
    strcpy(Log,argv[6]);

    /*Initialize Files*/
    initFiles(fileIn,fileOut,Log);

    /*Create and initialize Circular Buffer*/
    buffer = createBuffer(bufSize);
    if(!buffer){
        perror("Buffer not created!");
        exit(1);
    }

    pthread_t tidIN[nIN];
    tData *inTData = (tData*)malloc(nIN*sizeof*tData);

    /*Intilaize each in thread*/
    for(int i =0; i<nIN; i++){
        inTData[i]->ID = i;
        inTData[i]->tid = &tidIN[i];

        /*Create thread and check if it is creater properly*/
        if (pthread_create (&tidIN[i],NULL,inThread,&inTData[i])!= 0) {
                    perror("Failed to create thread!");
                    exit(1);
        }

    }




    freeBuffer(buffer)
    return 0;
}

void* inThread(void*arg){
    tData *data=(tData*)arg;
    while(true){
        nsleep();
        read_byte(data->ID,&data->item);

    }
}