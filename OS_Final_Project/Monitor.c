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

//global variables containg number of activate readers and writers
int numberOfActiveReaders = 0;
int numberOfActiveWriters = 0;

//global variables containing number of waiting readers and writers
int numberOfWaitingReaders = 0;
int numberOfWaitingWriters = 0;

//variables to hold our signals
//int canRead = 0;
//int canWrite = 0;
pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;

//mutex lock for writers
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writerMutex = PTHREAD_MUTEX_INITIALIZER;

//function prototypes
void *print_message_function(void *ptr);
void *intialEntryTime(void *ptr);
void *writeToFile(void *ptr);
void *endWritingToFile(void *ptr);
void *readFromFile(void *ptr);
void *endReadingFromFile(void *ptr);

int main(int argc, char** argv){
	//seed random number generator
	srand(time(NULL));
	char str[20];
	strcpy(str, argv[1]);
	int numWriters = atoi(str);
	strcpy(str, argv[2]);
	int numReaders = atoi(str);
    pthread_t activeTreads[numWriters+numReaders];
    int  iret, i, threadCounter = 0;

	int *id1 = malloc(sizeof(int));
	int *id2 = malloc(sizeof(int));

	*id1 = 1;
	*id2 = 0;

	for(i = 0; i < numWriters; i++){
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, id1);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}
		threadCounter++;
	}

	for(i = 0; i < numReaders; i++){
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, id2);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}	
		threadCounter++;
	}
    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
	
	for(i = 0; i < numWriters+numReaders; i++){
		pthread_join(activeTreads[i], NULL);
	}
	
	free(id1);
	free(id2);
	printf("Didn't crash early\n");
    return 0;
}

void *intialEntryTime(void *ptr){
	/*
		This function generates the initial wait time before the thread will begin execution
		and attempt to access the file
	*/
	int randomNumber = rand() % 1000;
	printf("Sleeping for %i milliseconds\n", randomNumber);
	fflush(stdout);
	usleep(randomNumber); //doesn't stop other threads from executing

	//if the thread is for a writer then call writeToFile, otherwise call readFromFile
	if(*(int *)ptr == 1){
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
	if(numberOfActiveWriters > 0 || numberOfActiveReaders > 0){
		//print waiting message
		printf("Writer waiting\n");
		fflush(stdout);
		
		//enter the waiting queue, and wait for condition canWrite to be true
		++numberOfWaitingWriters;
		
		//calculate the time to wait
		struct timespec timeToWait;
		struct timeval timeNow;

		int ms = rand() % 10000;

		printf("Writer Wait Time: %i\n", ms);

		gettimeofday(&timeNow,NULL);

		timeToWait.tv_sec = time(NULL) + ms / 1000;
    	timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
    	timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    	timeToWait.tv_nsec %= (1000 * 1000 * 1000);

		//timeToWait.tv_nsec = timeNow.tv_usec * 1000 + (ms % 1000) * 1000000;

		int n = pthread_cond_timedwait(&canWrite, &writerMutex, &timeToWait);
		
		//check if the thread got into the process quick enough, or cancelled the job
		printf("N = %i\n", n);
		if(n != 0){
			printf("Writer waited too long and cancelled job\n");
			//decrement number of waiting writers
			--numberOfWaitingWriters;
			//check to see what message to broadcast(if any)
			if(numberOfWaitingReaders > 0 && numberOfActiveWriters == 0){
				pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
				pthread_cond_broadcast(&canRead);
			} else if(numberOfWaitingWriters > 0 && numberOfActiveWriters == 0){
				pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
				pthread_cond_signal(&canWrite);
			}
			
			pthread_exit(pthread_self());
		}

		--numberOfWaitingWriters;
	}

	//set the number of writers flag
	numberOfActiveWriters = 1;

	printf("Writer writing\n");
	fflush(stdout);

	//simulate "writing" the file
	usleep(1000);
	
	//set number of activate Writers to 0
	numberOfActiveWriters = 0;

	//check if there are waiting readers, if there are then signal readers to begin reading
	//otherwise let another writer write if they are waiting
	if(numberOfWaitingReaders > 0){
		pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
		pthread_cond_broadcast(&canRead);
	} else if(numberOfWaitingWriters > 0){
		pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
		pthread_cond_signal(&canWrite);
	}
	printf("Releasing file\n");
	fflush(stdout);
	//call function to release file
	//endWritingToFile(NULL);
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//if the file is not free, then wait for file to be free
	if(numberOfActiveWriters > 0 || numberOfWaitingWriters > 0){
		printf("Reader waiting\n");
		fflush(stdout);
		//increment number of waiting readers
		++numberOfWaitingReaders;
		
		//calculate the time to wait
		struct timespec timeToWait;
		struct timeval timeNow;

		int ms = rand() % 10000;

		printf("Writer Wait Time: %i\n", ms);

		gettimeofday(&timeNow,NULL);

		timeToWait.tv_sec = time(NULL) + ms / 1000;
    	timeToWait.tv_nsec = timeNow.tv_usec * 1000 + 1000 * 1000 * (ms % 1000);
    	timeToWait.tv_sec += timeToWait.tv_nsec / (1000 * 1000 * 1000);
    	timeToWait.tv_nsec %= (1000 * 1000 * 1000);

		//timeToWait.tv_nsec = timeNow.tv_usec * 1000 + (ms % 1000) * 1000000;
		
		int n = pthread_cond_timedwait(&canRead, &readerMutex, &timeToWait);
		
		//check if the thread got into the process quick enough, or cancelled the job
		printf("N = %i\n", n);
		if(n != 0){
			printf("Reader waited too long and cancelled job\n");
			//decrement number of waiting writers
			--numberOfWaitingReaders;
			//check to see what message to broadcast(if any)
			if(numberOfWaitingReaders > 0 && numberOfActiveWriters == 0){
				pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
				pthread_cond_broadcast(&canRead);
			} else if(numberOfWaitingWriters > 0 && numberOfActiveWriters == 0){
				pthread_mutex_unlock(&writerMutex); //unlcok the writer mutex to allow for writers later on to access it
				pthread_cond_signal(&canWrite);
			}
			
			pthread_exit(pthread_self());
		}
		
		//wait on condition
		//pthread_cond_wait(&canRead, &readerMutex);
		//pthread_mutex_unlock(&readerMutex); //unlcok the reader mutex to allow for readers later on to access it
		//remove from waiting queue
		--numberOfWaitingReaders;
		
		printf("Reader is no longer waiting\n");
		fflush(stdout);
	}
	
	//increment number of active Readers
	++numberOfActiveReaders;
	printf("Reader reading\n");

	//signal other waiting reader threads that they can read
	if(numberOfWaitingReaders > 0){
		pthread_mutex_unlock(&readerMutex);
		pthread_cond_broadcast(&canRead);
		printf("Told other reader to read\n");
		fflush(stdout);
	}

	//simulate reading
	usleep(1000); //simulate "reading" the file
	
	printf("Done reading\n");
	fflush(stdout);
	
	//decrement number of waiting readers
	--numberOfActiveReaders;
	if(numberOfActiveReaders == 0 && numberOfWaitingWriters > 0){
		pthread_cond_signal(&canWrite);
	}
}