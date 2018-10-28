#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject
{
	FILE *rfile;
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cons_cond;
	pthread_cond_t prod_cond;
	int full; //-1 at the end
} so_t;

void *producer(void *arg)
{
	so_t *so = arg;

	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	size_t len = 0, read = 0;
	char *line = NULL;

	while (1)
	{
		read = getline(&line, &len, rfile);

		pthread_mutex_lock(&so->lock);

		while (so->full == 1) // while the buffer is full, wait
			pthread_cond_wait(&so->prod_cond, &so->lock);

		if(read == -1)
		{
			pthread_cond_signal(&so->prod_cond);
			so->line = NULL;
			so->full = -1;
		}
		else
		{
			if(line[read-1]=='\n')
				line[read-1]='\0'; //deleting the line jump at the end
			so->line = strdup(line);
			so->full = 1;
		}
		so->linenum++;

		pthread_cond_signal(&so->cons_cond);
		pthread_mutex_unlock(&so->lock);

		if (read == -1) // end of file
			break;
	}

	printf("a producer exits\n");	
	pthread_exit(NULL);
}

void *consumer(void *arg)
{
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	char *line;
	int i = 0;

	while (1)
	{
		pthread_mutex_lock(&so->lock);

		while (so->full == 0) // when the buffer is empty, then whait
			pthread_cond_wait(&so->cons_cond, &so->lock);
		
		if (so->line != NULL)
		{
			line = so->line;

			printf("\t%02d->%s\n", so->linenum, so->line);
			so->full = 0;

			pthread_cond_signal(&so->prod_cond);
			pthread_mutex_unlock(&so->lock);
		}
		else 
		{
			pthread_cond_signal(&so->cons_cond);
			pthread_mutex_unlock(&so->lock);
			break;
		}

		i++;
	}
	*ret = i;
	printf("a consumer exits\n");	
	pthread_exit(ret);
}

#define MAX_CONS 50
int main(int argc, char *argv[])
{
	pthread_t prod, cons[MAX_CONS];
	int Ncons = 1;
	int *ret;
	int i;
	FILE *rfile;

	//empty call
	if (argc == 1)
	{
		printf("usage: ./prod_cons <readfile> #Consumers\n");
		exit(0);
	}

	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));

	//file path
	rfile = fopen((char *)argv[1], "r");
	if (rfile == NULL)
	{
		perror("rfile");
		exit(0);
	}

	share->rfile = rfile;
	share->line = NULL;
	pthread_mutex_init(&share->lock, NULL);

	//producer
	pthread_create(&prod, NULL, producer, share);

	//consumers
	if (argv[2] != NULL)
	{
		Ncons = atoi(argv[2]);
		if (Ncons > MAX_CONS)
			Ncons = MAX_CONS;
		if (Ncons <= 0)
			Ncons = 1;
	}
	for (i = 0; i < Ncons; i++)
		pthread_create(&cons[i], NULL, consumer, share);

	//joinig
	for (i = 0; i < Ncons; i++)
	{
		pthread_join(cons[i], (void **)&ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	
	pthread_join(prod, (void **)&ret);
	printf("main: producer_0 left\n");

	pthread_exit(NULL);

	exit(0);
}

/*
to compile:
gcc -pthread one_prod_n_cons.c
*/