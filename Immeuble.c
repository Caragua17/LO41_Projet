
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int msqid;
int shmid;
int *dwellerList;

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete IPC objects in order to exit well the program.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		shmdt(dwellerList);
		shmctl(shmid, IPC_RMID, NULL);
		msgctl(msqid, IPC_RMID, NULL);
		printf("\n: Objets IPC supprimés !\n\n");
		exit(0);
	}
}
/*=============================================================================
traitantSIGUSR1: executed when a 'SIGUSR1' signal is intercepted.
Send by a dweller just after his registration, to print the updated list.
*/
void traitantSIGUSR(int num){

	if(num == SIGUSR1){
		printf("\n: Mise à jour de la Liste des résidents\n");
		for(int i=0; i<100; i++){
			if(shm_read(dwellerList,i,0) != 0){
				printf(": [ %d | %d | %d ]\n",shm_read(dwellerList,i,0),\
					shm_read(dwellerList,i,1), shm_read(dwellerList,i,2));
			}
		}
	}
	else{/*SIGUSR2*/}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){

	/*-------------------------------------------------------------------------
	*	Création des 3 Ascenseurs (avec recouvrement) + signals
	*------------------------------------------------------------------------*/
	clearScreen();
	printf("\n");
	
	pid_t pid = 1;
	int elevators = 3;
	int status;
	char *param = malloc(sizeof(char));
	
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);	
	
	while(pid > 0 && elevators > 0){
		pid = fork();	
		if(pid == 0){ // dans le FILS
			sprintf(param, "%i", elevators);
			execl("./Ascenseur", "Ascenseur", param, NULL);
		}
		elevators--;
	} // dans le PERE, on attend la fin des ascenseurs...
	wait(&status); wait(&status); wait(&status);
	
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour Table des résidents 
	*------------------------------------------------------------------------*/
	printf("\n: Creation de la Mémoire partagée...\n");
	
	if((shmid = shmget(KEY, 3*DWELLERS*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec.\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": shm ID = %d\n", shmid);
		dwellerList = (int*)shmat(shmid, NULL, 0);
		shm_init(dwellerList, 3*DWELLERS);
	}
	shm_write(dwellerList,0,0,getpid());
	
	printf("\n: Initialisation de la Liste des résidents\n");
	printf(": [ %d | %d | %d ]\n",shm_read(dwellerList,0,0),\
		shm_read(dwellerList,0,1), shm_read(dwellerList,0,2));

	/*-------------------------------------------------------------------------
	*	Création de la File de Message (Visiteur-Resident) 
	*------------------------------------------------------------------------*/
	printf("\n: Creation de la file de message...\n");
	
	if((msqid = msgget(KEY, IPC_CREAT|IPC_EXCL|0755)) == -1){
		perror("\033[1m\033[31m: Echec.\033[0m\n\n");
		exit(1);
	}
	else{
		printf(": msq ID = %d\n", msqid);
	}
	/*-------------------------------------------------------------------------
	*	BOUCLE INFINIE
	*------------------------------------------------------------------------*/
	while(1);	
	
	/*-------------------------------------------------------------------------
	*	Suppression des objets IPC shm & msq
	*------------------------------------------------------------------------*/
	
	shmdt(dwellerList);
	shmctl(shmid, IPC_RMID, NULL);
	msgctl(msqid, IPC_RMID, NULL);
	printf("\n: Objets IPC supprimés !\n\n");

	return EXIT_SUCCESS;
}
