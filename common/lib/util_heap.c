#include "libutil.h"
#include <stdlib.h>
#include <string.h>

#include <util_heap.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(util_heap);

//#ifdef CONFIG_MINIMAL_LIBC_MALLOC

struct heap_node USER_MEMORY_STORE[MAX_MEM_STORE_SIZE] = {0};

static unsigned long int cur_max = 0;

static int mem_store_overflow_flag = 0;
static int mem_store_err_flag = 0;

void list_memory_array() {
    for (unsigned long int i=0; i<cur_max; i++) {
		LOG_WRN("[%lu] ptr: %s func: %s addr: 0x%lx size: %u", i, USER_MEMORY_STORE[i].var_name, USER_MEMORY_STORE[i].func_name, USER_MEMORY_STORE[i].addr, USER_MEMORY_STORE[i].size );
	}
}

static void find_memory_node_from_array(unsigned long int parm_addr)
{
	size_t ret_size = 0;
	if (cur_max > MAX_MEM_STORE_SIZE) {
		LOG_ERR("OUT OF STOCK!!!");
		mem_store_overflow_flag = 1;
	}

	for (int i=0; i<cur_max; i++) {
		if (parm_addr == USER_MEMORY_STORE[i].addr) {
			ret_size = USER_MEMORY_STORE[i].size;
			for (int j=i; j<cur_max-1; j++) {
				USER_MEMORY_STORE[j].addr = USER_MEMORY_STORE[j+1].addr;
				USER_MEMORY_STORE[j].size = USER_MEMORY_STORE[j+1].size;
			}

			USER_MEMORY_STORE[cur_max-1].addr = 0;
			USER_MEMORY_STORE[cur_max-1].size = 0;
            USER_MEMORY_STORE[cur_max-1].var_name = 0;
            USER_MEMORY_STORE[cur_max-1].func_name = 0;
			//heap_total -= ret_size;
			cur_max--;
			break;
		}
	}
}

void malloc_record(void *ptr, size_t size, char *ptr_name, const char *func_name)
{
    static int is_init = 0;
    if (is_init) {
        is_init = 1;
    }

    if (!mem_store_overflow_flag && !mem_store_err_flag) {
		if (USER_MEMORY_STORE[cur_max].addr) {
			printf("error occur get cur_max %lu: %lu!\n", cur_max, USER_MEMORY_STORE[cur_max].addr);
			for (unsigned long int i=0; i<cur_max; i++) {
				printf("[%lu] addr %lu size %u\n", i, USER_MEMORY_STORE[i].addr, USER_MEMORY_STORE[i].size );
			}
			mem_store_err_flag = 1;
		} else {
			//printf("add new space at %lu with %lu\n", cur_max, (unsigned long int)ret);
			USER_MEMORY_STORE[cur_max].var_name = ptr_name;
            USER_MEMORY_STORE[cur_max].func_name = func_name;
            USER_MEMORY_STORE[cur_max].addr = (unsigned long int)ptr;
            USER_MEMORY_STORE[cur_max].size = size;
			cur_max++;
		}
	}
}

void free_record(void *ptr, char *ptr_name, const char *func_name)
{
    find_memory_node_from_array((unsigned long int)ptr);
}

//#endif
