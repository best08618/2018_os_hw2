#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//data base class???

#define VALID_CHAR 128
#define MAX_PROD 50
#define MAX_CONS 50

#pragma region structures definitions

	typedef struct file_acess
	{
		FILE *rfile;
		int linenum;
		char *line;
		pthread_mutex_t lock;
	}s_file_acess;

	typedef struct buffer
	{
		int linenum;
		char *line;
		//struct buffer *next; //liknked list
		pthread_mutex_t lock;
		pthread_cond_t cons_cond;
		pthread_cond_t prod_cond;
		int full; //-1 at the end
	}s_buffer;

	typedef struct stat
	{
		pthread_mutex_t lock;
		int count[VALID_CHAR];
	}s_stat;

	typedef struct complex_obj
	{
		struct file_acess *file_acess;
		struct buffer *buffer;
		struct stat *stat;
	}s_complex_obj;

	#pragma endregion



#pragma region threads

	void *producer(void *args)
	{
		//printf("a producer enters\n"); //control

		struct complex_obj *co = args;
		struct file_acess *fa = co->file_acess;
		struct buffer *bf = co->buffer;

		size_t len = 0, read = 0;
		int linenum = 0;
		char *line = NULL;

		while (1)
		{

			//file-read CS
			pthread_mutex_lock(&fa->lock);
			
				read = getline(&line, &len, fa->rfile);
				fa->linenum++;
				linenum = fa->linenum;
			
			pthread_mutex_unlock(&fa->lock);

			//buffer CS
			pthread_mutex_lock(&bf->lock);
		
			while (bf->full == 1) // while the buffer is full, wait
				pthread_cond_wait(&bf->prod_cond, &bf->lock);

			if (read == -1) //end of file
			{
				pthread_cond_broadcast(&bf->prod_cond);
				bf->line = NULL;
				bf->full = -1;
			}
			else 
			{
				if (line[read-1] == '\n'){	line[read-1] = '\0'; }//deleting the line jump at the end

				bf->line = strdup(line); //share the line
				bf->linenum = linenum;
				bf->full = 1;
			}

			pthread_cond_signal(&bf->cons_cond);
			pthread_mutex_unlock(&bf->lock);

			if (read == -1) // end of file
				break;
		}

		//printf("a producer exits\n"); //control
		pthread_exit(NULL);
	}

	void *consumer(void *args)
	{
		//printf("a consumer enters\n"); //control

		struct complex_obj *co = args;
		struct buffer *bf = co->buffer;
		struct stat *st = co->stat;

		char *line;
		int count[VALID_CHAR];
		char c; //for iteration

		//reading lines
		while (1)
		{
			pthread_mutex_lock(&bf->lock);

			while (bf->full == 0) // when the buffer is empty, then whait
				pthread_cond_wait(&bf->cons_cond, &bf->lock);

			if (bf->full == -1) //end of file
			{
				pthread_cond_signal(&bf->cons_cond);
				pthread_mutex_unlock(&bf->lock);
				break;
			}

			line = strdup(bf->line);
			bf->full = 0;

			//printf("[L:%02d] %s\n", bf->linenum, bf->line); //control

			pthread_cond_signal(&bf->prod_cond);
			pthread_mutex_unlock(&bf->lock);
			
			//iterating line to gather statistics
			for(int i =0; ; i++){
				c = line[i]; //identify current
				if(c=='\0') break;
				count[c]++; //count "one more" to wichever character that is
			}
		}

		pthread_mutex_lock(&st->lock);
			//iterating statistics's count and adding local count
			for(int i=0; i<VALID_CHAR; i++){
				st->count[i] += count[i];
			}
		pthread_mutex_unlock(&st->lock);

		//printf("a consumer exits\n"); //control
		pthread_exit(NULL);
	}

#pragma endregion




#pragma region methods

	int init_producers(char *qty, pthread_t *prod, void *complex_obj)
	{

		int i = 0, limit = 1;
		if (qty != NULL)
		{
			
			int num = atoi(qty);
			if (num > MAX_PROD)
				limit = MAX_PROD;
				
			else if (num < 1)
				limit = 1;
			else
				limit = num;
		}

		for (; i < limit; i++){
			pthread_create(&prod[i], NULL, producer, complex_obj);
		}

		return(i); //how many treads where created, should be ecual to qty
	}

	int init_consumers(char *qty, pthread_t *cons, void *complex_obj)
	{
		int i = 0, limit = 1;
		if (qty != NULL)
		{
			int num = atoi(qty);
			if (num > MAX_CONS)
				limit = MAX_CONS;
				
			else if (num < 1)
				limit = 1;
			else
				limit = num;
		}

		for (; i < limit; i++){
			pthread_create(&cons[i], NULL, consumer, complex_obj);
		}

		return(i); //how many treads where created, should be ecual to qty
	}

	struct complex_obj *create_cplx_obj()
	{
		//declaring and initializing structures (shared objects):
		struct complex_obj *complex_obj = malloc(sizeof(struct complex_obj));
		memset(complex_obj, 0, sizeof(struct complex_obj));
		
		//file acces
		struct file_acess *file_acess = malloc(sizeof(struct file_acess));
		memset(file_acess, 0, sizeof(struct file_acess));
		complex_obj->file_acess = file_acess;
		//buffer
		struct buffer *buffer = malloc(sizeof(struct buffer));
		memset(buffer, 0, sizeof(struct buffer));
		complex_obj->buffer = buffer;
		//statistics
		struct stat *statistics = malloc(sizeof(struct stat));
		memset(statistics, 0, sizeof(struct stat));
		complex_obj->stat = statistics;

		return complex_obj;
	}

	void print_file_statistics(int * stat_array)
	{

		//header
		printf("\n----------STATISTICS----------\n");
		//printf("  lines-----nn\n");
		//printf("  letters---nn\n");
		printf("\n  character  ocurrences\n");

		//
		for(int i=0; i<VALID_CHAR; i++){
			if(stat_array[i] != 0)
				printf("  [%3d:%c]         %02d\n", i, i, stat_array[i]);
		}
		printf("<non-ocurring characters are omited>\n");
		printf("\n----------STATISTICS----------\n\n");

		return;
	}

	///main entry point of the program
	int main(int argc, char *argv[])
	{
		pthread_t prod[MAX_PROD], cons[MAX_CONS];
		struct complex_obj *co;

		//empty call validation
		if (argc == 1)
		{
			printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
			exit(0);
		}

		co = create_cplx_obj();
		
		//file path
		FILE *rfile = fopen((char *)argv[1], "r");
		if (rfile == NULL)
		{
			perror("rfile");
			exit(0);
		}
		co->file_acess->rfile = rfile;
		co->file_acess->line = NULL;
		co->file_acess->linenum = 0;
		co->buffer->linenum = 0;

		//locks for the treads
		pthread_mutex_init(&co->file_acess->lock, NULL);
		pthread_mutex_init(&co->buffer->lock, NULL);
		pthread_mutex_init(&co->stat->lock, NULL);

		//acrual threads
		int prod_c = init_producers(argv[2], prod, co);
		int cons_c = init_consumers(argv[3], cons, co);

		void * ret;
		for (int i = 0 ; i < cons_c ; i++) {
			pthread_join(cons[i], (void **) &ret);
			//printf("main: consumer %d re-joined main tread.\n", i); //control
		}

		print_file_statistics(co->stat->count);

		exit(0);
	}

#pragma endregion

/*
	to compile:
	gcc -pthread n_prod_n_cons.c
	*/