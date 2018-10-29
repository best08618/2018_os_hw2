1. condition variables must be in a loop, to check over again
2. ubuntu 14 manual page

multithreading
3. by creating more buffer(circular buffer, link list...), checking if all the buffer is full, otherwise can fill in the buffer
4. one consumer can use one buffer
	every buffer needs a own lock
	using cond to stop buffer from being used in producer side

5. pointer pointing to buffer is not shared
6. lastly must join up all the threads

get dt from file
check condition, buffer full or emtpy using condion variable
	mutex_lock(&lock) -> 
	while(condition == false)
	{
		cond_wait(&cond, &lock);
		mutex_unlock(&lock);
	}


typedef struct sharedobject
{
	pthread_cond_t cond;
} 

void *producer()

void *consumer()


