/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
  NAME: I2C TARGET DEVICE
  FILE: hal_i2c_target.c
  DESCRIPTION: There is 1 callback function "i2c_target_cb" for I2C target ISR handle and user APIs for user access.
  AUTHOR: MouchenHung
  DATE/VERSION: 2021.12.09 - v1.4.2
  Note: 
    (1) Shall not modify code in this file!!!

    (2) "hal_i2c_target.h" must be included!

    (3) User APIs follow check-rule before doing task 
          [api]                               [.is_init] [.is_register]
        * i2c_target_control                   X          X
        * i2c_target_read                      O          X
        * i2c_target_status_get                X          X
        * i2c_target_status_print              X          X
        * i2c_target_cfg_get                   O          X
                                              (O: must equal 1, X: no need to check)

    (4) I2C target function/api usage recommend
        [ACTIVATE]
          Use "i2c_target_control()" to register/modify/unregister target bus
        [READ]
          Use "i2c_target_read()" to read target queue message

    (5) Target queue method: Zephyr api, unregister the bus while full msgq, register back while msgq get space.
*/

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/i2c.h>
#include "hal_i2c_target.h"
#include "libutil.h"
#include <shell/shell.h>
#include "ssif.h"

/* LOG SET */
#include <logging/log.h>
#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
LOG_MODULE_REGISTER(hal_i2c_target);

/* I2C target device arr */
static struct i2c_target_device i2c_target_device_global[MAX_TARGET_NUM] = { 0 };

/* I2C target config modify lock */
struct k_mutex i2c_target_mutex[MAX_TARGET_NUM];

/* static function declare */
static int do_i2c_target_cfg(uint8_t bus_num, struct _i2c_target_config *cfg);
static int do_i2c_target_register(uint8_t bus_num);
static int do_i2c_target_unregister(uint8_t bus_num);

static bool do_something_while_rd_start(void *arg)
{
	ARG_UNUSED(arg);
	return true;
}

static int i2c_target_write_requested(struct i2c_slave_config *config)
{
	CHECK_NULL_ARG_WITH_RETURN(config, 1);

	struct i2c_target_data *data;
	data = CONTAINER_OF(config, struct i2c_target_data, config);

	data->target_wr_msg.msg_length = 0;
	memset(data->target_wr_msg.msg, 0x0, MAX_I2C_TARGET_BUFF);
	data->wr_buffer_idx = 0;
	data->skip_msg_wr = false;

	return 0;
}

static int i2c_target_write_received(struct i2c_slave_config *config, uint8_t val)
{
	CHECK_NULL_ARG_WITH_RETURN(config, 1);

	struct i2c_target_data *data;
	data = CONTAINER_OF(config, struct i2c_target_data, config);

	if (data->wr_buffer_idx >= MAX_I2C_TARGET_BUFF) {
		LOG_ERR("Buffer_idx over limit!");
		return 1;
	}
	data->target_wr_msg.msg[data->wr_buffer_idx++] = val;

	return 0;
}

static int i2c_target_read_requested(struct i2c_slave_config *config, uint8_t *val)
{
	CHECK_NULL_ARG_WITH_RETURN(config, 1);
	CHECK_NULL_ARG_WITH_RETURN(val, 1);

	struct i2c_target_data *data;
	data = CONTAINER_OF(config, struct i2c_target_data, config);

	if (data->prefix_rd_func) {
		if (data->prefix_rd_func(data) == false)
			data->skip_msg_wr = true;
	}

	if (!data->target_rd_msg.msg_length) {
		LOG_WRN("Data not ready");
		return 1;
	}

	data->rd_buffer_idx = 0;
	*val = data->target_rd_msg.msg[data->rd_buffer_idx++];

	return 0;
}

static int i2c_target_read_processed(struct i2c_slave_config *config, uint8_t *val)
{
	CHECK_NULL_ARG_WITH_RETURN(config, 1);
	CHECK_NULL_ARG_WITH_RETURN(val, 1);

	struct i2c_target_data *data;
	data = CONTAINER_OF(config, struct i2c_target_data, config);

	if (data->target_rd_msg.msg_length - data->rd_buffer_idx == 0) {
		LOG_DBG("No remain buffer to read!");
		return 1;
	}

	if (data->rd_buffer_idx >= MAX_I2C_TARGET_BUFF) {
		LOG_ERR("Buffer_idx over limit!");
		return 1;
	}

	*val = data->target_rd_msg.msg[data->rd_buffer_idx++];

	return 0;
}

static int i2c_target_stop(struct i2c_slave_config *config)
{
	CHECK_NULL_ARG_WITH_RETURN(config, 1);

	struct i2c_target_data *data;
	data = CONTAINER_OF(config, struct i2c_target_data, config);

	int ret = 1;

	if (data->wr_buffer_idx) {
		if (data->skip_msg_wr == true) {
			ret = 0;
			goto clean_up;
		}

		data->target_wr_msg.msg_length = data->wr_buffer_idx;

		/* try to put new node to message queue */
		uint8_t status = k_msgq_put(&data->target_wr_msgq_id, &data->target_wr_msg, K_NO_WAIT);
		if (status) {
			LOG_ERR("Can't put new node to message queue on bus[%d], cause of %d",
				data->i2c_bus, status);
			goto clean_up;
		}

		/* if target queue is full, unregister the bus target to prevent next message handle */
		if (!k_msgq_num_free_get(&data->target_wr_msgq_id)) {
			LOG_DBG("Target queue is full, unregister bus[%d]", data->i2c_bus);
			do_i2c_target_unregister(data->i2c_bus);
		}
	}

	if (data->rd_buffer_idx) {
		if (data->target_rd_msg.msg_length - data->rd_buffer_idx) {
			LOG_WRN("Read buffer doesn't read complete");
		}
	}

	ret = 0;

clean_up:
	data->rd_buffer_idx = 0;
	data->wr_buffer_idx = 0;

	return ret;
}

static const struct i2c_slave_callbacks i2c_target_cb = {
	.write_requested = i2c_target_write_requested,
	.read_requested = i2c_target_read_requested,
	.write_received = i2c_target_write_received,
	.read_processed = i2c_target_read_processed,
	.stop = i2c_target_stop,
};

/*
  - Name: i2c_target_status_get (ESSENTIAL for every user API)
  - Description: Get current status of i2c target.
  - Input:
      * bus_num: Bus number with zero base
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_error_status")
*/
uint8_t i2c_target_status_get(uint8_t bus_num)
{
	uint8_t ret = I2C_TARGET_HAS_NO_ERR;

	if (bus_num >= MAX_TARGET_NUM) {
		ret |= I2C_TARGET_BUS_INVALID;
		goto out;
	}

	char controllername[10];
	if (snprintf(controllername, sizeof(controllername), "%s%d", I2C_DEVICE_PREFIX, bus_num) <
	    0) {
		LOG_ERR("I2C controller name parsing error!");
		ret = I2C_TARGET_CONTROLLER_ERR;
		goto out;
	}

	const struct device *tmp_device = device_get_binding(controllername);
	if (!tmp_device) {
		ret |= I2C_TARGET_CONTROLLER_ERR;
		goto out;
	}

	if (!i2c_target_device_global[bus_num].is_init) {
		ret |= I2C_TARGET_NOT_INIT;
	}

	if (!i2c_target_device_global[bus_num].is_register) {
		ret |= I2C_TARGET_NOT_REGISTER;
	}

out:
	return ret;
}

/*
  - Name: i2c_target_cfg_get (OPTIONAL)
  - Description: Get current cfg of i2c target.
  - Input:
      * bus_num: Bus number with zero base
      * cfg: cfg structure(controller name is not support!)
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
uint8_t i2c_target_cfg_get(uint8_t bus_num, struct _i2c_target_config *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, I2C_TARGET_API_INPUT_ERR);

	/* check input */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status &
	    (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR | I2C_TARGET_NOT_INIT)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;

	cfg->address = data->config.address;
	cfg->i2c_msg_count = data->target_wr_msgq_id.max_msgs;

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: i2c_target_status_print (OPTIONAL|DEBUGUSE)
  - Description: Get current status of i2c target queue.
  - Input:
      * bus_num: Bus number with zero base
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
uint8_t i2c_target_status_print(uint8_t bus_num)
{
	/* check input */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status & (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;
	struct i2c_target_device *target_info = &i2c_target_device_global[bus_num];
	printf("=============================\n");
	printf("Target bus[%d] monitor\n", bus_num);
	printf("* init:        %d\n", target_info->is_init);
	printf("* register:    %d\n", target_info->is_register);
	printf("* address:     0x%x\n", data->config.address);
	printf("* status:      %d/%d\n", k_msgq_num_used_get(&data->target_wr_msgq_id),
	       data->target_wr_msgq_id.max_msgs);
	printf("=============================\n");

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: i2c_target_read
  - Description: Try to get message from i2c target message queue.
  - Input:
      * bus_num: Bus number with zero base
      * *buff: Message that readed back from queue
      * buff_len: Length of buffer
      * *msg_len: Read-back message's length
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
uint8_t i2c_target_read(uint8_t bus_num, uint8_t *buff, uint16_t buff_len, uint16_t *msg_len)
{
	CHECK_NULL_ARG_WITH_RETURN(buff, I2C_TARGET_API_INPUT_ERR);
	CHECK_NULL_ARG_WITH_RETURN(msg_len, I2C_TARGET_API_INPUT_ERR);

	/* check input, support while bus target is unregistered */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status &
	    (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR | I2C_TARGET_NOT_INIT)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;
	struct i2c_msg_package local_buf;

	/* wait if there's no any message in message queue */
	uint8_t ret = k_msgq_get(&data->target_wr_msgq_id, &local_buf, K_FOREVER);
	if (ret) {
		LOG_ERR("Can't get new node from message queue on bus[%d], cause of %d",
			data->i2c_bus, ret);
		return I2C_TARGET_API_MSGQ_ERR;
	}

	if (buff_len < local_buf.msg_length) {
		memcpy(buff, &(local_buf.msg), buff_len);
		*msg_len = buff_len;
	} else {
		memcpy(buff, &(local_buf.msg), local_buf.msg_length);
		*msg_len = local_buf.msg_length;
	}

	/* if bus target has been unregister cause of queue full previously, then register it on */
	if (k_msgq_num_used_get(&data->target_wr_msgq_id) ==
	    (data->target_wr_msgq_id.max_msgs - 1)) {
		LOG_DBG("Target queue has available space, register bus[%d]", data->i2c_bus);

		if (do_i2c_target_register(bus_num)) {
			LOG_ERR("Target queue register bus[%d] failed!", data->i2c_bus);
			return I2C_TARGET_API_BUS_GET_FAIL;
		}
	}

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: i2c_target_write
  - Description: Try to put message to i2c target message queue.
  - Input:
      * bus_num: Bus number with zero base
      * *buff: Message that put in queue
      * buff_len: Length of buffer
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
uint8_t i2c_target_write(uint8_t bus_num, uint8_t *buff, uint16_t buff_len)
{
	CHECK_NULL_ARG_WITH_RETURN(buff, I2C_TARGET_API_INPUT_ERR);

	/* check input, support while bus target is unregistered */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status &
	    (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR | I2C_TARGET_NOT_INIT)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;

	if (buff_len > MAX_I2C_TARGET_BUFF) {
		LOG_WRN("Given data length %d over limit %d", buff_len, MAX_I2C_TARGET_BUFF);
		buff_len = MAX_I2C_TARGET_BUFF;
	}

	memcpy(data->target_rd_msg.msg, buff, buff_len);
	data->target_rd_msg.msg_length = buff_len;

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: i2c_target_control
  - Description: Register controller for user api.
  - Input:
      * bus_num: Bus number with zero base
      * *cfg: Config settings structure
      * mode: check "i2c_target_api_control_mode"
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
int i2c_target_control(uint8_t bus_num, struct _i2c_target_config *cfg,
		       enum i2c_target_api_control_mode mode)
{
	/* Check input and target status */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status & (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	int ret;

	switch (mode) {
	/* Case1: do config then register (if already config before, then modify config set) */
	case I2C_CONTROL_REGISTER:
		CHECK_NULL_ARG_WITH_RETURN(cfg, I2C_TARGET_API_INPUT_ERR);

		ret = do_i2c_target_cfg(bus_num, cfg);
		if (ret) {
			LOG_ERR("Bus[%d] config failed with errorcode %d!", bus_num, ret);
			return ret;
		}

		ret = do_i2c_target_register(bus_num);
		if (ret) {
			LOG_ERR("Bus[%d] register failed with errorcode %d!", bus_num, ret);
			return ret;
		}

		break;

	/* Case2: do unregister only, config not affected */
	case I2C_CONTROL_UNREGISTER:
		ret = do_i2c_target_unregister(bus_num);
		if (ret) {
			LOG_ERR("Bus[%d] unregister failed with errorcode %d!", bus_num, ret);
			return ret;
		}

		break;

	default:
		return I2C_TARGET_API_INPUT_ERR;
	}

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: do_i2c_target_cfg
  - Description: To initialize I2C target config, or modify config after initialized.
  - Input:
      * bus_num: Bus number with zero base
      * *bus_name: Bus controler name with string(it's used to get binding from certain zephyr device tree driver)
      * target_address: Given a target adress for BIC itself
      * _max_msg_count: Maximum count of messages in message queue
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
static int do_i2c_target_cfg(uint8_t bus_num, struct _i2c_target_config *cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(cfg, I2C_TARGET_API_INPUT_ERR);

	int ret = I2C_TARGET_API_NO_ERR;

	/* check input, support while bus target is unregistered */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status & (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	/* need unregister first */
	if (!(target_status & I2C_TARGET_NOT_REGISTER)) {
		int status = do_i2c_target_unregister(bus_num);
		if (status) {
			LOG_ERR("Target bus[%d] unregister failed cause of %d!", bus_num, status);
			return I2C_TARGET_API_BUS_GET_FAIL;
		}
	}

	/* Mutex init here */
	if (target_status & I2C_TARGET_NOT_INIT) {
		if (k_mutex_init(&i2c_target_mutex[bus_num])) {
			LOG_ERR("Target bus[%d] mutex init - failed!", bus_num);
			return I2C_TARGET_API_LOCK_ERR;
		}
		LOG_DBG("Target bus[%d] mutex init - success!", bus_num);
	}

	if (k_mutex_lock(&i2c_target_mutex[bus_num], K_MSEC(1000))) {
		LOG_ERR("Target bus[%d] mutex lock failed!", bus_num);
		return I2C_TARGET_API_LOCK_ERR;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;
	char *i2C_target_queue_buffer;

	/* do init, Only one time init for each bus target */
	if (target_status & I2C_TARGET_NOT_INIT) {
		LOG_DBG("Bus[%d] is going to init!", bus_num);

		data->i2c_bus = bus_num;

		char controllername[10];
		if (snprintf(controllername, sizeof(controllername), "%s%d", I2C_DEVICE_PREFIX,
			     bus_num) < 0) {
			LOG_ERR("I2C controller name parsing error!");
			ret = I2C_TARGET_API_MEMORY_ERR;
			goto unlock;
		}

		data->i2c_controller = device_get_binding(controllername);
		if (!data->i2c_controller) {
			LOG_ERR("I2C controller not found!");
			ret = -EINVAL;
			goto unlock;
		}

		data->config.callbacks = &i2c_target_cb;
	}
	/* do modify, modify config set after init */
	else {
		LOG_DBG("Bus[%d] is going to modified!", bus_num);

		k_msgq_purge(&data->target_wr_msgq_id);

		SAFE_FREE(data->target_wr_msgq_id.buffer_start);
	}

	data->max_msg_count = cfg->i2c_msg_count;
	data->config.address = cfg->address >> 1; // to 7-bit target address

	if (cfg->prefix_rd_func)
		data->prefix_rd_func = cfg->prefix_rd_func;
	else
		data->prefix_rd_func = do_something_while_rd_start;

	i2C_target_queue_buffer = malloc(data->max_msg_count * sizeof(struct i2c_msg_package));
	if (!i2C_target_queue_buffer) {
		LOG_ERR("I2C target bus[%d] msg queue memory allocate failed!", data->i2c_bus);
		ret = I2C_TARGET_API_MEMORY_ERR;
		goto unlock;
	}

	k_msgq_init(&data->target_wr_msgq_id, i2C_target_queue_buffer,
		    sizeof(struct i2c_msg_package), data->max_msg_count);

	i2c_target_device_global[bus_num].is_init = 1;

	LOG_DBG("I2C target bus[%d] message queue create success with count %d!", data->i2c_bus,
		data->max_msg_count);

unlock:
	if (k_mutex_unlock(&i2c_target_mutex[bus_num])) {
		LOG_ERR("Mutex unlock failed!");
	}

	return ret;
}

/*
  - Name: do_i2c_target_register
  - Description: Set config to register for enable i2c target.
  - Input:
      * bus_num: Bus number with zero base
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
static int do_i2c_target_register(uint8_t bus_num)
{
	/* Check input and target status */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status &
	    (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR | I2C_TARGET_NOT_INIT)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	/* Check register status */
	if (!(target_status & I2C_TARGET_NOT_REGISTER)) {
		LOG_ERR("Bus[%d] has already been registered, please unregister first!", bus_num);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;

	/* check whether msgq is full */
	if (!k_msgq_num_free_get(&data->target_wr_msgq_id)) {
		LOG_ERR("Bus[%d] msgq is already full, can't register now, please read out message first!",
			bus_num);
		return I2C_TARGET_API_MSGQ_ERR;
	}

	int ret = i2c_slave_register(data->i2c_controller, &data->config);
	if (ret)
		return ret;

	i2c_target_device_global[bus_num].is_register = 1;

	return I2C_TARGET_API_NO_ERR;
}

/*
  - Name: do_i2c_target_unregister
  - Description: Set config to register for disable i2c target.
  - Input:
      * bus_num: Bus number with zero base
      * mutex_flag: skip check if 1, otherwise 0
  - Return: 
      * 0, if no error
      * others, get error(check "i2c_target_api_error_status")
*/
static int do_i2c_target_unregister(uint8_t bus_num)
{
	/* Check input and target status */
	uint8_t target_status = i2c_target_status_get(bus_num);
	if (target_status &
	    (I2C_TARGET_BUS_INVALID | I2C_TARGET_CONTROLLER_ERR | I2C_TARGET_NOT_INIT)) {
		LOG_ERR("Bus[%d] check status failed with error status 0x%x!", bus_num,
			target_status);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	/* Check register status */
	if (target_status & I2C_TARGET_NOT_REGISTER) {
		LOG_ERR("Bus[%d] has already been unregistered, please register first!", bus_num);
		return I2C_TARGET_API_BUS_GET_FAIL;
	}

	struct i2c_target_data *data = &i2c_target_device_global[bus_num].data;

	int ret = i2c_slave_unregister(data->i2c_controller, &data->config);
	if (ret)
		return ret;

	i2c_target_device_global[bus_num].is_register = 0;

	return I2C_TARGET_API_NO_ERR;
}

void cmd_target_register(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_warn(shell, "Help: i2cterget register <bus> <register/unregister>");
		return;
	}

	uint8_t bus = strtol(argv[1], NULL, 10);
	uint8_t flag = strtol(argv[2], NULL, 10);

	struct _i2c_target_config cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.address = 0x40;
	cfg.i2c_msg_count = 0x0A;

	if (flag == 1) {
		if (i2c_target_control(bus, &cfg, I2C_CONTROL_REGISTER) != I2C_TARGET_API_NO_ERR) {
			shell_error(shell, "Failed to register target");
		}
	} else {
		if (i2c_target_control(bus, &cfg, I2C_CONTROL_UNREGISTER) !=
		    I2C_TARGET_API_NO_ERR) {
			shell_error(shell, "Failed to unregister target");
		}
	}
}

void cmd_ssif_init(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: i2cterget ssif_init <i2c_bus>");
		return;
	}

	uint8_t bus = strtol(argv[1], NULL, 10);

	shell_info(shell, "SSIF %d init!", bus);

	/* only create 1 ssif channel */
	struct ssif_init_cfg *cfg = (struct ssif_init_cfg *)malloc(1 * sizeof(struct ssif_init_cfg));
	if (!cfg) {
		shell_error(shell, "Failed to malloc ssif cfg list");
		return;
	}

	cfg[0].i2c_bus = bus;
	cfg[0].addr = 0x40;
	cfg[0].target_msgq_cnt = 0x0A;

	ssif_device_init(cfg, 1);
}

void cmd_target_info(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_warn(shell, "Help: i2cterget ssif_status <i2c_bus>");
		return;
	}

	uint8_t bus = strtol(argv[1], NULL, 10);

	ssif_dev *ssif_inst = ssif_inst_get_by_bus(bus);
	if (!ssif_inst) {
		shell_error(shell, "Could not find ssif inst by i2c bus %d", bus);
		return;
	}

	shell_print(shell, "SSIF[%d] bus %d addr 0x%x:", ssif_inst->index, ssif_inst->i2c_bus, ssif_inst->addr);
	shell_print(shell, "* current status: 0x%d", ssif_inst->cur_status);
	shell_print(shell, "* current address lock: %s", ssif_inst->addr_lock == true ? "LOCK":"UNLOCK");
	shell_print(shell, "* error status: 0x%x", ssif_inst->err_status);
	shell_print(shell, "* error idx: %d/%d", ssif_inst->err_idx, SSIF_ERR_RCD_SIZE);
	shell_print(shell, "* error list:");
	shell_hexdump(shell, ssif_inst->err_status_lst, SSIF_ERR_RCD_SIZE);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i2ctarget_cmds,
			       SHELL_CMD(register, NULL, "REGISTER.", cmd_target_register),
			       SHELL_CMD(ssif_init, NULL, "SSIF init.", cmd_ssif_init),
					SHELL_CMD(ssif_info, NULL, "SSIF info.", cmd_target_info),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(i2ctarget, &sub_i2ctarget_cmds, "i2c target", NULL);