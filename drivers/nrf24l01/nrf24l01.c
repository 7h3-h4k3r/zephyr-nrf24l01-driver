#define DT_DRV_COMPAT ziq_nrf24l01

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "nrf24l01.h"

LOG_MODULE_REGISTER(nrf,CONFIG_NRF24L01_LOG_LEVEL);
#define NRF24L01_SPI_OP (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))

static int reg_read(const struct device *dev,uint8_t reg)
{
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;

    uint8_t tx_data[2] = {reg , 0x00};
    uint8_t rx_data[2] = {0};


    struct spi_buf tx_buf = {
        .buf = tx_data,
        .len = sizeof(tx_data),
    };
    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1,
    };

    struct spi_buf rx_buf = {
        .buf = rx_data,
        .len = sizeof(rx_data),
    };
    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1,
    };

    int ret = spi_transceive_dt(&cfg->spi, &tx_bufs, &rx_bufs);
    if (ret < 0) {
        return ret;
    }

    if (data != NULL){
        data->status = rx_data[0]; 
        data->buf = rx_data[1];
    }
    return 0;
}
static int reg_write(const struct device *dev , uint8_t reg ,uint8_t val)
{
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;

    uint8_t tx_data[2] = { (0x20 | reg), val };
    uint8_t rx_data[2] = { 0};

    struct spi_buf tx_buf = {.buf=tx_data,.len=sizeof(tx_data),};
    struct spi_buf_set tx_bufs = {.buffers = &tx_buf,.count=1,};

    struct spi_buf rx_buf = {.buf=rx_data,.len=sizeof(rx_data),};
    struct spi_buf_set rx_bufs = {.buffers=&rx_buf,.count=1,};

    int err = spi_transceive_dt(&cfg->spi,&tx_bufs,&rx_bufs);

    if (err > 0){
        return err;
    }

    if (data != NULL){
        data->buf = rx_data[0];
    }

    return 0;
}

static int nrf24l01_INIT(const struct device *dev){
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;

    int ret;

    if(!gpio_is_ready_dt(&cfg->chipen)){
        LOG_ERR("chip enable GPIO pin is not ready");
        return -ENODEV;
    }

    if(!gpio_is_ready_dt(&cfg->cs)){
        LOG_ERR("chip select GPIO pin is not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&cfg->chipen,GPIO_OUTPUT_INACTIVE);

    if(ret < 0){
        LOG_ERR("chip enable is gpio is failed");
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&cfg->cs,GPIO_OUTPUT_INACTIVE);

    if (ret < 0){
        LOG_ERR("chip select gpio pin is failed");
        return ret;
    }
    k_msleep(10);
    gpio_pin_set_dt(&cfg->chipen,0);
    LOG_INF("[1] Chip enabled reset");
    gpio_pin_set_dt(&cfg->cs,1);
    LOG_INF("[2] Chip select is On");

    
    if(!spi_is_ready_dt(&cfg->spi)){
        LOG_ERR("Serial perifiral interface Bus is not ready");
        return -ENODEV;
    }

    LOG_INF("[3] Spi device ready to rx/tx");

    reg_write(dev,SETUP_AW,0b11);
    ret = reg_read(dev,SETUP_AW);
    if (ret > 0){
        LOG_ERR("nrf24l01 Hardware full-duplex not responding");
        return -ENODEV;
    }
    if (data->buf != 0b11){
        LOG_ERR("nrf24l01 Hardware not responding");
        return 0;
    }
    if (data != NULL){
        data->status = 1;
    }

    LOG_INF("Device harsware is respinding Successfully");
    return 0;
}


#define NRF24L01_DEFINE(inst)                                                \
	static nrf_data nrf_data_##inst;                     \
	static const nrf_conf nrf_conf_##inst = {        \
		.spi = SPI_DT_SPEC_INST_GET(inst, NRF24L01_SPI_OP),       \
		.chipen = GPIO_DT_SPEC_INST_GET(inst, chip_en_gpios),   \
        .cs = SPI_CS_GPIOS_DT_SPEC_INST_GET(inst)\
	};                                                                  \
	DEVICE_DT_INST_DEFINE(inst,                                         \
			      nrf24l01_INIT,                                 \
			      NULL,                                         \
			      &nrf_data_##inst,                         \
			      &nrf_conf_##inst,                       \
			      POST_KERNEL,                                  \
			      CONFIG_NRF24L01_INIT_PRIORITY,             \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(NRF24L01_DEFINE)