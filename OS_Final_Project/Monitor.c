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
#include <sys/types.h>

//unique ID for each writer/reader(same reader and writer can have the same unique id)
static __thread int uniqueID;

//amount of time for the thread to wait before terminating
int timeToWaitInMS;

//global variables containg number of activate readers and writers
int numberOfActiveReaders = 0;
int numberOfActiveWriters = 0;

//global variables containing number of waiting readers and writers
int numberOfWaitingReaders = 0;
int numberOfWaitingWriters = 0;

//variables to hold signals to wake the threads
pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;

//mutex lock for readers and writers when accessing the monitor
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writerMutex = PTHREAD_MUTEX_INITIALIZER;

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
	for(i = 0; i < numWriters+numReaders; i++){
		pthread_join(activeTreads[i], NULL);
	}

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
	//Writer Monitor to ensure that writers only access the file when it is avaiable
	if(numberOfActiveWriters > 0 || numberOfActiveReaders > 0){
		//increment number of waiting writers
		++numberOfWaitingWriters;
		
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

		//wait for the file to become available
		int n = pthread_cond_timedwait(&canWrite, &writerMutex, &timeToWait);
		
		//check if the thread got into the process quick enough, or cancelled the job
		if(n != 0){
			printf("Writer waited too long and cancelled job\n");
			//decrement number of waiting writers
			--numberOfWaitingWriters;

			pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it

			//check to see what message to broadcast(if any)
			if(numberOfWaitingReaders > 0 && numberOfActiveWriters == 0){
				pthread_cond_broadcast(&canRead);
			} else if(numberOfWaitingWriters > 0 && numberOfActiveWriters == 0){
				pthread_cond_signal(&canWrite);
			}
			
			pthread_exit(pthread_self());
		}

		//decrement the number of waiting writers
		--numberOfWaitingWriters;
	}

	//set the number of writers flag
	numberOfActiveWriters = 1;

	printf("File is being written by writer thread: %i\n", uniqueID);
	fflush(stdout);

	//simulate "writing" the file
	usleep(1000*1000);
	
	//set number of activate Writers to 0
	numberOfActiveWriters = 0;

	pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it

	//check if there are waiting readers, if there are then signal readers to begin reading
	//otherwise let another writer write if they are waiting
	if(numberOfWaitingReaders > 0){
		pthread_cond_broadcast(&canRead);
	} else if(numberOfWaitingWriters > 0){
		pthread_cond_signal(&canWrite);
	}
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//Reader Monitor to ensure that readers only access the file when it is avaiable
	if(numberOfActiveWriters > 0 || numberOfWaitingWriters > 0){
		//increment number of waiting readers
		++numberOfWaitingReaders;
		
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

		//wait for the file to become available
		int n = pthread_cond_timedwait(&canRead, &readerMutex, &timeToWait);
		
		//check if the thread got into the process quick enough, or cancelled the job
		if(n != 0){
			printf("Reader waited too long and cancelled job\n");
			//decrement number of waiting writers
			--numberOfWaitingReaders;

			pthread_mutex_unlock(&readerMutex); //unlcok the writer mutex to allow for writers later on to access it

			//check to see what message to broadcast(if any)
			if(numberOfWaitingReaders > 0 && numberOfActiveWriters == 0){
				pthread_cond_broadcast(&canRead);
			} else if(numberOfWaitingWriters > 0 && numberOfActiveWriters == 0){
				pthread_cond_signal(&canWrite);
			}
			
			pthread_exit(pthread_self());
		}
		
		//decrement the number of waiting readers
		--numberOfWaitingReaders;
	}
	
	//signal other waiting reader threads that they can read
	if(numberOfWaitingReaders > 0){
		pthread_mutex_unlock(&readerMutex);
		pthread_cond_broadcast(&canRead);
		fflush(stdout);
	}

	//increment number of active Readers
	++numberOfActiveReaders;

	printf("File is being read by reader thread: %i\n", uniqueID);
	fflush(stdout);

	//simulate "reading" the file
	usleep(1000*1000);
	
	
	//decrement number of waiting readers
	--numberOfActiveReaders;

	//signal writers to write if there are no active readers or writers
	if(numberOfActiveReaders == 0 && numberOfWaitingWriters > 0){
		pthread_cond_signal(&canWrite);
	}
}