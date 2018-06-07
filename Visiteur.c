
#include "struct.h"

/*=============================================================================
traitantSIGINT: executed when a 'Ctrl+C' signal is intercepted.
Will kill the processus.
*/
void traitantSIGINT(int num){

	if(num == SIGINT){
		printf("\033[1m\033[31m\n: Accès refusé.\033[0m\n\n");
		exit(1);
	}
}
/*=============================================================================
MAIN FUNCTION
*/
int main(int argc, char* argv[]){
	
	if(argc != 3){
		perror("\033[1m\033[31m: use /Visiteur [floor] [door].\033[0m\n\n");
		exit(1);
	}
	int path[2] = {atoi(argv[1]), atoi(argv[2])};
	
	printf("\n: Visiteur %i. Etage %i, porte %i.\n",\
		getpid(), path[0], path[1]);
		
	signal(SIGINT, traitantSIGINT);

	/*-------------------------------------------------------------------------
	*	Connexion à la memoire partagée (Dweller List)
	*------------------------------------------------------------------------*/
	int shmid;
	int *ptr;
	int dest;
		
	if((shmid = shmget(KEY, 3*DWELLERS*sizeof(int), 0755)) == -1){
		perror("\033[1m\033[31m: Echec de connexion.\033[0m\n\n");
		exit(1);
	}
	else{
		printf(": Connecté ! (msq ID = %d)\n", shmid);
	}
	ptr = (int*)shmat(shmid, NULL, 0);
	
	for(int i=0; i<100; i++){
		if(shm_read(ptr,i,1) == path[0]){
			if (shm_read(ptr,i,2) == path[1]){
				dest = shm_read(ptr,i,0);
			}
		}
	}
	/*-------------------------------------------------------------------------
	*	Connexion file de message (Visiteur-Résident)
	*------------------------------------------------------------------------*/
	int msqid;
	
	if((msqid = msgget(KEY, IPC_EXCL|0755)) == -1){
		perror("\033[1m\033[31m: Echec de connexion.\033[0m\n");
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
	}
	
	/*
	COMMUNIQUE AVEC IMMEUBLE POUR CONNAITRE NUM ASCENSEUR
	COMMUNIQUE AVEC ASCENSEUR
	*/
	
	shmdt(ptr);
	printf("\n");

	return EXIT_SUCCESS;
}
