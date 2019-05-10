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

//global variables containg number of activate readers
int readerCount = 0;

//unique ID for each writer/reader(same reader and writer can have the same unique id)
static __thread int uniqueID;

//amount of time for the thread to wait before terminating
int timeToWaitInMS;

//semaphore variable
sem_t mutex;

//mutex to prevent multiple readers from access the semaphore at the same time
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;

//variable to message readers when they can read
pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;

//function prototypes
void *intialEntryTime(int ptr[2]);
void *writeToFile(void *ptr);
void *readFromFile(void *ptr);

int main(int argc, char** argv){
	//seed random number generator
	srand(time(NULL));

	//variables to hold the errore code in creating threads, for loop counter, and the number of threads
    int  iret, i, threadCounter = 0;
	
	//get command line arguments
	char str[20];
	strcpy(str, argv[1]);
	timeToWaitInMS = atoi(str);
	strcpy(str, argv[2]);
	int numWriters = atoi(str);
	strcpy(str, argv[3]);
	int numReaders = atoi(str);
	
	//create array of threads to be active
    pthread_t activeTreads[numWriters+numReaders];

	//initialize semaphore
	if(sem_init(&mutex, 0, 1) != 0){
		exit(0);
	}

	//create Writer Threads
	//array to hold passed parameters to the threads
	int arrWriters[numWriters][2];
	//create writer threads
	for(i = 0; i < numWriters; i++){
		//set passed parameters
		arrWriters[i][0] = 1;
		arrWriters[i][1] = i;

		//create thread
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, arrWriters[i]);

		//check if thread was correctly made
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}

		//increment thread counter
		threadCounter++;
	}

	//create Reader Threads
	//array to hold passed parameters to the threads
	int arrReaders[numReaders][2];

	//create reader threads
	for(i = 0; i < numReaders; i++){
		//set passed parameters
		arrReaders[i][0] = 0;
		arrReaders[i][1] = i;

		//create thread
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, arrReaders[i]);

		//check if thread was correctly made
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}

		//increment thread counter
		threadCounter++;
	}
	
	//join all threads
	for(i = 0; i < threadCounter; i++){
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
	//generate random number and then sleep for x milliseconds
	int randomNumber = rand() % 1000;
	usleep(randomNumber*1000); //doesn't stop other threads from executing

	//store unique identifier
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
	//struct to hold the times
	struct timespec timeToWait;
	struct timeval timeNow;

	//get the initial time
	gettimeofday(&timeNow,NULL);

	//calculate time to wait
	timeToWait.tv_sec = time(NULL) + timeToWaitInMS / 1000;
	timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (timeToWaitInMS % 1000);
	timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

	//wait for semaphore to become available
	int n = sem_timedwait(&mutex, &timeToWait);

	//lock reader mutex so that readers do not read at the same time as a writer
	pthread_mutex_lock(&readerMutex);

	//if writer could not get file in time, then exit
	if(n != 0){
		printf("Writer waited too long and exited\n");
		pthread_exit(pthread_self());
	}

	//output message stating which reader is reading to the file
	printf("File is being written by writer thread: %i\n", uniqueID);
	fflush(stdout);

	//simulate writing to file
	usleep(1000*1000);

	//unlock reader mutex, and signal reader that they can read
	//add the resource back to the pool anc signal that readers can also read
	pthread_mutex_unlock(&readerMutex);
	pthread_cond_signal(&canRead);
	sem_post(&mutex);
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//struct to hold the times
	struct timespec timeToWait;
	struct timeval timeNow;

	//get the initial time
	gettimeofday(&timeNow, NULL);

	//calculate time to wait
	timeToWait.tv_sec = time(NULL) + timeToWaitInMS / 1000;
    timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (timeToWaitInMS % 1000);
    timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    timeToWait.tv_nsec %= (1000 * 1000 * 1000);

	//wait for the first reader to come in to lock all 
	//other readers from access the semaphore at the same time
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

	//output message stating which reader is reading to the file
	printf("File is being read by reader thread: %i\n", uniqueID);
	fflush(stdout);

	//unlock mutex for other readers, but don't signal them to wake yet
	pthread_mutex_unlock(&readerMutex);

	//first reader locks writers from writing, and signals other waiting readers to read
	if(readerCount == 1){
		sem_wait(&mutex);
		pthread_cond_broadcast(&canRead);
	}

	//simulate "reading" the file
	usleep(1000*1000);

	//decrement number of readers
	--readerCount;

	//last reader releases resource back to the pool
	if(readerCount == 0){
		sem_post(&mutex);
	}

	//unlock the mutex to allow other readers to access the readerMutex
	pthread_mutex_unlock(&readerMutex);
}