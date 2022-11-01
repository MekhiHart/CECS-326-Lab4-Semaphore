#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
//using namespace std;

struct CLASS {
	int response[10];	// each child writes its child number here
	int index;
};// how to implement struct into this??

//argv[0] = name
//argv[1] = childNum
int main(int argc, char **argv){
	const int SIZE = sizeof(struct CLASS);
	int shm_fd;
	int *i; // local variable that will hold the value of the next free index
	struct CLASS *shm_base;
	int childNum = atoi(argv[1]); // index needed to change;
	const char *semName = "/SEM";
	printf("Slave %d begins execution\n",childNum);
	printf("I am child number %d, received shared memory name %s\n",childNum,argv[0]);

	shm_fd = shm_open(argv[0], O_RDWR, 0666); // opens shared memory file assoicated with the name given
	if (shm_fd == -1) {// Shared memory failed
		printf("ERROR from Slave %d. Shared Memory failed; shm_open() failed: %s\n",childNum, strerror(errno));
	 	exit(1);
	}
	shm_base = (struct CLASS *) mmap(0, SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); // returns a memory address of the assoicated file descriptor passed in with the instructions to write on it
	if (shm_base == MAP_FAILED) { // map failed
		printf("ERROR from Slave %d. Map failed; shm_mmap() failed: %s\n",childNum, strerror(errno));
 		/* close and unlink */
		exit(1); // terminates program
 	}

    	sem_t *mutex_sem = sem_open( semName, O_CREAT, 0666, 1); // Opens semaphore
    	if (mutex_sem == SEM_FAILED) {
        	printf("ERROR from Slave %d: sem_open failed(): %s\n",childNum, strerror(errno));
        	exit(1);
    	}

//	if (sem_unlink(semName) == -1) { // Unlinks 
//        	printf("ERROR from Slave %d: sem_unlink() failed: %s\n",childNum, strerror(errno));
//        	exit(1);
//    	}

//	printf("Next free index: %d\n",shm_base->index);
	printf("Slave %d acquires access to shared memory segment, and structures it according to struct CLASS\n",childNum);


    	if (sem_wait(mutex_sem) == -1) { // sem wait
        	printf("ERROR from Slave %d sem_wait() failed: %s/n",childNum, strerror(errno));
        	exit(1);
    	}
	// Critical Section START
	printf("Slave %d enters its critical section\n",childNum);
	i = &(shm_base->index);
	printf("Slave %d copies index to a local variable i\n",childNum);

	// writes into shared memory
	shm_base->response[*i] = childNum;  
	printf("Slave %d writes its child number in response[%d] in shared memory\n",childNum,*i);

	*i += 1; // increments shm_base index
	printf("Slave %d increments index\n",childNum);
	printf("Slave %d exits critical section\n",childNum);
	
	// Critical Section END

    	if (sem_post(mutex_sem) == -1) {
        	printf("ERROR from Slave %d: sem_post() failed: %s\n",childNum, strerror(errno));
        	exit(1);
    	}

//	if (sem_unlink(semName) == -1) { // Unlinks 
  //      	printf("ERROR from Slave %d: sem_unlink() failed: %s\n",childNum, strerror(errno));
//	       	exit(1);
//    	}

/* done with semaphore, close it & free up resources 
   allocated to it */
//    	if (sem_close(mutex_sem) == -1) {
//	       	printf("ERROR from Slave %d: sem_close() failed: %s\n",childNum, strerror(errno));
//        	exit(1);
//	}

	// removes shared memory segment
	if (munmap(shm_base, SIZE) == -1) { // removes memory mapping 
 		printf("ERROR from Slave %d. Unmap failed; munmap() failed; %s\n",childNum, strerror(errno));
 		exit(1);
 	}
	if (close(shm_fd) == -1) { // closes shared memory segment
 		printf("ERROR from Slave %d. Close failed; close() failed: %s\n",childNum,strerror(errno));
 		exit(1);
	 }

	printf("Slave %d closed access to shared memory and terminates\n",childNum);
	printf("Slave %d exits\n",childNum);
	exit(1); // terminates program
}
