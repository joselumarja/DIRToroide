#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <definitions.h>
#include <utilities.h>

void neightborsToroid(int Size, int Side, int Rank, int *Neightbors);
int neighbourNorth(int Rank, int Side, int Size);
int neighbourSouth(int Rank, int Side, int Size);
int neighbourEast(int Rank, int Side);
int neighbourWest(int Rank, int Side);

void parseArgv(int argc, char *argv[], char *DataFilePath, int *ToroidSide);
void checkArgv(int Size, int ToroidSide, char *DataFilePath);
void notifyErrorAndClose(MPI_Comm comm, short ErrorCode);
void printUsage();

void dispatchDataToNodes(char *DataFilePath, int Size, MPI_Comm comm, int *RootData);

void calcMinToroidNetwork(int Side,int *Neightbors, int LocalData, int *Min); 

int main(int argc, char *argv[])
{
	int Rank, Size, Data, Min, Side;
	clock_t ExecutionTime;
	int Neightbors[N_NEIGHTBORS_TOROID];
	MPI_Status status;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
	MPI_Comm_size(MPI_COMM_WORLD, &Size);

	if(Rank==ROOT)
	{
		char DataFilePath[MAX_DATA_BLOCK];
		int ToroidSide;
		
		parseArgv(argc, argv, DataFilePath, &ToroidSide);
		
		checkArgv(Size, ToroidSide, DataFilePath);

		short StatusCode = STATUS_OK;
		
		if(MPI_Bcast(&StatusCode, 1, MPI_SHORT, ROOT, MPI_COMM_WORLD)!=MPI_SUCCESS)
		{
			fprintf(stderr,"Fail in processes notification\n");
			exit(EXIT_FAILURE);
		}
		
		dispatchDataToNodes(DataFilePath, Size, MPI_COMM_WORLD, &Data);
		
	}
	else
	{
		short StatusCode;
		MPI_Bcast(&StatusCode, 1, MPI_SHORT, ROOT, MPI_COMM_WORLD);
		
		if(StatusCode<0)
		{
			fprintf(stderr, "Stop process %d\n",Rank);
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}
		
		MPI_Recv(&Data, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}
	
	Side = atoi(argv[2]);
	
	neightborsToroid(Size, Side, Rank, Neightbors);
	
	ExecutionTime=clock();	
	calcMinToroidNetwork(Side, Neightbors, Data, &Min);
	ExecutionTime=clock()-ExecutionTime;
	
	if(Rank==ROOT)
	{
		printf("Execution Time of The Algorithm: %f\n", (ExecutionTime/(double)CLOCKS_PER_SEC));
		printf("Min of the network: %d\n", Min);
	}
	
	MPI_Finalize();
	
	return EXIT_SUCCESS;
}
	
void parseArgv(int argc, char *argv[], char *DataFilePath, int *ToroidSide)
{
	
	if(argc!=3)
	{
		printUsage();
		notifyErrorAndClose(MPI_COMM_WORLD, WRONG_ARGUMENT_ERROR_CODE);
	}
	
	strcpy(DataFilePath,argv[1]);
	*ToroidSide=atoi(argv[2]);
}

void checkArgv(int Size, int ToroidSide, char *DataFilePath)
{
	
	if(Size!=(ToroidSide*ToroidSide))
	{
		fprintf(stderr,"Error, wrong Toroid Size\n");
		notifyErrorAndClose(MPI_COMM_WORLD, WRONG_NUMBER_OF_LAUNCHED_PROCESSES);
	}
	
	if(isFile(DataFilePath)==-1)
	{
		fprintf(stderr, "Error, in the specified file path\n"); 
		notifyErrorAndClose(MPI_COMM_WORLD, WRONG_FILE_PATH);
	}
	
	if(countDataNumber(DataFilePath)<Size)
	{
		fprintf(stderr, "Error, not enought data for toroid\n");
		notifyErrorAndClose(MPI_COMM_WORLD, NOT_ENOUGHT_DATA_FOR_PROCESSES);
	}
	
}

void printUsage()
{
	printf("Correct use of the program: mpirun -n <Number of processes to launch> ./Toroid <Data file path> <Toroid Side>\n");
}

void notifyErrorAndClose(MPI_Comm comm, short ErrorCode)
{
	if(MPI_Bcast(&ErrorCode, 1, MPI_SHORT, ROOT, comm)!=MPI_SUCCESS)
	{
		fprintf(stderr,"Fail in processes notification\n");
	}
	MPI_Finalize();
	exit(EXIT_FAILURE);
}

void dispatchDataToNodes(char *DataFilePath, int Size, MPI_Comm comm, int *RootData)
{
	FILE *DataFile;

	int DataToSend,i;
	
	if((DataFile = fopen(DataFilePath,"r"))==NULL)
	{
		fprintf(stderr, "Error in open file %s : %s\n",DataFilePath,strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	fscanf(DataFile,"%d,",RootData);
	
	for(i=1; i<Size; i++)
	{
		fscanf(DataFile,"%d,",&DataToSend);
		MPI_Send(&DataToSend, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	}
	
	fclose(DataFile);
}

void calcMinToroidNetwork(int Side,int *Neightbors, int LocalData, int *Min)
{
	int i,ReceivedData;
	*Min=LocalData;
	
	for(i=0; i<Side; i++)
	{
		MPI_Send(Min, 1, MPI_INT, Neightbors[NEIGHTBOUR_NORTH_TOROID], 0, MPI_COMM_WORLD);
		MPI_Recv(&ReceivedData, 1, MPI_INT, Neightbors[NEIGHTBOUR_SOUTH_TOROID], MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		if(ReceivedData<*Min)
		{
			*Min=ReceivedData;
		}
	} 
	
	for(i=0; i<Side; i++)
	{
		MPI_Send(Min, 1, MPI_INT, Neightbors[NEIGHTBOUR_EAST_TOROID], 0, MPI_COMM_WORLD);
		MPI_Recv(&ReceivedData, 1, MPI_INT, Neightbors[NEIGHTBOUR_WEST_TOROID], MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		if(ReceivedData<*Min)
		{
			*Min=ReceivedData;
		}
	}
}
	
void neightborsToroid(int Size, int Side, int Rank, int *Neightbors)
{	
	Neightbors[NEIGHTBOUR_NORTH_TOROID]=neighbourNorth(Rank,Side,Size);
	Neightbors[NEIGHTBOUR_SOUTH_TOROID]=neighbourSouth(Rank,Side,Size);
	Neightbors[NEIGHTBOUR_EAST_TOROID]=neighbourEast(Rank, Side);
	Neightbors[NEIGHTBOUR_WEST_TOROID]=neighbourWest(Rank, Side);
}
	
int neighbourNorth(int Rank, int Side, int Size)
{
	int Neighbour;
	
	Neighbour = Rank + Side;
	
	if(Neighbour>=Size)
	{
		Neighbour = Neighbour - Size;
	}
	
	return Neighbour;
}

int neighbourSouth(int Rank, int Side, int Size)
{
	int Neighbour;
	
	Neighbour = Rank - Side;
	
	if(Neighbour<0)
	{
		Neighbour = Size + Neighbour;
	}
	
	return Neighbour;
}

int neighbourEast(int Rank, int Side)
{
	int Neighbour;
	
	if(((Rank+1)%Side)==0)
	{
		Neighbour = Rank - (Side-1);
		
		return Neighbour;
	}
	
	Neighbour = Rank + 1;
	
	return Neighbour;
}

int neighbourWest(int Rank, int Side)
{
	int Neighbour;
	
	if((Rank%Side)==0)
	{
		Neighbour = Rank + (Side - 1);
		
		return Neighbour;
	}
	
	Neighbour = Rank -1;
	
	return Neighbour;
}
