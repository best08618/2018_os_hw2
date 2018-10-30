#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

typedef struct sharedobject 
{
	FILE *rfile;
	int linenum;
	char *line;
	int rear;
	int front;
	int full;
	char* buffer[BUFSIZE];
	pthread_cond_t checkEmpty;	//for consumer
	pthread_cond_t checkBuffer; //for producer
	pthread_mutex_t lock; //control entry into buffer
	//pthread_cond_t cond; 
} so_t;


void *producer(void *arg) 
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i =0;
	FILE *rfile = so->rfile;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	so->front = so->rear = 0;

	while (1) 
	{
		read = getdelim(&line, &len, '\n', rfile); //read line is stored in 'line'
		pthread_mutex_lock(&so->lock);

		while((so->rear+1)%BUFSIZE == so->front) //when buffer is full
		{
			pthread_cond_wait(&so->checkBuffer, &so->lock); //producer cannot produce anymore
		}
		so->rear = so->rear+1;	
	
		if (read == -1) //no more data in the file 
		{  
			//so->full = 1;
			so->line = NULL;
			pthread_cond_signal(&so->checkEmpty);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		so->linenum = i;
		so->line = strdup(line);      /* share the line */
		so->buffer[so->rear] = so->line;
		i++;
//		so->full = 1;
		pthread_cond_signal(&so->checkEmpty); //before unlocking, we send signal to another thread to wake it up
		pthread_mutex_unlock(&so->lock);	// release the lock
	}
//	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;
	
	while (1) {
		pthread_mutex_lock(&so->lock);
		while(so->buffer[so->rear] == so->buffer[so->front]) //empty buffer
		{
			pthread_cond_wait(&so->checkEmpty, &so->lock); //consumer wait
			if(so->full == 1 ) //when it reaches the end of file, end the thread
				break;
		}
		if(so->full == 1)
		{
			pthread_cond_signal(&so->checkEmpty);
            pthread_mutex_unlock(&so->lock);
			break;
		}
		so->line = so->buffer[so->front +1];
		so->front = so->front+1;
		line = so->line;
		
		if (line == NULL)
		{
			so->full = 1;
			pthread_cond_signal(&so->checkEmpty);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
//		free(so->line);
		i++;
		//so->full = 0;
		pthread_cond_signal(&so->checkBuffer);
		pthread_mutex_unlock(&so->lock);
	}
	printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}

	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");

	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}

	if (argv[2] != NULL) 
	{
		Nprod = atoi(argv[2]); //convert string into integer and get number of prod
		if(Nprod > 100) 
			Nprod == 100;
		if(Nprod == 0) 
			Nprod == 1;
	}else Nprod == 1;
	printf("Number of producer: %d\n", Nprod);	


	if (argv[3] != NULL) 
	{
		Ncons = atoi(argv[3]); //get number of cons
		if(Ncons > 100)
           Ncons == 100;
     
	   if(Ncons == 0)
            Ncons == 1;
    }else Ncons == 1;
	printf("Number of consumer: %d\n", Ncons);


	printf("----------------------------------------\n");


	share->rfile = rfile;
	share->line = NULL;
	share->full = 0;
	pthread_mutex_init(&share->lock, NULL); //initializing lock
	pthread_cond_init(&share->checkBuffer, NULL);
	pthread_cond_init(&share->checkEmpty, NULL);


	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);

	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);


	printf("main continuing\n");


	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}


	pthread_exit(NULL);
	exit(0);
}

