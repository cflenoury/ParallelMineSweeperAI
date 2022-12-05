#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <iomanip>
#include <cmath>
using namespace std;

#define n 16//Number of cells on intermediate board

///Global vars/// - shared memory
int solutionGrid[n][n]; //solution
int mainGrid[n][n]; //Full game grid
int mine_count = 40;//Number of mines in intermediate game
int flag_count = 0;//Number of flags in intermediate board
long unsigned int p; //num of threads
bool win_cond = false;
bool lose_cond = false;
int subgrid_dim;

//Create barriers
pthread_barrier_t barrier1; 
pthread_barrierattr_t attr1;


//These are 2 3D vectors that will hold the indexes of cells to be flagged
//and cells to be clicked. The first 2 dimensions of the vectors will be
//associated with indexes of the different threads. The last dimension will
//hold indexes of cells (in the form of pairs) and will grow to match the
//number of cells to be either clicked or flagged as indicated by the thread
//it belongs to.
vector<vector<pair<int,int>>> flag_vec;
vector<vector<pair<int,int>>> click_vec;

//Struct used to pass arguements to threads
struct thread_data{//Holds indexes of the cell in the top left corner of a thread's subgrid
	int i_indx;
	int j_indx;
	int id;
};


//Double Set Single Point Functions
void AFN(int row, int col, int threadID){

	int flagged_neighbors = 0;

	std::vector<std::pair<int,int>> cell_vec;
	
	for(int i = row-1; i<row + 2; i++){
		for(int j  = col-1; j < col + 2; j++){
			if((i*j >= 0) && (i < n) && (j < n)){
				if(mainGrid[i][j] == -2){
					flagged_neighbors++;						
				}else if(mainGrid[i][j] == -1){
					cell_vec.push_back(std::pair<int,int>(i,j));
				}
			}
		}
	}
	//compare flagged _neighbors and number on cell
	if( flagged_neighbors == mainGrid[row][col] ){
		//Add all surrounding cells to click vec
		for(auto& x : cell_vec){
			click_vec[threadID].push_back(x);//Add pair of indexes to click_vec for its subgrid/thread
			mainGrid[x.first][x.second] = 10;//Change the cell's to be clicked to have an 'S' on them to indicate that they are safe to click (useful for AMN)
		}
	}

}

void AMN(int row, int col, int threadID){

	int unrevealed_neighbors = 0;
	int temp_flags = 0;

	std::vector<std::pair<int,int>> cell_vec;
	
	//If the number of unrevealed cells is equal to the number on the cell of interest

	if( row*col == 0 || row*col == 15*15){//Count the number of unrevealed neighbors

		if(row == 0 && col == 0){//Top left

			for(int i = 0; i < 2; i++){
				for(int j = 0; j < 2; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(pair<int,int>(i,j)); //add unrevealed cell to flag_vec
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(row == 0 && col == 15){//Top right

			for(int i = 0; i < 2; i++){
				for(int j = 14; j < n ; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(row == 15 && col == 0){//Bottom left

			for(int i = 14 ; i < n; i++){
				for(int j = 0; j < 2; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}
	
		}else if(row == 15 && col == 15){//Bottom right

			for(int i = 14 ; i < n; i++){
				for(int j = 14; j < n; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(row == 0){//Top edge

			for(int i = 0; i < 2; i++){
				for(int j = col-1; j < col+2; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(row == 15){//Bottom edge

			for(int i = 15 ; i < n; i++){
				for(int j = col-1; j < col+2; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(col == 0){//Left edge

			for(int i = row-1 ; i < row+2; i++){
				for(int j = col; j < col+2; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}else if(col == 15){//Right edge

			for(int i = row-1 ; i < row+2; i++){
				for(int j = col-1; j < n ; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;	
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

		}

	}else{//Middle cell

		for(int i = row-1 ; i < row+2; i++){
				for(int j = col-1; j < col+2 ; j++){
					if(mainGrid[i][j] == -1){
						unrevealed_neighbors++;
						cell_vec.push_back(std::pair<int,int>(i,j));
					}else if(mainGrid[i][j] == -2){
						temp_flags++;
					}
				}
			}

	}
	//compare unrevealed_neighbors and number on cell
	if( unrevealed_neighbors == mainGrid[row][col] - temp_flags){
		//Add unrevealed cells to click vec
		for(auto& x : cell_vec){
			flag_vec[threadID].push_back(x);//Add pair of indexes to click_vec for its subgrid/thread
		}
	}
}

void zero_cell_cleanup(int row, int col){

	for(int i = row-1; i<row + 2; i++){
		for(int j  = col-1; j < col + 2; j++){
			if((i*j >= 0) && (mainGrid[i][j] == -1 || mainGrid[i][j] == 10)){

				mainGrid[i][j] = solutionGrid[i][j];

				if(mainGrid[i][j] ==0){
					zero_cell_cleanup(i, j);
				}

			}
		}
	}

}

bool click_cells(vector<pair<int,int>> cells){

	if(cells.empty()){
		cout << "There is nothing to click tho\n";
		while(1){}
	}

	for(int w = 0; w < cells.size(); w++){
		int row = cells[w].first;
		int col = cells[w].second;

		cout << "Clicking cell "<<cells[w].first << ", " << cells[w].second << endl;

		if(solutionGrid[row][col] == 9){//mine
			mainGrid[row][col] = solutionGrid[row][col];

			lose_cond = true;
			
			return false;

		}else if(solutionGrid[row][col] == 0){//Empty cell

			//Reveal all unrevealed adjacent cells
			zero_cell_cleanup(row, col);

		}else{//Reveal cell
			mainGrid[row][col] = solutionGrid[row][col];
		}
	}

	return true;

}

void flag_cells(int threadID){

	if(flag_vec[threadID].empty()){
		cout << "There is nothing to flag tho\n";
		while(1){}
	}

	for(auto& x: flag_vec[threadID]){
		cout << "Flagging cell "<<x.first << ", " << x.second << endl;
		int row = x.first;
		int col = x.second;
		
		mainGrid[row][col] = -2;//Inidicate cell has a mine under it
		flag_count++;
	}
}

void print_game(){
	cout << "  ";
	for(int k = 0; k < n; k++){
		cout << std::setfill(' ') << std::setw(4) << k;
	}

	cout << endl;

	for(int i = 0; i < n; i++){
		cout <<std::setfill(' ') << std::setw(2)<< i << ' ';
		for(int j = 0; j < n; j++){
			cout << "[" <<std::setfill(' ') << std::setw(2) <<mainGrid[i][j] << "]";
		}
		cout << endl;
	}
}

void crap_shoot(int row, int col){

}

/*
Use thread ID to access the vector for that thread to add its actionable cells to.
To add to a subgrids list of cells to click/flag, use the threads ID to access the 
vector of pairs in either flag vec or click vec and push_back the pair of indexes
onto that vector.

The boss thread will go through all these vectors and determine what to click and what to flag
*/

//Threaded function
void *threadBlock(void* threadArg){
	struct thread_data * my_data;//Create pointer of type thread_data to reference thread data
	//from array that was passed into function

	my_data = (struct thread_data *) threadArg;//Contains the i and j indexes to start at
	int threadID = my_data->id;
	int row = my_data->i_indx;
	int col = my_data->j_indx;
	
	//thread 0 pictures
	if (threadID == 0){
		cout << "Making first move\n";
		//Random move
		//srand(time(NULL));
		//int a = rand()%16;
		//int b = rand()%16;
		int a = 9;
		int b = 9;

		pair<int,int> p(a, b);
		click_vec[threadID].push_back(p);

		// initial click
		click_cells(click_vec[threadID]);
		if(lose_cond){
			cout << "Unlucky first click :-(\n";
		}

		print_game();
		
		click_vec[threadID].clear();
	}
	
	
	// Barrier 1
	pthread_barrier_wait(&barrier1);//wait for initial click
	
	while( !win_cond && !lose_cond ){

		//Start at indexes specified for block associated with the thread and iterate through each cell in the matrix
		for(int i = my_data->i_indx; i < my_data->i_indx+(subgrid_dim); i++){
			for(int j = my_data->j_indx; j < my_data->j_indx + (subgrid_dim); j++){

				//if cell is unrevealed
				if(mainGrid[i][j] > 0 ){
					// DSSP functions
					AFN(i, j, threadID);
					AMN(i, j, threadID);
				}
			}
		}

		// Barrier 2 
		pthread_barrier_wait(&barrier1);//Wait for all threads to sync before examining click_vec and flag_vec

		//Only call more advanced algorithms if DSSP fails
		if(threadID==0){
			int click_vec_flag = 0;
			int flag_vec_flag = 0;
			for(auto& x: click_vec){
				if(!x.empty()){
					click_vec_flag=1; // this means not empty
					break;
				}
			}
			for(auto& x: flag_vec){
				if(!x.empty()){
					flag_vec_flag =1; // this means not empty
					break;
				}
			}
			if ((click_vec_flag==0)&&(flag_vec_flag==0)){
				//while(1){};
				lose_cond = true;
				// call other algorithms
			}
		}
		
		// Barrier 3	
		pthread_barrier_wait(&barrier1);//Wait for the entire board to be evalutated

		// click board
		if(!click_vec[threadID].empty()){
			click_cells(click_vec[threadID]);
		}

		// Barrier 4
		pthread_barrier_wait(&barrier1);//Wait for all cells to be clicked

		//flag cells
		if(!flag_vec[threadID].empty()){
			flag_cells(threadID);
		} 

		//Check win condition
		if(flag_count == mine_count){
			win_cond = true;
		}
		
		//while(1){}
		// Barrier 5
		pthread_barrier_wait(&barrier1);//Wait for all flags to be placed
		
		//Reset action vectors
		click_vec[threadID].clear();
		flag_vec[threadID].clear();

		if(threadID == 0){
			cout << "Current game state:\n";
			print_game();
		}

		pthread_barrier_wait(&barrier1);//Wait for all flags to be placed

	}

	pthread_exit((void*) threadArg); //Terminate thread

}

int main(int argc, char *argv[]){
	
	struct timespec start, stop; 
	double time;
	
	p = atoi(argv[1]);//p = number of threads
		
	subgrid_dim = n/sqrt(p);//Subgrids are of size n/p * n/p
	//This value helps determine the cells that a thread is in charge of evaluating.
	//Each cell will iterate from its index to its index + this value in the x and y directions to iterate through all its cells
	
	//int thread_grid_dim = p/2;//There are p/2 * p/2 subgrids created by the main grid.
	//(e.g. 4 threads = 2 x 2 grid of threads, 16 threads = 8 x 8 grid of threads)

	struct thread_data thread_data_array[p];//Each thread has index in the array of subgrids created from the main grid
	pthread_t threads[p];//Opaque, unique IDs
	
	//pthread_attr_init(&attr1);

	//Create barriers
	pthread_barrier_init(&barrier1, &attr1, p);

	//Init attribute and set the detached status
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);	

	//Read in file
	ifstream fi("solution_matrix.txt");
	string line;
	int i = 0;
	int j = 0;

	cout << "Reading in solution matrix\n";
	
	while(getline(fi, line)){

		stringstream ss(line);

		while (ss.good()) {
        string substr;
			getline(ss, substr, ',');
			solutionGrid[i][j] = stoi(substr);
			j++;
    	}
		i++;
		j = 0;
	}

	//cout << "Did we get here.\n";
	
	//Debug
	//print_game(); - replace mainGrid with solutionGrid

	fi.close();

	//Debug
	//return 0;

	//Initialize gameplay matrix
	int* begin = &mainGrid[0][0];
	size_t size = sizeof(mainGrid) / sizeof(mainGrid[0][0]);
	fill(begin, begin + size, -1);	

	int rc;
	int a =  0;//a indexes the thread_data_array which will store thread indexes in row major

	if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}

	//Have indexes increment by the size of the sub grids
	for (i=0; i<n; i+= subgrid_dim){//Row major traversal
		for (j=0; j<n; j+= subgrid_dim){

			//The indexes that each subgrid starts at (top left element in subgrid)
			thread_data_array[a].i_indx = i;
			thread_data_array[a].j_indx = j;
			thread_data_array[a].id = a;

			//cout << "ID for thread is " << a << endl;

			//Add a vector to hold the indexes; add a new vector for each subgrid/thread
			std::vector<std::pair<int,int>> vop;
			flag_vec.push_back(vop);
			click_vec.push_back(vop);
			
			rc = pthread_create(&threads[a], &attr, threadBlock, (void *) &thread_data_array[a] );
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

	if(lose_cond){//Check to see if AI solved the board
		cout << "Game lost!\n";
	}else{
		cout << "Board solved!\n";
	}
	print_game();//Print final game state
	// for(int i = 0; i < n; i++){
	// 	for(int j = 0; j < n; j++){
	// 		cout << "[" << solutionGrid[i][j] << "]";
	// 	}
	// 	cout << endl;
	// }

	printf("Number of FLOPs = %lu, Execution time = %f sec,\n%lf MFLOPs per sec\n", 2*n*n*n, time, 1/time/1e6*2*n*n*n);		
	
	return 0;
}
