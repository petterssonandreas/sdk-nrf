/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr.h>
#include <dfu/mcuboot.h>
#include <dfu/dfu_target_mcuboot.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <stdio.h>
#include "update.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app_update_main, 4);

#if CONFIG_APPLICATION_VERSION == 2
#define NUM_LEDS 2
#else
#define NUM_LEDS 1
#endif

/* Use sem to not try to do a new download while a download is in progress */
K_SEM_DEFINE(downloading_sem, 1, 1);

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("Received error from fota_download: %d", evt->cause);
		break;
	case FOTA_DOWNLOAD_EVT_FINISHED:
		LOG_INF("FOTA_DOWNLOAD_EVT_FINISHED");
		break;

	default:
		LOG_WRN("Unhandled event: %d", evt->id);
		break;
	}

	k_sem_give(&downloading_sem);
}

static void update_start(void)
{
	int err;

	err = fota_download_start(CONFIG_DOWNLOAD_HOST, CONFIG_DOWNLOAD_FILE,
				  SEC_TAG, 0, 0);
	if (err != 0) {
		LOG_ERR("fota_download_start() failed, err %d", err);
	}
}

void main(void)
{
	int err;

	LOG_INF("HTTP application update sample started");

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		LOG_ERR("Failed to initialize modem library!");
		return;
	}
	/* This is needed so that MCUBoot won't revert the update */
	boot_write_img_confirmed();

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		LOG_ERR("fota_download_init() failed, err %d", err);
		return;
	}

	err = update_sample_init(&(struct update_sample_init_params){
					.update_start = update_start,
					.num_leds = NUM_LEDS
				});
	if (err != 0) {
		LOG_ERR("update_sample_init() failed, err %d", err);
		return;
	}

	int counter = 0;

	/* Endless loop that tries to do a FW download */
	while (true) {
		k_sem_take(&downloading_sem, K_FOREVER);
		LOG_INF("Calling update_start, counter: %d", counter);
		update_start();
		counter++;
		k_sleep(K_MSEC(300));
	}


}
