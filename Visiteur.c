
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int status; 		//1: want enter, 2: auth and move, 3: arrived and go out

/*=============================================================================
dealingSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will kill the processus.
*/
void dealingSIGINT(int num)
{
	if(num == SIGINT)
	{
		if(status == 1)
		{
			printf("\033[1m\033[31m\n: Accès refusé.\033[0m\n\n");
			exit(1);
		}
		else
		{
			printf("\n: Au revoir.\n\n");
			exit(1);
		}
	}
}
/*=============================================================================
dealingSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
*/
void dealingSIGUSR(int num){

	if(num == SIGUSR1)
	{
		printf("\033[1m\033[32m: Arrivé à destination !\033[0m\n");
		status = 3;
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
	
	Checking arguments, initializing signals and some variables.
	*/
	clearScreen();
	status = 1;
	
	if(argc != 3)
	{
		perror("\033[1m\033[31m: use /Visiteur [floor] [door].\033[0m\n\n");
		exit(1);
	}
	int path[2] = {atoi(argv[1]), atoi(argv[2])};
	printf("\n: Visiteur %i. \n: Veut se rendre au %ie étage, porte %i.\n",\
		getpid(), path[0], path[1]);
		
	signal(SIGINT, dealingSIGINT);
	signal(SIGUSR1, dealingSIGUSR);

	/*-------------------------------------------------------------------------
	*	ACCESSING 'DWELLERLIST'
	*--------------------------------------------------------------------------
	
	This shared memory stocks all the dwellers addresses.
	We are looking into it to find the pid of the dweller we want to call.
	If we find a dweller living in the place we want to access, he's
	designated as receiver for our authorization request.
	*/
	int shmDL;
	int *dwellerList;
	int dest;
		
	if((shmDL = shmget(KEY_DL, 3*BUILDING_DWELLERS*sizeof(int), 0755)) == -1)
	{
		perror("\033[1m\033[31m\n: Echec de connexion (shmDL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": Connecté à shmDL !\n");
	}
	dwellerList = (int*)shmat(shmDL, NULL, 0);
	
	for(int i=0; i<100; i++)
	{
		if(shm_read(dwellerList,i,1) == path[0])
		{
			if (shm_read(dwellerList,i,2) == path[1])
				dest = shm_read(dwellerList,i,0);
		}
	}
	/*-------------------------------------------------------------------------
	*	ASKING AUTHORIZATION BY MESSAGE QUEUE
	*--------------------------------------------------------------------------
	
	A message is sent to the receiver identified just before.
	If the dweller answer to that request, authorization is done.
	Otherwise, the guest will receive a SIGINT signal.
	*/
	int msqid;
	
	if((msqid = msgget(KEY_VR, IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m\n: Echec de connexion (msqVR).\033[0m\n");
		exit(1);
	}
	msqbuf req1;
	req1.sender = getpid();
	req1.dest = dest;
	sprintf(req1.text, "%s", "J'aimerais entrer svp.");
	
	msq_send(msqid, dest, req1);
	
	msqbuf ans;
	ans = msq_receive(msqid, getpid());
	
	if(dest != ans.sender)
	{
		printf("\033[1m\033[31m: Mauvais expéditeur.\033[0m\n");
		exit(1);
	}
	else
	{
		printf("\033[1m\033[32m: Autorisation accordée !\033[0m\n");
		status = 2;
	}
	/*-------------------------------------------------------------------------
	*	ASKING THE BUILDING TO KNOW WHICH ELEVATOR TAKE
	*--------------------------------------------------------------------------
	*/
	int msqVI;
	
	if((msqVI = msgget(KEY_VI, IPC_EXCL|0755)) == -1)
	{
		perror("\033[1m\033[31m: Echec de création (msqVI).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf("\n: Connecté à msqVI !\n");
	}
	msqbuf req2;
	req2.sender = getpid();
	sprintf(req2.text, "%i", 0);
	
	msq_send(msqVI, shm_read(dwellerList,0,0), req2);
	
	msqbuf dir;
	dir = msq_receive(msqVI, getpid());
	long id = atoi(dir.text);
	
	/*-------------------------------------------------------------------------
	*	REGISTERING IN 'WAITINGLIST'
	*--------------------------------------------------------------------------
	
	Now we now in which elevator the guest has to go.
	The guest will register himself in the 'waitingList' of the elevator.
	*/
	int shmWL;
	int *waitingList;
		
	if((shmWL = shmget(KEY_WL+id,3*ELEVATOR_WAITSIZE*sizeof(int),0755)) == -1)
	{
		perror("\033[1m\033[31m\n: Echec de connexion (shmWL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf("\n: Connecté à shmWL de l'ascenseur %i !\n", 1);
	}
	waitingList = (int*)shmat(shmWL, NULL, 0);
	
	int index = 0;
	while(shm_read(waitingList,index,0) != 0){ index++; }
	
	shm_write(waitingList, index, 0, getpid());
	shm_write(waitingList, index, 1, 0);
	shm_write(waitingList, index, 2, path[0]);
	
	printf(": Je suis à l'étage 0.\n");
	kill(shm_read(waitingList,0,0), SIGUSR1); // ELEVATOR CALLED
	
	/*-------------------------------------------------------------------------
	*	THE GUEST IS LEAVING THE BUILDING
	*--------------------------------------------------------------------------
	
	When the elevator dropp off the guest at the right floor, it also sends him
	a signal which change current status. While status is different of 3, the
	guest can't do anything.
	Then, after a random time, he will decide to leave the building: calling
	again the elevator (registering in one of the 'waitingList').
	When he finally goes out, the process is killed.
	*/
	while(status != 3);
	
	delay(rand()%(10-5)+5);
	
	index = 0;
	while(shm_read(waitingList,index,0) != 0){ index++; }
	
	shm_write(waitingList, index, 0, getpid());
	shm_write(waitingList, index, 1, path[0]);
	shm_write(waitingList, index, 2, 0);

	printf("\n: Je suis à l'étage %d.\n",path[0]);
	kill(shm_read(waitingList,0,0), SIGUSR1); // ELEVATOR CALLED
	
	kill(getpid(),SIGINT);

	return EXIT_SUCCESS;
}
