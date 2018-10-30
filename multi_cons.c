#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define MAXBUF 1000

typedef struct buffer {
	char* line;
	int linenum;
	size_t len;
} buffer_t; // Set Buffer

typedef struct sharedobject {
   FILE *rfile;
   pthread_cond_t full_cv;
   pthread_mutex_t lock;
   pthread_mutex_t head_lock;
   pthread_cond_t empty_cv;
   int count;
   int head;
   int eof;
   int tail;
   buffer_t buf[MAXBUF];
} so_t;


void *producer(void *arg) {
   so_t *so = arg;
   int *ret = malloc(sizeof(int));
   FILE *rfile = so->rfile;
   int i = 0;
   char *line = NULL;
   size_t len = 0;
   ssize_t read = 0;
   int newtail = 0;

   while (1) {

      read = getdelim(&line, &len, '\n', rfile);

      pthread_mutex_lock(&so->lock);

      newtail = so->tail;
      newtail = (newtail+1) % MAXBUF;      

      while(so->count > MAXBUF-1){      
	 pthread_cond_wait(&so->full_cv,&so->lock);
      } // When Buffer is full wait

      if (read == -1) {
         so->eof = 1;
         so->buf[newtail].line = NULL;
         pthread_cond_signal(&so->empty_cv);
         pthread_mutex_unlock(&so->lock);
         break;
      }

      so->buf[newtail].linenum = i;
      so->buf[newtail].line = strdup(line);
      so->buf[newtail].len = len;
      i++;
      so->tail = newtail;
      (so->count)++;

      pthread_cond_signal(&so->empty_cv);
      pthread_mutex_unlock(&so->lock);
   }
   free(line);
   printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
   *ret = i;
   pthread_exit(ret);
}

void *consumer(void *arg) {
   so_t *so = arg;
   int *ret = malloc(sizeof(int));
   int i = 0;
   char *line = NULL;
   size_t length = 0;
   size_t linelength = 0;
   int line_num = 1;
   int newhead = 0;

   while(1){

      pthread_mutex_lock(&so->lock);

       while(so->count == 0){
	if(so->eof == 1){ // End
	 pthread_cond_broadcast(&so->empty_cv);
	 pthread_mutex_unlock(&so->lock);
         break;
	}
	else{
         pthread_cond_wait(&so->empty_cv, &so->lock);
	} // When Buffer is empty, wait
      }

	if(so->count == 0 && so->eof == 1) break;

      newhead = so->head;
      newhead = (newhead+1) % MAXBUF;
      so->head = newhead;

      line = so->buf[newhead].line;
      printf("Cons_%x: [%02d:%02d] %s",
         (unsigned int)pthread_self(), i, so->buf[newhead].linenum, line);
      (so->count)--;

      linelength = so->buf[newhead].len;
      pthread_cond_signal(&so->full_cv);
      pthread_mutex_unlock(&so->lock);

      linelength = strlen(line);
		
      free(line);
      i++;
   }
   printf("Cons: %d lines\n", i); // Print number of lines
   *ret = i; 
   pthread_exit(ret);
}


int main (int argc, char *argv[])
{
   pthread_t prod[100];
   pthread_t cons[100];
   int Nprod, Ncons;
   int rc = 0;   long t;
   int *ret;
   int i;
   FILE *rfile = NULL;
   int sum = 0;

   if (argc == 1) {
      printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
      exit (0);
   }

   so_t *share = malloc(sizeof(so_t));
   memset(share, 0, sizeof(so_t));
   memset(share->buf, 0, MAXBUF);

   rfile = fopen((char *) argv[1], "rb");
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

  printf("argc: %d, argv[1]: %s, argv[2]: %s, argv[3]: %s\n", argc, argv[1], argv[2], argv[3]);

   share->rfile = rfile;
   share->count = 0;
   share->head = 0;
   share->tail = 0;
   share->eof = 0;

   for(i = 0; i < MAXBUF+1 ; i++){
      share->buf[i].line = NULL;
      share->buf[i].len = 0;
}
   pthread_mutex_init(&share->lock, NULL);
   pthread_mutex_init(&share->head_lock, NULL);
   pthread_cond_init(&share->full_cv, NULL);
   pthread_cond_init(&share->empty_cv, NULL);

   for (i = 0 ; i < Nprod ; i++)
      pthread_create(&prod[i], NULL, producer, share);
   for (i = 0 ; i < Ncons ; i++)
      pthread_create(&cons[i], NULL, consumer, share);
   printf("Main continuing\n");

   for (i = 0 ; i < Ncons ; i++) {
      rc = pthread_join(cons[i], (void **) &ret);
      printf("Main: consumer_%d joined with %d\n", i, *ret);
   } // Print consumer result
   for (i = 0 ; i < Nprod ; i++) {
      rc = pthread_join(prod[i], (void **) &ret);
      printf("Main: producer_%d joined with %d\n", i, *ret);
   } // Print producer result

   pthread_exit(NULL);
   exit(0);
}

