#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <vector>

#define n 16//Number of cells on intermediate board

///Global vars/// - shared memory
char mainGrid[n][n]; //Full game grid
int mine_count = 40;//Number of mines in intermediate game
int flag_count = 0;//Number of flags in intermediate board
long unsigned int p; //num of threads

//These are 2 3D vectors that will hold the indexes of cells to be flagged
//and cells to be clicked. The first 2 dimensions of the vectors will be
//associated with indexes of the different threads. The last dimension will
//hold indexes of cells (in the form of pairs) and will grow to match the
//number of cells to be either clicked or flagged as indicated by the thread
//it belongs to.
std::vector<std::vector<std::pair>> flag_vec;
std::vector<std::vector<std::pair>> click_vec;

//Struct used to pass arguements to threads
struct thread_data{//Holds indexes of the threads (where the subgrid start)
	int i_indx;
	int j_indx;
};

/*
Use thread ID to access the vector for that thread to add its actionable cells to.
To add to a subgrids list of cells to click/flag, use the threads ID to access the 
vector of pairs in either flag vec or click vec and push_back the pair of indexes
onto that vector.

The boss thread will go through all these vectors and determine what to click and what to flag
*/

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
		
	int sub_grid_dim = n/p;//Subgrids are of size n/p * n/p
	//This value helps determine the cells that a thread is in charge of evaluating.
	//Each cell will iterate from its index to its index + this value in the x and y directions to iterate through all its cells
	
	//int thread_grid_dim = p/2;//There are p/2 * p/2 subgrids created by the main grid.
	//(e.g. 4 threads = 2 x 2 grid of threads, 16 threads = 8 x 8 grid of threads)

	struct thread_data thread_data_array[p];//Each thread has index in the array of subgrids created from the main grid
	pthread_t threads[p];//Opaque, unique IDs
	pthread_attr_t attr;//Create pthread attribute obj

	//Init attribute and set the detached status
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc;
	int a =  0;//a indexes the thread_data_array which will store thread indexes in row major
			
	if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}
	
	//Have indexes increment by the size of the sub grids
	for (i=0; i<n; i+= sub_grid_dim){//Row major traversal
		for (j=0; j<n; j+= sub_grid_dim){

			//The indexes that each subgrid starts at (top left element in subgrid)
			thread_data_array[a].i_indx = i;
			thread_data_array[a].j_indx = j;

			//Add a vector to hold the indexes; add a new vector for each subgrid/thread
			std::vector<std::pair> vop;
			flag_block.push_back(vop);
			click_block.push_back(vop);
			
			rc = pthread_create(&threads[a], &attr, multiplyBlock, (void *) &thread_data_array[a] );
			if (rc) { printf("ERROR; return code from pthread_create() is %d\n", rc); exit(-1);}
			a++;
		}
	}//By the end of this nested for loop, flag_block and click_block should be the same size as the number of threads
	
	//Release attribute
	pthread_attr_destroy(&attr);

	//Join threads back together before displaying calculating time
	for(int t=0; t<(p); t++) {
		rc = pthread_join(threads[t], NULL);
		if (rc) { printf(" joining error %d ", rc); exit(-1);}
	}

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror("clock gettime");}		
	time = (stop.tv_sec - start.tv_sec)+ (double)(stop.tv_nsec - start.tv_nsec)/1e9;

	printf("Number of FLOPs = %lu, Execution time = %f sec,\n%lf MFLOPs per sec\n", 2*n*n*n, time, 1/time/1e6*2*n*n*n);		
	
	return 0;
}
