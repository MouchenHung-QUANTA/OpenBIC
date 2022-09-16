#ifndef PLAT_CLASS_H
#define PLAT_CLASS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SYS_DUAL,
    SYS_SINGLE,
} system_class_t;

typedef enum {
    SRC_MAIN,
    SRC_SECOND,
} source_class_t;

uint8_t get_system_class();
uint8_t get_source_idx();
void set_source_idx(source_class_t idx);

void init_platform_config();

#endif
