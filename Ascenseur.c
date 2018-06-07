
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int shmWL;
int *waitingList;

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete IPC objects then kill the processus.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		shmdt(waitingList);
		shmctl(shmWL, IPC_RMID, NULL);
		printf("\n: Objets IPC supprimés !\n\n");
		exit(0);
	}
}
/*=============================================================================
traitantSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
*/
void traitantSIGUSR(int num){

	if(num == SIGUSR1){

	}
	else{
	
	}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){

	//clearScreen();
	
	int current = 0;
	int goingUp = 1;

	if(argc == 2){
		int id = atoi(argv[1]);
		printf("\n: Ascenseur %i en fonctionnement !", id);
	}
	else{
		printf("\033[1m\033[31m\n: Ascenseur en panne !\033[0m\n\n");
	}
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);
	/*
	printf("\33[1A\033[1m: étage actuel ( \033[33m%i\033[37m )\n", current);
	*/
	
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour Liste d'attente 
	*------------------------------------------------------------------------*/
	if((shmWL = shmget(KEY_WL, 300*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec.\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": shm ID = %d\n", shmWL);
		waitingList = (int*)shmat(shmWL, NULL, 0);
		shm_init(waitingList, 300);
	}
	
	kill(getpid(), SIGINT);
	
	printf("\n");
	
	return EXIT_SUCCESS;
}
