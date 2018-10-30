# 2018_os_HW2: multithreading

## n_prod_n_cons.c
### compiling
> gcc -pthread n_prod_n_cons.c

### runing
> ./prod_cons file_name num_producer num_Consumer

`num_producer` and `num_Consumer` are optional.

## structures

### file_acess
for the producers
* `FILE` to read the lines
* `mutex` for synchronization

### buffer
for the producers and consumers
* line;
* `buffer`, "next" for linked list
* `mutex` for synchronization
* last; as a flag when the end of the buffer is 

### stat
for the consumers
* array `count` the position is the character, the value is the occurrences
* `mutex` for synchronization

### complex_obj
to pass the shared objects to the funtions
* `file_acess`
* `buffer` "head" to put new buffers
* `buffer` "tail" to read the buffers
* `stat` shared statistics

## threads
### producer
operation:
1. read line from the file
2. create a new buffer with the line
4. put the new buffer at the head of the buffer stream
5. repeat until no more lines are found

### consumer
operation:
1. get the buffer at the tail of the stream
2. analyze the line in the buffer
3. repeat until the end is reached
4. add the statistics on the shared statistics

## mehtods
### main
**entry of the program**

operation:
1. initialize the shared objects
2. initialize producer and consumer threads
3. wait for the treads to finish
4. print the statistics

### init_consumers
fills the passed array with the indicated amount of threads

### init_producers
fills the passed array with the indicated amount of threads

### complex_obj
returns a newly initialized complex object

### print_file_statistics
it explins itself
