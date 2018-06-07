
#include "struct.h"

/*=============================================================================
GLOBAL VARIABLES USED HERE
*/
int *ptr;

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will delete registration in the Dweller List and then exit.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		printf("\n: Au revoir.\n\n");
		int index = 0;
		while(shm_read(ptr,index,0) != getpid()){
			index++;
		}
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
int main(int argc, char* argv[]){

	/*-------------------------------------------------------------------------
	*	Verification des arguments + initialisation signals
	*------------------------------------------------------------------------*/
	if(argc != 3){
		printf("\033[1m\033[31m: use /Resident [floor] [door].\033[0m\n\n");
		exit(1);
	}
	signal(SIGINT, traitantSIGINT);
	
	int path[2] = {atoi(argv[1]), atoi(argv[2])};
	
	printf("\n: Résident %d, etage %d, porte %d.\n",\
		getpid(), path[0], path[1]);
	
	/*-------------------------------------------------------------------------
	*	Connexion et édition de la memoire partagée (Dweller List)
	*------------------------------------------------------------------------*/
	int shmid;
	
	if((shmid = shmget(KEY, 3*DWELLERS*sizeof(int), 0755)) == -1){
		printf("\033[1m\033[31m: Echec de connexion.\033[0m\n\n");
		exit(1);
	}
	else{
		printf(": Connecté ! (msq ID = %d)\n", shmid);
	}
	ptr = (int*)shmat(shmid, NULL, 0);
	
	printf(": Identification (Immeuble %d)\n", shm_read(ptr,0,0));
	
	int index = 0;
	
	while(shm_read(ptr,index,0) != 0){
		index++;
	}
	shm_write(ptr, index, 0, getpid());
	shm_write(ptr, index, 1, path[0]);
	shm_write(ptr, index, 2, path[1]);
	
	printf(": Liste des résidents mise à jour !\n");	
	kill(shm_read(ptr,0,0), SIGUSR1);

	/*-------------------------------------------------------------------------
	*	Connexion file de message (Visiteur-Résident)
	*------------------------------------------------------------------------*/
	int msqid;
		
	if((msqid = msgget(KEY, IPC_EXCL|0755)) == -1){
		printf("\033[1m\033[31m: Echec de connexion.\033[0m\n\n");
		exit(1);
	}

	while(1){
		long dest = msq_receive(msqid, getpid());
		printf("\n: Permettre l'accès ? (o/n) : ");
		char c = 'a';
		while(c != 'o' && c != 'n'){
			c = getchar();
		}
		if(c == 'o'){
			msqbuf msg;
			msg.sender = getpid();
			msg.dest = dest;
			sprintf(msg.text, "%s", "OK");
			msq_send(msqid, dest, msg);
		}
		else{
			kill(dest, SIGINT);
		}
	}
	
	shmdt(ptr);	
	printf("\n");

	return EXIT_SUCCESS;
}
