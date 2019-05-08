//simulate readers and writers
//have between 1 and 9 readers
//have between 1 and 3 writers
//each reader or writer waits for a random time from 0-1000 milliseconds, 
//and then tries to access the file for a random time between 0-1000 milliseconds
//define a data structure to hold threads which are using the file

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

//global variables containg number of activate readers and writers
int readerCount = 0;
int timeToWaitInMS;
//unique ID for each writer/reader(same reader and writer can have the same unique id)
static __thread int uniqueID;

sem_t mutex;

//variables to hold our signals
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;

//function prototypes
void *intialEntryTime(int ptr[2]);
void *writeToFile(void *ptr);
void *endWritingToFile(void *ptr);
void *readFromFile(void *ptr);
void *endReadingFromFile(void *ptr);

int main(int argc, char** argv){
	//seed random number generator
	srand(time(NULL));
	char str[20];
	strcpy(str, argv[1]);
	timeToWaitInMS = atoi(str);
	strcpy(str, argv[2]);
	int numWriters = atoi(str);
	strcpy(str, argv[3]);
	int numReaders = atoi(str);
    pthread_t activeTreads[numWriters+numReaders];
    int  iret, i, threadCounter = 0;

	//initialize semaphore
	if(sem_init(&mutex, 0, 1) != 0){
		exit(0);
	}

	int arrWriters[numWriters][2];
	//create writer threads
	for(i = 0; i < numWriters; i++){
		arrWriters[i][0] = 1;
		arrWriters[i][1] = i;
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, arrWriters[i]);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}
		threadCounter++;
	}

	int arrReaders[numReaders][2];
	//create reader threads
	for(i = 0; i < numReaders; i++){
		arrReaders[i][0] = 0;
		arrReaders[i][1] = i;
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, arrReaders[i]);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}	
		threadCounter++;
	}
	
	for(i = 0; i < numWriters+numReaders; i++){
		pthread_join(activeTreads[i], NULL);
	}
	
	printf("Didn't crash early\n");
    return 0;
}

void *intialEntryTime(int ptr[2]){
	/*
		This function generates the initial wait time before the thread will begin execution
		and attempt to access the file
	*/

	int randomNumber = rand() % 1000;
	//printf("Sleeping for %i milliseconds\n", randomNumber);
	//fflush(stdout);
	usleep(randomNumber*1000); //doesn't stop other threads from executing

	uniqueID = ptr[1];
	//if the thread is for a writer then call writeToFile, otherwise call readFromFile
	if(ptr[0] == 1){
		writeToFile(NULL);
	} else{
		readFromFile(NULL);
	}
}

void *writeToFile(void *ptr){
	/*
		This function will check if there if the writer can begin writing to the file
		at a given time
	*/
	//check if file is free to be written to
	//print waiting message
	//printf("Writer waiting\n");
	fflush(stdout);
		
	//calculate the time to wait before ending the request to write to the file
	struct timespec timeToWait;
	struct timeval timeNow;

	//printf("Writer Wait Time: %i\n", timeToWaitInMS);

	gettimeofday(&timeNow,NULL);

	timeToWait.tv_sec = time(NULL) + timeToWaitInMS / 1000;
	timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (timeToWaitInMS % 1000);
	timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

	//int n = pthread_cond_timedwait(&canWrite, &writerMutex, &timeToWait);
	int n = sem_timedwait(&mutex, &timeToWait);

	//if writer could not get file in time, then exit
	if(n != 0){
		printf("Writer waited too long and exited\n");
		pthread_exit(pthread_self());
	}

	printf("File is being written by writer thread: %i\n", uniqueID);
	//printf("Writer writing\n");
	//fflush(stdout);
	//simulate writing to file
	usleep(1000*1000);

	//printf("Writer releasing file\n");
	//fflush(stdout);
	sem_post(&mutex);
	pthread_cond_signal(&canRead);
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//if the file is not free, then wait for file to be free
	//printf("Reader waiting\n");
	//fflush(stdout);

	//calculate the time to wait
	struct timespec timeToWait;
	struct timeval timeNow;

	//printf("Reader Wait Time: %i\n", timeToWaitInMS);

	gettimeofday(&timeNow, NULL);

	timeToWait.tv_sec = time(NULL) + timeToWaitInMS / 1000;
    timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (timeToWaitInMS % 1000);
    timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

	int n = pthread_cond_timedwait(&canRead, &readerMutex, &timeToWait);

	//check if the thread got into the process quick enough, or cancelled the job
	if(n != 0){
		printf("Reader waited too long and exited\n");
		//if thread didn't get signal in allocatted wait time
		//unlock reader mutex
		pthread_mutex_unlock(&readerMutex);
		
		//exit thread
		pthread_exit(pthread_self());
	}

	//increment number of waiting readers
	++readerCount;

	printf("File is being read by reader thread: %i\n", uniqueID);

	//unlock mutex for other readers, but don't signal them to wake yet
	pthread_mutex_unlock(&readerMutex);

	//first reader locks writers from writing, and signals other waiting readers to read
	if(readerCount == 1){
		sem_wait(&mutex);
		pthread_cond_broadcast(&canRead);
	}

	//printf("Reader reading\n");

	//simulate "reading" the file
	usleep(1000*1000);

	//decrement number of readers
	--readerCount;

	//last reader allows writers to write
	if(readerCount == 0){
		sem_post(&mutex);
	}

	//signal other waiting reader threads that they can read
	pthread_mutex_unlock(&readerMutex);
}