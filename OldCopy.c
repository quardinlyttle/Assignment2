/* Family Name: Lyttle
Given Name(s): Quardin
Student Number: 215957889
EECS Login ID: lyttleqd
YorkU email address: lyttleqd@my.yorku.ca
*/
#define _GNU_SOURCE
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
} BufferItem;

/* Circular Buffer Definition */
typedef struct {
    BufferItem *buffer;
    int read;
    int write; 
    int size;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} circBuffer;

typedef struct {
    int ID;
    BufferItem item;
} tData;

/* Global Variables */
int nIN, nOUT, bufSize, done;
circBuffer* buffer;
FILE *ogFile, *copyFile, *logFile;
pthread_mutex_t readMutex, writeMutex, logMutex, doneMutex;

// Initialize global files and variables
void initFiles(char* fileIn, char* fileOut, char* log) {
    /* Open file to be copied */
    if (!(ogFile = fopen(fileIn, "r"))) {
        perror("Could not open file to be copied");
        exit(1);
    }

    /* Open file copying to */
    if (!(copyFile = fopen(fileOut, "w"))) {
        perror("Could not open file to be copied");
        exit(1);
    }

    /* Open Logfile */
    if (!(logFile = fopen(log, "w"))) {
        perror("Could not open log file");
        exit(1);
    }

    // Initialize mutexes
    if (pthread_mutex_init(&readMutex, NULL) != 0) {
        perror("Reader Mutex initialization failed");
    }
    if (pthread_mutex_init(&writeMutex, NULL) != 0) {
        perror("Writer Mutex initialization failed");
    }
    if (pthread_mutex_init(&logMutex, NULL) != 0) {
        perror("Log Mutex initialization failed");
    }
    if (pthread_mutex_init(&doneMutex, NULL) != 0) {
        perror("Done Mutex initialization failed");
    }
    //done = 0;
}

// nanosleep for <10ms
int nsleep() {
    struct timespec tim = {0, 0};
    tim.tv_sec = 0;
    tim.tv_nsec = rand() % 10000000L;
    if (nanosleep(&tim, NULL) < 0) {
        fprintf(stderr, "Nano sleep system call failed\n");
        exit(1); // ERROR
    }
    return 0;
}

// Create and initialize buffer
circBuffer* createBuffer(int bufSize) {
    // Allocate memory for buffer
    circBuffer *buffer = (circBuffer*)malloc(sizeof(circBuffer));
    if (!buffer) {
        perror("Buffer not allocated in memory properly");
        exit(1);
    }

    // Allocate memory for array of buffer items
    buffer->buffer = (BufferItem*)malloc(bufSize * sizeof(BufferItem));
    if (!buffer->buffer) {
        perror("Buffer not allocated in memory properly");
        free(buffer);
        exit(1);
    }

    buffer->read = 0;
    buffer->write = 0;
    buffer->size = bufSize;

    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        free(buffer->buffer);
        free(buffer);
    }

    sem_init(&buffer->empty, 0, bufSize);
    sem_init(&buffer->full, 0, 0);

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

//write to log file
void toLog(char* info, int ID, BufferItem *item, int index){
    pthread_mutex_lock(&logMutex);
    fprintf(logFile,"%s PT%d O%ld B%d I%d\n",info,ID,item->offset,item->data,index);
    pthread_mutex_unlock(&logMutex);
}


//Read from file.
void read_byte(int thread, BufferItem *item){
    pthread_mutex_lock(&readMutex); //Engage reader mutex lock

    //Get current file ofset
    if((item->offset=ftell(ogFile))<0){
        pthread_mutex_unlock(&readMutex);
        perror("Cannot seek input file!");
        exit(1);
    }

    //read the next char. 
    if( (item->data=fgetc(ogFile)) < 0) {
        if(feof(ogFile)){
            printf("read_byte PT%d EOF pthread_exit(0)\n", thread);
            pthread_mutex_unlock(&readMutex); /* Release read mutex lock */
            pthread_exit(0); //EOF
        }
        else{
            perror("fgetc error");
            pthread_mutex_unlock(&readMutex);
            exit(1);
        }
    }    
    toLog("read_byte", thread, item, -1); //Log read
    pthread_mutex_unlock(&readMutex); //Release reader mutex lock

}

//write to file
void write_byte(int thread, BufferItem item){
    pthread_mutex_lock(&writeMutex); //Engage writing mutex

    //Setoffset
    if (fseek(copyFile, item.offset, SEEK_SET) == -1) {
        fprintf(stderr, "error setting output file position to %u\n",(unsigned int) item.offset);
        pthread_mutex_unlock(&writeMutex); 
        exit(-1);
    }
    //Write character
    if (fputc(item.data, copyFile) == EOF) {
        fprintf(stderr, "error writing byte %d to output file\n", item.data);
        pthread_mutex_unlock(&writeMutex); 
        exit(-1);
    }    

    toLog("write_byte", thread, &item, -1); //Log read
    pthread_mutex_unlock(&writeMutex); //Release reader mutex lock
}

//Produce to buffer
void produce(int thread, BufferItem item){
    sem_wait(&buffer->empty); //Empty buffer slot wait
    pthread_mutex_unlock(&buffer->mutex);//lock buffer
    buffer->buffer[buffer->write]=item;//write item to buffer
    toLog("produce",thread,&item,buffer->write); //log production
    buffer->write = (buffer->write +1)%buffer->size; //increment buffer head/writing section
    pthread_mutex_unlock(&buffer->mutex);//unlock buffer
    sem_post(&buffer->full);//incremenet semaphore. 
}


//Consume to buffer
void consume(int thread, BufferItem *item){
    sem_wait(&buffer->full); //full buffer slot wait
    pthread_mutex_unlock(&buffer->mutex);//lock buffer
    *item=buffer->buffer[buffer->read];//read item to buffer
    toLog("consume",thread,item,buffer->read); //log production
    buffer->read = (buffer->read +1)%buffer->size; //increment buffer tail/reading section
    pthread_mutex_unlock(&buffer->mutex);//unlock buffer
    sem_post(&buffer->empty);//incremenet semaphore. 
}

void readComplete(){
    pthread_mutex_lock(&doneMutex);
    done =1;
    pthread_mutex_unlock(&doneMutex);
}

/*NOTE I need to make sure the writing is also finished*/
void checkComplete(){
    int check=0;
    pthread_mutex_lock(&doneMutex);
    check = done;
    pthread_mutex_unlock(&doneMutex);
    if(check==1){
        pthread_exit(0);
    }
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
    bufSize = atoi(argv[5]);

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
    initFiles(fileIn, fileOut, Log);

    // Free memory allocated to file names
    free(fileIn);
    free(fileOut);
    free(Log);

    /*Create and initialize Circular Buffer*/
    buffer = createBuffer(bufSize);
    if(!buffer){
        perror("Buffer not created!");
        exit(1);
    }

    pthread_t tidIN[nIN];
    tData *inTData = (tData*)malloc(nIN*sizeof(tData));
    if (inTData == NULL) {
        perror("Failed to allocate memory for inTData");
        exit(1);
    }

    /*Intilaize each in thread*/
    for(int i =0; i<nIN; i++){
        inTData[i].ID = i;

        /*Create thread and check if it is creater properly*/
        if (pthread_create (&tidIN[i],NULL,inThread,&inTData[i])!= 0){
                    perror("Failed to create thread!");
                    exit(1);
        }
    }

    pthread_t tidOUT[nOUT];
    tData *outTData = (tData*)malloc(nIN*sizeof(tData));
    if (outTData == NULL) {
        perror("Failed to allocate memory for inTData");
        exit(1);
    }

    /*Intilaize each in thread*/
    for(int i =0; i<nOUT; i++){
        outTData[i].ID = i;

        /*Create thread and check if it is creater properly*/
        if (pthread_create (&tidOUT[i],NULL,outThread,&outTData[i])!= 0){
                    perror("Failed to create thread!");
                    exit(1);
        }
    }

    int inJoiners = nIN;
    while(inJoiners>0){
        for(int i=0; i<nIN;i++){

            int ret = pthread_tryjoin_np(tidIN[i], NULL);
            if (ret == 0) {
                /*Debugging line*/ 
                printf("Thread joined successfully\n");
                inJoiners--;
            } 
            else if (ret == EBUSY) {
                printf("Thread has not terminated yet\n");
                continue;
            } 
            else {
                printf("Error joining thread\n");
            }

        }
    }    
    if(inJoiners==0){
        printf("Writer threads finished\n");
        
        while(1){
            pthread_mutex_lock(&buffer->mutex);
            if(buffer->write==buffer->read){
                pthread_mutex_unlock(&buffer->mutex);
                break;
            }
            else{
                pthread_mutex_unlock(&buffer->mutex);
            }
        }
        readComplete();
    }

    int outJoiners = nOUT;
    while(outJoiners>0){
        for(int i=0; i<nOUT;i++){

            int ret = pthread_tryjoin_np(tidOUT[i], NULL);
            if (ret == 0) {
                /*Debugging line*/ 
                //printf("Thread joined successfully\n");
                outJoiners--;
            } 
            else if (ret == EBUSY) {
                //printf("Thread has not terminated yet\n");
                continue;
            } 
            else {
                printf("Error joining thread\n");
            }

        }
    }  






    freeBuffer(buffer);
    free(inTData);
    free(outTData);    
    fclose(logFile);
    fclose(ogFile);
    fclose(copyFile);
    return 0;
}

void* inThread(void*arg){
    tData *data=(tData*)arg;
    while(1){
        nsleep();
        read_byte(data->ID,&data->item);
        nsleep();
        produce(data->ID,data->item);
    }
}

void* outThread(void*arg){
    tData *data=(tData*)arg;
    while(1){    
        nsleep();
        consume(data->ID,&data->item);
        nsleep();
        write_byte(data->ID,data->item);
        checkComplete();
    }
}