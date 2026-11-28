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

    if (data != NULL){
        data->buf = rx_data[0];
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

static int flush_rx_tx(const struct device *dev,bool tx)
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

static int reg_write_bytes(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len)
{
    const nrf_conf *cfg = dev->config;
    uint8_t cmd = 0x20 | (reg & 0x1F);

    struct spi_buf tx_bufs[] = {
        { .buf = &cmd, .len = 1 },
        { .buf = (void *)buf, .len = len },
    };
    const struct spi_buf_set tx_set = {
        .buffers = tx_bufs,
        .count = ARRAY_SIZE(tx_bufs),
    };

    return spi_write_dt(&cfg->spi, &tx_set);
}

static int open_tx_pipe(const struct device *dev)
{   
    nrf_data *data = dev->data;
    int ret;

    ret = reg_write_bytes(dev,RX_ADDR_P0,data->pipe_address_tx,NRF_ADDR_LEN);

    if(ret < 0){
        return ret;
    }

    ret = reg_write_bytes(dev,TX_ADDR,data->pipe_address_tx,NRF_ADDR_LEN);

    if (ret < 0){
        return ret;
    }
    
    ret = reg_write(dev,RX_PW_P0,sizeof(data->payload));
    return ret;
}

static int open_rx_pipe(const struct device *dev, uint8_t pipe_id )
{
    nrf_data *data = dev->data;
    int ret;

    if (pipe_id > 5)
    {
        return -EINVAL;
    }

    if (pipe_id == 0){
        memcpy(data->pipe0_read_addr,data->pipe_address_rx,NRF_ADDR_LEN);
        data->pipe0_read_addr[NRF_ADDR_LEN] = '\0';
    }
    if (pipe_id < 2){
        reg_write_bytes(dev,RX_ADDR_P0 + pipe_id,data->pipe_address_rx,NRF_ADDR_LEN);
    }else{
        reg_write(dev,RX_ADDR_P0 + pipe_id, data->pipe_address_rx[0]);
    }

    reg_write(dev,RX_PW_P0 + pipe_id,sizeof(data->payload));
    ret = reg_read(dev,EN_RXADDR);
    if (ret < 0){
        LOG_ERR("Open rx error");
        return 0;
    }
    return reg_write(dev,EN_RXADDR,data->buf | (1 << pipe_id));
}


static int read_status(const struct device *dev)
{
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;
    uint8_t cmd = NOP;

    struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
    struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };

    struct spi_buf rx_buf = { .buf = &data->buf, .len = 1 };
    struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

    return spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
}

static int send_done(const struct device *dev)
{
    int status = read_status(dev);
    int ret;
    nrf_data *data = dev->data;
    if (!(status & (TX_DS | MAX_RT))){
        return -1;
    }
    ret = reg_write(dev ,STATUS,RX_DR | TX_DS |MAX_RT);
    uint8_t new_st = data->buf;
    if (ret <0 ){
        return ret;
    }
   
    ret = reg_read(dev, CONFIG);
    if (ret < 0){
        return 0;
    }
    ret = reg_write(dev,CONFIG,data->buf & ~PWR_UP);
    if (ret < 0){
        return 0;
    }
    if (new_st & TX_DS ) {
        return 1;
    }
    else {
        return 2;
    }
}
static int stop_listing(const struct device *dev)
{
    const nrf_conf *cfg = dev->config;
    int ret;

    ret = gpio_pin_set_dt(&cfg->chipen,0);
    if (ret  < 0 ){
        return ret;
    }

    flush_rx_tx(dev,true);
    flush_rx_tx(dev,false);

    return 0;
}

static int send_start(const struct device *dev, const uint8_t *buf, size_t len)
{
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;
    const size_t max_payload = sizeof(data->payload);
    int ret;

    if (buf == NULL || len == 0 || len > max_payload) {
        return -EINVAL;
    }

    /* 1. Power up in TX mode (PWR_UP = 1, PRIM_RX = 0) */
    ret = reg_read(dev, CONFIG);
    if (ret < 0) {
        return ret;
    }
    ret = reg_write(dev, CONFIG, (data->buf | PWR_UP) & ~PRIM_RX);
    if (ret < 0) {
        return ret;
    }
    k_usleep(1500); /* 1.5 ms crystal oscillator startup delay */

    /* 2. Prepare 32-byte payload padded with 0x00 if len < 32 */
    uint8_t tx_payload[32] = { 0 };
    memcpy(tx_payload, buf, len);

    /* 3. Write W_TX_PAYLOAD command + padded payload in one continuous SPI transfer */
    uint8_t cmd = W_TX_PAYLOAD;
    struct spi_buf tx_bufs[] = {
        { .buf = &cmd, .len = 1 },
        { .buf = tx_payload, .len = max_payload },
    };
    const struct spi_buf_set tx_set = {
        .buffers = tx_bufs,
        .count = ARRAY_SIZE(tx_bufs),
    };

    ret = spi_write_dt(&cfg->spi, &tx_set);
    if (ret < 0) {
        return ret;
    }

    /* 4. Pulse CE high for >10 us to trigger transmission */
    gpio_pin_set_dt(&cfg->chipen, 1);
    k_busy_wait(15);
    gpio_pin_set_dt(&cfg->chipen, 0);

    return 0;
}

int send(const struct device *dev, const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    nrf_data *data = dev->data;
    int ret;

    /* 1. Start transmission */
    ret = send_start(dev, buf, len);
    if (ret < 0) {
        return ret;
    }

    /* 2. Poll send_done() until finished or timeout */
    int64_t start = k_uptime_get();
    int result = -1; // -1 == not finished (Python's None)

    while (result == -1 && (k_uptime_get() - start) < timeout_ms) {
        result = send_done(dev); // 1 == success, 2 == fail, -1 == busy
        if (result == -1) {
            k_usleep(100); 
        }
    }

   
    if (result == -1) {
        flush_rx_tx(dev, true); // true = FLUSH_TX
        ret = reg_read(dev, CONFIG);
        if (ret == 0) {
            reg_write(dev, CONFIG, data->buf & ~PWR_UP);
        }
        return -ETIMEDOUT;
    }

    
    if (result == 2) {
        return -EIO;
    }

    return 0;
}


static int nrf24l01_INIT(const struct device *dev){
    const nrf_conf *cfg = dev->config;
    nrf_data *data = dev->data;
    const uint8_t default_address_tx[NRF_ADDR_LEN] ={0xE7 ,0xE7,0xE7,0xE7,0xE7};
    
    memcpy(data->pipe_address_tx,default_address_tx,NRF_ADDR_LEN);
    data->pipe_address_tx[NRF_ADDR_LEN] = '\0';

    const uint8_t default_address_rx[NRF_ADDR_LEN] = {0xD2,0xF0,0xF0,0xF0,0xF0};
    memcpy(data->pipe_address_rx,default_address_rx,NRF_ADDR_LEN);
    data->pipe_address_rx[NRF_ADDR_LEN] = '\0';

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
    flush_rx_tx(dev,false);
    flush_rx_tx(dev,true);
    LOG_INF("Flush Buffers Done");

    
    LOG_INF("Device harsware is respinding Successfully");
    return 0;
}

static const struct nrf24l01_driver_api nrf24l01_api ={
    .open_tx = open_tx_pipe,
    .open_rx = open_rx_pipe,
    .stop_li = stop_listing,
    .send = send,
};
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
			      &nrf24l01_api);

DT_INST_FOREACH_STATUS_OKAY(NRF24L01_DEFINE)