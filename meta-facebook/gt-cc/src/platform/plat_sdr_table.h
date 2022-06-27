#ifndef PLAT_SDR_TABLE_H
#define PLAT_SDR_TABLE_H

#include <stdint.h>

#define MAX_SENSOR_SIZE 150

uint8_t plat_get_sdr_size();
void load_sdr_table(void);

#endif
