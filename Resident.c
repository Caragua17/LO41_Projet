
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int *ptr;

/*=============================================================================
dealingSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete registration in the Dweller List and then exit.
*/
void dealingSIGINT(int num)
{
	if(num == SIGINT)
	{
		printf("\n: Au revoir.\n\n");
		int index = 0;
		
		while(shm_read(ptr,index,0) != getpid()){ index++; }
		
		shm_write(ptr, index, 0, 0);
		shm_write(ptr, index, 1, 0);
		shm_write(ptr, index, 2, 0);
		
		kill(shm_read(ptr,0,0), SIGUSR1);
		exit(0);
	}
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
	
	if(argc != 3)
	{
		printf("\033[1m\033[31m: use /Resident [floor] [door].\033[0m\n\n");
		exit(1);
	}
	signal(SIGINT, dealingSIGINT);
	
	int path[2] = {atoi(argv[1]), atoi(argv[2])};
	
	printf("\n: Résident %d\n: Je vis au %de étage, porte %d.\n",\
		getpid(), path[0], path[1]);
		
	/*-------------------------------------------------------------------------
	*	CONNECTION TO THE SEMAPHORE SEMDL
	*--------------------------------------------------------------------------
	
	Initialize semaphores for shared processes. This semaphore will secure the
	access of the 'dwellerList' shared memory.
	*/
	sem_t *semDL = sem_open("semDL", 0);
	
	if(semDL == SEM_FAILED)
	{
		perror("\033[1m\033[31m: Echec (semDL).\033[0m\n\n");
		exit(1);
	}
	/*-------------------------------------------------------------------------
	*	REGISTERING IN 'DWELLERLIST'
	*--------------------------------------------------------------------------
	
	This shared memory will stock all the dwellers addresses.
	We search for a free line, then we save the dweller in (pid, floor, door).
	We send a signal to the Building process to refresh the printed lists.
	*/
	int shmDL;
	
	if((shmDL = shmget(KEY_DL, 3*BUILDING_DWELLERS*sizeof(int), 0755)) == -1)
	{
		printf("\033[1m\033[31m: Echec de connexion (shmDL).\033[0m\n\n");
		exit(1);
	}
	else
	{
		printf(": Connecté à shmDL !\n");
	}
	ptr = (int*)shmat(shmDL, NULL, 0);
	
	printf(": Enregistrement (Immeuble %d)\n", shm_read(ptr,0,0));
	int index = 0;
	
	sem_wait(semDL); // LOCK ACCESS TO THE SHARED MEMORY
	
	while(shm_read(ptr,index,0) != 0){ index++; }
	
	shm_write(ptr, index, 0, getpid());
	shm_write(ptr, index, 1, path[0]);
	shm_write(ptr, index, 2, path[1]);
	
	sem_post(semDL); // UNLOCK ACCESS TO THE SHARED MEMORY
	
	printf(": Liste des résidents mise à jour !\n");	
	kill(shm_read(ptr,0,0), SIGUSR1);

	/*-------------------------------------------------------------------------
	*	ANSWERING A GUEST
	*--------------------------------------------------------------------------
	
	If a message is received, the dweller is asked to answer to the guest.
	If he answers 'yes', an other message is sent to the guest.
	Otherwise, the guest is killed thanks to a SIGINT signal.
	*/
	int msqid;
		
	if((msqid = msgget(KEY_VR, IPC_EXCL|0755)) == -1)
	{
		printf("\033[1m\033[31m\n: Echec de connexion (msqVR).\033[0m\n\n");
		exit(1);
	}
	while(TRUE)
	{
		long dest;
		msqbuf req;
		
		req = msq_receive(msqid, getpid());
		dest = req.sender;
		
		printf("\n: Permettre l'accès de %li ? (o/n) : ", dest);
		char c = 'a';
		
		while(c != 'o' && c != 'n')
		{
			c = getchar();
		}
		if(c == 'o')
		{
			msqbuf ans;
			ans.sender = getpid();
			ans.dest = dest;
			sprintf(ans.text, "%s", "OK");
			msq_send(msqid, dest, ans);
			printf("\n\033[1m\033[32m: Autorisation accordée !\033[0m\n");
		}
		else
		{
			kill(dest, SIGINT);
		}
	}// END WHILE

	return EXIT_SUCCESS;
}
