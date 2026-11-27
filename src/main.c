#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ctype.h>


LOG_MODULE_REGISTER(main,CONFIG_NRF24L01_LOG_LEVEL);

static const struct device *const nrfdev = DEVICE_DT_GET(DT_NODELABEL(nrf24l01));

int main(void)
{

    int ret;

    if(!device_is_ready(nrfdev)){
        LOG_ERR("NRF24L01 device is not ready");
        return -ENODEV;
    }

    LOG_INF("Device are ready to commnnication");

}