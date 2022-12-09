#ifndef UTIL_HEAP_H
#define UTIL_HEAP_H

#include <stdint.h>

struct heap_node {
    char *var_name;
    const char *func_name;
	unsigned long int addr;
	size_t size;
};

#define MAX_MEM_STORE_SIZE 1000
extern struct heap_node USER_MEMORY_STORE[MAX_MEM_STORE_SIZE];

void malloc_record(void *ptr, size_t size, char *ptr_name, const char *func_name);
void free_record(void *ptr, char *ptr_name, const char *func_name);
void list_memory_array();

#endif