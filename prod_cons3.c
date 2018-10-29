#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256


typedef struct sharedobject {
	FILE *rfile;
	char *line[100];
	int front;
	int rear;
	int full;
	pthread_mutex_t lock;
	pthread_cond_t cond_cons;
	pthread_cond_t cond_prod;
} so_t;

int stat [MAX_STRING_LENGTH];
int stat2 [ASCII_SIZE];

void cal_distribute(char* str)
{
	char* cptr = str;
	char *substr = NULL;
	char *brka = NULL;
	char *sep = "{}()[],.;\" \n\t^";
	int length;

        for(char*pstr = str ; *pstr!='\0'; pstr++){
                if((*pstr >= 'a' ) && (*pstr <= 'z')){
                        stat2[*pstr]++;
                }
                else if(( *pstr >= 'A' ) && (*pstr <= 'Z')){
                        stat2[*pstr]++;
                }
        }

	for (substr = strtok_r(cptr, sep, &brka);substr;substr = strtok_r(NULL, sep, &brka))
	{
		length = strlen(substr);
		if(length >= 30)
			length = 30;
		stat[length -1] ++;

	}
	return ;
}

void print_distribute(){
	int sum =0;
	int i;
	for (i = 0 ; i < 30 ; i++) {
		sum += stat[i];
	}
	printf("*** print out distributions *** \n");
	printf("  #ch  freq \n");
	for (i = 0 ; i < 30 ; i++) {
		int j = 0;
		int num_star = stat[i]*80/sum;
		printf("[%3d]: %4d \t", i+1, stat[i]);
		for (j = 0 ; j < num_star ; j++)
			printf("*");
		printf("\n");
	}
	printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
	printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
			stat2['A']+stat2['a'], stat2['B']+stat2['b'],  stat2['C']+stat2['c'],  stat2['D']+stat2['d'],  stat2['E']+stat2['e'],
			stat2['F']+stat2['f'], stat2['G']+stat2['g'],  stat2['H']+stat2['h'],  stat2['I']+stat2['i'],  stat2['J']+stat2['j'],
			stat2['K']+stat2['k'], stat2['L']+stat2['l'],  stat2['M']+stat2['m'],  stat2['N']+stat2['n'],  stat2['O']+stat2['o'],
			stat2['P']+stat2['p'], stat2['Q']+stat2['q'],  stat2['R']+stat2['r'],  stat2['S']+stat2['s'],  stat2['T']+stat2['t'],
			stat2['U']+stat2['u'], stat2['V']+stat2['v'],  stat2['W']+stat2['w'],  stat2['X']+stat2['x'],  stat2['Y']+stat2['y'],
			stat2['Z']+stat2['z']);
}

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		pthread_mutex_lock(&so->lock);
		while(((so->front)%100) == ((so->rear+1)%100) ){ // when the buffer is full, then do not update 
			pthread_cond_wait(&so->cond_cons,&so->lock);
		}
		if (read == -1) { // end of file 
			so->line[(++(so->rear))%100] = NULL;
			pthread_cond_signal(&so->cond_prod);
			pthread_mutex_unlock(&so->lock); // unlock the thread
			break;
		}
		so->line[(++(so->rear))%100] = strdup(line);      /* share the line */
		i++;
		pthread_cond_signal(&so->cond_prod);
		pthread_mutex_unlock(&so->lock);
	}
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
		pthread_mutex_lock(&so->lock); // lock the thread
		while(((so->front)%100) == ((so->rear)%100)){ //when the buffer is empty
			pthread_cond_wait(&so->cond_prod,&so->lock);
		}
		line = so->line[++(so->front) % 100];
		if (line == NULL) {
			so->front = so->front - 1;
			pthread_cond_signal(&so->cond_prod);
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->front, line);
		cal_distribute(line);
		i++;
		pthread_cond_signal(&so->cond_cons);
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
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;
	memset(stat, 0, sizeof(stat));
	memset(stat2, 0, sizeof(stat));
	share->rfile = rfile;
	memset(share->line,0,sizeof(share->line));
	share->front= share->rear= 0;
	share-> full = 0;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->cond_cons, NULL);
	pthread_cond_init(&share->cond_prod,NULL);
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		pthread_cond_broadcast(&share->cond_prod);
		printf("main: consumer_%d joined with %d\n", i, *ret);
		
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	print_distribute();
	pthread_exit(NULL);
	exit(0);
}
