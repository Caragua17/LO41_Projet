
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int shmWL;
int *waitingList;
int *elevatorList;
int status;
int passengers[ELEVATOR_CAPACITY][3];

/*=============================================================================
emptyLists: will check if someone is waiting the elevator or if someone is in.
Return TRUE if the 'waitingList' and 'passengers' lists are empty.
*/
int emptyLists()
{
	int nobody = TRUE;
	
	for(int i=1; i<ELEVATOR_WAITSIZE; i++)
	{
		if(shm_read(waitingList, i, 0) != 0)
			nobody = FALSE;
	}
	for(int i=0; i<ELEVATOR_CAPACITY; i++)
	{
		if(passengers[i][0] != 0)
			nobody = FALSE;
	}
	return nobody;
}
/*=============================================================================
needStop: decide if the elevator needs to stop at 'current' floor.
Return TRUE if there is someone to pickup or drop off at the 'current' floor.
*/
int needStop(int current)
{
	for(int i=1; i<ELEVATOR_WAITSIZE; i++)
	{
		// If someone is waiting at this floor:
		if(shm_read(waitingList, i, 1) == current\
		&& shm_read(waitingList, i, 0) != 0 )
			return TRUE;
	}
	for(int i=0; i<ELEVATOR_CAPACITY; i++)
	{
		//If someone wants to leave out at this floor:
		if(passengers[i][2] == current && passengers[i][0] != 0)
			return TRUE;
	}
	return FALSE;
}
/*=============================================================================
dealingSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete IPC objects then kill the processus.
*/
void dealingSIGINT(int num)
{
	if(num == SIGINT)
	{
		shmdt(waitingList);
		shmctl(shmWL, IPC_RMID, NULL);
		printf("\n: Objets IPC supprimés !\n\n");
		exit(0);
	}
}
/*=============================================================================
dealingSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
	SIGUSR1: signal received if a guest has requested the elevator.
	SIGUSR2: ...
*/
void dealingSIGUSR(int num)
{
	if(num == SIGUSR1)
	{
		if(emptyLists() != TRUE)
		{
			/*
			Print 'waitingList' and 'passengers', act like a refresh.
			*/
			printf("---------------------------------\n: Waiting list\n");
			for(int i=1; i<ELEVATOR_WAITSIZE; i++)
			{
				if(shm_read(waitingList,i,0) != 0)
				{
					printf(": [ %d | %d | %d ]\n", shm_read(waitingList,i,0),\
						shm_read(waitingList,i,1), shm_read(waitingList,i,2));
				}
			}
			printf("\n: Passengers list\n");
			for(int i=0; i<ELEVATOR_CAPACITY; i++)
			{
				if(passengers[i][0] != 0)
				{
					printf(": [ %d | %d | %d ]\n", passengers[i][0],\
						passengers[i][1], passengers[i][2]);
				}
			}
			printf("---------------------------------\n");
			status = STOP;
		}
		else{
			printf("---------------------------------\n: Listes vides");
			printf("\n---------------------------------\n");
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

	id: id of the elevator (1-2-3)
	
	current: floor where is the elevator
	capacity: number of remaining places in the elevator
	
	goingUp: boolean allowing to know in which direction is going the elevator
	working: boolean which aims to know if the elevator needs a technician
	*/
	int id;
	int current = 0;
	int capacity = ELEVATOR_CAPACITY;
	int goingUp = TRUE;					// TRUE: going up, FALSE: going down
	int working = TRUE;

	clearScreen();

	if(argc == 2)
	{
		id = atoi(argv[1]);
		printf("\n: Ascenseur %i en fonctionnement !\n\n", id);
	}
	else
	{
		printf("\033[1m\033[31m\n: Ascenseur en panne !\033[0m\n\n");
		exit(0);
	}
	signal(SIGINT, dealingSIGINT);
	signal(SIGUSR1, dealingSIGUSR);
	
	/*-------------------------------------------------------------------------
	*	REGISTERING IN THE ELEVATOR LIST
	*--------------------------------------------------------------------------
	
	Register its pid, current floor and capacity in this list.
	Usefull infos for the 'Building' process in order to decide in which
	elevator it's going to redirect a guest.
	*/	
	int shmEL;
	
	if((shmEL = shmget(KEY_EL, 4*3*sizeof(int), 0755)) == -1)
	{
			perror("\033[1m\033[31m: Echec de connexion (shmEL).\033[0m\n\n");
			exit(1);
	}
	else
	{
		printf(": Connecté à shmEL !\n");
		elevatorList = (int*)shmat(shmEL, NULL, 0);
	}
	shm_write(elevatorList,id,0,getpid());
	shm_write(elevatorList,id,1,current);
	shm_write(elevatorList,id,2,capacity);	
	
	/*-------------------------------------------------------------------------
	*	CREATE SHARED MEMORY 'WAITING LIST'
	*--------------------------------------------------------------------------
	
	This shared memory will stock all guests waiting for the elevator with:
	their pid, current floor and destination floor.
	Used by the elevator algorithm to determine its way.
	*/
	if((shmWL = shmget(KEY_WL+id, 3*ELEVATOR_WAITSIZE*sizeof(int),\
	IPC_CREAT|IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec de création (shmWL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": Création de shmWL (id=%d).\n", shmWL);
		waitingList = (int*)shmat(shmWL, NULL, 0);
		shm_init(waitingList, 300);
		shm_write(waitingList, 0, 0, getpid());
	}
	printf("\n: Etage actuel ( \033[33m\033[1m%i\033[0m )\n", current);
	status = STOP;

	/*-------------------------------------------------------------------------
	*	ELEVATOR ALGORITHM
	*--------------------------------------------------------------------------
	
	Seen as a state machine with 3 distincts states:
	1. MOVE: the elevator is moving between floors,
	2. STOP: currently at a floor,
	3. STANDBY: No one is using it, waiting for new arrivals.
	*/
	while(working == TRUE)
	{
		switch(status)
		{
		case STANDBY:
			break;
			
		case MOVE:
		/*
		if the current direction is UP then the elevator check if there is
		passengers who want to go up or guests waiting in upper floors.
			Otherwise, the elevator will change its direction.
		*/	
			if(goingUp == TRUE)
			{
				int callUp = FALSE;
				
				for(int i=0; i<ELEVATOR_WAITSIZE; i++)
				{
					if(shm_read(waitingList,i,1) > current)
						callUp = TRUE;
				}
				for(int i=0; i<ELEVATOR_CAPACITY; i++)
				{
					if(passengers[i][2] > current)
						callUp = TRUE;
				}
				goingUp = callUp;
			}
			/*
			Same but checking if there is someone else in lower floors or
			passengers who want to go down.
			*/	
			else
			{
				int callDown = FALSE;
				for(int i=0; i<ELEVATOR_WAITSIZE; i++)
				{
					if(shm_read(waitingList,i,1) < current)
						callDown = TRUE;
				}
				for(int i=0; i<ELEVATOR_CAPACITY; i++)
				{
					if(passengers[i][2] < current)
						callDown = TRUE;
				}
				goingUp = !callDown;
			}
			/*
			Now it's time to "move" the elevator at the next floor (up or down)
			There is a delay to make this move more real.
			After modifying the current floor, we update the 'elevatorList'.
			*/	
			delay(ELEVATOR_MOVING_T);
			
			if(goingUp == TRUE && current < 25)
				current++;
			else if(goingUp == FALSE && current > 0)
				current--;
				
			shm_write(elevatorList,id,1,current);
			printf(": Etage actuel ( \033[33m\033[1m%i\033[0m )\n", current);
			/*
			Next status will be 'STOP' at the next floor...
			*/
			status = STOP;
			break;
			
		case STOP:
		/*
		if a stop is needed (guests at this floor or passengers going out here)
		we register new passengers and remove them from the 'waitingList'.
		Passengers who want to go out are just removed from 'passengers'.
		A clock is started to wait N seconds for new arrivals.
		*/	
    		if( needStop(current) == TRUE )
    		{
    			clock_t start, end;
    			start=clock();
				
				while(((end=clock())-start) < ELEVATOR_STOPPING*CLOCKS_PER_SEC)
				{
					for(int i=1; i<ELEVATOR_WAITSIZE; i++)
					{
						/*
						Looking for new passengers waiting at this floor:
						*/
						if (current == shm_read(waitingList, i, 1)\
						&& shm_read(waitingList, i, 0) != 0)
						{
							int index = 0;
							/*
							Transfer the registration from 'waitingList' to
							'passengers' list. 
							*/
							while(passengers[index][0] != 0){ index++; }
							
							passengers[index][0] = shm_read(waitingList, i, 0);
							passengers[index][1] = shm_read(waitingList, i, 1);
							passengers[index][2] = shm_read(waitingList, i, 2);

							shm_write(waitingList, i, 0, 0);
							shm_write(waitingList, i, 1, 0);
							shm_write(waitingList, i, 2, 0);
							
							printf("\033[36m\033[1m");
							printf(": Le passager %d est monté.\n\033[0m",\
								passengers[index][0]);
							/*
							We send a signal to refresh the printed lists and
							we update the remained capacity.
							*/
							kill(getpid(),SIGUSR1);
							capacity--;
						}
					}
					for(int i=0; i<ELEVATOR_CAPACITY; i++)
					{
						/*
						Looking for any passengers who want to go out:
						if it's time for them to go out, we send them a signal.
						Their entry is removed from 'passengers'.
						*/
						if(passengers[i][2] == current && passengers[i][0] !=0)
						{
							kill(passengers[i][0], SIGUSR1);
							printf("\033[36m\033[1m");
							printf(": Le passager %d est descendu.\n\033[0m",\
								passengers[i][0]);	
							passengers[i][0] = 0;
							passengers[i][1] = 0;
							passengers[i][2] = 0;
							/*
							We send a signal to refresh the printed lists and
							we update the remained capacity.
							*/
							kill(getpid(),SIGUSR1);
							capacity++;
						}
					}
    			}
    		}
    		/*
    		The new capacity is saved in the shared memory.
    		If the lists are empty: the elevator will enter its STANDBY mode.
    		Otherwise, it will enter in its MOVE status.
    		*/
			shm_write(elevatorList,id,2,capacity);
    		
			if( emptyLists() == TRUE )
			{
				status = STANDBY;
				printf("\033[35m\033[1m: En attente...\033[0m\n");
			}
			else
			{
				status = MOVE;
			}
			break;
		}
	}// END WHILE
	
	return EXIT_SUCCESS;
}
