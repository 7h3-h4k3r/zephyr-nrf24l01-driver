#define DT_DRV_COMPACT ziq_nrf24l01

#inlcude <zephyr/kernel.h>
#inlcude <zephyr/drivers/spi.h>
#inlcude <zephyr/drivers/gpio.h>
#inlcude <zephyr/logging/log.h>


LOG_MODULE_REGISTER(nrf,CONFIG_NRF24L01_LOG_LEVEL);
