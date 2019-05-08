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
#include <stdatomic.h>
atomic_flag temp = ATOMIC_FLAG_INIT;

int timeToWaitInMS;

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
	timeToWaitInMS = atoi(str);
	strcpy(str, argv[2]);
	int numWriters = atoi(str);
	strcpy(str, argv[3]);
	int numReaders = atoi(str);
    pthread_t activeTreads[numWriters+numReaders];
    int  iret, i, threadCounter = 0;

	int *id1 = malloc(sizeof(int));
	int *id2 = malloc(sizeof(int));

	*id1 = 1;
	*id2 = 0;
	//create writer threads
	for(i = 0; i < numWriters; i++){
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, id1);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}
		threadCounter++;
	}

	//create reader threads
	for(i = 0; i < numReaders; i++){
		iret = pthread_create(&activeTreads[threadCounter], NULL, intialEntryTime, id2);
		if(iret){
        	fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
        	exit(EXIT_FAILURE);
    	}	
		threadCounter++;
	}
	
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
	//printf("Sleeping for %i milliseconds\n", randomNumber);
	//fflush(stdout);
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
	//print waiting message
	printf("Writer waiting\n");
	fflush(stdout);
		
	//calculate the time to wait before ending the request to write to the file
	struct timeval timeBegin;
	struct timeval timeNow;

	printf("Writer Wait Time: %i\n", timeToWaitInMS);

	gettimeofday(&timeBegin,NULL);

	while(atomic_flag_test_and_set(&temp)){
		gettimeofday(&timeBegin,NULL);
		if((timeNow.tv_usec - timeBegin.tv_usec % 1000) > timeToWaitInMS){
			printf("Time waited: %li\n", (timeNow.tv_usec - timeBegin.tv_usec % 10000));
			printf("Writer waited too long and terminted\n");
			pthread_exit(pthread_self());
		}
	}

	printf("Writer writing\n");
	fflush(stdout);
	//simulate writing to file
	usleep(1000*1000);

	printf("Writer releasing file\n");
	fflush(stdout);
	atomic_flag_clear(&temp);
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//if the file is not free, then wait for file to be free
	printf("Reader waiting\n");
	fflush(stdout);

	//calculate the time to wait before ending the request to write to the file
	struct timeval timeBegin;
	struct timeval timeNow;

	printf("Reader Wait Time: %i\n", timeToWaitInMS);

	gettimeofday(&timeBegin,NULL);

	while(atomic_flag_test_and_set(&temp)){
		gettimeofday(&timeBegin,NULL);
		if((timeNow.tv_usec - timeBegin.tv_usec % 1000) > timeToWaitInMS){
			printf("Time waited: %li\n", (timeNow.tv_usec - timeBegin.tv_usec % 1000));
			printf("Reader waited too long and terminted\n");
			pthread_exit(pthread_self());
		}
	}

	printf("Reader reading\n");

	//simulate "reading" the file
	usleep(1000*1000);

	//signal other waiting reader threads that they can read
	printf("Done reading\n");
	fflush(stdout);
	atomic_flag_clear(&temp);
}