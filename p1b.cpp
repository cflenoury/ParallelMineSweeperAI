#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string>
#include <iostream>

#define n 2000

//Struct used to pass arguements to threads
struct thread_data{//Holds indexes of the threads
	int i_indx;
	int j_indx;
	//pthread_mutex_t * mutexp;//Pointer to the mutex for the thread
};

//Global variables (shared memory)
long unsigned int p; 
double **A, **B, **C;
pthread_mutex_t* mutexes;

//Threaded function
void *multiplyBlock(void* threadArg){
	struct thread_data * my_data;//Create pointer of type thread_data to reference thread data
	//from array that was passed into function

	my_data = (struct thread_data *) threadArg;//Contains the i and j indexes to start at

	//Start at indexes specified for block associated with the thread
	for(int i = my_data->i_indx; i < my_data->i_indx+(n/p); i++){
		for(int j = my_data->j_indx; j < my_data->j_indx + (n/p); j++){
			for(int k = 0; k < n; k++){
				//pthread_mutex_lock(my_data->mutexp);//Acquire lock to alter C[i][k]
				pthread_mutex_lock(&mutexes[i]);//Acquire lock to alter C[i][k]
				C[i][k] = C[i][k] + A[i][j]*B[j][k];
				pthread_mutex_lock(&mutexes[i]);//Acquire lock to alter C[i][k]
				//pthread_mutex_unlock(my_data->mutexp);//Release lock
			}
		}
	}

	pthread_exit((void*) threadArg); //Terminate thread

}

int main(int argc, char *argv[]){
	
	int i, j;
	struct timespec start, stop; 
	double time;
	
	p = atoi(argv[1]);//p^2 = number of threads
	
	A = (double**) malloc (sizeof(double*)*n);
	B = (double**) malloc (sizeof(double*)*n);
	C = (double**) malloc (sizeof(double*)*n);	

	for (i=0; i<n; i++) {
		A[i] = (double*) malloc(sizeof(double)*n);
		B[i] = (double*) malloc(sizeof(double)*n);
		C[i] = (double*) malloc(sizeof(double)*n);
	}
	
	for (i=0; i<n; i++){
		for(j=0; j< n; j++){
			A[i][j]=i;
			B[i][j]=i+j;
			C[i][j]=0;
		}
	}

	struct thread_data thread_data_array[p*p];//Each thread has specific data
	pthread_t threads[p*p];//Opaque, unique IDs (?)
	pthread_attr_t attr;//Create pthread attribute obj

	//Allocate memeory to hold mutex
	mutexes = (pthread_mutex_t*) malloc (sizeof(pthread_mutex_t)*n);//p mutexes, for each row of array blocks

	//Initialize mutexes
	for (i=0;i<n;i++) pthread_mutex_init(&mutexes[i],NULL);

	//Init attribute and set the detached status
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc; //a indexes the thread_data_array
	int a =  0;
	int ndp = n/p;
			
	if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}
	
	//Have indexes increment by the size of the blocks
	for (i=0; i<n; i+= ndp){
		for (j=0; j<n; j+= ndp){

			//Set the index values of each thread to match that of the indexes during its instantion
			thread_data_array[a].i_indx = i;
			thread_data_array[a].j_indx = j;

			// if(i == 0){
			// 	thread_data_array[a].mutexp = &mutexes[0];
			// }else{
			// 	thread_data_array[a].mutexp = &mutexes[ndp/i];
			// }
			
			rc = pthread_create(&threads[a], &attr, multiplyBlock, (void *) &thread_data_array[a] );
			if (rc) { printf("ERROR; return code from pthread_create() is %d\n", rc); exit(-1);}
			a++;
		}
	}		
	
	//Release attribute
	pthread_attr_destroy(&attr);

	//Join threads back together before displaying C[100][100]
	for(int t=0; t<(p*p); t++) {
		rc = pthread_join(threads[t], NULL);
		if (rc) { printf(" joining error %d ", rc); exit(-1);}
	}

	for(int t = 0; t < p; t++){
		pthread_mutex_destroy (&mutexes[t]);	
	}	

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror("clock gettime");}		
	time = (stop.tv_sec - start.tv_sec)+ (double)(stop.tv_nsec - start.tv_nsec)/1e9;

	printf("Number of FLOPs = %lu, Execution time = %f sec,\n%lf MFLOPs per sec\n", 2*n*n*n, time, 1/time/1e6*2*n*n*n);		
	printf("C[100][100]=%f\n", C[100][100]);

	// release memory
	for (i=0; i<n; i++) {
		free(A[i]);
		free(B[i]);
		free(C[i]);
	}
	free(A);
	free(B);
	free(C);
	return 0;
}
