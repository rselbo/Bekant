#include "lin.h"
#include <Arduino.h>
#include "driver/uart.h"

constexpr const int BAUD_RATE = 19200;
constexpr const int SYNC_BREAK_LENGTH = 13;

constexpr const int PULSE_WIDTH_MICROSECONDS = 1000000 / BAUD_RATE;
constexpr const int SYNC_BREAK_MICROSECONDS = SYNC_BREAK_LENGTH * PULSE_WIDTH_MICROSECONDS;
// SYNC_BREAK_LENGTH + sync + id + 10 databytes (LIN has max 8 bytes + 1 checksum, we add another byte wait as slave sync time)
constexpr const int MAX_FRAME_TIME_MICROSECONDS = (SYNC_BREAK_LENGTH + 10 + 10 + (10*10)) * PULSE_WIDTH_MICROSECONDS;
constexpr const int RX_PIN = GPIO_NUM_16;
constexpr const int TX_PIN = GPIO_NUM_17;

#define VERIFY_READ(val, expected, fail, wrongValue) \
if(val == -1) { data[0] = val;      \
  return fail;        \
}                     \
if(val != expected) { data[0] = val;\
  return wrongValue;  \
}                     \

Lin::Lin()
{
}

void Lin::setup()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TX_PIN, RX_PIN, -1, -1);
    uart_driver_install(UART_NUM_2, 512, 0, 0, NULL, 0);
    uart_enable_rx_intr(UART_NUM_2);
}

void Lin::writeFrame(uint8_t id, const uint8_t *data, int dataLen)
{
    uint8_t addr = (id & 0x3f) | addrParity(id);
    uint8_t cksum = dataChecksum(data, dataLen, addr);
    uint8_t header[2] = {0x55, addr};

    uint64_t start = esp_timer_get_time();
    
    writeSyncBreak();
    uart_write_bytes(UART_NUM_2, header, 2);
    uart_write_bytes(UART_NUM_2, data, dataLen);
    uart_write_bytes(UART_NUM_2, &cksum, 1);

    // make sure we are done writing data before we return 
    int64_t delay = (start + (SYNC_BREAK_MICROSECONDS + PULSE_WIDTH_MICROSECONDS*2 + ((dataLen + 3) * 10 * PULSE_WIDTH_MICROSECONDS))) - esp_timer_get_time();
    if(delay > 0) {
      usleep(delay);
    }
}

void Lin::writeSyncBreak()
{
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);
  usleep(SYNC_BREAK_MICROSECONDS);
  digitalWrite(TX_PIN, HIGH);
  usleep(PULSE_WIDTH_MICROSECONDS*2);
  uart_set_pin(UART_NUM_2, TX_PIN, RX_PIN, -1, -1);
}

int Lin::receiveFrame(uint8_t id, uint8_t *data, int dataLen)
{
    uint8_t addr = (id & 0x3f) | addrParity(id);
    uint8_t header[2] = {0x55, addr};

    writeSyncBreak();
    // empty any data in the buffers
    uart_flush_input(UART_NUM_2);
    uart_write_bytes(UART_NUM_2, header, 2);

    usleep(SYNC_BREAK_MICROSECONDS + PULSE_WIDTH_MICROSECONDS*2 + PULSE_WIDTH_MICROSECONDS*20);

    uint64_t timeout = esp_timer_get_time() + MAX_FRAME_TIME_MICROSECONDS * 2;
    // verify that we hear our sync and address
    int readVal = 0;
    do {
      readVal = readByte(timeout);
    } while( readVal != -1 && readVal != 0x55);
    VERIFY_READ(readVal, 0x55, Lin::ReadSyncFail, Lin::ReadSyncWrongValue);
    readVal = readByte(timeout);
    VERIFY_READ(readVal, addr, Lin::ReadIdFail, Lin::ReadIdWrongValue);

    for(int i=0;i < dataLen; ++i) {
      readVal = readByte(timeout);
      if(readVal == -1) {
        return ReadTimeout;
      }
      data[i] = static_cast<uint8_t>(readVal & 0xff);
    }

    readVal = readByte(timeout);
    if(dataChecksum(data, dataLen, addr) != readVal) {
      return ReadWrongChecksum;
    }
    return dataLen;
}


/* Lin defines its checksum as an inverted 8 bit sum with carry */
uint8_t Lin::dataChecksum(const uint8_t* message, uint8_t messageLen, uint8_t addr)
{
    uint16_t sum = addr == 0x3c ? 0 : addr;
    for(;messageLen-- > 0;) {
      sum += *(message++);
      if(sum >= 256) {
        sum -= 255;
      }
    }
    return (~sum);
}

/* Create the Lin ID parity */
#define LINBIT(data,shift) ((addr>>shift)&1)
uint8_t Lin::addrParity(uint8_t addr)
{
  uint8_t p0 = LINBIT(addr,0) ^ LINBIT(addr,1) ^ LINBIT(addr,2) ^ LINBIT(addr,4);
  uint8_t p1 = ~(LINBIT(addr,1) ^ LINBIT(addr,3) ^ LINBIT(addr,4) ^ LINBIT(addr,5));
  return (p0 | (p1<<1))<<6;
}

int Lin::readByte(int64_t timeout)
{
  uint8_t data = 0;
  while(timeout > esp_timer_get_time()) {
    int ret = uart_read_bytes(UART_NUM_2, &data, 1, 0);
    if(ret == 1) {
      return data;
    }
    usleep(PULSE_WIDTH_MICROSECONDS);
  }
  return -1;
}