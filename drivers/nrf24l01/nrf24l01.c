#define DT_DRV_COMPAT ziq_nrf24l01

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "nrf24l01.h"

LOG_MODULE_REGISTER(nrf,CONFIG_NRF24L01_LOG_LEVEL);
#define NRF24L01_SPI_OP (SPI_OP_MODE_MASTER | SPI_WORD_SET(8)| SPI_TRANSFER_MSB)

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
        data->buf = rx_data[1];
    }
    return 0;
}
static int reg_write(const struct device *dev , uint8_t reg ,uint8_t val)
{
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;

    uint8_t tx_data[2] = { (0x20 | (reg & 0x1F)), val };
    uint8_t rx_data[2] = { 0};

    struct spi_buf tx_buf = {.buf=tx_data,.len=sizeof(tx_data),};
    struct spi_buf_set tx_bufs = {.buffers = &tx_buf,.count=1,};

    struct spi_buf rx_buf = {.buf=rx_data,.len=sizeof(rx_data),};
    struct spi_buf_set rx_bufs = {.buffers=&rx_buf,.count=1,};

    int err = spi_transceive_dt(&cfg->spi,&tx_bufs,&rx_bufs);

    if (err < 0){
        return err;
    }
    return 0;
}
static int set_power_speed(const struct device *dev,uint8_t power , uint8_t speed)
{
    uint16_t setup;
    int ret;
    nrf_data  *data = dev->data;
    ret = reg_read(dev,RF_SETUP);
    if (ret < 0){
        LOG_ERR("set_power_speed read Register failed ");
        return -1;
    }
    setup = data->buf & 0b11010001;

    ret = reg_write(dev,RF_SETUP,setup | power | speed);

    if (ret < 0){
        LOG_ERR("set_power_speed write Register Failed");
        return -1;
    }

    LOG_INF("set the power and speed succssfully");
    return 0;
}
static int set_crc(const struct device *dev ,uint8_t len)
{
    uint16_t config;
    int ret;
    nrf_data *data = dev->data;
    ret = reg_read(dev,CONFIG);
    if (ret < 0){
        LOG_ERR("set_crc read config error");
        return -1;
    }
    
    config = data->buf & ~(CRCO | EN_CRC);

    if (len == 0){
        
    }else if(len == 1){
        config |= EN_CRC;
    }else{

        config |= EN_CRC | CRCO;
    }

    ret = reg_write(dev,CONFIG,config);

    if (ret < 0){
        LOG_ERR("set_crc write failed");
        return -1;
    }

    LOG_INF("set_crc done");
    return 0;

}

static int set_channel(const struct device *dev)
{
    int ret;
    ret = reg_write(dev,RF_CH,45);
    if (ret < 0){
        LOG_ERR("Register RF_CH failed set_channel");
        return -1;
    }

    return 0;
}

static int flash_rx_tx(const struct device *dev,bool tx)
{
    const nrf_conf *cfg = dev->config;
    uint8_t cmd = tx ? FLUSH_TX : FLUSH_RX;
    uint8_t status = 0;

    struct spi_buf tx_buf = {.buf=&cmd,.len=1,};
    struct spi_buf_set tx_bufs = {.buffers = &tx_buf,.count=1,};

    struct spi_buf rx_buf = {.buf=&status,.len=1,};
    struct spi_buf_set rx_bufs = {.buffers=&rx_buf,.count=1,};

    return spi_transceive_dt(&cfg->spi,&tx_bufs,&rx_bufs);
}


static int nrf24l01_INIT(const struct device *dev){
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;

    int ret;

    if(!gpio_is_ready_dt(&cfg->chipen)){
        LOG_ERR("chip enable GPIO pin is not ready");
        return -ENODEV;
    }

    
    ret = gpio_pin_configure_dt(&cfg->chipen,GPIO_OUTPUT_INACTIVE);

    if(ret < 0){
        LOG_ERR("chip enable is gpio is failed");
        return ret;
    }
    
  

   
    k_msleep(10);
    gpio_pin_set_dt(&cfg->chipen,0);
    LOG_INF("[1] Chip enabled reset");

    
    if(!spi_is_ready_dt(&cfg->spi)){
        LOG_ERR("Serial perifiral interface Bus is not ready");
        return -ENODEV;
    }

    LOG_INF("[3] Spi device ready to rx/tx");
    LOG_INF("Test the Register");

    reg_read(dev, 0x05);
    printk("initial read %d \n", data->buf);

    reg_write(dev, 0x05, 40);
    reg_read(dev, 0x05);   // <-- எழுதிய பிறகு திரும்பவும் படிக்க வேண்டும்!
    printk("after write %d\n", data->buf);

    reg_write(dev, 0x05, 8);
    reg_read(dev, 0x05);   // <-- திரும்பவும் படிக்க வேண்டும்!
    printk("restore %d \n", data->buf);
    
    reg_write(dev,SETUP_AW,0b11);
    ret = reg_read(dev,SETUP_AW);
    if (ret < 0){
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

    LOG_INF("Auto retransmit delay: 1750us Auto retransmit count :8 ");
    reg_write(dev,DYNPD,0);
    reg_write(dev,SETUP_RETR,(6 << 4) | 8);

    ret = set_power_speed(dev,POWER_3,SPEED_250K);
    if (ret < 0){
        LOG_ERR("failed to set power and speed");
        return -1;
    }
    LOG_INF("Set rf Power and speed");
    ret = set_crc(dev,2);
    if (ret < 0){
        LOG_ERR("set crc failed ");
        return -1;
    }
    ret = reg_write(dev,STATUS,RX_DR|TX_DS|MAX_RT);
    if (ret < 0){
        LOG_ERR("clear status flag failed");
        return -1;
    }

    ret = set_channel(dev);
    if (ret < 0){
        LOG_ERR("set channel failed");
        return -1;
    }
    LOG_INF("set channel done");
    flash_rx_tx(dev,false);
    flash_rx_tx(dev,true);
    LOG_INF("Flush Buffers Done");

    
    LOG_INF("Device harsware is respinding Successfully");
    return 0;
}


#define NRF24L01_DEFINE(inst)                                                \
	static nrf_data nrf_data_##inst;                     \
	static const nrf_conf nrf_conf_##inst = {        \
		.spi = SPI_DT_SPEC_INST_GET(inst, NRF24L01_SPI_OP),       \
		.chipen = GPIO_DT_SPEC_INST_GET(inst, chip_en_gpios),   \
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