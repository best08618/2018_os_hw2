#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256

int stat[MAX_STRING_LENGTH];
int stat2[ASCII_SIZE];

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

/*void char_stat(FILE *file)
{
	int rc = 0;
    size_t length = 0;
    int i = 0;
    FILE *rfile = NULL;
    char *line = NULL;
    int line_num = 1;
    int sum = 0;

	printf("Printing out character statistic: \n");
	
	memset(stat, 0, sizeof(stat));
	memset(stat2, 0, sizeof(stat));

	while (1) {
        char *cptr = NULL;
        char *substr = NULL;
        char *brka = NULL;
        char *sep = "{}()[],;\" \n\t^";
        // For each line,
        rc = getdelim(&line, &length, '\n', rfile);

        if (rc == -1) break;
        cptr = line;

#ifdef _IO_
        printf("[%3d] %s\n", line_num++, line);
#endif
        for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka))
        {
            length = strlen(substr);
            // update stats
#ifdef _IO_
            printf("length: %d\n", (int)length);
#endif
            cptr = cptr + length + 1;
            if (length >= 30) length = 30;
            stat[length-1]++;
			if (*cptr == '\0') break;
        }
        cptr = line;
        for (int i = 0 ; i < length ; i++) 
		{
            if (*cptr < 256 && *cptr > 1) 
			{
                stat2[*cptr]++;
#ifdef _IO_
               printf("# of %c(%d): %d\n", *cptr, *cptr, stat    2[*cptr]);
#endif
            }
            cptr++;
        }
    }
    // sum
    sum = 0;
	for (i = 0 ; i < 30 ; i++) 
	{
        sum += stat[i];
    }
    // print out distributions
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");
    for (i = 0 ; i < 30 ; i++) 
	{
        int j = 0;
        int num_star = stat[i]*80/sum;
        printf("[%3d]: %4d \t", i+1, stat[i]);
        for (j = 0 ; j < num_star ; j++) printf("*");
        printf("\n");
    }
    printf("       A        B        C        D        E        F        G        H        I        J        K        L            M        N        O        P        Q        R        S       T        U        V        W        X        Y        Z\n"    );
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",stat2['A']+stat2['a'], stat2['B']+stat2['b'],  stat2['C']+stat2['c'],  stat2['D']+stat2['d'],  stat2['E']+stat2['e'], stat2['F']+stat2['f'], stat2['G']+stat2['g'],  stat2['H']+stat2['h'],  stat2['I']+stat2['i'],  stat2['J']+stat2['j'], stat2['K']+stat2['k'], stat2['L']+stat2['l'],  stat2['M']+stat2['m'],  stat2['N']+stat2['n'],  stat2['O']+stat2['o'], stat2['P']+stat2['p'], stat2['Q']+stat2['q'],  stat2['R']+stat2['r'],  stat2['S']+stat2['s'],  stat2['T']+stat2['t'],  stat2['U']+stat2['u'], stat2['V']+stat2['v'],  stat2['W']+stat2['w'],  stat2['X']+stat2['x'],  stat2['Y']+stat2['y'],  stat2['Z']+stat2['z']);
    if (line != NULL) free(line);
    // Close the file
    fclose(rfile);
    return;
	
}
*/

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


	memset(stat, 0, sizeof(stat));
    memset(stat2, 0, sizeof(stat));


	
	pthread_exit(NULL);
	exit(0);
}

