
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
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>

#define KEY_DL 100 // DwellerList
#define KEY_WL 200 // WaitingList
#define KEY_EL 200 // ElevatorList
#define KEY_VR 300 // msq Visiteur-Résident
#define KEY_VI 301 // msq Visiteur-Immeuble
#define KEY_SE 500
#define BUILDING_DWELLERS 100
#define ELEVATOR_CAPACITY 10
#define ELEVATOR_WAITSIZE 20
#define ELEVATOR_MOVING_T 1
#define ELEVATOR_STOPPING 3
#define TRUE 1
#define FALSE 0
#define STANDBY 0
#define STOP 1
#define MOVE 2
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
typedef struct MessageQueueBuffer
{
	long dest;
	long sender;
	char text[100];
}
msqbuf;

/*=============================================================================
pause: the program will wait a delay before executing the rest of the code.
*/
void delay(int i)
{
    clock_t start,end;
    start=clock();
    while(((end=clock())-start)<=i*CLOCKS_PER_SEC);
}
/*=============================================================================
shm_init: will initialize all the shm values, in order to prevent search issues
*/
void shm_init(int* ptr, int size)
{
	for(int i=0; i<size; i++)
	{
		*ptr = 0;
		ptr++;
	}
}
/*=============================================================================
shm_read: function used to get a value stored in the shared memory
returns 'shm[i][j]' value thanks to the start address 'ptr'
*/
int shm_read(int *ptr, int i, int j)
{
	ptr += 3*i + j;
	return *ptr;
}
/*=============================================================================
shm_write: function used to set a value in the shared memory
saves 'value' in 'shm[i][j]' thanks to the start address 'ptr'
*/
void shm_write(int *ptr, int i, int j, int value)
{
	ptr += 3*i + j;
	*ptr = value;
}
/*=============================================================================
msq_send: function used to send informations in a message queue
sends struct 'msg' of type 'msgtype' in the queue n°'qid'
*/
void msq_send(int qid, int msgtype, msqbuf msg)
{
	msg.dest = msgtype;
	if(msgsnd(qid, &msg, sizeof(msg.sender)+sizeof(msg.text),\
	IPC_NOWAIT) == -1)
	{
		printf("\033[1m\033[31m: Echec de l'envoi.\033[0m\n\n");
		exit(1);
	}
}
/*=============================================================================
msq_receive: function used to read informations in a message queue
prints struct 'msg' if it's of type 'msgtype' in the queue n°'qid'
*/
msqbuf msq_receive(int qid, int msgtype)
{
	msqbuf msg;
	if(msgrcv(qid, &msg, sizeof(msg.sender)+sizeof(msg.text), msgtype,\
	MSG_NOERROR) == -1)
	{
		msg.dest = -1;
		msg.sender = -1;
	}
	else
	{
		printf("\n\033[1m\033[32m: Réception...\033[0m\n");
		printf(": > Dest:%li & sender:%li \n: > \033[1m%s\033[0m\n",\
			msg.dest, msg.sender, msg.text);
	}
	return msg;
}

#endif //struct_h
