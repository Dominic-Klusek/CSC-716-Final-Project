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

//atomic flag to act as the global variable for test and set
atomic_flag temp = ATOMIC_FLAG_INIT;
atomic_int numberOfReaders = ATOMIC_VAR_INIT(0);

//unique id for each reader/writer thread
static __thread int uniqueID;

//amount of time for the thread to wait before terminating
int timeToWaitInMS;

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
	struct timeval timeBegin;
	struct timeval timeNow;

	//get the intial tiome
	gettimeofday(&timeBegin,NULL);
	
	//while the atomic flag returns true, get the time of day and compare to when the process started
	//if time is greater than the time to wait, then exit thread,
	//else continue checking for flag
	while(atomic_flag_test_and_set(&temp)){
		gettimeofday(&timeBegin,NULL);
		if(((timeNow.tv_usec - timeBegin.tv_usec) % 1000) > timeToWaitInMS){
			printf("Time waited: %li\n", (timeNow.tv_usec - timeBegin.tv_usec % 10000));
			printf("Writer waited too long and terminted\n");
			pthread_exit(pthread_self());
		}
	}

	//output message stating which writer is writing to the file
	printf("File is being written by writer thread: %i\n", uniqueID);
	fflush(stdout);

	//simulate writing to the file by sleeping for 1 second
	usleep(1000*1000);

	//clear the flag for the next reader/writer
	atomic_flag_clear(&temp);
}

void *readFromFile(void *ptr){
	/*
		This function will check if the file is free and call reading function when able to
	*/
	//struct to hold the times
	struct timeval timeBegin;
	struct timeval timeNow;

	//get the initial time
	gettimeofday(&timeBegin,NULL);

	//while the atomic flag returns true, get the time of day and compare to when the process started
	//if time is greater than the time to wait, then exit thread,
	//else continue checking for flag
	while(atomic_flag_test_and_set(&temp)){
		gettimeofday(&timeBegin,NULL);
		if(((timeNow.tv_usec - timeBegin.tv_usec) % 1000) > timeToWaitInMS){
			printf("Time waited: %li\n", (timeNow.tv_usec - timeBegin.tv_usec % 1000));
			printf("Reader waited too long and terminted\n");
			pthread_exit(pthread_self());
		}

		//if a single reader got past the test and set, then all readers are allowed
		if((int)atomic_load(&numberOfReaders) > 0){
			break;
		}
	}

	//increment number of readers
	atomic_fetch_add(&numberOfReaders, 1);

	//output message stating which reader is reading to the file
	printf("File is being read by reader thread: %i\n", uniqueID);
	fflush(stdout);

	//simulate "reading" the file
	usleep(1000*1000);

	//decrement number of readers
	atomic_fetch_sub(&numberOfReaders, 1);

	//final check to see if flag clearing is necessary
	if((int)atomic_load(&numberOfReaders) == 0){
		atomic_flag_clear(&temp);
	}
}