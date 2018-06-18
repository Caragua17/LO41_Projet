
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int shmWL;
int *waitingList;
int *elevatorList;

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

	if(num == SIGUSR1){ // A guest has requested the elevator
		int index = 0;
		printf("\n: Waiting list\n");
		for(int i=1; i<ELEVATOR_WAITSIZE; i++){
			if(shm_read(waitingList,i,0) != 0){
				printf(": [ %d | %d | %d ]\n", shm_read(waitingList,i,0),\
					shm_read(waitingList,i,1), shm_read(waitingList,i,2));
			}
		}
		printf("\n: Passengers list\n");
		for(int i=0; i<ELEVATOR_CAPACITY; i++){
			if(passengers[i][0] != 0){
				printf(": [ %d | %d | %d ]\n", passengers[i][0],\
					passengers[i][1], passengers[i][2]);
			}
		}
	}
	else{/*SIGUSR2*/}
}
/*=============================================================================
*/
int firstLineAvailable(){
	
	int index = 0;
	
	while(passengers[index][0] != 0){
		index++;
	}
	return index;
}

/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){

	int current = 0; // current floor
	int goingUp = TRUE; // 1:UP 0:DOWN
	int id;
	int working = 1; // 0:Intervention du technicien

	clearScreen();

	if(argc == 2){
		id = atoi(argv[1]);
		printf("\n: Ascenseur \033[1m%i\033[0m en fonctionnement !\n\n", id);
	}
	else{
		printf("\033[1m\033[31m\n: Ascenseur en panne !\033[0m\n\n");
		exit(0);
	}
	signal(SIGINT, traitantSIGINT);
	signal(SIGUSR1, traitantSIGUSR);
	
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
	}
	shm_write(elevatorList,id,0,getpid());
	
	/*-------------------------------------------------------------------------
	*	Création Mémoire partagée pour Liste d'attente 
	*------------------------------------------------------------------------*/
	if((shmWL = shmget(KEY_WL+id, 3*ELEVATOR_WAITSIZE*sizeof(int),\
		IPC_CREAT|IPC_EXCL|0755)) == -1){
			perror("\033[1m\033[31m: Echec de création (shmWL).\033[0m\n\n");
			exit(1);
	}
	else{
		printf(": Création de shmWL (id=%d).\n", shmWL);
		waitingList = (int*)shmat(shmWL, NULL, 0);
		shm_init(waitingList, 300);
		shm_write(waitingList, 0, 0, getpid());
	}
	
	printf("\n\033[1m: étage actuel ( \033[33m%i\033[37m )\n", current);
	
	int status = STOP;
	
	while(working == 1){
		
		switch(status){
		
		case STANDBY: /*Personne n'utilise l'ascenseur*/
			break;
			
		case MOVE: /*En mouvement (détermine la montée ou la descente)*/
			if(goingUp == TRUE){
				int callUp = FALSE;
				for(int i=0; i<ELEVATOR_WAITSIZE; i++){
					if(shm_read(waitingList,i,1) > current){
						callUp = TRUE;
					}
				}
				for(int i=0; i<ELEVATOR_CAPACITY; i++){
					if(passengers[i][2] > current){
						callUp = TRUE;
					}
				}
				goingUp = callUp;
			}
			else{/*goingUp == FALSE*/
				int callDown = FALSE;
				for(int i=0; i<ELEVATOR_WAITSIZE; i++){
					if(shm_read(waitingList,i,1) < current){
						callDown = TRUE;
					}
				}
				for(int i=0; i<ELEVATOR_CAPACITY; i++){
					if(passengers[i][2] < current){
						callDown = TRUE;
					}
				}
				goingUp = !callDown;
			}
			if(goingUp == TRUE && current < 25){
				current++;
			}
			else if(goingUp == FALSE && current > 0){
				current--;
			}
			printf("\33[1A\033[1m: étage actuel ( \033[33m%i\033[37m )\n", current);
			status = STOP;
			break;
			
		case STOP: /*Ascenseur arrêté à un étage (charge et décharge)*/
			for(int i=1; i<ELEVATOR_WAITSIZE; i++){
				if (current == shm_read(waitingList, i, 1) && shm_read(waitingList, i, 0) != 0){
					int index = firstLineAvailable();
					passengers[index][0] = shm_read(waitingList, i, 0);
					passengers[index][1] = shm_read(waitingList, i, 1);
					passengers[index][2] = shm_read(waitingList, i, 2);
					shm_write(waitingList, i, 0, 0);
					shm_write(waitingList, i, 1, 0);
					shm_write(waitingList, i, 2, 0);				
				}
			}
			for(int i=1; i<ELEVATOR_CAPACITY; i++){
				if(passengers[i][2] == current && passengers[i][0] != 0){
					//kill(passengers[i][0], SIGINT);
					passengers[i][0] = 0;
					passengers[i][1] = 0;
					passengers[i][2] = 0;
					kill(getpid(),SIGUSR1);
				}
			}
			status = MOVE;
			break;
		}
	}
	
	printf("\n");
	shmctl(shmWL, IPC_RMID, NULL);
	
	return EXIT_SUCCESS;
}
