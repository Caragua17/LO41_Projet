
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int msqVR;
int msqVI;
int shmDL;
int shmEL;
int *dwellerList;
int *elevatorList;

/*=============================================================================
dealingSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete IPC objects in order to exit well the program.
*/
void dealingSIGINT(int num)
{
	if(num == SIGINT)
	{
		for(int i=0; i<100; i++)
		{
			if(shm_read(dwellerList,i,0) != 0)
				kill(shm_read(dwellerList,i,0), SIGINT);
		}
		shmdt(dwellerList);
		shmctl(shmDL, IPC_RMID, NULL);
		msgctl(msqVR, IPC_RMID, NULL);
		msgctl(msqVI, IPC_RMID, NULL);
		shmctl(shmEL, IPC_RMID, NULL);
		printf("\n: Objets IPC supprimés !\n\n");
		exit(0);
	}
}
/*=============================================================================
dealingSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
Send by a dweller just after his registration, to print the updated list.
*/
void dealingSIGUSR(int num)
{
	clearScreen();
	
	printf(": shmEL = %d\n", shmEL);
	printf(": shmDL = %d\n", shmDL);
	printf(": msqVR = %d\n", msqVR);
	
	if(num == SIGUSR1)
	{
		printf("\n: Mise à jour de la Liste des résidents\n");
		
		for(int i=0; i<100; i++)
		{
			if(shm_read(dwellerList,i,0) != 0)
				printf(": [ %d | %d | %d ]\n",shm_read(dwellerList,i,0),\
					shm_read(dwellerList,i,1), shm_read(dwellerList,i,2));
		}
	}
	else{/*SIGUSR2*/}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[])
{
	/*-------------------------------------------------------------------------
	*	INITIALIZATION
	*--------------------------------------------------------------------------
	
	Mainly for signals.
	*/
	clearScreen();

	signal(SIGINT, dealingSIGINT);
	signal(SIGUSR1, dealingSIGUSR);
	
	/*-------------------------------------------------------------------------
	*	CREATE SEMAPHORE
	*--------------------------------------------------------------------------
	
	printf("\n: Creation des sémaphores...\n");
	
	int semDL;
	
	if((semDL = sem_open("semDL", O_CREAT, O_RDWR, 1)) == -1)
	{
		perror("\033[1m\033[31m: Echec (semDL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": shmSE = %d\n", semDL);
	}
*/
	/*-------------------------------------------------------------------------
	*	CREATE SHARED MEMORY 'DWELLERLIST'
	*--------------------------------------------------------------------------
	
	This shared memory will stock all the dwellers addresses. 
	It's like a 2-dimensional table, with each {pid, floor, door}.
	The Building process pid is saved on the first line.
	*/
	printf("\n: Creation de la table des résidents...\n");
	
	if((shmDL = shmget(KEY_DL, 3*BUILDING_DWELLERS*sizeof(int),\
	IPC_CREAT|IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec (shmDL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": shmDL = %d\n", shmDL);
		dwellerList = (int*)shmat(shmDL, NULL, 0);
		shm_init(dwellerList, 3*BUILDING_DWELLERS);
	}
	shm_write(dwellerList,0,0,getpid());
		
	/*-------------------------------------------------------------------------
	*	CREATE SHARED MEMORY 'ELEVATORLIST'
	*--------------------------------------------------------------------------
	
	Register the {pid, current floor,capacity} of each elevator.
	Usefull infos for the 'Building' process in order to decide in which
	elevator it's going to redirect a guest.
	The Building process pid is also saved on the first line.
	*/
	printf("\n: Creation de la table des ascenseurs...\n");
	
	if((shmEL = shmget(KEY_EL, 4*3*sizeof(int),\
	IPC_CREAT|IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec (shmEL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": shmEL = %d\n", shmEL);
		elevatorList = (int*)shmat(shmEL, NULL, 0);
		shm_init(elevatorList, 4*3);
	}
	shm_write(dwellerList,0,0,getpid());

	/*-------------------------------------------------------------------------
	*	CREATE MESSAGE QUEUE GUEST-DWELLER
	*--------------------------------------------------------------------------
	
	Used by guests in order to ask authorization at a specific dweller to enter
	inside the building
	*/
	printf("\n: Creation de la file de message Visiteur-Resident...\n");
	
	if((msqVR = msgget(KEY_VR, IPC_CREAT|IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec (msqVR).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": msqVR = %d\n", msqVR);
	}
	/*-------------------------------------------------------------------------
	*	CREATE MESSAGE QUEUE BUILDING-GUEST
	*--------------------------------------------------------------------------
	
	Here is the message queue which will be used to assign a scpecific
	elevator for each guest who just entered the building.
	*/
	printf("\n: Creation de la file de message Visiteur-Immeuble...\n");
	
	if((msqVI = msgget(KEY_VI, IPC_CREAT|IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec (msqVI).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": msqVI = %d\n", msqVI);
	}
	/*-------------------------------------------------------------------------
	*	ELEVATOR CHOICE ALGORITHM
	*--------------------------------------------------------------------------
	
	Receive the request from a guest.
	Choose to assign the guest at an elevator which is already on the right
	floor, or which is the nearest.
	If there is no elevator with remained capacity, it will assign the guest
	to a random elevator
	*/
	while(1)
	{
		int distance = 25;
		int bestElevator = 0;
		msqbuf req;
		req = msq_receive(msqVI, getpid());
		
		if(req.dest != -1 && req.sender != -1)
		{
			for(int i=1; i<=3; i++)
			{
				// On the right floor?
				if(shm_read(elevatorList,i,1) == 0\
				&& shm_read(elevatorList,i,2) > 0)
				{
					printf(": BEST (rdc) %d\n", i);
					bestElevator = i;
				}
				// The nearest?
				else if(shm_read(elevatorList,i,1) < distance\
					 && shm_read(elevatorList,i,2) > 0\
					 && bestElevator == 0)
				{
					printf(": BEST (distance) %d\n", i);
					distance = shm_read(elevatorList,i,1);
					bestElevator = i;
				}
				// No places, so random:
				else if(bestElevator == 0)
				{
					printf(": BEST (random) %d\n", i);
					bestElevator = rand()%(3)+1;
				}
			}
			req.dest = req.sender;
			req.sender = getpid();
			sprintf(req.text, "%i", bestElevator);
			msq_send(msqVI, req.dest, req);
		}
	}// END WHILE
	
	return EXIT_SUCCESS;
}
