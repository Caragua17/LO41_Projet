
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int shmWL;
int *waitingList;
int *elevatorList;

int count;
int passengers[ELEVATOR_CAPACITY][3];

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
		int index = 0;
		printf("\n: Waiting list\n");
		for(int i=0; i<count; i++){
			if(shm_read(waitingList,i,0) != 0){
				printf(": [ %d | %d | %d ]\n", shm_read(waitingList,i,0),\
					shm_read(waitingList,i,1), shm_read(waitingList,i,2));
			}
		}
		printf("\n: Passengers list\n");
		for(int i=0; i<count; i++){
			printf(": [ %d | %d | %d ]\n", passengers[count][0],\
				passengers[count][1], passengers[count][2]);
		}
	}
	else{/*SIGUSR2*/}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){

	int current = 0;
	int goingUp = 1;
	int id;
	int working = 1;

	clearScreen();

	if(argc == 2){
		id = atoi(argv[1]);
		printf("\n: Ascenseur %i en fonctionnement !\n\n", id);
	}
	else{
		printf("\033[1m\033[31m\n: Ascenseur en panne !\033[0m\n\n");
		exit(0);
	}
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);

	printf("\33[1A\033[1m: étage actuel ( \033[33m%i\033[37m )\n", current);
	
	/*-------------------------------------------------------------------------
	*	Connexion table des Ascenseurs
	*------------------------------------------------------------------------*/	
	int shmEL;
	
	if((shmEL = shmget(KEY_EL, 4*3*sizeof(int), 0755)) == -1){
			perror("\033[1m\033[31m: Echec de connexion (shmEL).\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": Connecté à shmEL !\n");
		elevatorList = (int*)shmat(shmEL, NULL, 0);
		shm_init(elevatorList, 4*3);
	}
	shm_write(elevatorList,id,0,getpid());
	
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour Liste d'attente 
	*------------------------------------------------------------------------*/
	if((shmWL = shmget(KEY_WL+id, 300*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec de création (shmWL).\033[0m\n\n");
			exit(1);
	}
	else{
		printf("\n: Création de shmWL (id=%d).\n", shmWL);
		waitingList = (int*)shmat(shmWL, NULL, 0);
		shm_init(waitingList, 300);
	}
	count = 0;
	
	while(working == 1){
		for(int i=0; i<ELEVATOR_CAPACITY; i++){
			if (current == shm_read(waitingList, i, 1)){
				passengers[count][0] = shm_read(waitingList, i, 0);
				passengers[count][1] = shm_read(waitingList, i, 1);
				passengers[count][2] = shm_read(waitingList, i, 2);
				count++;
			}
		}
	}
	
	printf("\n");
	
	return EXIT_SUCCESS;
}
