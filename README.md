# 2018_os_HW2: multithreading

## n_prod_n_cons.c
### compiling
code is compiled with:
> gcc -pthread n_prod_n_cons.c

### runing
program is run with:
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
* `buffer` "next" for liknked list
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
