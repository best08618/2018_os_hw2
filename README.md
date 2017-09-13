# OS hw2
OS homework #2

The second homework is about multi-thread programming with some synchronization.
Thread is a unit of execution; a thread has execution context, 
	which includes the registers, stack.
Note that address space (memory) is shared among threads in a process, 
	so there is no clear separation and protection for memory access among threads.

The example code includes some primitive code for multiple threads usage.
It basically tries to read a file and print it out on the screen.
It consists of three threads: main thread for admin job
	second thread serves as a producer: reads lines from a file, and put the line string on the shared buffer
	third thread serves as a consumer:  get strings from the shared buffer, and print the line out on the screen

Unfortunately, the code is not working because threads runs independently from others, 
the result is that different threads access invalid memory, and have wrong value, and crash.
To make it working, you have to touch the code so that the threads have correct value.

To have correct values in threads, you need to keep consistency for data touched by multiple threads.
To keeping consistency, you should carefully control the execution among threads, which is called as synchronization.

pthread_mutex_lock()/unlock are the functions for pthreads synchorinization.

The goals from HW2 are 
1. correct the code for prod_cons.c so that it works with 1 producer and 1 consumer
2. enhance it to support multiple producers
3. enhance it to support multiple consumers
4. enhance it to support multiple (concurrent) producers and consumers.

Happy hacking
Seehwan
