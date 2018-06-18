
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int status; //1: want enter 2: auth and move 3: arrived and go out

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will kill the processus.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		if(status != INSIDE){
			printf("\033[1m\033[31m\n: Accès refusé.\033[0m\n\n");
			exit(1);
		}
		else{
			printf("\033[1m\033[32m\n: Arrivé à destination !\033[0m\n\n");
			exit(1);
		}
	}
}
/*=============================================================================
traitantSIGUSR: executed when a 'SIGUSR1/SIGUSR2' signal is intercepted.
*/
void traitantSIGUSR(int num){

	if(num == SIGUSR1){

	}
	else{/*SIGUSR2*/
	
	}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){
	
	clearScreen();
	status = WAITING;
	
	if(argc != 3){
		perror("\033[1m\033[31m: use /Visiteur [floor] [door].\033[0m\n\n");
		exit(1);
	}
	int path[2] = {atoi(argv[1]), atoi(argv[2])};
	
	printf("\n: Visiteur %i. \n: Veut se rendre au %ie étage, porte %i.\n",\
		getpid(), path[0], path[1]);
		
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);

	/*-------------------------------------------------------------------------
	*	Connexion à la memoire partagée (Dweller List)
	*------------------------------------------------------------------------*/
	int shmDL;
	int *dwellerList;
	int dest;
		
	if((shmDL = shmget(KEY_DL, 3*BUILDING_DWELLERS*sizeof(int), 0755)) == -1){
		perror("\033[1m\033[31m\n: Echec de connexion (shmDL).\033[0m\n\n");
		exit(1);
	}
	else{
		printf(": Connecté à shmDL !\n");
	}
	dwellerList = (int*)shmat(shmDL, NULL, 0);
	
	for(int i=0; i<100; i++){
		if(shm_read(dwellerList,i,1) == path[0]){
			if (shm_read(dwellerList,i,2) == path[1]){
				dest = shm_read(dwellerList,i,0);
			}
		}
	}
	/*-------------------------------------------------------------------------
	*	Connexion file de message (Visiteur-Résident)
	*------------------------------------------------------------------------*/
	int msqid;
	
	if((msqid = msgget(KEY_VR, IPC_EXCL|0755)) == -1){
		perror("\033[1m\033[31m\n: Echec de connexion (msqVR).\033[0m\n");
		exit(1);
	}
	msqbuf msg;
	msg.sender = getpid();
	msg.dest = dest;
	sprintf(msg.text, "%s", "J'aimerais entrer svp.");
	
	msq_send(msqid, dest, msg);
	
	if(dest != msq_receive(msqid, getpid()) ){
		printf("\033[1m\033[31m: Mauvais expéditeur.\033[0m\n");
		exit(1);
	}
	else{
		printf("\033[1m\033[32m\n: Autorisation accordée !\033[0m\n");
		status = INSIDE;
	}
	
	/*
	COMMUNIQUE AVEC IMMEUBLE POUR CONNAITRE NUM ASCENSEUR
	
	
	/*-------------------------------------------------------------------------
	*	Connexion à la memoire partagée (waiting List)
	*------------------------------------------------------------------------*/
	int shmWL;
	int *waitingList;
		
	if((shmWL = shmget(KEY_WL+1, 3*ELEVATOR_WAITSIZE*sizeof(int),0755)) == -1){
		perror("\033[1m\033[31m\n: Echec de connexion (shmWL).\033[0m\n\n");
		exit(1);
	}
	else{
		printf("\n: Connecté à shmWL de l'ascenseur %i !\n", 1);
	}
	waitingList = (int*)shmat(shmWL, NULL, 0);
	
	int index = 0;
	while(shm_read(waitingList,index,0) != 0){
		index++;
	}
	shm_write(waitingList, index, 0, getpid());
	shm_write(waitingList, index, 1, 0);
	shm_write(waitingList, index, 2, path[0]);
	
	printf("\n: Je suis à l'étage 0.\n");
	
	kill(shm_read(waitingList,0,0), SIGUSR1); // APPEL ASCENSEUR
	
	while(1);
	
	shmdt(waitingList);	
	shmdt(dwellerList);
	printf("\n");

	return EXIT_SUCCESS;
}
