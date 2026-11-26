target_include_directories(app PRIVATE )
target_sources_ifdef(CONFIG_NRF24L01 app PRIVATE nrf24l01.c)

