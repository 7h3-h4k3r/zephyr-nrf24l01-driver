#ifndef NRF24L01_H_
#define NRF24L01_H_

#include <zephyr/device.h>
#include <stdint.h>
#include <errno.h>

#define CONFIG 0x00
#define EN_RXADDR 0x02
#define SETUP_AW 0x03
#define SETUP_RETR 0x04
#define RF_CH 0x05
#define RF_SETUP 0x06
#define STATUS 0x07
#define RX_ADDR_P0 0x0A
#define TX_ADDR 0x10
#define RX_PW_P0 0x11
#define FIFO_STATUS 0x17
#define DYNPD 0x1C
#define EN_CRC 0x08
#define CRCO 0x04
#define PWR_UP 0x02
#define PRIM_RX 0x01
#define POWER_0 0x00
#define POWER_1 0x02
#define POWER_2 0x04
#define POWER_3 0x06
#define SPEED_1M 0x00
#define SPEED_2M 0x08
#define SPEED_250K 0x20
#define RX_DR 0x40
#define TX_DS 0x20
#define MAX_RT 0x10
#define RX_EMPTY 0x01
#define R_RX_PL_WID 0x60
#define R_RX_PAYLOAD 0x61
#define W_TX_PAYLOAD 0xA0
#define FLUSH_TX 0xE1
#define FLUSH_RX 0xE2
#define NOP 0xFF



/*NRF24L01 Configuration nrf_conf have spi and gpio */

typedef struct{
    struct spi_dt_spec spi;
    struct gpio_dt_spec chipen;

}nrf_conf;


typedef struct{
    uint8_t status;
    uint8_t buf;
    uint8_t payload[32];
    uint8_t pipe0_read_addr[5];
}nrf_data;


#endif