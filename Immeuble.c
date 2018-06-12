
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int msqid;
int shmDL;
int shmEL;
int *dwellerList;
int *elevatorList;

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete IPC objects in order to exit well the program.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		for(int i=0; i<100; i++){
			if(shm_read(dwellerList,i,0) != 0){
				kill(shm_read(dwellerList,i,0), SIGINT);
			}
		}
		shmdt(dwellerList);
		shmctl(shmDL, IPC_RMID, NULL);
		msgctl(msqid, IPC_RMID, NULL);
		shmctl(shmEL, IPC_RMID, NULL);
		printf("\n: Objets IPC supprimés !\n\n");
		exit(0);
	}
}
/*=============================================================================
traitantSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
Send by a dweller just after his registration, to print the updated list.
*/
void traitantSIGUSR(int num){

	clearScreen();
	printf(": shm ID = %d\n", shmDL);
	printf(": msq ID = %d\n", msqid);
	
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

	clearScreen();
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);	
	
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour Table des résidents 
	*------------------------------------------------------------------------*/
	printf("\n: Creation de la table des résidents...\n");
	
	if((shmDL = shmget(KEY_DL, 3*BUILDING_DWELLERS*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec.\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": shm ID = %d\n", shmDL);
		dwellerList = (int*)shmat(shmDL, NULL, 0);
		shm_init(dwellerList, 3*BUILDING_DWELLERS);
	}
	shm_write(dwellerList,0,0,getpid());
	
	printf("\n: Initialisation de la Liste des résidents\n");
	printf(": [ %d | %d | %d ]\n",shm_read(dwellerList,0,0),\
		shm_read(dwellerList,0,1), shm_read(dwellerList,0,2));
		
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour les pid ascenseurs
	*------------------------------------------------------------------------*/
	printf("\n: Creation de la table des ascenseurs...\n");
	
	if((shmEL = shmget(KEY_EL, 4*3*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec.\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": shm ID = %d\n", shmEL);
		elevatorList = (int*)shmat(shmEL, NULL, 0);
		shm_init(elevatorList, 4*3);
	}
	shm_write(dwellerList,0,0,getpid());

	/*-------------------------------------------------------------------------
	*	Création de la File de Message (Visiteur-Resident) 
	*------------------------------------------------------------------------*/
	printf("\n: Creation de la file de message...\n");
	
	if((msqid = msgget(KEY_VR, IPC_CREAT|IPC_EXCL|0755)) == -1){
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
	
	return EXIT_SUCCESS;
}
