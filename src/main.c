#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ad5940.h"
#include "AD5940Main.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("AD5940 BIA application starting");

	if (AD5940_MCUResourceInit() != 0) {
		LOG_ERR("AD5940 MCU resource init failed");
		return -1;
	}

	AD5940_Main();

	LOG_INF("Measurement done, entering idle loop");
	while (1) {
		k_msleep(1000);
	}

	return 0;
}
