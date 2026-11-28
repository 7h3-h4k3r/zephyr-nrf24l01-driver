#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <ctype.h>
#include "nrf24l01.h"


LOG_MODULE_REGISTER(main,CONFIG_NRF24L01_LOG_LEVEL);

static const struct device *const nrfdev = DEVICE_DT_GET(DT_NODELABEL(nrf24l01));

int main(void)
{

    
    if(!device_is_ready(nrfdev)){
        LOG_ERR("NRF24L01 device is not ready");
        return -ENODEV;
    }

    LOG_INF("Device are ready to communication");

    /* 2. Replaces: open_tx_pipe, open_rx_pipe, stop_listening */
    nrf24_open_tx_pipe(nrfdev);
    nrf24_open_rx_pipe(nrfdev, 1);
    nrf24_stop_listening(nrfdev);

    /* 3. Replaces: counter = 0; while True: ... */
    int32_t counter = 0;

    while (1) {
        /* Replaces: struct.pack("i", counter) -> passes &counter and sizeof(counter) */
        int ret = nrf24_send(nrfdev, (const uint8_t *)&counter, sizeof(counter), 500);

        if (ret == 0) {
            printk("Sent: %d\n", counter);
            counter++;
        } else {
            /* Replaces: except OSError: */
            printk("Transmission failed (err: %d)\n", ret);
        }

        /* Replaces: time.sleep(1) */
        k_sleep(K_SECONDS(1));
    }

    return 0;
 
}