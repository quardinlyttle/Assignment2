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
    //nIN = nOUT = bufSize = done = 0;
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

// Free the allocated buffer memory as necessary
void freeBuffer(circBuffer* buffer) {
    if (buffer) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->buffer);
        free(buffer);
    }
}
// Write to log file
void toLog(char* info, int ID, BufferItem item, int index) {
    pthread_mutex_lock(&logMutex);
    fprintf(logFile, "%s PT%d O%ld B%d I%d\n", info, ID, item.offset, item.data, index);
    pthread_mutex_unlock(&logMutex);
}

// Read from file
void read_byte(int thread, BufferItem *item) {
    pthread_mutex_lock(&readMutex); // Engage reader mutex lock

    // Get current file offset
    if ((item->offset = ftell(ogFile)) < 0) {
        pthread_mutex_unlock(&readMutex);
        perror("Cannot seek input file!");
        exit(1);
    }

    // Read the next char
    if ((item->data = fgetc(ogFile)) == EOF) {
        if (feof(ogFile)) {
            printf("read_byte PT%d EOF pthread_exit(0)\n", thread);
            pthread_mutex_unlock(&readMutex); // Release read mutex lock
            pthread_exit(0); // EOF
        } else {
            perror("fgetc error");
            pthread_mutex_unlock(&readMutex);
            exit(1);
        }
    }
    toLog("read_byte", thread, *item, -1); // Log read
    pthread_mutex_unlock(&readMutex); // Release reader mutex lock
}

// Write to file
void write_byte(int thread, BufferItem item) {
    pthread_mutex_lock(&writeMutex); // Engage writing mutex

    // Set offset
    if (fseek(copyFile, item.offset, SEEK_SET) == -1) {
        fprintf(stderr, "error setting output file position to %u\n", (unsigned int)item.offset);
        pthread_mutex_unlock(&writeMutex);
        exit(-1);
    }
    // Write character
    if (fputc(item.data, copyFile) == EOF) {
        fprintf(stderr, "error writing byte %d to output file\n", item.data);
        pthread_mutex_unlock(&writeMutex);
        exit(-1);
    }

    toLog("write_byte", thread, item, -1); // Log write
    pthread_mutex_unlock(&writeMutex); // Release write mutex lock
}


// Produce to buffer
void produce(int thread, BufferItem item) {
    sem_wait(&buffer->empty); // Empty buffer slot wait
    pthread_mutex_lock(&buffer->mutex); // Lock buffer
    buffer->buffer[buffer->write] = item; // Write item to buffer
    toLog("produce", thread, item, buffer->write); // Log production
    buffer->write = (buffer->write + 1) % buffer->size; // Increment buffer head/writing section
    pthread_mutex_unlock(&buffer->mutex); // Unlock buffer
    sem_post(&buffer->full); // Increment semaphore
}

// Consume from buffer
void consume(int thread, BufferItem *item) {
    sem_wait(&buffer->full); // Full buffer slot wait
    pthread_mutex_lock(&buffer->mutex); // Lock buffer
    *item = buffer->buffer[buffer->read]; // Read item from buffer
    toLog("consume", thread, *item, buffer->read); // Log consumption
    buffer->read = (buffer->read + 1) % buffer->size; // Increment buffer tail/reading section
    pthread_mutex_unlock(&buffer->mutex); // Unlock buffer
    sem_post(&buffer->empty); // Increment semaphore
}

void readComplete() {
    printf("Reading done!\n");
    pthread_mutex_lock(&doneMutex);
    done = 1;
    pthread_mutex_unlock(&doneMutex);
    printf("Done now = %d\n",done);
}

/* NOTE: Ensure writing is also finished */
void checkComplete() {
    int check = 0;
    pthread_mutex_lock(&doneMutex);
    check = done;
    pthread_mutex_unlock(&doneMutex);
    //printf("Just closed mutex for done %d\n",done);
    if (check == 1) {
        printf("output thread done!\n");
        pthread_exit(NULL);
    }


}

/* Declared thread functions */
void* inThread(void* arg);
void* outThread(void* arg);

/***************** MAIN FUNCTION ******************/
int main(int argc, char* argv[]){
    printf("Starting\n");
    /*Check for arguments and ensure none are missing*/
    if(argc < 7){
        perror("Please check to ensure you have all the required arguments");
        exit(1);
    }

    /*Assigning number of threads and file names based on command line inputs*/
    nIN = atoi(argv[1]);
    nOUT = atoi(argv[2]);
    bufSize = atoi(argv[5]);

    // original file
    size_t length = strlen(argv[3]) + 1;
    char *fileIn = malloc(length);
    if (fileIn == NULL){
        perror("Memory allocation for string failed!");
        exit(1);
    }
    strcpy(fileIn, argv[3]);

    // copy file
    length = strlen(argv[4]) + 1;
    char *fileOut = malloc(length);
    if (fileOut == NULL){
        perror("Memory allocation for string failed!");
        exit(1);
    }
    strcpy(fileOut, argv[4]);

    // Log file
    length = strlen(argv[6]) + 1;
    char *Log = malloc(length);
    if (Log == NULL){
        perror("Memory allocation for string failed!");
        exit(1);
    }
    strcpy(Log, argv[6]);

    /*Initialize Files*/
    initFiles(fileIn, fileOut, Log);

    // Free memory allocated to file names
    free(fileIn);
    free(fileOut);
    free(Log);

    /*Create and initialize Circular Buffer*/
    buffer = createBuffer(bufSize);
    if (!buffer){
        perror("Buffer not created!");
        exit(1);
    }

    pthread_t threads[nIN + nOUT];
    tData *threadData = malloc((nIN + nOUT) * sizeof(tData));
    if (threadData == NULL) {
        perror("Failed to allocate memory for thread data");
        exit(1);
    }

    /* Create nIN number of IN threads */
    for (int i = 0; i < nIN; ++i) {
        threadData[i].ID = i;
        if (pthread_create(&threads[i], NULL, inThread, &threadData[i]) != 0) {
            perror("Failed to create the IN thread");
        }
    }

    /* Create nOUT number of OUT threads */
    for (int i = nIN; i < (nOUT + nIN); ++i) {
        threadData[i].ID = i;
        if (pthread_create(&threads[i], NULL, outThread, &threadData[i]) != 0) {
            perror("Failed to create the OUT thread");
        }
    }

    /* Join IN threads */
    for (int i = 0; i < nIN; ++i) {
        pthread_join(threads[i], NULL);
    }

    /* Wait for buffer to be empty */
    while (1) {
        printf("waiting to be done!\n");
        pthread_mutex_lock(&buffer->mutex);
        printf("write: %d read: %d\n", buffer->write, buffer->read);
        if (buffer->write == buffer->read) {
            pthread_mutex_unlock(&buffer->mutex);
            break;
        }
        pthread_mutex_unlock(&buffer->mutex);
        nsleep();
    }
    readComplete();

    /* Join OUT threads */
    for (int i = nIN; i < (nOUT + nIN); ++i) {
        nsleep();
        printf("Thread ID%ld, array ID: %d\n",threads[i],i);
        pthread_join(threads[i], NULL);
    }
    printf("Done!\n");
    // Cleanup
    freeBuffer(buffer);
    free(threadData);
    fclose(logFile);
    fclose(copyFile);
    fclose(ogFile);
    return 0;
}

/* Threads Functions */

/* IN Thread */
void* inThread(void* arg) {
    tData *data = (tData*)arg;
    srand(pthread_self());

    while (1) {
        nsleep(); // Sleep for 10ms
        read_byte(data->ID, &data->item); // Read byte into tData's item
        nsleep(); // Sleep for 10ms
        produce(data->ID, data->item); // Produce tData's item
    }
}

/* OUT Thread */
void* outThread(void* arg) {
    tData *data = (tData*)arg;
    srand(pthread_self());

    while (1) {
        nsleep();
        checkComplete();
        if(done==1){
            printf("ID %d is still running for some weird reason.\n",data->ID);
        }
        nsleep(); // Sleep for 10ms
        consume(data->ID, &data->item); // Consume into tData's item
        nsleep(); // Sleep for 10ms
        write_byte(data->ID, data->item); // Write tData's item
    }
}