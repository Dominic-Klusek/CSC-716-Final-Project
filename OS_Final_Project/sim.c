#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv){
	if(argc != 5){
		printf("Not enough variables inputted in order to proceed\n");
		exit(0);
	}

	//character array to 
	char str[20];
	strcpy(str, argv[1]);
	int timeToWaitInMS = atoi(str);
	strcpy(str, argv[2]);
	int numWriters = atoi(str);
	strcpy(str, argv[3]);
	int numReaders = atoi(str);

	if(strcmp(argv[4], "Semaphore") == 0){
		printf("Method: Semaphore\n");
		execve("Semaphore", (char **)argv, NULL);
	} else if(strcmp(argv[4], "Monitor") == 0){
		printf("Method: Monitor\n");
		execve("Monitor", (char **)argv, NULL);
	} else{
		printf("Method: Test and Set\n");
		execve("Test", (char **)argv, NULL);
	}
}