#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h> //just for waiting


#define VALID_CHAR 94
#define STARTING_CHAR 32
#define MAX_PROD 30
#define MAX_CONS 30

#pragma region structures definitions

	typedef struct file_acess
	{
		FILE *rfile;
		pthread_mutex_t lock;
	}s_file_acess;

	typedef struct buffer
	{
		char *line;
		struct buffer *next; //for liknked list
		pthread_mutex_t lock;
		int last; //flag; at the end of the buffer
	}s_buffer;

	typedef struct stat
	{
		pthread_mutex_t lock;
		int count[VALID_CHAR];
	}s_stat;

	typedef struct complex_obj
	{
		struct file_acess *file_acess;
		struct buffer *bf_head;
		struct buffer *bf_tail;
		struct stat *stat;
	}s_complex_obj;

	#pragma endregion



#pragma region threads

	void *producer(void *args)
	{
		//printf("a producer enters\n"); //control

		struct complex_obj *co = args;
		struct file_acess *fa = co->file_acess;
		struct buffer *bf_new;
		struct buffer *bf_tmp;
		
		//lines counter
		int *ret = malloc(sizeof(int));
		int i = 0;

		size_t len = 0, read = 0;
		char *line = NULL;

		while (1)
		{
			//file-read CS
			pthread_mutex_lock(&fa->lock);
				read = getline(&line, &len, fa->rfile);
			pthread_mutex_unlock(&fa->lock);

			//create new buffer
			bf_new = malloc(sizeof(s_buffer));
			memset(bf_new, 0, sizeof(s_buffer));
			pthread_mutex_init(&bf_new->lock, NULL);

			//fill that new buffer
			if (read == -1) //end of file
			{
				bf_new->line = NULL;
				bf_new->last = 1;
			}
			else
			{
				bf_new->line = strdup(line);
				i++;
			}

			//put that new buffer at the head
			pthread_mutex_lock(&co->bf_head->lock);
				bf_tmp = co->bf_head;
				co->bf_head->next=bf_new;
				co->bf_head=bf_new;
			pthread_mutex_unlock(&bf_tmp->lock);

			if (read == -1) // end of file
				break;
		}

		//printf("a producer exits\n"); //control
		*ret = i;
		pthread_exit(ret);
	}

	void *consumer(void *args)
	{
		unsigned char * ptc = (char *)pthread_self();
		printf("%02x-%02x consumer enters\n", ptc[2], ptc[3]); //control

		struct complex_obj *co = args;
		struct buffer *bf_read;
		int *ret = malloc(sizeof(int));

		char *line;
		int count[VALID_CHAR]; //local statistics
		char c; //for iteration
		int lines = 0;

		struct timespec tm = { .tv_sec = 0, .tv_nsec = 30000000};

		while (1)
		{
			//geting the tail
			while(1){
				nanosleep(&tm, NULL);	/*	I made the tread whait a little
										because if i did`nt, it was too fast
										the others where not geting a chance
										(this anoyed me for a while) */

				pthread_mutex_lock(&co->bf_tail->lock);

				if(co->bf_tail->last == 1){
					//it's last; use this buffer
					bf_read = co->bf_tail;
					pthread_mutex_unlock(&co->bf_tail->lock);
					break; // continue with the buffer
				}
				else if(co->bf_tail->next != NULL) {
					//we can continue; cut off the tail
					bf_read = co->bf_tail;
					co->bf_tail = co->bf_tail->next;
					//unlocking it even though no one can access it anymore
					pthread_mutex_unlock(&bf_read->lock);
					break; // continue with the buffer
				}
				else
					//misiging next buffer; ask again
					pthread_mutex_unlock(&co->bf_tail->lock);
			}
			
			//analize the buffer
			if (bf_read->last==1){
				break; //stop reading lines 
			}
			else{
				//gather statistics
				line = strdup(bf_read->line);
				for (int i = 0;; i++)
				{
					c = line[i];
					if (c<STARTING_CHAR) break; //ignore non-redable ones
					count[c-STARTING_CHAR]++; //count one more of "c"
				}

				lines++;
			}

			//printf("%s.\n", bf_read->line); //control
		}

		//publishing; adding local-statistics to the shared-statistics
		pthread_mutex_lock(&co->stat->lock);
			for(int i=0; i<VALID_CHAR; i++){
				co->stat->count[i] += count[i];
			}
		pthread_mutex_unlock(&co->stat->lock);

		//printf("a consumer exits\n"); //control

		*ret = lines;
		pthread_exit(ret);
	}

#pragma endregion



#pragma region methods

	int init_producers(char *qty, pthread_t *prod, void *complex_obj)
	{
		int i = 0, limit = 1; //limit =1 by default
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

		return(i); //how many treads where created, should be equal to qty
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

		return(i); //how many treads where created, should be equal to qty
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
		buffer->line = "";
		complex_obj->bf_head = buffer;
		complex_obj->bf_tail = buffer;
		//statistics
		struct stat *statistics = malloc(sizeof(struct stat));
		memset(statistics, 0, sizeof(struct stat));
		complex_obj->stat = statistics;

		
		//locks for the treads
		pthread_mutex_init(&file_acess->lock, NULL);
		pthread_mutex_init(&buffer->lock, NULL);
		pthread_mutex_init(&statistics->lock, NULL);

		return complex_obj;
	}

	void print_file_statistics(int lines, int * stat_array)
	{

		//header
		printf("\n----------STATISTICS----------\n");
		printf("  lines-------%d\n\n", lines);
		//printf("  characters--nn\n");
		printf("\n  character  ocurrences  dozens\n");

		//data
		for(int i=0; i<VALID_CHAR; i++){
			int n = stat_array[i];
			if(n != 0){
				printf("   [%3d: %c]      %2d        ", i+STARTING_CHAR, i+STARTING_CHAR, n);
				for (n /= 12; n>0 ;n--) printf("*");
				printf("\n");
			}
		}

		//footer
		printf("<non-ocurring characters are omited>\n");
		printf("\n----------STATISTICS----------\n\n");

		return;
	}

	///main entry point of the program
	int main(int argc, char *argv[])
	{
		pthread_t prod[MAX_PROD], cons[MAX_CONS];
		struct complex_obj *co;
		int *ret;
		int lines = 0;

		//empty call validation
		if (argc == 1)
		{
			printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
			exit(0);
		}

		co = create_cplx_obj();
		
		//file path
		FILE *rfile = fopen((char *)argv[1], "r");
		if (rfile == NULL) {
			perror("rfile");
			exit(0);
		}
		co->file_acess->rfile = rfile;

		//init and start threads
		int prod_c = init_producers(argv[2], prod, co);
		int cons_c = init_consumers(argv[3], cons, co);
		
		for (int i = 0 ; i < prod_c ; i++) {
			pthread_join(prod[i], (void **) &ret);
			lines += *ret;
			//printf("main: producer %d re-joined after %d lines.\n", i+1, *ret); //control
		} //control */
		
		for (int i = 0 ; i < cons_c ; i++) {
			pthread_join(cons[i], (void **) &ret);
			//printf("main: consumer %d re-joined after %d lines.\n", i+1, *ret); //control
		} //control */

		print_file_statistics(lines, co->stat->count);

		exit(0);
	}

#pragma endregion

/*
	to compile:
	gcc -pthread n_prod_n_cons.c
	*/