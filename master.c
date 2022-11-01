#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
//#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
//#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
//#include <string>
//#include <cstdio>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
//using namespace std;

struct CLASS { // struct shared memory implementation
	int response[10];	// each child writes its child number here
	int index; // represents the next index to right the child in
};

void display(struct CLASS *base, int numChildren); // hoisted function

int main(int argc, char** argv) {
	pid_t cpid; // fork id
	int shm_fd; // shared memory file descriptor 
	int num_children = atoi(argv[1]); // takes argv[1] from char** to int
	const int SIZE = sizeof(struct CLASS);
	const char *name = argv[2]; // shm name
	struct CLASS  *shm_base; // base address of base memory
	//struct CLASS *shm_ptr; // moveable ptr

	const char *semName = "/SEM";


	printf("Master begins execution\n");
	printf("This is %d\n", getpid());
	printf("Num children: %d. shm name: %s\n",num_children,name);
	shm_fd = shm_open(name,O_CREAT | O_RDWR,0666); // creates a shared memory object and returns an int file descriptor

	if (shm_fd == -1){ // Creating shared memory fails
		printf("ERROR from Master: Shared memory failed; shm_open() failed\n");
		exit(1);
	}
	ftruncate(shm_fd, SIZE); // configures size of the shared memory object
	shm_base = (struct CLASS *)  mmap(0,SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd,0); // associates memory segment into return pointer
	if (shm_base == MAP_FAILED){
		printf("ERROR from Master: Map failed; mmap() failed\n");
		exit(1);
	}


	shm_base->index = 0;

    	sem_t *mutex_sem = sem_open( semName, O_CREAT, 0666, 1); // Creates a semaphore intialized to 1
    	if (mutex_sem == SEM_FAILED) {
        	printf("ERROR from Master: sem_open failed(): %s\n", strerror(errno));
        	exit(1);
        }


	if (sem_unlink(semName) == -1) { //Unlinks
        	printf("observer: sem_unlink() failed: %s\n", strerror(errno));
        	exit(1);
	}


	for (int i=0; i < num_children; i++){ //creates multiple child processes depending on childNumber

		cpid = fork(); // creates child process
		char childNum[5]; //buffer for second argument in execlp()
		sprintf(childNum,"%d",i + 1);
		if (cpid == 0){ // child process

			execlp("./slave",name,childNum,NULL);
		}
	}
	while(wait(NULL) != -1); // blocks code after it until all children have terminated
	printf("Master received termination signals from all %d child processes\n",num_children);
	printf("Contents of shared memory segment filled by child processes:\n");
	display(shm_base,num_children);

//    	if (sem_unlink(semName) == -1) { //Unlinks
//        	printf("ERROR from Master: sem_unlink() failed: %s\n", strerror(errno));
//        	exit(1);
//    	}

    	if (sem_close(mutex_sem) == -1) { // Deallocated semaphore
        	printf("ERROR from Master: sem_close failed: %s\n", strerror(errno));
        	exit(1);
    	}
	// Removes shared memory

	if (munmap(shm_base, SIZE) == -1) { // removes mapped memory segment
 		printf("ERROR from Master: Unmap failed; munmap() failed: %s\n", strerror(errno));
 		exit(1);
	}

	if (close(shm_fd) == -1) { // closes shared memory segment
 		printf("ERROR from Master: Close failed; close() failed: %s\n", strerror(errno));
		exit(1); // terminates program
	 }

	exit(0); // terminates program
}

void display(struct CLASS *base, int numChildren){
	for (int i =0; i < numChildren; i++){
		printf("%d ", base->response[i]);
	}
	printf("\n");
}
