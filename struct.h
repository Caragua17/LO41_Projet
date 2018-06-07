
#ifndef struct_h
#define struct_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdarg.h>
#include <signal.h>

#define KEY_DL 201
#define KEY_WL 202
#define KEY_VR 101
#define DWELLERS 100
/*
30 Noir		31 Rouge	32 Vert		33 Jaune
34 Bleu		35 Magenta	36 Cyan		37 Blanc
*/
#define clearScreen() printf("\033[H\033[2J")
#define colorScreen(param) printf("\033[%sm",param)

/*=============================================================================
struct MessageQueueBuffer (or msqbuf)
will stock informations send in the message queue
*/
typedef struct MessageQueueBuffer{

	long dest;
	long sender;
	char text[100];
}
msqbuf;

/*=============================================================================
shm_init: will initialize all the shm values, in order to prevent search issues
*/
void shm_init(int* ptr, int size){
	
	for(int i=0; i<size; i++){
		*ptr = 0;
		ptr++;
	}
}
/*=============================================================================
shm_read: function used to get a value stored in the shared memory
returns 'shm[i][j]' value thanks to the start address 'ptr'
*/
int shm_read(int *ptr, int i, int j){

	ptr += 3*i + j;
	return *ptr;
}
/*=============================================================================
shm_write: function used to set a value in the shared memory
saves 'value' in 'shm[i][j]' thanks to the start address 'ptr'
*/
void shm_write(int *ptr, int i, int j, int value){

	ptr += 3*i + j;
	*ptr = value;
}
/*=============================================================================
msq_send: function used to send informations in a message queue
sends struct 'msg' of type 'msgtype' in the queue n°'qid'
*/
void msq_send(int qid, int msgtype, msqbuf msg){

	msg.dest = msgtype;
	if(msgsnd(qid, &msg, sizeof(msg.sender)+sizeof(msg.text),\
		IPC_NOWAIT) == -1){
			printf("\033[1m\033[31m: Echec de l'envoi.\033[0m\n\n");
			exit(1);
	}
}
/*=============================================================================
msq_receive: function used to read informations in a message queue
prints struct 'msg' if it's of type 'msgtype' in the queue n°'qid'
*/
long msq_receive(int qid, int msgtype){

	msqbuf msg;
	if(msgrcv(qid, &msg, sizeof(msg.sender)+sizeof(msg.text), msgtype,\
		MSG_NOERROR) == -1){
			printf("\033[1m\033[31m: Echec de la reception.\033[0m\n\n");
			return -1;
	}
	else{
		printf("\n\033[1m\033[32m: Réception...\033[0m\n");
		printf(": > Dest:%li & sender:%li \n: > \033[1m%s\033[0m\n",\
			msg.dest, msg.sender, msg.text);
		return msg.sender;
	}
}

#endif //struct_h
