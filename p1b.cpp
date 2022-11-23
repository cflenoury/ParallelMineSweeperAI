#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <vector>

#define n 16

///Global vars/// - shared memory

char mainGrid[n][n]; //Full game grid
int mine_count = 40;
int flag_count = 0;
long unsigned int p; //num of threads

//Struct used to pass arguements to threads
struct thread_data{//Holds indexes of the threads (where the subgrid start)
	int i_indx;
	int j_indx;

	//Pointers to flag array and click array(?)


};

//Global variables (shared memory)
//double **A, **B, **C;
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
	
	//Create a subgrid for each thread to process	
	int sub_grid_dim = n/p;//one dimmension of the subgrids (which are all of equal size)
	//^^^ use this value to determine the cells that a subgrid is in charge of evaluating by
	//having the cell iterate from its index to its index + this value in the x and y directions
	
	int thread_grid_dim = p/2;//one dimmension of threads in the grid (e.g. 4 threads = 2 by two grid of threads)

	//Create 2 2D arrays that are dynamic in one dimmension. These arrays will hold the cells to be flagged
	// and cells to be clicked indicated by specific threads
	std::vector<std::pair> flag_array[thread_grid_dim][thread_grid_dim];//A 2 d array of vectors of pairs (indeces of cells)
	std::vector<std::pair> click_array[thread_grid_dim][thread_grid_dim];

	//QUESTION: How can we send these arrays to each thread in a way that allows all alterations to persist?

	struct thread_data thread_data_array[p];//Each thread has specific data
	pthread_t threads[p];//Opaque, unique IDs
	pthread_attr_t attr;//Create pthread attribute obj

	//Init attribute and set the detached status
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc; //a indexes the thread_data_array
	int a =  0;
			
	if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}
	
	//Have indexes increment by the size of the sub grids
	for (i=0; i<n; i+= sub_grid_dim){//Row major traversal
		for (j=0; j<n; j+= sub_grid_dim){

			//Subgrid (thread) indexes
			thread_data_array[a].i_indx = i;
			thread_data_array[a].j_indx = j;

			//Provide pointers to the arrays
			//Each thread gets the same pointers
			
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
