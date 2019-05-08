#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

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

	printf("String: %s\n", argv[4]);
	if(strcmp(argv[4], "Semaphore") == 0){
		execve("Semaphore", (char **)argv, NULL);
	} else if(strcmp(argv[4], "Monitor") == 0){
		execve("Monitor", (char **)argv, NULL);
	} else{
		execve("Test", (char **)argv, NULL);
	}
}