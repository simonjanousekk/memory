#ifndef SIM_SPI_H
#define SIM_SPI_H

// SPI shim: every byte written goes to the Sharp Memory LCD emulator
// (sim/src/sim_lcd.cpp), which decodes the real wire protocol.

#include <Arduino.h>

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

void sim_lcd_spi_byte(uint8_t b);

class SPISettings {
 public:
  SPISettings() {}
  SPISettings(uint32_t /*clock*/, uint8_t /*bit_order*/, uint8_t /*mode*/) {}
};

class SPIClass {
 public:
  void begin(int8_t /*sck*/ = -1, int8_t /*miso*/ = -1, int8_t /*mosi*/ = -1, int8_t /*ss*/ = -1) {}
  void end() {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  void write(uint8_t b) { sim_lcd_spi_byte(b); }
  void writeBytes(const uint8_t* data, uint32_t size) {
    while (size--) sim_lcd_spi_byte(*data++);
  }
  uint8_t transfer(uint8_t b) {
    sim_lcd_spi_byte(b);
    return 0;
  }
};

extern SPIClass SPI;

#endif
