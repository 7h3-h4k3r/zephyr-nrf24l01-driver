import re
text = """CONFIG = const(0x00)
EN_RXADDR = const(0x02)
SETUP_AW = const(0x03)
SETUP_RETR = const(0x04)
RF_CH = const(0x05)
RF_SETUP = const(0x06)
STATUS = const(0x07)
RX_ADDR_P0 = const(0x0A)
TX_ADDR = const(0x10)
RX_PW_P0 = const(0x11)
FIFO_STATUS = const(0x17)
DYNPD = const(0x1C)

# CONFIG register
EN_CRC = const(0x08)  # enable CRC
CRCO = const(0x04)  # CRC encoding scheme; 0=1 byte, 1=2 bytes
PWR_UP = const(0x02)  # 1=power up, 0=power down
PRIM_RX = const(0x01)  # RX/TX control; 0=PTX, 1=PRX

# RF_SETUP register
POWER_0 = const(0x00)  # -18 dBm
POWER_1 = const(0x02)  # -12 dBm
POWER_2 = const(0x04)  # -6 dBm
POWER_3 = const(0x06)  # 0 dBm
SPEED_1M = const(0x00)
SPEED_2M = const(0x08)
SPEED_250K = const(0x20)

# STATUS register
RX_DR = const(0x40)  # RX data ready; write 1 to clear
TX_DS = const(0x20)  # TX data sent; write 1 to clear
MAX_RT = const(0x10)  # max retransmits reached; write 1 to clear

# FIFO_STATUS register
RX_EMPTY = const(0x01)  # 1 if RX FIFO is empty

# constants for instructions
R_RX_PL_WID = const(0x60)  # read RX payload width
R_RX_PAYLOAD = const(0x61)  # read RX payload
W_TX_PAYLOAD = const(0xA0)  # write TX payload
FLUSH_TX = const(0xE1)  # flush TX FIFO
FLUSH_RX = const(0xE2)  # flush RX FIFO
NOP = const(0xFF)  # use to read STATUS """


pattern = r'(\w+)\s*=\s*const\((0x[0-9A-Fa-f]+)\)'

results = re.findall(pattern, text)

for commend,hec_value in results:
    print(f"#define {commend} {hec_value}")
    
    