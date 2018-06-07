
#include "struct.h"

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
	/*
	printf("\33[1A\033[1m: étage actuel ( \033[33m%i\033[37m )\n", current);
	*/
	printf("\n");
	
	return EXIT_SUCCESS;
}
