#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <device.h>
#include "hal_snoop.h"
#include "ipmi.h"
#include "ipmb.h"

const struct device *snoop_dev;
uint8_t *snoop_data;
static uint8_t *snoop_read_buffer;
int snoop_read_num[2] = {0};
static bool proc_postcode_ok = false;

K_THREAD_STACK_DEFINE( snoop_thread , SNOOP_STACK_SIZE );
struct k_thread snoop_thread_handler;
k_tid_t snoop_tid;

K_THREAD_STACK_DEFINE( snoop_msgq , SNOOP_MSGQ_THREAD_STACK_SIZE );
struct k_thread snoop_msgq_handler;
k_tid_t snoop_msgq_tid;

#define MAX_SNOOP_MSG_COUNT 100
struct k_msgq snoop_msgq_id;

typedef struct __attribute__((__packed__)) postcode_msg_package {
    uint8_t cur_postcode;
} postcode_msg_package_t;

/* LOG SET */
#include <logging/log.h>
LOG_MODULE_REGISTER(snoop);

struct k_mutex snoop_mutex;

void snoop_init(){
  snoop_dev = device_get_binding(DT_LABEL(DT_NODELABEL(snoop)));
  if (!snoop_dev) {
    printf("No snoop device found\n");
    return;
  }

  if (k_mutex_init(&snoop_mutex)) {
    printf("<error> Snoop mutex init - failed!\n");
    return;
  }

  uint8_t *snoop_msg_queue_buffer = malloc(MAX_SNOOP_MSG_COUNT * sizeof(postcode_msg_package_t));
  if (!snoop_msg_queue_buffer) {
    printf("<error> snoop_msg_queue_buffer malloc failed!\n");
    return;
  }

  k_msgq_init(&snoop_msgq_id, snoop_msg_queue_buffer, sizeof(postcode_msg_package_t), MAX_SNOOP_MSG_COUNT);

  return;
}

void copy_snoop_read_buffer( uint8_t offset ,int size_num, uint8_t *buffer ){
  if ( ! k_mutex_lock(&snoop_mutex, K_MSEC(1000)) ){
    memcpy( &buffer[0] , &snoop_read_buffer[offset] , size_num - offset );
    memcpy( &buffer[ size_num - offset ] , &snoop_read_buffer[0] , offset );
  }else{
    printf("copy snoop buffer lock fail\n");
  }
  if ( k_mutex_unlock(&snoop_mutex) ){
    printf("copy snoop buffer unlock fail\n");
  }
}

bool get_postcode_ok() {
  return proc_postcode_ok;
}

void reset_postcode_ok() {
  proc_postcode_ok = false;
}

void snoop_msgq_read(){
  static int post_code_tmp;
  while (1) {
    uint8_t cur_postcode;
    k_msgq_get(&snoop_msgq_id, &cur_postcode, K_FOREVER);

    if (snoop_read_num[0] == SNOOP_MAX_LEN) {
      snoop_read_num[0] = 0;
      snoop_read_num[1] = 1;
    }

    snoop_read_buffer[ snoop_read_num[0]++ ] = cur_postcode;
    post_code_tmp++;

    ipmi_msg msg;
    memset(&msg, 0, sizeof(ipmi_msg));

    msg.InF_source = Self_IFs;
    msg.InF_target = BMC_IPMB_IFs;
    msg.netfn = 0x38;
    msg.cmd = 0x08;
    msg.data[0] = 0x9C;
    msg.data[1] = 0x9C;
    msg.data[2] = 0x00;
    msg.data[3] = 1;
    msg.data[4] = cur_postcode;
    msg.data_len = 5;

    if ( ipmb_read(&msg, IPMB_inf_index_map[msg.InF_target]) )
      LOG_ERR("POST CODE transfer to BMC with byte[%d-%d]: 0x%x failed!\n",  snoop_read_num[0], snoop_read_num[1], msg.data[4]);
    else
      LOG_WRN("(%-3d/%-3d)POST CODE transfer to BMC success with byte[%-3d-%-3d-%d]: 0x%x",  k_msgq_num_used_get(&snoop_msgq_id), MAX_SNOOP_MSG_COUNT, post_code_tmp, snoop_read_num[0], snoop_read_num[1], msg.data[4]);
  }
}

void snoop_read(){
  int rc;
  snoop_read_buffer = malloc( sizeof(uint8_t) * SNOOP_MAX_LEN );
  if (!snoop_read_buffer)
    return;
  uint32_t POST_CODE_NUMBERS = 0;
  while (1) {
    if (snoop_aspeed_read(snoop_dev, 0, snoop_data, true) == 0){
      if ( ! k_mutex_lock(&snoop_mutex, K_NO_WAIT) ){
        proc_postcode_ok = true;
        //LOG_WRN("postcode[%d] = 0x%x", POST_CODE_NUMBERS, *snoop_data);
        POST_CODE_NUMBERS++;

        uint8_t ret = k_msgq_put(&snoop_msgq_id, snoop_data, K_NO_WAIT);
        if (ret)
          LOG_ERR("Can't put new postcode to SNOOP message queue, cause of %d", ret);
      }else{
        LOG_ERR("snoop read lock fail\n");
      }
      if ( k_mutex_unlock(&snoop_mutex) ){
        LOG_ERR("snoop read unlock fail\n");
      }
    }
  }
}

void snoop_start_thread(){
  snoop_init();
  if ( snoop_tid != NULL ){
    k_thread_abort(snoop_msgq_tid);
    k_thread_abort(snoop_tid);
  }
  snoop_msgq_tid = k_thread_create(&snoop_msgq_handler, snoop_msgq,
                              K_THREAD_STACK_SIZEOF(snoop_msgq),
                              snoop_msgq_read, NULL, NULL, NULL,
                              CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
  k_thread_name_set(&snoop_msgq_handler, "snoop_msgq_thread");

  snoop_tid = k_thread_create(&snoop_thread_handler, snoop_thread,
                              K_THREAD_STACK_SIZEOF(snoop_thread),
                              snoop_read, NULL, NULL, NULL,
                              CONFIG_MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
  k_thread_name_set(&snoop_thread_handler, "snoop_thread");
}
